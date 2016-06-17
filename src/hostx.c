/*
 * Xephyr - A kdrive X server thats runs in a host X window.
 *          Authored by Matthew Allum <mallum@o-hand.com>
 *
 * Copyright © 2004 Nokia
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Nokia not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. Nokia makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * NOKIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL NOKIA BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hostx.h"
#include "input.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>             /* for memset */
#include <errno.h>
#include <time.h>
#include <err.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>

#include <X11/keysym.h>
#include <xf86.h>
#include <xf86Priv.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>
#include <xcb/shape.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/randr.h>
#ifdef GLAMOR
#include <epoxy/gl.h>
#include "glamor.h"
#include "ephyr_glamor_glx.h"
#endif
#include "ephyrlog.h"
#include "ephyr.h"

static int HostXWantDamageDebug = 0;

extern Bool EphyrWantResize;

extern const char *ephyrTitle;
Bool ephyr_glamor = FALSE;

extern const char *ephyrResName;
extern Bool ephyrResNameFromConfig;
extern xcb_intern_atom_cookie_t cookie_WINDOW_STATE,
                                cookie_WINDOW_STATE_FULLSCREEN;

static void hostx_set_fullscreen_hint(ScrnInfoPtr pScrn);

#define host_depth_matches_server(_vars) (priv->depth == (_vars)->server_depth)

int
hostx_want_screen_geometry(ScrnInfoPtr pScrn,
                           unsigned int *width, unsigned int *height,
                           int *x, int *y) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    if (priv && (priv->win_pre_existing != None ||
                 priv->output != NULL ||
                 priv->use_fullscreen == TRUE)) {
        *x = priv->win_x;
        *y = priv->win_y;
        *width = priv->win_width;
        *height = priv->win_height;
        return 1;
    }

    return 0;
}

void
hostx_set_display_name(ScrnInfoPtr pScrn, const char *name) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    priv->server_dpy_name = strdup(name);
}

void
hostx_set_screen_number(ScrnInfoPtr pScrn, int number) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    if (priv) {
        priv->mynum = number;
        hostx_set_win_title(pScrn, "");
    }
}

xcb_cursor_t
hostx_get_empty_cursor(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    return priv->empty_cursor;
}

Bool
hostx_want_preexisting_window(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    return priv && priv->win_pre_existing;
}

void
hostx_get_output_geometry(ScrnInfoPtr pScrn,
                          const char *output,
                          int *x, int *y,
                          unsigned int *width, unsigned int *height) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    int i, name_len = 0, output_found = FALSE;
    char *name = NULL;
    xcb_generic_error_t *error;
    xcb_randr_query_version_cookie_t version_c;
    xcb_randr_query_version_reply_t *version_r;
    xcb_randr_get_screen_resources_cookie_t screen_resources_c;
    xcb_randr_get_screen_resources_reply_t *screen_resources_r;
    xcb_randr_output_t *randr_outputs;
    xcb_randr_get_output_info_cookie_t output_info_c;
    xcb_randr_get_output_info_reply_t *output_info_r;
    xcb_randr_get_crtc_info_cookie_t crtc_info_c;
    xcb_randr_get_crtc_info_reply_t *crtc_info_r;

    /* First of all, check for extension */
    if (!hostx_has_extension(pScrn, &xcb_randr_id)) {
        fprintf(stderr, "\nHost X server does not support RANDR extension (or it's disabled).\n");
        exit(1);
    }

    /* Check RandR version */
    version_c = xcb_randr_query_version(priv->conn, 1, 2);
    version_r = xcb_randr_query_version_reply(priv->conn,
                                              version_c,
                                              &error);

    if (error != NULL || version_r == NULL) {
        fprintf(stderr, "\nFailed to get RandR version supported by host X server.\n");
        exit(1);
    } else if (version_r->major_version < 1 ||
               (version_r->major_version == 1 && version_r->minor_version < 2)) {
        free(version_r);
        fprintf(stderr, "\nHost X server doesn't support RandR 1.2, needed for -output usage.\n");
        exit(1);
    }

    free(version_r);

    /* Get list of outputs from screen resources */
    screen_resources_c = xcb_randr_get_screen_resources(priv->conn,
                                                        priv->winroot);
    screen_resources_r = xcb_randr_get_screen_resources_reply(priv->conn,
                                                              screen_resources_c,
                                                              NULL);
    randr_outputs = xcb_randr_get_screen_resources_outputs(screen_resources_r);

    for (i = 0; !output_found && i < screen_resources_r->num_outputs; i++) {
        /* Get info on the output */
        output_info_c = xcb_randr_get_output_info(priv->conn,
                                                  randr_outputs[i],
                                                  XCB_CURRENT_TIME);
        output_info_r = xcb_randr_get_output_info_reply(priv->conn,
                                                        output_info_c,
                                                        NULL);

        /* Get output name */
        name_len = xcb_randr_get_output_info_name_length(output_info_r);
        name = malloc(name_len + 1);
        strncpy(name, (char*)xcb_randr_get_output_info_name(output_info_r), name_len);
        name[name_len] = '\0';

        if (!strcmp(name, output)) {
            output_found = TRUE;

            /* Check if output is connected */
            if (output_info_r->crtc == XCB_NONE) {
                free(name);
                free(output_info_r);
                free(screen_resources_r);
                fprintf(stderr, "\nOutput %s is currently disabled (or not connected).\n", output);
                exit(1);
            }

            /* Get CRTC from output info */
            crtc_info_c = xcb_randr_get_crtc_info(priv->conn,
                                                  output_info_r->crtc,
                                                  XCB_CURRENT_TIME);
            crtc_info_r = xcb_randr_get_crtc_info_reply(priv->conn,
                                                        crtc_info_c,
                                                        NULL);

            /* Get CRTC geometry */
            *x = crtc_info_r->x;
            *y = crtc_info_r->y;
            *width = crtc_info_r->width;
            *height = crtc_info_r->height;

            free(crtc_info_r);
        }

        free(name);
        free(output_info_r);
    }

    free(screen_resources_r);

    if (!output_found) {
        fprintf(stderr, "\nOutput %s not available in host X server.\n", output);
        exit(1);
    }
}

