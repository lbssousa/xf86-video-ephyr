/*
 * Xephyr - A kdrive X server thats runs in a host X window.
 *          Authored by Matthew Allum <mallum@openedhand.com>
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

#include "ephyr.h"

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
 #include <fb.h>
 #include <micmap.h>
 #include <mipointer.h>
 #include <shadow.h>
 #include <xf86.h>
 #include <xf86Module.h>
 #include <xf86str.h>

 #include "compat-api.h"

 #define EPHYR_VERSION 0
 #define EPHYR_NAME "EPHYR"
 #define EPHYR_DRIVER_NAME "ephyr"

 #define EPHYR_MAJOR_VERSION PACKAGE_VERSION_MAJOR
 #define EPHYR_MINOR_VERSION PACKAGE_VERSION_MINOR
 #define EPHYR_PATCHLEVEL PACKAGE_VERSION_PATCHLEVEL

 #define TIMER_CALLBACK_INTERVAL 20

 #ifndef HW_SKIP_CONSOLE
 #define HW_SKIP_CONSOLE 4
 #endif

/* End of driver.c includes
 ******************************************************
 * Beginning of former ephyr.c */
extern Bool ephyr_glamor;

Bool ephyrNoDRI = FALSE;
Bool ephyrNoXV = FALSE;

static int mouseState = 0;
static Rotation ephyrRandr = RR_Rotate_0;

Bool EphyrWantGrayScale = 0;
Bool EphyrWantResize = 0;
Bool EphyrWantNoHostGrab = 0;

#if 0
Bool
ephyrInitialize(KdCardInfo * card, EphyrPriv * priv)
{
    OsSignal(SIGUSR1, hostx_handle_signal);

    priv->base = 0;
    priv->bytes_per_line = 0;
    return TRUE;
}

Bool
ephyrCardInit(KdCardInfo * card)
{
    EphyrPriv *priv;

    priv = (EphyrPriv *) malloc(sizeof(EphyrPriv));
    if (!priv)
        return FALSE;

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
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
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

    if (EphyrWantGrayScale)
        pScrn->depth = 8;

    if (pScrn->depth && pScrn->depth != hostx_get_depth()) {
        if (pScrn->depth < hostx_get_depth()
            && (pScrn->depth == 24 || pScrn->depth == 16
                || pScrn->depth == 8)) {
            scrpriv->server_depth = pScrn->depth;
        }
        else
            ErrorF
                ("\nXephyr: requested screen depth not supported, setting to match hosts.\n");
    }

    pScrn->depth = hostx_get_server_depth(pScrn);
    pScrn->rate = 72;

    if (pScrn->depth <= 8) {
        if (EphyrWantGrayScale)
            pScrn->fb.visuals = ((1 << StaticGray) | (1 << GrayScale));
        else
            pScrn->fb.visuals = ((1 << StaticGray) |
                                  (1 << GrayScale) |
                                  (1 << StaticColor) |
                                  (1 << PseudoColor) |
                                  (1 << TrueColor) | (1 << DirectColor));

        pScrn->mask.red = 0x00;
        pScrn->mask.green = 0x00;
        pScrn->mask.blue = 0x00;
        pScrn->depth = 8;
        pScrn->bitsPerPixel = 8;
    }
    else {
        pScrn->fb.visuals = (1 << TrueColor);

        if (pScrn->depth <= 15) {
            pScrn->depth = 15;
            pScrn->bitsPerPixel = 16;
        }
        else if (pScrn->depth <= 16) {
            pScrn->depth = 16;
            pScrn->bitsPerPixel = 16;
        }
        else if (pScrn->depth <= 24) {
            pScrn->depth = 24;
            pScrn->bitsPerPixel = 32;
        }
        else if (pScrn->depth <= 30) {
            pScrn->depth = 30;
            pScrn->bitsPerPixel = 32;
        }
        else {
            ErrorF("\nXephyr: Unsupported screen depth %d\n", pScrn->depth);
            return FALSE;
        }

        hostx_get_visual_masks(pScrn, &redMask, &greenMask, &blueMask);

        pScrn->mask.red = (Pixel) redMask;
        pScrn->mask.green = (Pixel) greenMask;
        pScrn->mask.blue = (Pixel) blueMask;

    }

    scrpriv->randr = pScrn->randr;

    return ephyrMapFramebuffer(pScrn);
}
#endif

