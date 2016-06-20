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

#define HOST_DEPTH_MATCHES_SERVER(_vars) ((_vars)->depth == (_vars)->server_depth)

extern int HostXWantDamageDebug;

extern Bool EphyrWantResize;

extern const char *ephyrTitle;
Bool ephyr_glamor = FALSE;

extern const char *ephyrResName;
extern Bool ephyrResNameFromConfig;
extern xcb_intern_atom_cookie_t cookie_WINDOW_STATE,
                                cookie_WINDOW_STATE_FULLSCREEN;

static void hostx_set_fullscreen_hint(ScrnInfoPtr pScrn);

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

    if (HOST_DEPTH_MATCHES_SERVER(priv)) {
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

    if (HOST_DEPTH_MATCHES_SERVER(priv)) {
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

int
hostx_get_screen(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    return priv->screen;
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