static void
hostx_toggle_damage_debug(void) {
    HostXWantDamageDebug ^= 1;
}

void
hostx_handle_signal(int signum) {
    hostx_toggle_damage_debug();
    EPHYR_DBG("Signal caught. Damage Debug:%i\n", HostXWantDamageDebug);
}

void
hostx_set_title(const char *title) {
    ephyrTitle = title;
}

#ifdef __SUNPRO_C
/* prevent "Function has no return statement" error for x_io_error_handler */
#pragma does_not_return(exit)
#endif

int
hostx_get_depth(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    return priv->depth;
}

int
hostx_get_server_depth(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    return priv ? priv->server_depth : 0;
}

int
hostx_get_bpp(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    if (!priv) {
        return 0;
    }

    if (host_depth_matches_server(priv)) {
        return priv->visual->bits_per_rgb_value;
    } else {
        return priv->server_depth; /*XXX correct ?*/
    }
}

void
hostx_get_visual_masks(ScrnInfoPtr pScrn,
                       CARD32 *rmsk, CARD32 *gmsk, CARD32 *bmsk) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    if (!priv) {
        return;
    }

    if (host_depth_matches_server(priv)) {
        *rmsk = priv->visual->red_mask;
        *gmsk = priv->visual->green_mask;
        *bmsk = priv->visual->blue_mask;
    } else if (priv->server_depth == 16) {
        /* Assume 16bpp 565 */
        *rmsk = 0xf800;
        *gmsk = 0x07e0;
        *bmsk = 0x001f;
    } else {
        *rmsk = 0x0;
        *gmsk = 0x0;
        *bmsk = 0x0;
    }
}

static int
hostx_calculate_color_shift(unsigned long mask) {
    int shift = 1;

    /* count # of bits in mask */
    while ((mask = (mask >> 1))) {
        shift++;
    }

    /* cmap entry is an unsigned char so adjust it by size of that */
    shift = shift - sizeof(unsigned char) * 8;

    if (shift < 0) {
        shift = 0;
    }

    return shift;
}

void
hostx_set_cmap_entry(ScreenPtr pScreen, unsigned char idx,
                     unsigned char r, unsigned char g, unsigned char b) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;