#if 0
void *
ephyrWindowLinear(ScreenPtr pScreen,
                  CARD32 row,
                  CARD32 offset, int mode, CARD32 *size, void *closure)
{
    EphyrPriv *priv = pScreenPriv->card->driver;

    if (!pScreenPriv->enabled)
        return 0;

    *size = priv->bytes_per_line;
    return priv->base + row * priv->bytes_per_line + offset;
}
#endif

/**
 * Figure out display buffer size. If fakexa is enabled, allocate a larger
 * buffer so that fakexa has space to put offscreen pixmaps.
 */
int
ephyrBufferHeight(ScrnInfoPtr pScrn)
{
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
ephyrMapFramebuffer(ScrnInfoPtr pScrn)
{
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
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

    priv->base =
        hostx_screen_init(pScrn, pScrn->x, pScrn->y,
                          pScrn->virtualX, pScrn->virtualY, buffer_height,
                          &priv->bytes_per_line, &pScrn->fb.bitsPerPixel);

    if ((scrpriv->randr & RR_Rotate_0) && !(scrpriv->randr & RR_Reflect_All)) {
        scrpriv->shadow = FALSE;

        pScrn->fb.byteStride = priv->bytes_per_line;
        pScrn->fb.pixelStride = pScrn->virtualX;
        pScrn->fb.frameBuffer = (CARD8 *) (priv->base);
    }
    else {
        /* Rotated/Reflected so we need to use shadow fb */
        scrpriv->shadow = TRUE;

        EPHYR_LOG("allocing shadow");

        KdShadowFbAlloc(pScrn,
                        scrpriv->randr & (RR_Rotate_90 | RR_Rotate_270));
    }

    return TRUE;
}
#endif

#if 0
void
ephyrSetScreenSizes(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

    if (scrpriv->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        pScreen->width = pScrn->virtualX;
        pScreen->height = pScrn->virtualY;
        pScreen->mmWidth = pScrn->widthmm;
        pScreen->mmHeight = pScrn->heightmm;
    }
    else {
        pScreen->width = pScrn->virtualY;
        pScreen->height = pScrn->virtualX;
        pScreen->mmWidth = pScrn->heightmm;
        pScreen->mmHeight = pScrn->widthmm;
    }
}

Bool
ephyrUnmapFramebuffer(ScrnInfoPtr pScrn)
{
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

    if (scrpriv->shadow)
        KdShadowFbFree(pScrn);

    /* Note, priv->base will get freed when XImage recreated */

    return TRUE;
}
#endif

static void
ephyrInternalDamageRedisplay(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
    RegionPtr pRegion;

    if (!scrpriv || !scrpriv->pDamage)
        return;

    pRegion = DamageRegion(scrpriv->pDamage);

    if (RegionNotEmpty(pRegion)) {
        int nbox;
        BoxPtr pbox;

        if (ephyr_glamor) {
            ephyr_glamor_damage_redisplay(scrpriv->glamor, pRegion);
        } else {
            nbox = RegionNumRects(pRegion);
            pbox = RegionRects(pRegion);

            while (nbox--) {
                hostx_paint_rect(pScrn,
                                 pbox->x1, pbox->y1,
                                 pbox->x1, pbox->y1,
                                 pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
                pbox++;
            }
        }
        DamageEmpty(scrpriv->pDamage);
    }
}

static void
ephyrInternalDamageBlockHandler(void *data, OSTimePtr pTimeout, void *pRead)
{
    ScreenPtr pScreen = (ScreenPtr) data;

    ephyrInternalDamageRedisplay(pScreen);
}

static void
ephyrInternalDamageWakeupHandler(void *data, int i, void *LastSelectMask)
{
    /* FIXME: Not needed ? */
}

Bool
ephyrSetInternalDamage(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
    PixmapPtr pPixmap = NULL;

    scrpriv->pDamage = DamageCreate((DamageReportFunc) 0,
                                    (DamageDestroyFunc) 0,
                                    DamageReportNone, TRUE, pScreen, pScreen);

    if (!RegisterBlockAndWakeupHandlers(ephyrInternalDamageBlockHandler,
                                        ephyrInternalDamageWakeupHandler,
                                        (void *) pScreen))
        return FALSE;

    pPixmap = (*pScreen->GetScreenPixmap) (pScreen);

    DamageRegister(&pPixmap->drawable, scrpriv->pDamage);

    return TRUE;
}

void
ephyrUnsetInternalDamage(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

    DamageDestroy(scrpriv->pDamage);

    RemoveBlockAndWakeupHandlers(ephyrInternalDamageBlockHandler,
                                 ephyrInternalDamageWakeupHandler,
                                 (void *) pScreen);
}

#if 0
#ifdef RANDR
Bool
ephyrRandRGetInfo(ScreenPtr pScreen, Rotation * rotations)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
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
        && !hostx_want_fullscreen()) {  /* only if no -parent switch */
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

    randr = KdSubRotation(scrpriv->randr, pScrn->randr);

    RRSetCurrentConfig(pScreen, randr, 0, pSize);

    return TRUE;
}

#if 0
Bool
ephyrRandRSetConfig(ScreenPtr pScreen,
                    Rotation randr, int rate, RRScreenSizePtr pSize)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
    Bool wasEnabled = pScreenPriv->enabled;
    EphyrScrPriv oldscr;
    int oldwidth, oldheight, oldmmwidth, oldmmheight;
    Bool oldshadow;
    int newwidth, newheight;

    if (pScrn->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        newwidth = pSize->width;
        newheight = pSize->height;
    }
    else {
        newwidth = pSize->height;
        newheight = pSize->width;
    }

    if (wasEnabled)
        KdDisableScreen(pScreen);

    oldscr = *scrpriv;

    oldwidth = pScrn->virtualX;
    oldheight = pScrn->virtualY;
    oldmmwidth = pScreen->mmWidth;
    oldmmheight = pScreen->mmHeight;
    oldshadow = scrpriv->shadow;

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
    scrpriv->randr = ephyrRandr;

    ephyrUnmapFramebuffer(pScrn);

    pScrn->virtualX = newwidth;
    pScrn->virtualY = newheight;

    if (!ephyrMapFramebuffer(pScrn))
        goto bail4;

    /* FIXME below should go in own call */

    if (oldshadow)
        KdShadowUnset(pScrn->pScreen);
    else
        ephyrUnsetInternalDamage(pScrn->pScreen);

    if (scrpriv->shadow) {
        if (!KdShadowSet(pScrn->pScreen,
                         scrpriv->randr, ephyrShadowUpdate, ephyrWindowLinear))
            goto bail4;
    }
    else {
        /* Without shadow fb ( non rotated ) we need
         * to use damage to efficiently update display
         * via signal regions what to copy from 'fb'.
         */
        if (!ephyrSetInternalDamage(pScrn->pScreen))
            goto bail4;
    }

    ephyrSetScreenSizes(pScrn->pScreen);

    /*
     * Set frame buffer mapping
     */
    (*pScreen->ModifyPixmapHeader) (fbGetScreenPixmap(pScreen),
                                    pScreen->width,
                                    pScreen->height,
                                    pScrn->fb.depth,
                                    pScrn->fb.bitsPerPixel,
                                    pScrn->fb.byteStride,
                                    pScrn->fb.frameBuffer);

    /* set the subpixel order */

    KdSetSubpixelOrder(pScreen, scrpriv->randr);

    if (wasEnabled)
        KdEnableScreen(pScreen);

    RRScreenSizeNotify(pScreen);

    return TRUE;

 bail4:
    EPHYR_LOG("bailed");

    ephyrUnmapFramebuffer(pScrn);
    *scrpriv = oldscr;
    (void) ephyrMapFramebuffer(pScrn);

    pScreen->width = oldwidth;
    pScreen->height = oldheight;
    pScreen->mmWidth = oldmmwidth;
    pScreen->mmHeight = oldmmheight;

    if (wasEnabled)
        KdEnableScreen(pScreen);
    return FALSE;
}
#endif

