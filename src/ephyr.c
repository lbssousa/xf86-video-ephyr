/*
 * Xephyr - A kdrive X server thats runs in a host X window.
 *     Authored by Matthew Allum <mallum@openedhand.com>
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
 * representations about the suitability of this software for any purpose. It
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

#include <sys/shm.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/randr.h>

#include "ephyr.h"
#include "hostx.h"

#include "inputstr.h"
#include "scrnintstr.h"
#include "ephyrlog.h"

#ifdef GLAMOR
#include "glamor.h"
#endif
#include "ephyr_glamor_glx.h"

/* End of former ephyr.c includes
 *******************************************************
 * Beginning of driver.c includes */

#include <stdlib.h>
#include <string.h>

#include <xorg-server.h>
#include <xorgVersion.h>
#include <fb.h>
#include <micmap.h>
#include <mipointer.h>
#include <shadow.h>
#include <xf86.h>
#include <xf86Priv.h>
#include <xf86Module.h>
#include <xf86str.h>
#include <xf86Optrec.h>

#include "compat-api.h"

#define EPHYR_VERSION 0
#define EPHYR_NAME "EPHYR"
#define EPHYR_DRIVER_NAME "ephyr"

#define EPHYR_MAJOR_VERSION PACKAGE_VERSION_MAJOR
#define EPHYR_MINOR_VERSION PACKAGE_VERSION_MINOR
#define EPHYR_PATCHLEVEL PACKAGE_VERSION_PATCHLEVEL

#define TIMER_CALLBACK_INTERVAL 20

#define HOST_DEPTH_MATCHES_SERVER(_vars) ((_vars)->depth == (_vars)->server_depth)

/* End of driver.c includes
 ******************************************************
 * Beginning of former ephyr.c */
extern Bool ephyr_glamor, ephyr_glamor_gles2;

Bool ephyrNoDRI = FALSE;
Bool ephyrNoXV = FALSE;

static int mouseState = 0;
static Rotation ephyrRandr = RR_Rotate_0;

int HostXWantDamageDebug = 0;
Bool EphyrWantGrayScale = 0;
Bool EphyrWantResize = 0;
Bool EphyrWantNoHostGrab = 0;

const char *ephyrTitle = NULL;
const char *ephyrResName = NULL;
Bool ephyrResNameFromConfig = FALSE;

#if 0
Bool
ephyrInitialize(KdCardInfo * card, EphyrPriv * priv) {
    OsSignal(SIGUSR1, hostx_handle_signal);

    priv->base = 0;
    priv->bytes_per_line = 0;
    return TRUE;
}

Bool
ephyrCardInit(KdCardInfo * card) {
    EphyrPriv *priv = (EphyrPriv *) malloc(sizeof(EphyrPriv));

    if (!priv) {
        return FALSE;
    }

    if (!ephyrInitialize(card, priv)) {
        free(priv);
        return FALSE;
    }

    card->driver = priv;

    return TRUE;
}
#endif

#if 0
Bool
ephyrScreenInitialize(ScrnInfoPtr pScrn)
{
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    int x = 0, y = 0;
    int width = 640, height = 480;
    CARD32 redMask, greenMask, blueMask;

    if (hostx_want_screen_geometry(pScrn, &width, &height, &x, &y)
        || !pScrn->virtualX || !pScrn->virtualY) {
        pScrn->virtualX = width;
        pScrn->virtualY = height;
        pScrn->x = x;
        pScrn->y = y;
    }

    if (EphyrWantGrayScale) {
        pScrn->depth = 8;
    }

    if (pScrn->depth && pScrn->depth != hostx_get_depth()) {
        if (pScrn->depth < hostx_get_depth()
            && (pScrn->depth == 24 || pScrn->depth == 16
                || pScrn->depth == 8)) {
            priv->server_depth = pScrn->depth;
        } else {
            ErrorF
                ("\nXephyr: requested screen depth not supported, setting to match hosts.\n");
        }
    }

    pScrn->depth = hostx_get_server_depth(pScrn);
    pScrn->rate = 72;

    if (pScrn->depth <= 8) {
        if (EphyrWantGrayScale) {
            pScrn->fb.visuals = ((1 << StaticGray) | (1 << GrayScale));
        } else {
            pScrn->fb.visuals = ((1 << StaticGray) |
                                 (1 << GrayScale) |
                                 (1 << StaticColor) |
                                 (1 << PseudoColor) |
                                 (1 << TrueColor) | (1 << DirectColor));
        }

        pScrn->mask.red = 0x00;
        pScrn->mask.green = 0x00;
        pScrn->mask.blue = 0x00;
        pScrn->depth = 8;
        pScrn->bitsPerPixel = 8;
    } else {
        pScrn->fb.visuals = (1 << TrueColor);

        if (pScrn->depth <= 15) {
            pScrn->depth = 15;
            pScrn->bitsPerPixel = 16;
        } else if (pScrn->depth <= 16) {
            pScrn->depth = 16;
            pScrn->bitsPerPixel = 16;
        } else if (pScrn->depth <= 24) {
            pScrn->depth = 24;
            pScrn->bitsPerPixel = 32;
        } else if (pScrn->depth <= 30) {
            pScrn->depth = 30;
            pScrn->bitsPerPixel = 32;
        } else {
            ErrorF("\nXephyr: Unsupported screen depth %d\n", pScrn->depth);
            return FALSE;
        }

        hostx_get_visual_masks(pScrn, &redMask, &greenMask, &blueMask);

        pScrn->mask.red = (Pixel) redMask;
        pScrn->mask.green = (Pixel) greenMask;
        pScrn->mask.blue = (Pixel) blueMask;

    }

    priv->randr = pScrn->randr;

    return ephyrMapFramebuffer(pScrn);
}
#endif

#if 0
void *
ephyrWindowLinear(ScreenPtr pScreen,
                  CARD32 row,
                  CARD32 offset, int mode, CARD32 *size, void *closure) {
    EphyrPriv *priv = pScreenPriv->card->driver;

    if (!pScreenPriv->enabled) {
        return 0;
    }

    *size = priv->bytes_per_line;
    return priv->base + row * priv->bytes_per_line + offset;
}
#endif

/**
 * Figure out display buffer size. If fakexa is enabled, allocate a larger
 * buffer so that fakexa has space to put offscreen pixmaps.
 */
int
ephyrBufferHeight(ScrnInfoPtr pScrn) {
    int buffer_height;

    /* TODO: replace kdrive's initAccel() with some xfree86 eqiuvalent (e.g. EXA)
      if (ephyrFuncs.initAccel == NULL)
      buffer_height = pScrn->virtualY;
      else
      buffer_height = 3 * pScrn->virtualY; */
    buffer_height = pScrn->virtualY;
    return buffer_height;
}

#if 0
Bool
ephyrMapFramebuffer(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    EphyrPriv *priv = pScrn->card->driver;
    KdPointerMatrix m;
    int buffer_height;

    EPHYR_LOG("pScrn->virtualX: %d, pScrn->virtualY: %d index=%d",
              pScrn->virtualX, pScrn->virtualY, pScrn->mynum);

    /*
     * Use the rotation last applied to ourselves (in the Xephyr case the fb
     * coordinate system moves independently of the pointer coordiante system).
     */
    KdComputePointerMatrix(&m, ephyrRandr, pScrn->virtualX, pScrn->virtualY);
    KdSetPointerMatrix(&m);

    buffer_height = ephyrBufferHeight(pScrn);

    priv->base = hostx_screen_init(pScrn, pScrn->x, pScrn->y,
                                   pScrn->virtualX, pScrn->virtualY,
                                   buffer_height,
                                   &priv->bytes_per_line,
                                   &pScrn->fb.bitsPerPixel);

    if ((priv->randr & RR_Rotate_0) && !(priv->randr & RR_Reflect_All)) {
        priv->shadow = FALSE;

        pScrn->fb.byteStride = priv->bytes_per_line;
        pScrn->fb.pixelStride = pScrn->virtualX;
        pScrn->fb.frameBuffer = (CARD8 *) (priv->base);
    } else {
        /* Rotated/Reflected so we need to use shadow fb */
        priv->shadow = TRUE;

        EPHYR_LOG("allocing shadow");

        KdShadowFbAlloc(pScrn,
                        priv->randr & (RR_Rotate_90 | RR_Rotate_270));
    }

    return TRUE;
}
#endif

