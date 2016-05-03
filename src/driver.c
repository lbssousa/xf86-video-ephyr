/*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *
 * Paulo Zanoni <pzanoni@mandriva.com>
 * Tuan Bui <tuanbui918@gmail.com>
 * Colin Cornaby <colin.cornaby@mac.com>
 * Timothy Fleck <tim.cs.pdx@gmail.com>
 * Colin Hill <colin.james.hill@gmail.com>
 * Weseung Hwang <weseung@gmail.com>
 * Nathaniel Way <nathanielcw@hotmail.com>
 * Laércio de Sousa <laerciosousa@sme-mogidascruzes.sp.gov.br>
 */

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xorg-server.h>
#include <fb.h>
#include <micmap.h>
#include <mipointer.h>
#include <shadow.h>
#include <xf86.h>
#include <xf86Module.h>
#include <xf86str.h>

#include "compat-api.h"

#include "ephyr.h"

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

typedef enum {
    OPTION_DISPLAY,
    OPTION_XAUTHORITY,
    OPTION_FULLSCREEN,
    OPTION_OUTPUT
} NestedOpts;

typedef enum {
    EPHYR_CHIP
} NestedType;

static SymTabRec EphyrChipsets[] = {
    { EPHYR_CHIP, "ephyr" },
    {-1,          NULL }
};

static OptionInfoRec EphyrOptions[] = {
    { OPTION_DISPLAY,    "Display",    OPTV_STRING,  {0}, FALSE },
    { OPTION_XAUTHORITY, "Xauthority", OPTV_STRING,  {0}, FALSE },
    { OPTION_FULLSCREEN, "Fullscreen", OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_OUTPUT,     "Output",     OPTV_STRING,  {0}, FALSE },
    { -1,                NULL,         OPTV_NONE,   {0}, FALSE }
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
NestedEnterVT(VT_FUNC_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NestedEnterVT\n");
    return TRUE;
}

static void
NestedLeaveVT(VT_FUNC_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NestedLeaveVT\n");
}

static Bool
NestedSwitchMode(SWITCH_MODE_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NestedSwitchMode\n");
    return TRUE;
}

static void
NestedAdjustFrame(ADJUST_FRAME_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NestedAdjustFrame\n");
}

static ModeStatus
NestedValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode,
                Bool verbose, int flags) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NestedValidMode:\n");

    if (!mode) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "NULL MODE!\n");
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "  name: %s\n", mode->name);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "  HDisplay: %d\n", mode->HDisplay);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "  VDisplay: %d\n", mode->VDisplay);
    return MODE_OK;
}

static void
NestedFreeScreen(FREE_SCREEN_ARGS_DECL) {
    SCRN_INFO_PTR(arg);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NestedFreeScreen\n");
}