Bool
ephyrRandRInit(ScreenPtr pScreen)
{
    rrScrPrivPtr pScrPriv;

    if (!RRScreenInit(pScreen))
        return FALSE;

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = ephyrRandRGetInfo;
    pScrPriv->rrSetConfig = ephyrRandRSetConfig;
    return TRUE;
}

static Bool
ephyrResizeScreen (ScreenPtr           pScreen,
                  int                  newwidth,
                  int                  newheight)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    RRScreenSize size = {0};
    Bool ret;
    int t;

    if (pScrn->randr & (RR_Rotate_90|RR_Rotate_270)) {
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
        if (!output)
            return FALSE;
        RROutputSetModes(output, NULL, 0, 0);
    }

    return ret;
}
#endif
#endif

Bool
ephyrCreateColormap(ColormapPtr pmap)
{
    return fbInitializeColormap(pmap);
}

Bool
ephyrInitScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

    EPHYR_LOG("pScreen->myNum:%d\n", pScreen->myNum);
    hostx_set_screen_number(pScrn, pScreen->myNum);
    if (EphyrWantNoHostGrab) {
        hostx_set_win_title(pScrn, "xephyr");
    } else {
        hostx_set_win_title(pScrn, "(ctrl+shift grabs mouse and keyboard)");
    }
    pScreen->CreateColormap = ephyrCreateColormap;