#if 0
void
ephyrSetScreenSizes(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    if (priv->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        pScreen->width = pScrn->virtualX;
        pScreen->height = pScrn->virtualY;
        pScreen->mmWidth = pScrn->widthmm;
        pScreen->mmHeight = pScrn->heightmm;
    } else {
        pScreen->width = pScrn->virtualY;
        pScreen->height = pScrn->virtualX;
        pScreen->mmWidth = pScrn->heightmm;
        pScreen->mmHeight = pScrn->widthmm;
    }
}

Bool
ephyrUnmapFramebuffer(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    if (priv->shadow) {
        KdShadowFbFree(pScrn);
    }

    /* Note, priv->base will get freed when XImage recreated */

    return TRUE;
}
#endif

static void
_ephyrPaintDebugRect(EphyrPrivatePtr priv,
                     int x, int y, int width, int height) {
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

static void
_ephyrPaintRect(EphyrPrivatePtr priv,
                int sx, int sy, int dx, int dy, int width, int height) {
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
        _ephyrPaintDebugRect(priv, dx, dy, width, height);
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

    if (!HOST_DEPTH_MATCHES_SERVER(priv)) {
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
ephyrInternalDamageRedisplay(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    RegionPtr pRegion;

    if (!priv || !priv->pDamage) {
        return;
    }

    pRegion = DamageRegion(priv->pDamage);

    if (RegionNotEmpty(pRegion)) {
        int nbox;
        BoxPtr pbox;

        if (ephyr_glamor) {
            ephyr_glamor_damage_redisplay(priv->glamor, pRegion);
        } else {
            nbox = RegionNumRects(pRegion);
            pbox = RegionRects(pRegion);

            while (nbox--) {
                _ephyrPaintRect(priv,
                         pbox->x1, pbox->y1,
                         pbox->x1, pbox->y1,
                         pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
                pbox++;
            }
        }
        DamageEmpty(priv->pDamage);
    }
}

static void
ephyrInternalDamageBlockHandler(void *data, OSTimePtr pTimeout, void *pRead) {
    ScreenPtr pScreen = (ScreenPtr) data;

    ephyrInternalDamageRedisplay(pScreen);
}

static void
ephyrInternalDamageWakeupHandler(void *data, int i, void *LastSelectMask) {
    /* FIXME: Not needed ? */
}

Bool
ephyrSetInternalDamage(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    PixmapPtr pPixmap = NULL;

    priv->pDamage = DamageCreate((DamageReportFunc) 0,
                                    (DamageDestroyFunc) 0,
                                    DamageReportNone, TRUE, pScreen, pScreen);

    if (!RegisterBlockAndWakeupHandlers(ephyrInternalDamageBlockHandler,
                                        ephyrInternalDamageWakeupHandler,
                                        (void *) pScreen)) {
        return FALSE;
    }

    pPixmap = (*pScreen->GetScreenPixmap)(pScreen);

    DamageRegister(&pPixmap->drawable, priv->pDamage);

    return TRUE;
}

void
ephyrUnsetInternalDamage(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    DamageDestroy(priv->pDamage);

    RemoveBlockAndWakeupHandlers(ephyrInternalDamageBlockHandler,
                                 ephyrInternalDamageWakeupHandler,
                                 (void *) pScreen);
}

#if 0
#ifdef RANDR
Bool
ephyrRandRGetInfo(ScreenPtr pScreen, Rotation * rotations) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    RRScreenSizePtr pSize;
    Rotation randr;
    int n = 0;

    struct {
        int width, height;
    } sizes[] = {
        {1600, 1200},
        {1400, 1050},
        {1280, 960},
        {1280, 1024},
        {1152, 864},
        {1024, 768},
        {832, 624},
        {800, 600},
        {720, 400},
        {480, 640},
        {640, 480},
        {640, 400},
        {320, 240},
        {240, 320},
        {160, 160},
        {0, 0}
    };

    EPHYR_LOG("mark");

    *rotations = RR_Rotate_All | RR_Reflect_All;

    if (!hostx_want_preexisting_window(pScrn)
        && !hostx_want_fullscreen()) { /* only if no -parent switch */
        while (sizes[n].width != 0 && sizes[n].height != 0) {
            RRRegisterSize(pScreen,
                           sizes[n].width,
                           sizes[n].height,
                           (sizes[n].width * pScrn->widthmm) / pScrn->virtualX,
                           (sizes[n].height * pScrn->heightmm) /
                           pScrn->virtualY);
            n++;
        }
    }

    pSize = RRRegisterSize(pScreen,
                           pScrn->virtualX,
                           pScrn->virtualY, pScrn->widthmm, pScrn->heightmm);

    randr = KdSubRotation(priv->randr, pScrn->randr);

    RRSetCurrentConfig(pScreen, randr, 0, pSize);

    return TRUE;
}

#if 0
Bool
ephyrRandRSetConfig(ScreenPtr pScreen,
                    Rotation randr, int rate, RRScreenSizePtr pSize) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    Bool wasEnabled = pScreenPriv->enabled;
    EphyrPrivate oldscr;
    int oldwidth, oldheight, oldmmwidth, oldmmheight;
    Bool oldshadow;
    int newwidth, newheight;

    if (pScrn->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        newwidth = pSize->width;
        newheight = pSize->height;
    } else {
        newwidth = pSize->height;
        newheight = pSize->width;
    }

    if (wasEnabled) {
        KdDisableScreen(pScreen);
    }

    oldscr = *priv;

    oldwidth = pScrn->virtualX;
    oldheight = pScrn->virtualY;
    oldmmwidth = pScreen->mmWidth;
    oldmmheight = pScreen->mmHeight;
    oldshadow = priv->shadow;

    /*
     * Set new configuration
     */

    /*
     * We need to store the rotation value for pointer coords transformation;
     * though initially the pointer and fb rotation are identical, when we map
     * the fb, the screen will be reinitialized and return into an unrotated
     * state (presumably the HW is taking care of the rotation of the fb), but the
     * pointer still needs to be transformed.
     */
    ephyrRandr = KdAddRotation(pScrn->randr, randr);
    priv->randr = ephyrRandr;

    ephyrUnmapFramebuffer(pScrn);

    pScrn->virtualX = newwidth;
    pScrn->virtualY = newheight;

    if (!ephyrMapFramebuffer(pScrn)) {
        goto bail4;
    }

    /* FIXME below should go in own call */

    if (oldshadow) {
        KdShadowUnset(pScrn->pScreen);
    } else {
        ephyrUnsetInternalDamage(pScrn->pScreen);
    }

    if (priv->shadow) {
        if (!KdShadowSet(pScrn->pScreen,
                         priv->randr,
                         ephyrShadowUpdate, ephyrWindowLinear)) {
            goto bail4;
        }
    } else {
        /* Without shadow fb ( non rotated ) we need
         * to use damage to efficiently update display
         * via signal regions what to copy from 'fb'.
         */
        if (!ephyrSetInternalDamage(pScrn->pScreen)) {
            goto bail4;
        }
    }

    ephyrSetScreenSizes(pScrn->pScreen);

    /*
     * Set frame buffer mapping
     */
    (*pScreen->ModifyPixmapHeader)(fbGetScreenPixmap(pScreen),
                                   pScreen->width,
                                   pScreen->height,
                                   pScrn->fb.depth,
                                   pScrn->fb.bitsPerPixel,
                                   pScrn->fb.byteStride,
                                   pScrn->fb.frameBuffer);

    /* set the subpixel order */

    KdSetSubpixelOrder(pScreen, priv->randr);

    if (wasEnabled) {
        KdEnableScreen(pScreen);
    }

    RRScreenSizeNotify(pScreen);

    return TRUE;

bail4:
    EPHYR_LOG("bailed");

    ephyrUnmapFramebuffer(pScrn);
    *priv = oldscr;
    (void) ephyrMapFramebuffer(pScrn);

    pScreen->width = oldwidth;
    pScreen->height = oldheight;
    pScreen->mmWidth = oldmmwidth;
    pScreen->mmHeight = oldmmheight;

    if (wasEnabled) {
        KdEnableScreen(pScreen);
    }

    return FALSE;
}
#endif

Bool
ephyrRandRInit(ScreenPtr pScreen) {
    rrprivPtr ppriv;

    if (!RRScreenInit(pScreen)) {
        return FALSE;
    }

    ppriv = rrGetpriv(pScreen);
    ppriv->rrGetInfo = ephyrRandRGetInfo;
    ppriv->rrSetConfig = ephyrRandRSetConfig;
    return TRUE;
}

static Bool
ephyrResizeScreen (ScreenPtr pScreen,
                   int newwidth,
                   int newheight) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    RRScreenSize size = {0};
    Bool ret;
    int t;

    if (pScrn->randr & (RR_Rotate_90 | RR_Rotate_270)) {
        t = newwidth;
        newwidth = newheight;
        newheight = t;
    }

    if (newwidth == pScrn->virtualX && newheight == pScrn->virtualY) {
        return FALSE;
    }

    size.width = newwidth;
    size.height = newheight;

    ret = ephyrRandRSetConfig (pScreen, pScrn->randr, 0, &size);

    if (ret) {
        RROutputPtr output;

        output = RRFirstOutput(pScreen);

        if (!output) {
            return FALSE;
        }

        RROutputSetModes(output, NULL, 0, 0);
    }

    return ret;
}
#endif
#endif

Bool
ephyrCreateColormap(ColormapPtr pmap) {
    return fbInitializeColormap(pmap);
}

#if 0
Bool
ephyrInitScreen(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    EPHYR_LOG("pScreen->myNum:%d\n", pScreen->myNum);
    hostx_set_screen_number(pScrn, pScreen->myNum);

    if (EphyrWantNoHostGrab) {
        _ephyrSetWinTitle(priv, "xephyr");
    } else {
        _ephyrSetWinTitle(priv, "(ctrl+shift grabs mouse and keyboard)");
    }

    pScreen->CreateColormap = ephyrCreateColormap;

#ifdef XV
    if (!ephyrNoXV) {
        if (ephyr_glamor) {
            ephyr_glamor_xv_init(pScreen);
        } else if (!ephyrInitVideo(pScreen)) {
            EPHYR_LOG_ERROR("failed to initialize xvideo\n");
        } else {
            EPHYR_LOG("initialized xvideo okay\n");
        }
    }
#endif /*XV*/

    return TRUE;
}
#endif

#if 0
Bool
ephyrFinishInitScreen(ScreenPtr pScreen) {
    /* FIXME: Calling this even if not using shadow.
     * Seems harmless enough. But may be safer elsewhere.
     */
    if (!shadowSetup(pScreen)) {
        return FALSE;
    }

#ifdef RANDR
    if (!ephyrRandRInit(pScreen)) {
        return FALSE;
    }
#endif

    return TRUE;
}
#endif

/**
 * Called by kdrive after calling down the
 * pScreen->CreateScreenResources() chain, this gives us a chance to
 * make any pixmaps after the screen and all extensions have been
 * initialized.
 */
#if 0
Bool
ephyrCreateResources(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    EPHYR_LOG("mark pScreen=%p mynum=%d shadow=%d",
              pScreen, pScreen->myNum, priv->shadow);

    if (priv->shadow) {
        return KdShadowSet(pScreen,
                           priv->randr,
                           ephyrShadowUpdate, ephyrWindowLinear);
    } else {
#ifdef GLAMOR
        if (ephyr_glamor) {
            ephyr_glamor_create_screen_resources(pScreen);
        }
#endif
        return ephyrSetInternalDamage(pScreen);
    }
}
#endif

#if 0
void
ephyrPreserve(KdCardInfo * card) {
}
#endif

Bool
ephyrEnable(ScreenPtr pScreen) {
    return TRUE;
}

Bool
ephyrDPMS(ScreenPtr pScreen, int mode) {
    return TRUE;
}

void
ephyrDisable(ScreenPtr pScreen) {
}

#if 0
void
ephyrRestore(KdCardInfo * card) {
}
#endif

#if 0
void
ephyrScreenFini(ScrnInfoPtr pScrn) {
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    if (priv->shadow) {
        KdShadowFbFree(pScrn);
    }
}
#endif

static Bool
ephyrCursorOffScreen(ScreenPtr *ppScreen, int *x, int *y) {
    return FALSE;
}

static void
ephyrCrossScreen(ScreenPtr pScreen, Bool entering) {
}

ScreenPtr ephyrCursorScreen; /* screen containing the cursor */

static void
ephyrWarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y) {
    OsBlockSIGIO();
    ephyrCursorScreen = pScreen;
    miPointerWarpCursor(inputInfo.pointer, pScreen, x, y);

    OsReleaseSIGIO();
}