Bool
NestedAddMode(ScrnInfoPtr pScrn, int width, int height) {
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

int
NestedValidateModes(ScrnInfoPtr pScrn) {
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

            if (!NestedAddMode(pScrn, width, height)) {
                return 0;
            }
        }
    } else {
        if (!NestedAddMode(pScrn, 640, 480)) {
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

static Bool NestedAllocatePrivate(ScrnInfoPtr pScrn) {
    if (pScrn->driverPrivate != NULL) {
        xf86Msg(X_WARNING, "NestedAllocatePrivate called for an already "
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
static Bool NestedPreInit(ScrnInfoPtr pScrn, int flags) {
    const char *displayName = getenv("DISPLAY");
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
    Bool fullscreen = FALSE;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NestedPreInit\n");

    if (flags & PROBE_DETECT) {
        return FALSE;
    }

    if (!NestedAllocatePrivate(pScrn)) {
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
               xf86GetOptValString(EphyrOptions, OPTION_XAUTHORITY),
               TRUE);
    }

    if (xf86GetOptValBool(EphyrOptions, OPTION_FULLSCREEN, &fullscreen)) {
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
    if (!NestedClientValidDepth(pScrn->depth)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Invalid depth: %d\n",
                   pScrn->depth);
        return FALSE;
    }*/

    if (NestedValidateModes(pScrn) < 1) {
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
NestedShadowUpdate(ScreenPtr pScreen, shadowBufPtr pBuf) {
    ephyrShadowUpdate(pScreen, pBuf);
}

static Bool
NestedSaveScreen(ScreenPtr pScreen, int mode) {
    xf86DrvMsg(pScreen->myNum, X_INFO, "NestedSaveScreen\n");
    return TRUE;
}

static Bool
NestedCreateScreenResources(ScreenPtr pScreen) {
    xf86DrvMsg(pScreen->myNum, X_INFO, "NestedCreateScreenResources\n");
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
    Bool ret;

    pScreen->CreateScreenResources = scrpriv->CreateScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = NestedCreateScreenResources;

    if(!shadowAdd(pScreen, pScreen->GetScreenPixmap(pScreen),
                  scrpriv->update, NULL, 0, 0)) {
        xf86DrvMsg(pScreen->myNum, X_ERROR, "NestedCreateScreenResources failed to shadowAdd.\n");
        return FALSE;
    }

    return ret;
}

void
NestedPrintMode(ScrnInfoPtr p, DisplayModePtr m) {
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

void
NestedPrintPscreen(ScrnInfoPtr p) {
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
    NestedPrintMode(p, p->currentMode);
    /*xf86DrvMsg(p->scrnIndex, X_INFO, "depthFrom: %\n");
    xf86DrvMsg(p->scrnIndex, X_INFO, "\n");*/
}

static Bool
NestedCloseScreen(CLOSE_SCREEN_ARGS_DECL) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NestedCloseScreen\n");

    shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));

#if 0 /* Disable event listening temporarily */
    RemoveBlockAndWakeupHandlers(NestedBlockHandler, NestedWakeupHandler, scrpriv);
#endif

    hostx_close_screen(pScrn);
    pScreen->CloseScreen = scrpriv->CloseScreen;
    return (*pScreen->CloseScreen)(CLOSE_SCREEN_ARGS);
}

/* Called at each server generation */
static Bool
NestedScreenInit(SCREEN_INIT_ARGS_DECL) {
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    EphyrScrPrivPtr scrpriv = pScrn->driverPrivate;
    Pixel redMask, greenMask, blueMask;
    char *fb_data;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NestedScreenInit\n");
    NestedPrintPscreen(pScrn);

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

    scrpriv->update = NestedShadowUpdate;
    pScreen->SaveScreen = NestedSaveScreen;

    if (!shadowSetup(pScreen)) {
        return FALSE;
    }

    scrpriv->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = NestedCreateScreenResources;

    scrpriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = NestedCloseScreen;

#if 0 /* Disable event listening temporarily */
    RegisterBlockAndWakeupHandlers(NestedBlockHandler, NestedWakeupHandler, scrpriv);
#endif

    return TRUE;
}

static void
NestedIdentify(int flags) {
    xf86PrintChipsets(EPHYR_NAME, "Driver for nested servers",
                      EphyrChipsets);
}

static Bool
NestedProbe(DriverPtr drv, int flags) {
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
                pScrn->Probe         = NestedProbe;
                pScrn->PreInit       = NestedPreInit;
                pScrn->ScreenInit    = NestedScreenInit;
                pScrn->SwitchMode    = NestedSwitchMode;
                pScrn->AdjustFrame   = NestedAdjustFrame;
                pScrn->EnterVT       = NestedEnterVT;
                pScrn->LeaveVT       = NestedLeaveVT;
                pScrn->FreeScreen    = NestedFreeScreen;
                pScrn->ValidMode     = NestedValidMode;
                foundScreen = TRUE;
            }
        }
    }

    return foundScreen;
}

static const OptionInfoRec *
NestedAvailableOptions(int chipid, int busid) {
    return EphyrOptions;
}

static Bool
NestedDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op, pointer ptr) {
    CARD32 *flag;
    xf86Msg(X_INFO, "NestedDriverFunc\n");

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

static void NestedFreePrivate(ScrnInfoPtr pScrn) {
    if (pScrn->driverPrivate == NULL) {
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                   "Double freeing NestedPrivate!\n");
        return;
    }

    free(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

#if 0 /* Disable event listening temporarily */
static void
NestedBlockHandler(pointer data, OSTimePtr wt, pointer LastSelectMask) {
    ephyrPoll();
}

static void
NestedWakeupHandler(pointer data, int i, pointer LastSelectMask) {
}
#endif

_X_EXPORT DriverRec EPHYR = {
    EPHYR_VERSION,
    EPHYR_DRIVER_NAME,
    NestedIdentify,
    NestedProbe,
    NestedAvailableOptions,
    NULL, /* module */
    0,    /* refCount */
    NestedDriverFunc,
    NULL, /* DeviceMatch */
    0     /* PciProbe */
};

static pointer
NestedSetup(pointer module, pointer opts, int *errmaj, int *errmin) {
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
    NestedSetup,
    NULL, /* teardown */
};