#ifdef XV
    if (!ephyrNoXV) {
        if (ephyr_glamor)
            ephyr_glamor_xv_init(pScreen);
        else if (!ephyrInitVideo(pScreen)) {
            EPHYR_LOG_ERROR("failed to initialize xvideo\n");
        }
        else {
            EPHYR_LOG("initialized xvideo okay\n");
        }
    }
#endif /*XV*/

    return TRUE;
}

#if 0
Bool
ephyrFinishInitScreen(ScreenPtr pScreen)
{
    /* FIXME: Calling this even if not using shadow.
     * Seems harmless enough. But may be safer elsewhere.
     */
    if (!shadowSetup(pScreen))
        return FALSE;

#ifdef RANDR
    if (!ephyrRandRInit(pScreen))
        return FALSE;
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
ephyrCreateResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

    EPHYR_LOG("mark pScreen=%p mynum=%d shadow=%d",
              pScreen, pScreen->myNum, scrpriv->shadow);

    if (scrpriv->shadow)
        return KdShadowSet(pScreen,
                           scrpriv->randr,
                           ephyrShadowUpdate, ephyrWindowLinear);
    else {
#ifdef GLAMOR
        if (ephyr_glamor)
            ephyr_glamor_create_screen_resources(pScreen);
#endif
        return ephyrSetInternalDamage(pScreen);
    }
}
#endif

#if 0
void
ephyrPreserve(KdCardInfo * card)
{
}
#endif

Bool
ephyrEnable(ScreenPtr pScreen)
{
    return TRUE;
}

Bool
ephyrDPMS(ScreenPtr pScreen, int mode)
{
    return TRUE;
}

void
ephyrDisable(ScreenPtr pScreen)
{
}

#if 0
void
ephyrRestore(KdCardInfo * card)
{
}
#endif

#if 0
void
ephyrScreenFini(ScrnInfoPtr pScrn)
{
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

    if (scrpriv->shadow) {
        KdShadowFbFree(pScrn);
    }
}
#endif