miPointerScreenFuncRec ephyrPointerScreenFuncs = {
    ephyrCursorOffScreen,
    ephyrCrossScreen,
    ephyrWarpCursor,
};


static ScreenPtr
screen_from_window(Window w) {
    int i = 0;

    for (i = 0; i < xf86NumScreens; i++) {
        ScreenPtr pScreen = xf86Screens[i]->pScreen;
        ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
        EphyrPrivatePtr priv = pScrn->driverPrivate;

        if (priv->win == w
            || priv->peer_win == w
            || priv->win_pre_existing == w) {
            return pScreen;
        }
    }

    return NULL;
}

static void
ephyrProcessErrorEvent(xcb_generic_event_t *xev) {
    xcb_generic_error_t *e = (xcb_generic_error_t *)xev;

    FatalError("X11 error\n"
               "Error code: %hhu\n"
               "Sequence number: %hu\n"
               "Major code: %hhu\tMinor code: %hu\n"
               "Error value: %u\n",
               e->error_code,
               e->sequence,
               e->major_code, e->minor_code,
               e->resource_id);
}

static void
ephyrProcessExpose(xcb_generic_event_t *xev) {
    xcb_expose_event_t *expose = (xcb_expose_event_t *) xev;
    ScreenPtr pScreen = screen_from_window(expose->window);
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    /* Wait for the last expose event in a series of cliprects
     * to actually paint our screen.
     */
    if (expose->count != 0) {
        return;
    }

    if (priv) {
        _ephyrPaintRect(priv, 0, 0, 0, 0,
                        priv->win_width,
                        priv->win_height);
    } else {
        EPHYR_LOG_ERROR("failed to get host screen\n");
    }
}

static void
ephyrProcessConfigureNotify(xcb_generic_event_t *xev) {
    xcb_configure_notify_event_t *configure =
        (xcb_configure_notify_event_t *) xev;
    ScreenPtr pScreen = screen_from_window(configure->window);
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    if (!priv ||
        (priv->win_pre_existing == None && !EphyrWantResize)) {
        return;
    }

#if 0
#ifdef RANDR
    ephyrResizeScreen(pScrn->pScreen, configure->width, configure->height);
#endif /* RANDR */
#endif
}

static void
ephyrProcessMouseMotion(xcb_generic_event_t *xev) {
}

static void
ephyrProcessKeyPress(xcb_generic_event_t *xev) {
}

static void
ephyrProcessKeyRelease(xcb_generic_event_t *xev) {
}

static void
ephyrProcessButtonPress(xcb_generic_event_t *xev) {
}

static void
ephyrProcessButtonRelease(xcb_generic_event_t *xev) {
}

static void
#if XORG_VERSION_MAJOR == 1 && XORG_VERSION_MINOR <= 18
ephyrPoll(ScrnInfoPtr pScrn) {
#else
ephyrXcbNotify(int fd, int ready, void *data) {
    ScrnInfoPtr pScrn = data;
#endif
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    xcb_generic_event_t *xev;

    while ((xev = xcb_poll_for_event(priv->conn)) != NULL) {
        switch (xev->response_type & 0x7f) {
        case 0:
            ephyrProcessErrorEvent(xev);
            break;

        case XCB_EXPOSE:
            ephyrProcessExpose(xev);
            break;

        case XCB_MOTION_NOTIFY:
            ephyrProcessMouseMotion(xev);
            break;

        case XCB_KEY_PRESS:
            ephyrProcessKeyPress(xev);
            break;

        case XCB_KEY_RELEASE:
            ephyrProcessKeyRelease(xev);
            break;

        case XCB_BUTTON_PRESS:
            ephyrProcessButtonPress(xev);
            break;

        case XCB_BUTTON_RELEASE:
            ephyrProcessButtonRelease(xev);
            break;

        case XCB_CONFIGURE_NOTIFY:
            ephyrProcessConfigureNotify(xev);
            break;
        }

        if (ephyr_glamor) {
            ephyr_glamor_process_event(xev);
        }

        free(xev);
    }

    /* If our XCB connection has died (for example, our window was
     * closed), exit now.
     */
    if (xcb_connection_has_error(priv->conn)) {
        CloseWellKnownConnections();
        OsCleanup(1);
        exit(1);
    }
}

#if 0
void
ephyrCardFini(KdCardInfo * card) {
    EphyrPriv *priv = card->driver;

    free(priv);
}
#endif

void
ephyrGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs) {
    /* XXX Not sure if this is right */

    EPHYR_LOG("mark");

    while (n--) {
        pdefs->red = 0;
        pdefs->green = 0;
        pdefs->blue = 0;
        pdefs++;
    }

}