/* need to calculate the shifts for RGB because server could be BGR. */
/* XXX Not sure if this is correct for 8 on 16, but this works for 8 on 24.*/
    static int rshift, bshift, gshift = 0;
    static int first_time = 1;

    if (first_time) {
        first_time = 0;
        rshift = hostx_calculate_color_shift(priv->visual->red_mask);
        gshift = hostx_calculate_color_shift(priv->visual->green_mask);
        bshift = hostx_calculate_color_shift(priv->visual->blue_mask);
    }

    priv->cmap[idx] = ((r << rshift) & priv->visual->red_mask)   |
                      ((g << gshift) & priv->visual->green_mask) |
                      ((b << bshift) & priv->visual->blue_mask);
}

void
hostx_close_screen(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    if (priv->have_shm) {
        xcb_shm_detach(priv->conn, priv->shminfo.shmseg);
        shmdt(priv->shminfo.shmaddr);
        shmctl(priv->shminfo.shmid, IPC_RMID, 0);
    } else {
        free(priv->ximg->data);
        priv->ximg->data = NULL;
    }

    xcb_image_destroy(priv->ximg);
    priv->ximg = NULL;
}

/**
 * hostx_screen_init creates the XImage that will contain the front buffer of
 * the ephyr screen, and possibly offscreen memory.
 *
 * @param width width of the screen
 * @param height height of the screen
 * @param buffer_height  height of the rectangle to be allocated.
 *
 * hostx_screen_init() creates an XImage, using MIT-SHM if it's available.
 * buffer_height can be used to create a larger offscreen buffer, which is used
 * by fakexa for storing offscreen pixmap data.
 */
void *
hostx_screen_init(ScrnInfoPtr pScrn,
                  int x, int y,
                  int width, int height, int buffer_height,
                  int *bytes_per_line, int *bits_per_pixel) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    Bool shm_success = FALSE;

    if (!priv) {
        fprintf(stderr, "%s: Error in accessing hostx data\n", __func__);
        exit(1);
    }

    EPHYR_DBG("host_screen=%p x=%d, y=%d, wxh=%dx%d, buffer_height=%d",
              pScrn, x, y, width, height, buffer_height);

    if (priv->ximg != NULL) {
        /* Free up the image data if previously used
         * i.ie called by server reset
         */
        hostx_close_screen(pScrn);
    }

    if (!ephyr_glamor && priv->have_shm) {
        priv->ximg = xcb_image_create_native(priv->conn,
                                             width,
                                             buffer_height,
                                             XCB_IMAGE_FORMAT_Z_PIXMAP,
                                             priv->depth,
                                             NULL,
                                             ~0,
                                             NULL);

        priv->shminfo.shmid =
            shmget(IPC_PRIVATE,
                   priv->ximg->stride * buffer_height,
                   IPC_CREAT | 0777);
        priv->ximg->data = shmat(priv->shminfo.shmid, 0, 0);
        priv->shminfo.shmaddr = priv->ximg->data;

        if (priv->ximg->data == (uint8_t *) -1) {
            EPHYR_DBG
                ("Can't attach SHM Segment, falling back to plain XImages");
            priv->have_shm = FALSE;
            xcb_image_destroy(priv->ximg);
            shmctl(priv->shminfo.shmid, IPC_RMID, 0);
        } else {
            EPHYR_DBG("SHM segment attached %p", priv->shminfo.shmaddr);
            priv->shminfo.shmseg = xcb_generate_id(priv->conn);
            xcb_shm_attach(priv->conn,
                           priv->shminfo.shmseg,
                           priv->shminfo.shmid,
                           FALSE);
            shm_success = TRUE;
        }
    }

    if (!ephyr_glamor && !shm_success) {
        EPHYR_DBG("Creating image %dx%d for screen priv=%p\n",
                  width, buffer_height, priv);
        priv->ximg = xcb_image_create_native(priv->conn,
                                             width,
                                             buffer_height,
                                             XCB_IMAGE_FORMAT_Z_PIXMAP,
                                             priv->depth,
                                             NULL,
                                             ~0,
                                             NULL);

        /* Match server byte order so that the image can be converted to
         * the native byte order by xcb_image_put() before drawing */
        if (host_depth_matches_server(priv)) {
            priv->ximg->byte_order = IMAGE_BYTE_ORDER;
        }

        priv->ximg->data =
            xallocarray(priv->ximg->stride, buffer_height);
    }

    {
        uint32_t mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        uint32_t values[2] = {width, height};
        xcb_configure_window(priv->conn, priv->win, mask, values);
    }

    if (priv->win_pre_existing == None && !EphyrWantResize) {
        /* Ask the WM to keep our size static */
        xcb_size_hints_t size_hints = {0};
        size_hints.max_width = size_hints.min_width = width;
        size_hints.max_height = size_hints.min_height = height;
        size_hints.flags = (XCB_ICCCM_SIZE_HINT_P_MIN_SIZE |
                            XCB_ICCCM_SIZE_HINT_P_MAX_SIZE);
        xcb_icccm_set_wm_normal_hints(priv->conn, priv->win,
                                      &size_hints);
    }

    xcb_map_window(priv->conn, priv->win);

    /* Set explicit window position if it was informed in
     * -screen option (WxH+X or WxH+X+Y). Otherwise, accept the
     * position set by WM.
     * The trick here is putting this code after xcb_map_window() call,
     * so these values won't be overriden by WM. */
    if (priv->win_explicit_position) {
        uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
        uint32_t values[2] = {x, y};
        xcb_configure_window(priv->conn, priv->win, mask, values);
    }

    xcb_aux_sync(priv->conn);

    priv->win_width = width;
    priv->win_height = height;
    priv->win_x = x;
    priv->win_y = y;