static Bool
ephyrCursorOffScreen(ScreenPtr *ppScreen, int *x, int *y)
{
    return FALSE;
}

static void
ephyrCrossScreen(ScreenPtr pScreen, Bool entering)
{
}

ScreenPtr ephyrCursorScreen; /* screen containing the cursor */

static void
ephyrWarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
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

#if 0
static KdScreenInfo *
screen_from_window(Window w)
{
    int i = 0;

    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];
        KdPrivScreenPtr kdscrpriv = KdGetScreenPriv(pScreen);
        ScrnInfoPtr pScrn = kdscrpriv->screen;
        EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

        if (scrpriv->win == w
            || scrpriv->peer_win == w
            || scrpriv->win_pre_existing == w) {
            return pScrn;
        }
    }

    return NULL;
}
#endif

#if 0 /* Disable event listening temporarily */
static void
ephyrProcessErrorEvent(xcb_generic_event_t *xev)
{
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
ephyrProcessExpose(xcb_generic_event_t *xev)
{
    xcb_expose_event_t *expose = (xcb_expose_event_t *)xev;
    ScrnInfoPtr pScrn = screen_from_window(expose->window);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

    /* Wait for the last expose event in a series of cliprects
     * to actually paint our screen.
     */
    if (expose->count != 0)
        return;

    if (scrpriv) {
        hostx_paint_rect(scrpriv->pScrn, 0, 0, 0, 0,
                         scrpriv->win_width,
                         scrpriv->win_height);
    } else {
        EPHYR_LOG_ERROR("failed to get host screen\n");
    }
}

static void
ephyrProcessConfigureNotify(xcb_generic_event_t *xev)
{
    xcb_configure_notify_event_t *configure =
        (xcb_configure_notify_event_t *)xev;
    ScrnInfoPtr pScrn = screen_from_window(configure->window);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

    if (!scrpriv ||
        (scrpriv->win_pre_existing == None && !EphyrWantResize)) {
        return;
    }

#ifdef RANDR
    ephyrResizeScreen(pScrn->pScreen, configure->width, configure->height);
#endif /* RANDR */
}

static void
ephyrXcbNotify(int fd, int ready, void *data)
{
    xcb_connection_t *conn = hostx_get_xcbconn();

    while (TRUE) {
        xcb_generic_event_t *xev = xcb_poll_for_event(conn);
        if (!xev) {
            /* If our XCB connection has died (for example, our window was
             * closed), exit now.
             */
            if (xcb_connection_has_error(conn)) {
                CloseWellKnownConnections();
                OsCleanup(1);
                exit(1);
            }

            break;
        }

        switch (xev->response_type & 0x7f) {
        case 0:
            ephyrProcessErrorEvent(xev);
            break;

        case XCB_EXPOSE:
            ephyrProcessExpose(xev);
            break;

        case XCB_CONFIGURE_NOTIFY:
            ephyrProcessConfigureNotify(xev);
            break;
        }

        if (ephyr_glamor)
            ephyr_glamor_process_event(xev);

        free(xev);
    }
}
#endif

#if 0
void
ephyrCardFini(KdCardInfo * card)
{
    EphyrPriv *priv = card->driver;

    free(priv);
}
#endif

void
ephyrGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
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
ephyrPutColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
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
    if (scrpriv->pDamage) {
        BoxRec box;
        RegionRec region;

        box.x1 = 0;
        box.y1 = 0;
        box.x2 = pScreen->width;
        box.y2 = pScreen->height;
        RegionInit(&region, &box, 1);
        DamageReportDamage(scrpriv->pDamage, &region);
        RegionUninit(&region);
    }
}

#if 0 /* Disable event listening temporarily */
static Status
MouseEnable(KdPointerInfo * pi)
{
    ((EphyrPointerPrivate *) pi->driverPrivate)->enabled = TRUE;
    SetNotifyFd(hostx_get_fd(), ephyrXcbNotify, X_NOTIFY_READ, NULL);
    return Success;
}