void
ephyrPutColors(ScreenPtr pScreen, int n, xColorItem * pdefs) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    int min, max, p;

    /* XXX Not sure if this is right */

    min = 256;
    max = 0;

    while (n--) {
        p = pdefs->pixel;
        if (p < min)
            min = p;
        if (p > max)
            max = p;

        hostx_set_cmap_entry(pScreen, p,
                             pdefs->red >> 8,
                             pdefs->green >> 8, pdefs->blue >> 8);
        pdefs++;
    }

    if (priv->pDamage) {
        BoxRec box;
        RegionRec region;

        box.x1 = 0;
        box.y1 = 0;
        box.x2 = pScreen->width;
        box.y2 = pScreen->height;
        RegionInit(&region, &box, 1);
        DamageReportDamage(priv->pDamage, &region);
        RegionUninit(&region);
    }
}

/* End of former ephyr.c
 ****************************************************************
 * Beginning of driver.c */

typedef enum {
    OPTION_DISPLAY,
    OPTION_XAUTHORITY,
    OPTION_RESNAME,
#ifdef GLAMOR
    OPTION_NOACCEL,
    OPTION_ACCELMETHOD,
#endif
    OPTION_PARENTWINDOW,
    OPTION_FULLSCREEN,
    OPTION_OUTPUT,
    OPTION_ORIGIN,
    OPTION_SWCURSOR
} ephyrOpts;

typedef enum {
    EPHYR_CHIP
} ephyrType;

static SymTabRec EphyrChipsets[] = {
    { EPHYR_CHIP, "ephyr" },
    {-1,          NULL }
};

static OptionInfoRec EphyrOptions[] = {
    { OPTION_DISPLAY,      "Display",      OPTV_STRING,  {0}, FALSE },
    { OPTION_XAUTHORITY,   "Xauthority",   OPTV_STRING,  {0}, FALSE },
    { OPTION_RESNAME,      "ResourceName", OPTV_STRING,  {0}, FALSE },
#ifdef GLAMOR
    { OPTION_NOACCEL,      "NoAccel",      OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_ACCELMETHOD,  "AccelMethod",  OPTV_STRING,  {0}, FALSE },
#endif
    { OPTION_PARENTWINDOW, "ParentWindow", OPTV_INTEGER, {0}, FALSE },
    { OPTION_FULLSCREEN,   "Fullscreen",   OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_OUTPUT,       "Output",       OPTV_STRING,  {0}, FALSE },
    { OPTION_ORIGIN,       "Origin",       OPTV_STRING,  {0}, FALSE },
    { OPTION_SWCURSOR,     "SWCursor",     OPTV_BOOLEAN, {0}, FALSE },
    { -1,                  NULL,           OPTV_NONE,    {0}, FALSE }
};

static XF86ModuleVersionInfo EphyrVersRec = {
    EPHYR_DRIVER_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    EPHYR_MAJOR_VERSION,
    EPHYR_MINOR_VERSION,
    EPHYR_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}          /* checksum */
};

static Bool
ephyrEnterVT(VT_FUNC_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrEnterVT\n");
    return TRUE;
}

static void
ephyrLeaveVT(VT_FUNC_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrLeaveVT\n");
}

static Bool
ephyrSwitchMode(SWITCH_MODE_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrSwitchMode\n");
    return TRUE;
}

static void
ephyrAdjustFrame(ADJUST_FRAME_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrAdjustFrame\n");
}

static ModeStatus
ephyrValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode,
               Bool verbose, int flags) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrValidMode:\n");

    if (!mode) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "NULL MODE!\n");
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, " name: %s\n", mode->name);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, " HDisplay: %d\n", mode->HDisplay);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, " VDisplay: %d\n", mode->VDisplay);
    return MODE_OK;
}

static void
ephyrFreeScreen(FREE_SCREEN_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrFreeScreen\n");
}

static Bool
ephyrAddMode(ScrnInfoPtr pScrn, int width, int height) {
    DisplayModePtr mode;
    char nameBuf[64];

    if (snprintf(nameBuf, 64, "%dx%d", width, height) >= 64) {
        return FALSE;
    }

    mode = xnfcalloc(1, sizeof(DisplayModeRec));
    mode->status = MODE_OK;
    mode->type = M_T_DRIVER;
    mode->HDisplay = width;
    mode->VDisplay = height;
    mode->name = strdup(nameBuf);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Adding mode %s\n", mode->name);

    /* Now add mode to pScrn->modes. We'll keep the list non-circular for now,
     * but we'll maintain pScrn->modes->prev to know the last element */
    mode->next = NULL;

    if (!pScrn->modes) {
        pScrn->modes = mode;
        mode->prev = mode;
    } else {
        mode->prev = pScrn->modes->prev;
        pScrn->modes->prev->next = mode;
        pScrn->modes->prev = mode;
    }

    return TRUE;
}

static int
ephyrValidateModes(ScrnInfoPtr pScrn) {
    DisplayModePtr mode;
    int i, width, height, ret = 0;
    int maxX = 0, maxY = 0;

    /* Print useless stuff */
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Monitor wants these modes:\n");

    for (mode = pScrn->monitor->Modes; mode != NULL; mode = mode->next) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, " %s (%dx%d)\n", mode->name,
                   mode->HDisplay, mode->VDisplay);
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Too bad for it...\n");

    /* If user requested modes, add them. If not, use 640x480 */
    if (pScrn->display->modes != NULL && pScrn->display->modes[0] != NULL) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "User wants these modes:\n");

        for (i = 0; pScrn->display->modes[i] != NULL; i++) {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO, " %s\n",
                       pScrn->display->modes[i]);

            if (sscanf(pScrn->display->modes[i], "%dx%d", &width,
                       &height) != 2) {
                xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                           "This is not the mode name I was expecting...\n");
                return 0;
            }

            if (!ephyrAddMode(pScrn, width, height)) {
                return 0;
            }
        }
    } else if (!pScrn->modes) {
        if (!ephyrAddMode(pScrn, 640, 480)) {
            return 0;
        }
    }

    pScrn->modePool = NULL;

    /* Now set virtualX, virtualY, displayWidth and virtualFrom */
    if (pScrn->display->virtualX >= pScrn->modes->HDisplay &&
        pScrn->display->virtualY >= pScrn->modes->VDisplay) {
        pScrn->virtualX = pScrn->display->virtualX;
        pScrn->virtualY = pScrn->display->virtualY;
    } else {
        /* XXX: if not specified, make virtualX and virtualY as big as the max X
         * and Y. I'm not sure this is correct */
        mode = pScrn->modes;

        while (mode != NULL) {
            if (mode->HDisplay > maxX) {
                maxX = mode->HDisplay;
            }

            if (mode->VDisplay > maxY) {
                maxY = mode->VDisplay;
            }

            mode = mode->next;
        }

        pScrn->virtualX = maxX;
        pScrn->virtualY = maxY;
    }

    pScrn->virtualFrom = X_DEFAULT;
    pScrn->displayWidth = pScrn->virtualX;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Virtual size: %dx%d\n",
               pScrn->virtualX, pScrn->virtualY);

    /* Calculate the return value */
    mode = pScrn->modes;

    while (mode != NULL) {
        mode = mode->next;
        ret++;
    }

    /* Finally, make the mode list circular */
    pScrn->modes->prev->next = pScrn->modes;

    return ret;
}

static Bool
ephyrAllocatePrivate(ScrnInfoPtr pScrn) {
    if (pScrn->driverPrivate != NULL) {
        xf86Msg(X_WARNING, "ephyrAllocatePrivate called for an already "
                "allocated private!\n");
        return FALSE;
    }

    pScrn->driverPrivate = xnfcalloc(sizeof(EphyrPrivate), 1);

    if (pScrn->driverPrivate == NULL) {
        return FALSE;
    }

    return TRUE;
}

static void
_ephyrUseResname(const char *name, Bool fromconfig) {
    ephyrResName = name;
    ephyrResNameFromConfig = fromconfig;
}