#ifdef GLAMOR
    if (ephyr_glamor) {
        *bytes_per_line = 0;
        *bits_per_pixel = 0;
        ephyr_glamor_set_window_size(priv->glamor,
                                     priv->win_width, priv->win_height);
        return NULL;
    } else
#endif
    {
        if (host_depth_matches_server(priv)) {
            if (bytes_per_line != NULL) {
                *bytes_per_line = priv->ximg->stride;
            }

            if (bits_per_pixel != NULL) {
                *bits_per_pixel = priv->ximg->bpp;
            }

            EPHYR_DBG("Host matches server");
            return priv->ximg->data;
        } else {
            int bytes_per_pixel = priv->server_depth >> 3;
            int stride = (width * bytes_per_pixel + 0x3) & ~0x3;

            if (bytes_per_line != NULL) {
                *bytes_per_line = stride;
            }

            if (bits_per_pixel != NULL) {
                *bits_per_pixel = priv->server_depth;
            }

            EPHYR_DBG("server bpp %i", bytes_per_pixel);
            priv->fb_data = xallocarray (stride, buffer_height);
            return priv->fb_data;
        }
    }
}

static void hostx_paint_debug_rect(ScrnInfoPtr pScrn,
                                   int x, int y, int width, int height);

void
hostx_paint_rect(ScrnInfoPtr pScrn,
                 int sx, int sy, int dx, int dy, int width, int height) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    EPHYR_DBG("painting in screen %d\n", priv->mynum);

#ifdef GLAMOR
    if (ephyr_glamor) {
        BoxRec box;
        RegionRec region;

        box.x1 = dx;
        box.y1 = dy;
        box.x2 = dx + width;
        box.y2 = dy + height;

        RegionInit(&region, &box, 1);
        ephyr_glamor_damage_redisplay(priv->glamor, &region);
        RegionUninit(&region);
        return;
    }