static void
MouseDisable(KdPointerInfo * pi)
{
    ((EphyrPointerPrivate *) pi->driverPrivate)->enabled = FALSE;
    RemoveNotifyFd(hostx_get_fd());
    return;
}
#endif

/* End of former ephyr.c
 ****************************************************************
 * Beginning of driver.c */

typedef enum {
    OPTION_DISPLAY,
    OPTION_XAUTHORITY,
#ifdef GLAMOR
    OPTION_NOACCEL,
    OPTION_ACCELMETHOD,
#endif
    OPTION_PARENTWINDOW,
    OPTION_FULLSCREEN,
    OPTION_OUTPUT,
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
#ifdef GLAMOR
    { OPTION_NOACCEL,      "NoAccel",      OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_ACCELMETHOD,  "AccelMethod",  OPTV_STRING,  {0}, FALSE },
#endif
    { OPTION_PARENTWINDOW, "ParentWindow", OPTV_INTEGER, {0}, FALSE },
    { OPTION_FULLSCREEN,   "Fullscreen",   OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_OUTPUT,       "Output",       OPTV_STRING,  {0}, FALSE },
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
    {0, 0, 0, 0} /* checksum */
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

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "  name: %s\n", mode->name);
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "  HDisplay: %d\n", mode->HDisplay);
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "  VDisplay: %d\n", mode->VDisplay);
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
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "  %s (%dx%d)\n", mode->name,
                  mode->HDisplay, mode->VDisplay);
   }

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Too bad for it...\n");

   /* If user requested modes, add them. If not, use 640x480 */
   if (pScrn->display->modes != NULL && pScrn->display->modes[0] != NULL) {
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "User wants these modes:\n");

       for (i = 0; pScrn->display->modes[i] != NULL; i++) {
           xf86DrvMsg(pScrn->scrnIndex, X_INFO, "  %s\n",
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
   } else {
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

   pScrn->driverPrivate = xnfcalloc(sizeof(EphyrScrPriv), 1);

   if (pScrn->driverPrivate == NULL) {
       return FALSE;
   }

   return TRUE;
}

/* Data from here is valid to all server generations */
static Bool
ephyrPreInit(ScrnInfoPtr pScrn, int flags) {
   const char *displayName = getenv("DISPLAY");
   const char *accelMethod;
   EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
   Bool fullscreen = FALSE;
   Bool noAccel = FALSE;

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrPreInit\n");

   if (flags & PROBE_DETECT) {
       return FALSE;
   }

   if (!ephyrAllocatePrivate(pScrn)) {
       xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to allocate private\n");
       return FALSE;
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
                            &scrpriv->win_pre_existing)) {
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Targeting parent window \"%x\"\n",
                  scrpriv->win_pre_existing);
   }

   if (xf86GetOptValBool(EphyrOptions,
                         OPTION_FULLSCREEN,
                         &fullscreen)) {
       if (fullscreen) {
           hostx_use_fullscreen();
       }

       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Fullscreen mode %s\n",
                  fullscreen ? "enabled" : "disabled");
   }

   if (xf86IsOptionSet(EphyrOptions, OPTION_OUTPUT)) {
       scrpriv->output = xf86GetOptValString(EphyrOptions,
                                             OPTION_OUTPUT);
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Targeting host X server output \"%s\"\n",
                  scrpriv->output);
   }

   xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

   if (hostx_get_xcbconn() != NULL) {
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Reusing current XCB connection to display %s\n",
                  displayName);
   } else if (!hostx_init()) {
       xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Can't open display: %s\n",
                  displayName);
       return FALSE;
   }

   if (!hostx_init_window(pScrn)) {
       xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Can't create window on display: %s\n",
                  displayName);
       return FALSE;
   }

   /* TODO: replace with corresponding Xephyr function.
   if (!ephyrClientValidDepth(pScrn->depth)) {
       xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Invalid depth: %d\n",
                  pScrn->depth);
       return FALSE;
   }*/

   if (ephyrValidateModes(pScrn) < 1) {
       xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes\n");
       return FALSE;
   }

   if (!pScrn->modes) {
       xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
       return FALSE;
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

   return TRUE;
}