static void
_ephyrSetFullscreenHint(EphyrPrivatePtr priv) {
    xcb_atom_t atom_WINDOW_STATE, atom_WINDOW_STATE_FULLSCREEN;
    xcb_intern_atom_cookie_t cookie_WINDOW_STATE,
                             cookie_WINDOW_STATE_FULLSCREEN;
    xcb_intern_atom_reply_t *reply;

    cookie_WINDOW_STATE = xcb_intern_atom(priv->conn, FALSE,
                                          strlen("_NET_WM_STATE"),
                                          "_NET_WM_STATE");
    cookie_WINDOW_STATE_FULLSCREEN =
        xcb_intern_atom(priv->conn, FALSE,
                        strlen("_NET_WM_STATE_FULLSCREEN"),
                        "_NET_WM_STATE_FULLSCREEN");
    reply = xcb_intern_atom_reply(priv->conn, cookie_WINDOW_STATE, NULL);
    atom_WINDOW_STATE = reply->atom;
    free(reply);

    reply = xcb_intern_atom_reply(priv->conn, cookie_WINDOW_STATE_FULLSCREEN,
                                  NULL);
    atom_WINDOW_STATE_FULLSCREEN = reply->atom;
    free(reply);

    xcb_change_property(priv->conn,
                        XCB_PROP_MODE_REPLACE,
                        priv->win,
                        atom_WINDOW_STATE,
                        XCB_ATOM_ATOM,
                        32,
                        1,
                        &atom_WINDOW_STATE_FULLSCREEN);
}

static Bool
_ephyrCheckExtension(EphyrPrivatePtr priv, xcb_extension_t *extension) {
    const xcb_query_extension_reply_t *rep = xcb_get_extension_data(priv->conn,
                                                                    extension);
    return rep && rep->present;
}

static void
_ephyrSetWinTitle(EphyrPrivatePtr priv, const char *extra_text) {
    if (!priv) {
        return;
    }

    if (ephyrTitle) {
        xcb_icccm_set_wm_name(priv->conn,
                              priv->win,
                              XCB_ATOM_STRING,
                              8,
                              strlen(ephyrTitle),
                              ephyrTitle);
    } else {
#define BUF_LEN 256
        char buf[BUF_LEN + 1];

        memset(buf, 0, BUF_LEN + 1);
        snprintf(buf, BUF_LEN, "Xorg on %s.%d %s",
                 priv->server_dpy_name ? priv->server_dpy_name : ":0",
                 priv->mynum, (extra_text != NULL) ? extra_text : "");

        xcb_icccm_set_wm_name(priv->conn,
                              priv->win,
                              XCB_ATOM_STRING,
                              8,
                              strlen(buf),
                              buf);
        xcb_flush(priv->conn);
    }
}

static Bool
_ephyrGetOutputGeometry(EphyrPrivatePtr priv) {
    Bool output_found = FALSE;

    if (priv->use_fullscreen) {
        xcb_screen_t *xscreen = xcb_aux_get_screen(priv->conn, priv->screen);
        priv->win_width = xscreen->width_in_pixels;
        priv->win_height = xscreen->height_in_pixels;
        priv->win_explicit_position = TRUE;
        output_found = TRUE;
    } else if (priv->output != NULL) {
        int i, name_len = 0;
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
        if (!_ephyrCheckExtension(priv, &xcb_randr_id)) {
            return FALSE;
        }

        /* Check RandR version */
        version_c = xcb_randr_query_version(priv->conn, 1, 2);
        version_r = xcb_randr_query_version_reply(priv->conn,
                                                  version_c,
                                                  &error);

        if (error != NULL || version_r == NULL) {
            return FALSE;
        } else if (version_r->major_version < 1 ||
                   (version_r->major_version == 1 && version_r->minor_version < 2)) {
            free(version_r);
            return FALSE;
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

            if (!strcmp(name, priv->output)) {
                output_found = TRUE;

                /* Check if output is connected */
                if (output_info_r->crtc == XCB_NONE) {
                    free(name);
                    free(output_info_r);
                    free(screen_resources_r);
                    return FALSE;
                }

                /* Get CRTC from output info */
                crtc_info_c = xcb_randr_get_crtc_info(priv->conn,
                                                      output_info_r->crtc,
                                                      XCB_CURRENT_TIME);
                crtc_info_r = xcb_randr_get_crtc_info_reply(priv->conn,
                                                            crtc_info_c,
                                                            NULL);

                /* Get CRTC geometry */
                priv->win_x = crtc_info_r->x;
                priv->win_y = crtc_info_r->y;
                priv->win_width = crtc_info_r->width;
                priv->win_height = crtc_info_r->height;

                priv->win_explicit_position = TRUE;
                priv->use_fullscreen = TRUE;
                free(crtc_info_r);
            }

            free(name);
            free(output_info_r);
        }

        free(screen_resources_r);
    }

    return output_found;
}

/* Data from here is valid to all server generations */
static Bool
ephyrPreInit(ScrnInfoPtr pScrn, int flags) {
    EphyrPrivatePtr priv;
    const char *displayName = getenv("DISPLAY");
    const char *accelMethod = NULL;
    Bool noAccel = FALSE;
    xcb_pixmap_t cursor_pxm;
    xcb_gcontext_t cursor_gc;
    uint16_t red, green, blue;
    uint32_t pixel;
    int index;
    xcb_screen_t *xscreen;
    xcb_rectangle_t rect = { 0, 0, 1, 1 };
    uint32_t attrs[2];
    uint32_t attr_mask = 0;
    char *tmpstr;
    char *class_hint;
    size_t class_len;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrPreInit\n");

    if (flags & PROBE_DETECT) {
        return FALSE;
    }

    if (!ephyrAllocatePrivate(pScrn)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to allocate private\n");
        return FALSE;
    } else {
        priv = pScrn->driverPrivate;
        priv->output = NULL;
        priv->use_fullscreen = FALSE;
        priv->use_sw_cursor = TRUE;
        priv->win_pre_existing = 0;
        priv->win_explicit_position = FALSE;
        priv->win_x = 0;
        priv->win_y = 0;
        priv->ximg = NULL;
    }

    if (!xf86SetDepthBpp(pScrn, 0, 0, 0, Support24bppFb | Support32bppFb)) {
        return FALSE;
    }

    xf86PrintDepthBpp(pScrn);

    if (pScrn->depth > 8) {
        rgb zeros = {0, 0, 0};

        if (!xf86SetWeight(pScrn, zeros, zeros)) {
            return FALSE;
        }
    }

    if (!xf86SetDefaultVisual(pScrn, -1)) {
        return FALSE;
    }

    pScrn->monitor = pScrn->confScreen->monitor; /* XXX */

    xf86CollectOptions(pScrn, NULL);
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, EphyrOptions);

    if (xf86IsOptionSet(EphyrOptions, OPTION_DISPLAY)) {
        displayName = xf86GetOptValString(EphyrOptions,
                                          OPTION_DISPLAY);
        setenv("DISPLAY", displayName, TRUE);
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Using display \"%s\"\n",
                   displayName);
    }

    if (xf86IsOptionSet(EphyrOptions, OPTION_XAUTHORITY)) {
        setenv("XAUTHORITY",
               xf86GetOptValString(EphyrOptions,
                                   OPTION_XAUTHORITY),
               TRUE);
    }

    if (xf86IsOptionSet(EphyrOptions, OPTION_RESNAME)) {
        _ephyrUseResname(xf86GetOptValString(EphyrOptions,
                                             OPTION_RESNAME),
                          TRUE);
    } else {
        _ephyrUseResname(xf86ServerName, FALSE);
    }

#ifdef GLAMOR
    if (xf86IsOptionSet(EphyrOptions, OPTION_ACCELMETHOD)) {
        accelMethod = xf86GetOptValString(EphyrOptions,
                                          OPTION_ACCELMETHOD);
        xf86GetOptValBool(EphyrOptions,
                          OPTION_NOACCEL,
                          &noAccel);

        if (!noAccel) {
            if (!xf86nameCompare(accelMethod, "glamor")) {
                ephyr_glamor = TRUE;
            } else if (!xf86nameCompare(accelMethod, "glamor_gles2")) {
                ephyr_glamor = TRUE;
                ephyr_glamor_gles2 = TRUE;
            }

            if (ephyr_glamor) {
                xf86DrvMsg(pScrn->scrnIndex, X_INFO, "GLAMOR support enabled.");
            }
        }
    }
