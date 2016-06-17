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

#ifndef _XLIBS_STUFF_H_
#define _XLIBS_STUFF_H_

#include <X11/X.h>
#include <X11/Xmd.h>
#include <xcb/xcb.h>
#include <xcb/render.h>
#include "ephyr.h"

#define EPHYR_WANT_DEBUG 0

#if (EPHYR_WANT_DEBUG)
#define EPHYR_DBG(x, a...) \
 fprintf(stderr, __FILE__ ":%d,%s() " x "\n", __LINE__, __func__, ##a)
#else
#define EPHYR_DBG(x, a...) do {} while (0)
#endif

int hostx_want_screen_geometry(ScrnInfoPtr pScrn,
                               unsigned int *width, unsigned int *height,
                               int *x, int *y);
int hostx_want_host_cursor(ScrnInfoPtr pScrn);
void hostx_use_sw_cursor(ScrnInfoPtr pScrn);
xcb_cursor_t hostx_get_empty_cursor(ScrnInfoPtr pScrn);
void hostx_get_output_geometry(ScrnInfoPtr pScrn,
                               const char *output,
                               int *x, int *y,
                               unsigned int *width, unsigned int *height);
int hostx_want_preexisting_window(ScrnInfoPtr pScrn);
void hostx_use_preexisting_window(unsigned long win_id);
void hostx_set_title(const char *name);
void hostx_handle_signal(int signum);
void hostx_set_display_name(ScrnInfoPtr pScrn, const char *name);
void hostx_set_screen_number(ScrnInfoPtr pScrn, int number);
void hostx_set_win_title(ScrnInfoPtr pScrn, const char *extra_text);
int hostx_get_depth(ScrnInfoPtr pScrn);
int hostx_get_server_depth(ScrnInfoPtr pScrn);
int hostx_get_bpp(ScrnInfoPtr pScrn);
void hostx_get_visual_masks(ScrnInfoPtr pScrn,
                            CARD32 *rmsk, CARD32 *gmsk, CARD32 *bmsk);
void hostx_set_cmap_entry(ScreenPtr pScreen, unsigned char idx,
                          unsigned char r, unsigned char g, unsigned char b);
void hostx_close_screen(ScrnInfoPtr pScrn);
void *hostx_screen_init(ScrnInfoPtr pScrn,
                        int x, int y,
                        int width, int height, int buffer_height,
                        int *bytes_per_line, int *bits_per_pixel);
void hostx_paint_rect(ScrnInfoPtr pScrn,
                      int sx, int sy, int dx, int dy, int width, int height);
int hostx_get_screen(ScrnInfoPtr pScrn);
int hostx_has_extension(ScrnInfoPtr pScrn, xcb_extension_t *extension);
int hostx_get_fd(ScrnInfoPtr pScrn);

#endif /*_XLIBS_STUFF_H_*/