static void
ephyrShadowUpdate(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

    EPHYR_LOG("slow paint");

    /* FIXME: Slow Rotated/Reflected updates could be much
     * much faster efficiently updating via tranforming
     * pBuf->pDamage  regions
     */
    shadowUpdateRotatePacked(pScreen, pBuf);
    hostx_paint_rect(pScrn, 0, 0, 0, 0, pScrn->virtualX, pScrn->virtualY);
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
   EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
   Bool ret;

   pScreen->CreateScreenResources = scrpriv->CreateScreenResources;
   ret = pScreen->CreateScreenResources(pScreen);
   pScreen->CreateScreenResources = ephyrCreateScreenResources;

   if(!shadowAdd(pScreen, pScreen->GetScreenPixmap(pScreen),
                 scrpriv->update, NULL, 0, 0)) {
       xf86DrvMsg(pScreen->myNum, X_ERROR, "ephyrCreateScreenResources failed to shadowAdd.\n");
       return FALSE;
   }

   return ret;
}

static void
ephyrPrintMode(ScrnInfoPtr p, DisplayModePtr m) {
   xf86DrvMsg(p->scrnIndex, X_INFO, "HDisplay   %d\n",   m->HDisplay);
   xf86DrvMsg(p->scrnIndex, X_INFO, "HSyncStart %d\n", m->HSyncStart);
   xf86DrvMsg(p->scrnIndex, X_INFO, "HSyncEnd   %d\n",   m->HSyncEnd);
   xf86DrvMsg(p->scrnIndex, X_INFO, "HTotal     %d\n",     m->HTotal);
   xf86DrvMsg(p->scrnIndex, X_INFO, "HSkew      %d\n",      m->HSkew);
   xf86DrvMsg(p->scrnIndex, X_INFO, "VDisplay   %d\n",   m->VDisplay);
   xf86DrvMsg(p->scrnIndex, X_INFO, "VSyncStart %d\n", m->VSyncStart);
   xf86DrvMsg(p->scrnIndex, X_INFO, "VSyncEnd   %d\n",   m->VSyncEnd);
   xf86DrvMsg(p->scrnIndex, X_INFO, "VTotal     %d\n",     m->VTotal);
   xf86DrvMsg(p->scrnIndex, X_INFO, "VScan      %d\n",      m->VScan);
}

static void
ephyrPrintPscreen(ScrnInfoPtr p) {
   /* XXX: finish implementing this someday? */
   xf86DrvMsg(p->scrnIndex, X_INFO, "Printing pScrn:\n");
   xf86DrvMsg(p->scrnIndex, X_INFO, "driverVersion: %d\n", p->driverVersion);
   xf86DrvMsg(p->scrnIndex, X_INFO, "driverName:    %s\n", p->driverName);
   xf86DrvMsg(p->scrnIndex, X_INFO, "pScreen:       %p\n", p->pScreen);
   xf86DrvMsg(p->scrnIndex, X_INFO, "scrnIndex:     %d\n", p->scrnIndex);
   xf86DrvMsg(p->scrnIndex, X_INFO, "configured:    %d\n", p->configured);
   xf86DrvMsg(p->scrnIndex, X_INFO, "origIndex:     %d\n", p->origIndex);
   xf86DrvMsg(p->scrnIndex, X_INFO, "imageByteOrder: %d\n", p->imageByteOrder);
   /*xf86DrvMsg(p->scrnIndex, X_INFO, "bitmapScanlineUnit: %d\n");
   xf86DrvMsg(p->scrnIndex, X_INFO, "bitmapScanlinePad: %d\n");
   xf86DrvMsg(p->scrnIndex, X_INFO, "bitmapBitOrder: %d\n");
   xf86DrvMsg(p->scrnIndex, X_INFO, "numFormats: %d\n");
   xf86DrvMsg(p->scrnIndex, X_INFO, "formats[]: 0x%x\n");
   xf86DrvMsg(p->scrnIndex, X_INFO, "fbFormat: 0x%x\n"); */
   xf86DrvMsg(p->scrnIndex, X_INFO, "bitsPerPixel: %d\n", p->bitsPerPixel);
   /*xf86DrvMsg(p->scrnIndex, X_INFO, "pixmap24: 0x%x\n"); */
   xf86DrvMsg(p->scrnIndex, X_INFO, "depth: %d\n", p->depth);
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

static Bool
ephyrCloseScreen(CLOSE_SCREEN_ARGS_DECL) {
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
   EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrCloseScreen\n");

   shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));

