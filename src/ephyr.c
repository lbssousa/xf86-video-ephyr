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
        pScrn->fb.depth = 8;

    if (pScrn->fb.depth && pScrn->fb.depth != hostx_get_depth()) {
        if (pScrn->fb.depth < hostx_get_depth()
            && (pScrn->fb.depth == 24 || pScrn->fb.depth == 16
                || pScrn->fb.depth == 8)) {
            scrpriv->server_depth = pScrn->fb.depth;
        }
        else
            ErrorF
                ("\nXephyr: requested screen depth not supported, setting to match hosts.\n");
    }

    pScrn->fb.depth = hostx_get_server_depth(pScrn);
    pScrn->rate = 72;

    if (pScrn->fb.depth <= 8) {
        if (EphyrWantGrayScale)
            pScrn->fb.visuals = ((1 << StaticGray) | (1 << GrayScale));
        else
            pScrn->fb.visuals = ((1 << StaticGray) |
                                  (1 << GrayScale) |
                                  (1 << StaticColor) |
                                  (1 << PseudoColor) |
                                  (1 << TrueColor) | (1 << DirectColor));

        pScrn->fb.redMask = 0x00;
        pScrn->fb.greenMask = 0x00;
        pScrn->fb.blueMask = 0x00;
        pScrn->fb.depth = 8;
        pScrn->fb.bitsPerPixel = 8;
    }
    else {
        pScrn->fb.visuals = (1 << TrueColor);

        if (pScrn->fb.depth <= 15) {
            pScrn->fb.depth = 15;
            pScrn->fb.bitsPerPixel = 16;
        }
        else if (pScrn->fb.depth <= 16) {
            pScrn->fb.depth = 16;
            pScrn->fb.bitsPerPixel = 16;
        }
        else if (pScrn->fb.depth <= 24) {
            pScrn->fb.depth = 24;
            pScrn->fb.bitsPerPixel = 32;
        }
        else if (pScrn->fb.depth <= 30) {
            pScrn->fb.depth = 30;
            pScrn->fb.bitsPerPixel = 32;
        }
        else {
            ErrorF("\nXephyr: Unsupported screen depth %d\n", pScrn->fb.depth);
            return FALSE;
        }

        hostx_get_visual_masks(pScrn, &redMask, &greenMask, &blueMask);

        pScrn->fb.redMask = (Pixel) redMask;
        pScrn->fb.greenMask = (Pixel) greenMask;
        pScrn->fb.blueMask = (Pixel) blueMask;

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
        pScreen->mmWidth = pScrn->virtualX_mm;
        pScreen->mmHeight = pScrn->virtualY_mm;
    }
    else {
        pScreen->width = pScrn->virtualY;
        pScreen->height = pScrn->virtualX;
        pScreen->mmWidth = pScrn->virtualY_mm;
        pScreen->mmHeight = pScrn->virtualX_mm;
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

void
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
                           (sizes[n].width * pScrn->virtualX_mm) / pScrn->virtualX,
                           (sizes[n].height * pScrn->virtualY_mm) /
                           pScrn->virtualY);
            n++;
        }
    }

    pSize = RRRegisterSize(pScreen,
                           pScrn->virtualX,
                           pScrn->virtualY, pScrn->virtualX_mm, pScrn->virtualY_mm);

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

void
ephyrScreenFini(ScrnInfoPtr pScrn)
{
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

    if (scrpriv->shadow) {
        KdShadowFbFree(pScrn);
    }
}

void
ephyrCloseScreen(ScreenPtr pScreen)
{
    ephyrUnsetInternalDamage(pScreen);
}

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