#endif

    if (xf86GetOptValInteger(EphyrOptions,
                             OPTION_PARENTWINDOW,
                             &priv->win_pre_existing)) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Targeting parent window 0x%x\n",
                   priv->win_pre_existing);
    }

    if (xf86GetOptValBool(EphyrOptions,
                          OPTION_FULLSCREEN,
                          &priv->use_fullscreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Fullscreen mode %s\n",
                   priv->use_fullscreen ? "enabled" : "disabled");
    }

    if (xf86IsOptionSet(EphyrOptions, OPTION_OUTPUT)) {
        priv->output = xf86GetOptValString(EphyrOptions,
                                           OPTION_OUTPUT);
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "Targeting host X server output \"%s\"\n",
                   priv->output);
    }

    if (xf86IsOptionSet(EphyrOptions, OPTION_ORIGIN)) {
        if (sscanf(xf86GetOptValString(EphyrOptions, OPTION_ORIGIN),
                   "%d %d", &priv->win_x, &priv->win_y) != 2) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Invalid value for option \"Origin\"\n");
            return FALSE;
        }

        priv->win_explicit_position = TRUE;
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Using origin x:%d y:%d\n",
                   priv->win_x, priv->win_y);
    }

    if (xf86GetOptValBool(EphyrOptions,
                          OPTION_SWCURSOR,
                          &priv->use_sw_cursor)) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Software cursor %s\n",
                   priv->use_sw_cursor ? "enabled" : "disabled");
    }

    xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

/******************** hostx_init *********************/
#ifdef GLAMOR
    if (ephyr_glamor) {
        priv->conn = ephyr_glamor_connect();
    } else
#endif
    {
        priv->conn = xcb_connect(NULL, &priv->screen);
    }

    if (!priv->conn || xcb_connection_has_error(priv->conn)) {
        priv->conn = NULL;
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Can't open display: %s\n",
                   displayName);
        return FALSE;
    }

    xscreen = xcb_aux_get_screen(priv->conn, priv->screen);
    priv->winroot = xscreen->root;
    priv->gc = xcb_generate_id(priv->conn);
    priv->depth = xscreen->root_depth;

#ifdef GLAMOR
    if (ephyr_glamor) {
        priv->visual = ephyr_glamor_get_visual();
    } else
#endif
    {
        priv->visual = xcb_aux_find_visual_by_id(xscreen, xscreen->root_visual);
    }

    xcb_create_gc(priv->conn, priv->gc, priv->winroot, 0, NULL);

    if (!xcb_aux_parse_color("red", &red, &green, &blue)) {
        xcb_lookup_color_cookie_t c =
            xcb_lookup_color(priv->conn, xscreen->default_colormap, 3, "red");
        xcb_lookup_color_reply_t *reply =
            xcb_lookup_color_reply(priv->conn, c, NULL);
        red = reply->exact_red;
        green = reply->exact_green;
        blue = reply->exact_blue;
        free(reply);
    }

    {
        xcb_alloc_color_cookie_t c = xcb_alloc_color(priv->conn,
                                                     xscreen->default_colormap,
                                                     red, green, blue);
        xcb_alloc_color_reply_t *r = xcb_alloc_color_reply(priv->conn, c, NULL);
        red = r->red;
        green = r->green;
        blue = r->blue;
        pixel = r->pixel;
        free(r);
    }

    xcb_change_gc(priv->conn, priv->gc, XCB_GC_FOREGROUND, &pixel);

    cursor_pxm = xcb_generate_id(priv->conn);
    xcb_create_pixmap(priv->conn, 1, cursor_pxm, priv->winroot, 1, 1);
    cursor_gc = xcb_generate_id(priv->conn);
    pixel = 0;
    xcb_create_gc(priv->conn, cursor_gc, cursor_pxm,
                  XCB_GC_FOREGROUND, &pixel);
    xcb_poly_fill_rectangle(priv->conn, cursor_pxm, cursor_gc, 1, &rect);
    xcb_free_gc(priv->conn, cursor_gc);
    priv->empty_cursor = xcb_generate_id(priv->conn);
    xcb_create_cursor(priv->conn,
                      priv->empty_cursor,
                      cursor_pxm, cursor_pxm,
                      0,0,0,
                      0,0,0,
                      1,1);
    xcb_free_pixmap(priv->conn, cursor_pxm);

    /* XXX: investigate this
    if (!hostx_want_host_cursor ()) {
        CursorVisible = TRUE;
    }*/

    /* Try to get share memory ximages for a little bit more speed */
    if (!_ephyrCheckExtension(priv, &xcb_shm_id) || getenv("XEPHYR_NO_SHM")) {
        fprintf(stderr, "\nNested Xorg unable to use SHM XImages\n");
        priv->have_shm = FALSE;
    } else {
        /* Really really check we have shm - better way ?*/
        xcb_shm_segment_info_t shminfo;
        xcb_generic_error_t *e;
        xcb_void_cookie_t cookie;
        xcb_shm_seg_t shmseg;

        priv->have_shm = TRUE;

        shminfo.shmid = shmget(IPC_PRIVATE, 1, IPC_CREAT|0777);
        shminfo.shmaddr = shmat(shminfo.shmid,0,0);

        shmseg = xcb_generate_id(priv->conn);
        cookie = xcb_shm_attach_checked(priv->conn, shmseg, shminfo.shmid,
                                        TRUE);
        e = xcb_request_check(priv->conn, cookie);

        if (e) {
            fprintf(stderr, "\nNested Xorg unable to use SHM XImages\n");
            priv->have_shm = FALSE;
            free(e);
        }

        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, 0);
    }

    xcb_flush(priv->conn);

    /* Setup the pause time between paints when debugging updates */

    priv->damage_debug_msec = 20000;    /* 1/50 th of a second */

    if (getenv("XEPHYR_PAUSE")) {
        priv->damage_debug_msec = strtol(getenv("XEPHYR_PAUSE"), NULL, 0);
        EPHYR_DBG("pause is %li\n", priv->damage_debug_msec);
    }
/******************** hostx_init *********************/

    /* TODO: replace with corresponding Xephyr function.
      if (!ephyrClientValidDepth(pScrn->depth)) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Invalid depth: %d\n",
            pScrn->depth);
      return FALSE;
      }*/

    if (_ephyrGetOutputGeometry(priv)) {
        if (!ephyrAddMode(pScrn, priv->win_width, priv->win_height)) {
            xf86DrvMsg(pScrn->scrnIndex,
                       X_WARNING,
                       "Failed to get fullscreen dimensions for display %s. Skipping.\n",
                       displayName);
        }
    }

    if (ephyrValidateModes(pScrn) < 1) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes\n");
        return FALSE;
    }

    if (!pScrn->modes) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
        return FALSE;
    }

    if (!priv->win_width) {
        priv->win_width = pScrn->virtualX;
    }

    if (!priv->win_height) {
        priv->win_height = pScrn->virtualY;
    }

    xf86SetCrtcForModes(pScrn, 0);

    pScrn->currentMode = pScrn->modes;

    xf86SetDpi(pScrn, 0, 0);

    if (!xf86LoadSubModule(pScrn, "shadow")) {
        return FALSE;
    }

    if (!xf86LoadSubModule(pScrn, "fb")) {
        return FALSE;
    }

    pScrn->memPhysBase = 0;
    pScrn->fbOffset = 0;

/********************* hostx_init_window *************/
    attrs[0] = XCB_EVENT_MASK_BUTTON_PRESS
             | XCB_EVENT_MASK_BUTTON_RELEASE
             | XCB_EVENT_MASK_POINTER_MOTION
             | XCB_EVENT_MASK_KEY_PRESS
             | XCB_EVENT_MASK_KEY_RELEASE
             | XCB_EVENT_MASK_EXPOSURE
             | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    attr_mask |= XCB_CW_EVENT_MASK;
    xscreen = xcb_aux_get_screen(priv->conn, priv->screen);
    priv->win = xcb_generate_id(priv->conn);
    priv->server_depth = priv->depth;

#ifdef GLAMOR
    if (priv->visual->visual_id != xscreen->root_visual) {
        attrs[1] = xcb_generate_id(priv->conn);
        attr_mask |= XCB_CW_COLORMAP;
        xcb_create_colormap(priv->conn,
                            XCB_COLORMAP_ALLOC_NONE,
                            attrs[1],
                            priv->winroot,
                            priv->visual->visual_id);
    }