#endif

    /*
     *  Copy the image data updated by the shadow layer
     *  on to the window
     */
    if (HostXWantDamageDebug) {
        hostx_paint_debug_rect(pScrn, dx, dy, width, height);
    }

    /*
     * If the depth of the ephyr server is less than that of the host,
     * the kdrive fb does not point to the ximage data but to a buffer
     * ( fb_data ), we shift the various bits from this onto the XImage
     * so they match the host.
     *
     * Note, This code is pretty new ( and simple ) so may break on
     *       endian issues, 32 bpp host etc.
     *       Not sure if 8bpp case is right either.
     *       ... and it will be slower than the matching depth case.
     */

    if (!host_depth_matches_server(priv)) {
        int x, y, idx, bytes_per_pixel = (priv->server_depth >> 3);
        int stride = (priv->win_width * bytes_per_pixel + 0x3) & ~0x3;
        unsigned char r, g, b;
        unsigned long host_pixel;

        EPHYR_DBG("Unmatched host depth priv=%p\n", priv);
        for (y = sy; y < sy + height; y++) {
            for (x = sx; x < sx + width; x++) {
                idx = y * stride + x * bytes_per_pixel;

                switch (priv->server_depth) {
                case 16:
                {
                    unsigned short pixel =
                        *(unsigned short *) (priv->fb_data + idx);

                    r = ((pixel & 0xf800) >> 8);
                    g = ((pixel & 0x07e0) >> 3);
                    b = ((pixel & 0x001f) << 3);

                    host_pixel = (r << 16) | (g << 8) | (b);

                    xcb_image_put_pixel(priv->ximg, x, y, host_pixel);
                    break;
                }
                case 8:
                {
                    unsigned char pixel =
                        *(unsigned char *) (priv->fb_data + idx);
                    xcb_image_put_pixel(priv->ximg, x, y,
                                        priv->cmap[pixel]);
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    if (priv->have_shm) {
        xcb_image_shm_put(priv->conn, priv->win,
                          priv->gc, priv->ximg,
                          priv->shminfo,
                          sx, sy, dx, dy, width, height, FALSE);
    } else {
        xcb_image_t *subimg = xcb_image_subimage(priv->ximg, sx, sy,
                                                 width, height, 0, 0, 0);
        xcb_image_t *img = xcb_image_native(priv->conn, subimg, 1);
        xcb_image_put(priv->conn, priv->win, priv->gc, img, dx, dy, 0);

        if (subimg != img) {
            xcb_image_destroy(img);
        }

        xcb_image_destroy(subimg);
    }

    xcb_aux_sync(priv->conn);
}

static void
hostx_paint_debug_rect(ScrnInfoPtr pScrn,
                       int x, int y, int width, int height) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    struct timespec tspec;
    xcb_rectangle_t rect = { .x = x, .y = y, .width = width, .height = height };
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *e;

    tspec.tv_sec = priv->damage_debug_msec / (1000000);
    tspec.tv_nsec = (priv->damage_debug_msec % 1000000) * 1000;

    EPHYR_DBG("msec: %li tv_sec %li, tv_msec %li",
              priv->damage_debug_msec, tspec.tv_sec, tspec.tv_nsec);

    /* fprintf(stderr, "Xephyr updating: %i+%i %ix%i\n", x, y, width, height); */

    cookie = xcb_poly_fill_rectangle_checked(priv->conn, priv->win,
                                             priv->gc, 1, &rect);
    e = xcb_request_check(priv->conn, cookie);
    free(e);

    /* nanosleep seems to work better than usleep for me... */
    nanosleep(&tspec, NULL);
}

int
hostx_get_screen(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    return priv->screen;
}

int
hostx_get_fd(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    return xcb_get_file_descriptor(priv->conn);
}

#ifdef GLAMOR
Bool
ephyr_glamor_init(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    priv->glamor = ephyr_glamor_glx_screen_init(priv->win);
    ephyr_glamor_set_window_size(priv->glamor,
                                 priv->win_width, priv->win_height);

    if (!glamor_init(pScreen, 0)) {
        FatalError("Failed to initialize glamor\n");
        return FALSE;
    }

    return TRUE;
}

Bool
ephyr_glamor_create_screen_resources(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    PixmapPtr screen_pixmap;
    uint32_t tex;

    if (!ephyr_glamor)
        return TRUE;

    /* kdrive's fbSetupScreen() told mi to have
     * miCreateScreenResources() (which is called before this) make a
     * scratch pixmap wrapping ephyr-glamor's NULL
     * KdScreenInfo->fb.framebuffer.
     *
     * We want a real (texture-based) screen pixmap at this point.
     * This is what glamor will render into, and we'll then texture
     * out of that into the host's window to present the results.
     *
     * Thus, delete the current screen pixmap, and put a fresh one in.
     */
    screen_pixmap = pScreen->GetScreenPixmap(pScreen);
    pScreen->DestroyPixmap(screen_pixmap);

    screen_pixmap = pScreen->CreatePixmap(pScreen,
                                          pScreen->width,
                                          pScreen->height,
                                          pScreen->rootDepth,
                                          GLAMOR_CREATE_NO_LARGE);

    pScreen->SetScreenPixmap(screen_pixmap);

    /* Tell the GLX code what to GL texture to read from. */
    tex = glamor_get_pixmap_texture(screen_pixmap);
    ephyr_glamor_set_texture(priv->glamor, tex);

    return TRUE;
}

void
ephyr_glamor_enable(ScreenPtr pScreen) {
}

void
ephyr_glamor_disable(ScreenPtr pScreen) {
}

void
ephyr_glamor_fini(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    glamor_fini(pScreen);
    ephyr_glamor_glx_screen_fini(priv->glamor);
    priv->glamor = NULL;
}
#endif
