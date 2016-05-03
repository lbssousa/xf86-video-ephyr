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

#ifndef _EPHYR_H_
#define _EPHYR_H_
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <xcb/xcb_image.h>

#include "os.h"                 /* for OsSignal() */
#include "hostx.h"
#include "exa.h"

#ifdef RANDR
#include "randrstr.h"
#endif

#include "damage.h"

typedef struct _ephyrPriv {
    CARD8 *base;
    int bytes_per_line;
} EphyrPriv;

typedef struct _ephyrFakexaPriv {
    ExaDriverPtr exa;
    Bool is_synced;

    /* The following are arguments and other information from Prepare* calls
     * which are stored for use in the inner calls.
     */
    int op;
    PicturePtr pSrcPicture, pMaskPicture, pDstPicture;
    void *saved_ptrs[3];
    PixmapPtr pDst, pSrc, pMask;
    GCPtr pGC;
} EphyrFakexaPriv;

typedef struct _ephyrScrPriv {
    /* ephyr server info */
    Rotation randr;
    Bool shadow;
    DamagePtr pDamage;
    EphyrFakexaPriv *fakexa;

    /* Host X window info */
    xcb_window_t win;
    xcb_window_t win_pre_existing;    /* Set via -parent option like xnest */
    xcb_window_t peer_win;            /* Used for GL; should be at most one */
    xcb_image_t *ximg;
    Bool win_explicit_position;
    int win_x, win_y;
    int win_width, win_height;
    int server_depth;
    const char *output;         /* Set via "Output" xorg.conf option */
    unsigned char *fb_data;     /* only used when host bpp != server bpp */
    xcb_shm_segment_info_t shminfo;

    ScrnInfoPtr screen;
    int mynum;                  /* Screen number */
    unsigned long cmap[256];

    CreateScreenResourcesProcPtr CreateScreenResources;
    CloseScreenProcPtr CloseScreen;
    ShadowUpdateProc update;

    /**
     * Per-screen Xlib-using state for glamor (private to
     * ephyr_glamor_glx.c)
     */
    struct ephyr_glamor *glamor;
} EphyrScrPriv, *EphyrScrPrivPtr;

extern miPointerScreenFuncRec ephyrPointerScreenFuncs;

#if 0
Bool
 ephyrInitialize(KdCardInfo * card, EphyrPriv * priv);

Bool
 ephyrCardInit(KdCardInfo * card);
#endif

Bool
ephyrScreenInitialize(ScrnInfoPtr pScrn);

Bool
 ephyrInitScreen(ScreenPtr pScreen);

Bool
 ephyrFinishInitScreen(ScreenPtr pScreen);

Bool
 ephyrCreateResources(ScreenPtr pScreen);

#if 0
void
 ephyrPreserve(KdCardInfo * card);
#endif

Bool
 ephyrEnable(ScreenPtr pScreen);

Bool
 ephyrDPMS(ScreenPtr pScreen, int mode);

void
 ephyrDisable(ScreenPtr pScreen);

#if 0
void
 ephyrRestore(KdCardInfo * card);
#endif

void
 ephyrScreenFini(ScrnInfoPtr pScrn);

void
ephyrCloseScreen(ScreenPtr pScreen);

#if 0
void
 ephyrCardFini(KdCardInfo * card);
#endif

void
 ephyrGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

void
 ephyrPutColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

Bool
 ephyrMapFramebuffer(ScrnInfoPtr pScrn);

void *ephyrWindowLinear(ScreenPtr pScreen,
                        CARD32 row,
                        CARD32 offset, int mode, CARD32 *size, void *closure);

void
 ephyrSetScreenSizes(ScreenPtr pScreen);

Bool
 ephyrUnmapFramebuffer(ScrnInfoPtr pScrn);

void
 ephyrUnsetInternalDamage(ScreenPtr pScreen);

Bool
 ephyrSetInternalDamage(ScreenPtr pScreen);

Bool
 ephyrCreateColormap(ColormapPtr pmap);

#ifdef RANDR
Bool
 ephyrRandRGetInfo(ScreenPtr pScreen, Rotation * rotations);

Bool

ephyrRandRSetConfig(ScreenPtr pScreen,
                    Rotation randr, int rate, RRScreenSizePtr pSize);
Bool
 ephyrRandRInit(ScreenPtr pScreen);

void
 ephyrShadowUpdate(ScreenPtr pScreen, shadowBufPtr pBuf);

#endif

extern Bool ephyrCursorInit(ScreenPtr pScreen);

extern int ephyrBufferHeight(ScrnInfoPtr pScrn);

/* ephyr_draw.c */

Bool
 ephyrDrawInit(ScreenPtr pScreen);

void
 ephyrDrawEnable(ScreenPtr pScreen);

void
 ephyrDrawDisable(ScreenPtr pScreen);

void
 ephyrDrawFini(ScreenPtr pScreen);

/* hostx.c glamor support */
Bool ephyr_glamor_init(ScreenPtr pScreen);
Bool ephyr_glamor_create_screen_resources(ScreenPtr pScreen);
void ephyr_glamor_enable(ScreenPtr pScreen);
void ephyr_glamor_disable(ScreenPtr pScreen);
void ephyr_glamor_fini(ScreenPtr pScreen);
void ephyr_glamor_host_paint_rect(ScreenPtr pScreen);

/*ephyvideo.c*/

Bool ephyrInitVideo(ScreenPtr pScreen);

/* ephyr_glamor_xv.c */
#ifdef GLAMOR
void ephyr_glamor_xv_init(ScreenPtr screen);
#else /* !GLAMOR */
static inline void
ephyr_glamor_xv_init(ScreenPtr screen)
{
}
#endif /* !GLAMOR */

#endif