#endif

    if (priv->win_pre_existing != XCB_WINDOW_NONE) {
        xcb_get_geometry_reply_t *prewin_geom;
        xcb_get_geometry_cookie_t cookie;
        xcb_generic_error_t *e = NULL;

        /* Get screen size from existing window */
        cookie = xcb_get_geometry(priv->conn,
                                  priv->win_pre_existing);
        prewin_geom = xcb_get_geometry_reply(priv->conn, cookie, &e);

        if (e) {
            free(e);
            free(prewin_geom);
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Can't create window on display: %s\n",
                       displayName);
            return FALSE;
        }

        priv->win_width  = prewin_geom->width;
        priv->win_height = prewin_geom->height;

        free(prewin_geom);

        xcb_create_window(priv->conn,
                          XCB_COPY_FROM_PARENT,
                          priv->win,
                          priv->win_pre_existing,
                          0, 0,
                          priv->win_width,
                          priv->win_height,
                          0,
                          XCB_WINDOW_CLASS_COPY_FROM_PARENT,
                          priv->visual->visual_id,
                          attr_mask,
                          attrs);
    } else {
        xcb_create_window(priv->conn,
                          XCB_COPY_FROM_PARENT,
                          priv->win,
                          priv->winroot,
                          0, 0, /* Window placement will be set after mapping */
                          priv->win_width,
                          priv->win_height,
                          0,
                          XCB_WINDOW_CLASS_COPY_FROM_PARENT,
                          priv->visual->visual_id,
                          attr_mask,
                          attrs);

        _ephyrSetWinTitle(priv,
                          "(ctrl+shift grabs mouse and keyboard)");

        tmpstr = getenv("RESOURCE_NAME");

        if (tmpstr && (!ephyrResNameFromConfig)) {
            ephyrResName = tmpstr;
        }

        class_len = strlen(ephyrResName) + 1 + strlen("Xorg") + 1;
        class_hint = malloc(class_len);

        if (class_hint) {
            strcpy(class_hint, ephyrResName);
            strcpy(class_hint + strlen(ephyrResName) + 1, "Xorg");
            xcb_change_property(priv->conn,
                                XCB_PROP_MODE_REPLACE,
                                priv->win,
                                XCB_ATOM_WM_CLASS,
                                XCB_ATOM_STRING,
                                8,
                                class_len,
                                class_hint);
            free(class_hint);
        }
    }

    if (!priv->use_sw_cursor) {
        /* Ditch the cursor, we provide our 'own' */
        xcb_change_window_attributes(priv->conn,
                                     priv->win,
                                     XCB_CW_CURSOR,
                                     &priv->empty_cursor);
    }
/********************* hostx_init_window *************/

    return TRUE;
}

static void
ephyrShadowUpdate(ScreenPtr pScreen, shadowBufPtr pBuf) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    EPHYR_LOG("slow paint");

    /* FIXME: Slow Rotated/Reflected updates could be much
     * much faster efficiently updating via tranforming
     * pBuf->pDamage regions
     */
    //shadowUpdateRotatePacked(pScreen, pBuf);
    _ephyrPaintRect(priv, 0, 0, 0, 0, pScrn->virtualX, pScrn->virtualY);
}

static Bool
ephyrSaveScreen(ScreenPtr pScreen, int mode) {
    xf86DrvMsg(pScreen->myNum, X_INFO, "ephyrSaveScreen\n");
    return TRUE;
}

static Bool
ephyrCreateScreenResources(ScreenPtr pScreen) {
    xf86DrvMsg(pScreen->myNum, X_INFO, "ephyrCreateScreenResources\n");
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    Bool ret;

    pScreen->CreateScreenResources = priv->CreateScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = ephyrCreateScreenResources;

    if (!shadowAdd(pScreen, pScreen->GetScreenPixmap(pScreen),
                   priv->update, NULL, 0, 0)) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                   "ephyrCreateScreenResources failed to shadowAdd.\n");
        return FALSE;
    }

    return ret;
}

static void
ephyrPrintMode(ScrnInfoPtr p, DisplayModePtr m) {
    xf86DrvMsg(p->scrnIndex, X_INFO, "HDisplay   %d\n", m->HDisplay);
    xf86DrvMsg(p->scrnIndex, X_INFO, "HSyncStart %d\n", m->HSyncStart);
    xf86DrvMsg(p->scrnIndex, X_INFO, "HSyncEnd   %d\n", m->HSyncEnd);
    xf86DrvMsg(p->scrnIndex, X_INFO, "HTotal     %d\n", m->HTotal);
    xf86DrvMsg(p->scrnIndex, X_INFO, "HSkew      %d\n", m->HSkew);
    xf86DrvMsg(p->scrnIndex, X_INFO, "VDisplay   %d\n", m->VDisplay);
    xf86DrvMsg(p->scrnIndex, X_INFO, "VSyncStart %d\n", m->VSyncStart);
    xf86DrvMsg(p->scrnIndex, X_INFO, "VSyncEnd   %d\n", m->VSyncEnd);
    xf86DrvMsg(p->scrnIndex, X_INFO, "VTotal    %d\n",  m->VTotal);
    xf86DrvMsg(p->scrnIndex, X_INFO, "VScan     %d\n",  m->VScan);
}

static void
ephyrPrintPscreen(ScrnInfoPtr p) {
    /* XXX: finish implementing this someday? */
    xf86DrvMsg(p->scrnIndex, X_INFO, "Printing pScrn:\n");
    xf86DrvMsg(p->scrnIndex, X_INFO, "driverVersion:  %d\n", p->driverVersion);
    xf86DrvMsg(p->scrnIndex, X_INFO, "driverName:     %s\n", p->driverName);
    xf86DrvMsg(p->scrnIndex, X_INFO, "pScreen:        %p\n", p->pScreen);
    xf86DrvMsg(p->scrnIndex, X_INFO, "scrnIndex:      %d\n", p->scrnIndex);
    xf86DrvMsg(p->scrnIndex, X_INFO, "configured:     %d\n", p->configured);
    xf86DrvMsg(p->scrnIndex, X_INFO, "origIndex:      %d\n", p->origIndex);
    xf86DrvMsg(p->scrnIndex, X_INFO, "imageByteOrder: %d\n", p->imageByteOrder);
    /*xf86DrvMsg(p->scrnIndex, X_INFO, "bitmapScanlineUnit: %d\n");
      xf86DrvMsg(p->scrnIndex, X_INFO, "bitmapScanlinePad: %d\n");
      xf86DrvMsg(p->scrnIndex, X_INFO, "bitmapBitOrder: %d\n");
      xf86DrvMsg(p->scrnIndex, X_INFO, "numFormats: %d\n");
      xf86DrvMsg(p->scrnIndex, X_INFO, "formats[]: 0x%x\n");
      xf86DrvMsg(p->scrnIndex, X_INFO, "fbFormat: 0x%x\n"); */
    xf86DrvMsg(p->scrnIndex, X_INFO, "bitsPerPixel:   %d\n", p->bitsPerPixel);
    /*xf86DrvMsg(p->scrnIndex, X_INFO, "pixmap24: 0x%x\n"); */
    xf86DrvMsg(p->scrnIndex, X_INFO, "depth:          %d\n", p->depth);
    ephyrPrintMode(p, p->currentMode);
    /*xf86DrvMsg(p->scrnIndex, X_INFO, "depthFrom: %\n");
      xf86DrvMsg(p->scrnIndex, X_INFO, "\n");*/
}

/* From former ephyr.c
  void
  ephyrCloseScreen(ScreenPtr pScreen)
  {
  ephyrUnsetInternalDamage(pScreen);
  }
 */

#if XORG_VERSION_MAJOR == 1 && XORG_VERSION_MINOR <= 18
static void
ephyrBlockHandler(pointer data, OSTimePtr wt, pointer LastSelectMask) {
    ScrnInfoPtr pScrn = data;
    ephyrPoll(pScrn);
}

static void
ephyrWakeupHandler(pointer data, int i, pointer LastSelectMask) {
}
#endif

static Bool
ephyrCloseScreen(CLOSE_SCREEN_ARGS_DECL) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrCloseScreen\n");

    shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));

#if XORG_VERSION_MAJOR == 1 && XORG_VERSION_MINOR <= 18
    RemoveBlockAndWakeupHandlers(ephyrBlockHandler,
                                 ephyrWakeupHandler,
                                 pScrn);
#else
    RemoveNotifyFd(xcb_get_file_descriptor(priv->conn));
#endif

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

    pScreen->CloseScreen = priv->CloseScreen;
    return (*pScreen->CloseScreen)(CLOSE_SCREEN_ARGS);
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

/* Called at each server generation */
static Bool
ephyrScreenInit(SCREEN_INIT_ARGS_DECL) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrPrivatePtr priv = pScrn->driverPrivate;
    Pixel redMask, greenMask, blueMask;
    Bool shm_success = FALSE;
    char *fb_data = NULL;
    unsigned int buffer_height = ephyrBufferHeight(pScrn);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrScreenInit\n");
    ephyrPrintPscreen(pScrn);

