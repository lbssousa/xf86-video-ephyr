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

typedef struct EphyrHostXVars EphyrHostXVars;

typedef struct {
    VisualID visualid;
    int screen;
    int depth;
    int class;
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    int colormap_size;
    int bits_per_rgb;
} EphyrHostVisualInfo;

typedef struct {
    int x, y;
    int width, height;
    int visualid;
} EphyrHostWindowAttributes;

typedef struct {
    int x, y, width, height;
} EphyrBox;

typedef struct {
    short x1, y1, x2, y2;
} EphyrRect;

int hostx_want_screen_geometry(ScrnInfoPtr pScrn, int *width, int *height, int *x, int *y);
int hostx_want_host_cursor(void);
void hostx_use_sw_cursor(void);
xcb_cursor_t hostx_get_empty_cursor(void);
void hostx_get_output_geometry(const char *output,
                               int *x, int *y,
                               int *width, int *height);

void hostx_use_fullscreen(void);
int hostx_want_fullscreen(void);
int hostx_want_preexisting_window(ScrnInfoPtr pScrn);
void hostx_use_preexisting_window(unsigned long win_id);
void hostx_use_resname(const char *name, int fromconfig);
void hostx_set_title(char *name);
void hostx_handle_signal(int signum);
Bool hostx_init(void);
Bool hostx_init_window(ScrnInfoPtr pScrn);
void hostx_add_screen(ScrnInfoPtr pScrn, unsigned long win_id, int screen_num, Bool use_geometry, const char *output);
void hostx_set_display_name(char *name);
void hostx_set_screen_number(ScrnInfoPtr pScrn, int number);
void hostx_set_win_title(ScrnInfoPtr pScrn, const char *extra_text);
int hostx_get_depth(void);
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
xcb_connection_t *hostx_get_xcbconn(void);
int hostx_get_screen(void);
int hostx_get_window(int a_screen_number);
int hostx_get_window_attributes(int a_window, EphyrHostWindowAttributes * a_attr);
int hostx_get_visuals_info(EphyrHostVisualInfo ** a_visuals, int *a_num_entries);
int hostx_create_window(int a_screen_number,
                        EphyrBox * a_geometry,
                        int a_visual_id, int *a_host_win /*out parameter */ );
int hostx_destroy_window(int a_win);
int hostx_set_window_geometry(int a_win, EphyrBox * a_geo);
int hostx_set_window_bounding_rectangles(int a_window,
                                         EphyrRect * a_rects, int a_num_rects);
int hostx_has_extension(xcb_extension_t *extension);
int hostx_get_fd(void);

#endif /*_XLIBS_STUFF_H_*/