#if 0 /* Disable event listening temporarily */
   RemoveBlockAndWakeupHandlers(ephyrBlockHandler, ephyrWakeupHandler, scrpriv);
#endif

   hostx_close_screen(pScrn);
   pScreen->CloseScreen = scrpriv->CloseScreen;
   return (*pScreen->CloseScreen)(CLOSE_SCREEN_ARGS);
}

/* Called at each server generation */
static Bool
ephyrScreenInit(SCREEN_INIT_ARGS_DECL) {
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
   EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
   Pixel redMask, greenMask, blueMask;
   char *fb_data;

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ephyrScreenInit\n");
   ephyrPrintPscreen(pScrn);

   fb_data = hostx_screen_init(pScrn,
                               pScrn->frameX0, pScrn->frameY0,
                               pScrn->virtualX, pScrn->virtualY,
                               ephyrBufferHeight(pScrn),
                               NULL, /* bytes per line/row (not used) */
                               &pScrn->bitsPerPixel);
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
                     pScrn->virtualX, pScrn->virtualY, pScrn->xDpi,
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

   scrpriv->update = ephyrShadowUpdate;
   pScreen->SaveScreen = ephyrSaveScreen;

   if (!shadowSetup(pScreen)) {
       return FALSE;
   }

   scrpriv->CreateScreenResources = pScreen->CreateScreenResources;
   pScreen->CreateScreenResources = ephyrCreateScreenResources;

   scrpriv->CloseScreen = pScreen->CloseScreen;
   pScreen->CloseScreen = ephyrCloseScreen;

#if 0 /* Disable event listening temporarily */
   RegisterBlockAndWakeupHandlers(ephyrBlockHandler, ephyrWakeupHandler, scrpriv);
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
               pScrn->driverName    = EPHYR_DRIVER_NAME;
               pScrn->name          = EPHYR_NAME;
               pScrn->Probe         = ephyrProbe;
               pScrn->PreInit       = ephyrPreInit;
               pScrn->ScreenInit    = ephyrScreenInit;
               pScrn->SwitchMode    = ephyrSwitchMode;
               pScrn->AdjustFrame   = ephyrAdjustFrame;
               pScrn->EnterVT       = ephyrEnterVT;
               pScrn->LeaveVT       = ephyrLeaveVT;
               pScrn->FreeScreen    = ephyrFreeScreen;
               pScrn->ValidMode     = ephyrValidMode;
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
ephyrDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op, pointer ptr) {
   CARD32 *flag;
   xf86Msg(X_INFO, "ephyrDriverFunc\n");

   /* XXX implement */
   switch(op) {
       case GET_REQUIRED_HW_INTERFACES:
           flag = (CARD32*)ptr;
           (*flag) = HW_SKIP_CONSOLE;
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

#if 0 /* Disable event listening temporarily */
static void
ephyrBlockHandler(pointer data, OSTimePtr wt, pointer LastSelectMask) {
   ephyrPoll();
}

static void
ephyrWakeupHandler(pointer data, int i, pointer LastSelectMask) {
}
#endif

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
       return (pointer)1;
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