/*********************** hostx_screen_init ***************************/
    if (!ephyr_glamor && priv->have_shm) {
        priv->ximg = xcb_image_create_native(priv->conn,
                                             priv->win_width,
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
                  priv->win_width, buffer_height, priv);
        priv->ximg = xcb_image_create_native(priv->conn,
                                             priv->win_width,
                                             buffer_height,
                                             XCB_IMAGE_FORMAT_Z_PIXMAP,
                                             priv->depth,
                                             NULL,
                                             ~0,
                                             NULL);

        /* Match server byte order so that the image can be converted to
         * the native byte order by xcb_image_put() before drawing */
        if (HOST_DEPTH_MATCHES_SERVER(priv)) {
            priv->ximg->byte_order = IMAGE_BYTE_ORDER;
        }

        priv->ximg->data =
            xallocarray(priv->ximg->stride, buffer_height);
    }

    if (priv->win_pre_existing == None && !EphyrWantResize) {
        /* Ask the WM to keep our size static */
        xcb_size_hints_t size_hints = {0};
        size_hints.max_width = size_hints.min_width = priv->win_width;
        size_hints.max_height = size_hints.min_height = priv->win_height;
        size_hints.flags = (XCB_ICCCM_SIZE_HINT_P_MIN_SIZE |
                            XCB_ICCCM_SIZE_HINT_P_MAX_SIZE);
        xcb_icccm_set_wm_normal_hints(priv->conn, priv->win,
                                      &size_hints);
    }

    if (priv->use_fullscreen) {
        _ephyrSetFullscreenHint(priv);
    }

    xcb_map_window(priv->conn, priv->win);

    /* Set explicit window position if it was informed in
     * "Origin" option (or retrieved from "Output" option).
     * Otherwise, accept the position set by WM.
     * The trick here is putting this code after xcb_map_window() call,
     * so these values won't be overriden by WM. */
    if (priv->win_explicit_position) {
        uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
        uint32_t values[2] = {priv->win_x, priv->win_y};
        xcb_configure_window(priv->conn, priv->win, mask, values);
    }

    xcb_aux_sync(priv->conn);

    #ifdef GLAMOR
    if (ephyr_glamor) {
        /* *bytes_per_line = 0; */
        pScrn->bitsPerPixel = 0;
        ephyr_glamor_set_window_size(priv->glamor,
                                     priv->win_width, priv->win_height);
    } else
    #endif
    {
        if (HOST_DEPTH_MATCHES_SERVER(priv)) {
            /*
            if (bytes_per_line != NULL) {
                *bytes_per_line = priv->ximg->stride;
            }
            */

            pScrn->bitsPerPixel = priv->ximg->bpp;

            EPHYR_DBG("Host matches server");
            fb_data = priv->ximg->data;
        } else {
            int bytes_per_pixel = priv->server_depth >> 3;
            int stride = (priv->win_width * bytes_per_pixel + 0x3) & ~0x3;

            /*
            if (bytes_per_line != NULL) {
                *bytes_per_line = stride;
            }
            */

            pScrn->bitsPerPixel = priv->server_depth;

            EPHYR_DBG("server bpp %i", bytes_per_pixel);
            priv->fb_data = xallocarray(stride, buffer_height);
            fb_data = priv->fb_data;
        }
    }
/*********************** hostx_screen_init ***************************/

    miClearVisualTypes();

    if (!miSetVisualTypesAndMasks(pScrn->depth,
                                  miGetDefaultVisualMask(pScrn->depth),
                                  pScrn->rgbBits, pScrn->defaultVisual,
                                  redMask, greenMask, blueMask)) {
        return FALSE;
    }

    if (!miSetPixmapDepths()) {
        return FALSE;
    }

    if (!fbScreenInit(pScreen,
                      fb_data,
                      priv->win_width, priv->win_height, pScrn->xDpi,
                      pScrn->yDpi, pScrn->displayWidth, pScrn->bitsPerPixel)) {
        return FALSE;
    }

    fbPictureInit(pScreen, 0, 0);

    xf86SetBlackWhitePixels(pScreen);
    xf86SetBackingStore(pScreen);
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    if (!miCreateDefColormap(pScreen)) {
        return FALSE;
    }

    priv->update = ephyrShadowUpdate;
    pScreen->SaveScreen = ephyrSaveScreen;

    if (!shadowSetup(pScreen)) {
        return FALSE;
    }

    priv->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = ephyrCreateScreenResources;

    priv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = ephyrCloseScreen;

#if XORG_VERSION_MAJOR == 1 && XORG_VERSION_MINOR <= 18
    RegisterBlockAndWakeupHandlers(ephyrBlockHandler,
                                   ephyrWakeupHandler,
                                   pScrn);
#else
    SetNotifyFd(xcb_get_file_descriptor(priv->conn),
                ephyrXcbNotify, X_NOTIFY_READ, NULL);
#endif

    return TRUE;
}

static void
ephyrIdentify(int flags) {
    xf86PrintChipsets(EPHYR_NAME, "Driver for nested servers",
                      EphyrChipsets);
}

static Bool
ephyrProbe(DriverPtr drv, int flags) {
    Bool foundScreen = FALSE;
    int numDevSections;
    GDevPtr *devSections;
    int i;

    ScrnInfoPtr pScrn;
    int entityIndex;

    if (flags & PROBE_DETECT) {
        return FALSE;
    }

    if ((numDevSections = xf86MatchDevice(EPHYR_DRIVER_NAME,
                                          &devSections)) <= 0) {
        return FALSE;
    }

    if (numDevSections > 0) {
        for(i = 0; i < numDevSections; i++) {
            pScrn = NULL;
            entityIndex = xf86ClaimNoSlot(drv, EPHYR_CHIP, devSections[i],
                                          TRUE);
            pScrn = xf86AllocateScreen(drv, 0);

            if (pScrn) {
                xf86AddEntityToScreen(pScrn, entityIndex);
                pScrn->driverVersion = EPHYR_VERSION;
                pScrn->driverName  = EPHYR_DRIVER_NAME;
                pScrn->name = EPHYR_NAME;
                pScrn->Probe = ephyrProbe;
                pScrn->PreInit = ephyrPreInit;
                pScrn->ScreenInit = ephyrScreenInit;
                pScrn->SwitchMode = ephyrSwitchMode;
                pScrn->AdjustFrame = ephyrAdjustFrame;
                pScrn->EnterVT = ephyrEnterVT;
                pScrn->LeaveVT = ephyrLeaveVT;
                pScrn->FreeScreen = ephyrFreeScreen;
                pScrn->ValidMode = ephyrValidMode;
                foundScreen = TRUE;
            }
        }
    }

    return foundScreen;
}

static const OptionInfoRec *
ephyrAvailableOptions(int chipid, int busid) {
    return EphyrOptions;
}

static Bool
ephyrDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op, void *data) {
    xorgHWFlags *flag;
    xf86Msg(X_INFO, "ephyrDriverFunc\n");

    /* XXX implement */
    switch(op) {
    case GET_REQUIRED_HW_INTERFACES:
        flag = data;
        *flag = HW_SKIP_CONSOLE;
        return TRUE;

    case RR_GET_INFO:
    case RR_SET_CONFIG:
    case RR_GET_MODE_MM:
    default:
        return FALSE;
    }
}

static void
ephyrFreePrivate(ScrnInfoPtr pScrn) {
    if (pScrn->driverPrivate == NULL) {
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                   "Double freeing ephyrPrivate!\n");
        return;
    }

    free(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

_X_EXPORT DriverRec EPHYR = {
    EPHYR_VERSION,
    EPHYR_DRIVER_NAME,
    ephyrIdentify,
    ephyrProbe,
    ephyrAvailableOptions,
    NULL, /* module */
    0,    /* refCount */
    ephyrDriverFunc,
    NULL, /* DeviceMatch */
    0     /* PciProbe */
};

static pointer
ephyrSetup(pointer module, pointer opts, int *errmaj, int *errmin) {
    static Bool setupDone = FALSE;

    if (!setupDone) {
        setupDone = TRUE;
        xf86AddDriver(&EPHYR, module, HaveDriverFuncs);
        return (pointer) 1;
    } else {
        if (errmaj) {
            *errmaj = LDR_ONCEONLY;
        }

        return NULL;
    }
}

_X_EXPORT XF86ModuleData ephyrModuleData = {
    &EphyrVersRec,
    ephyrSetup,
    NULL, /* teardown */
};
