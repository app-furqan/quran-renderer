//
// HarfBuzz-Skia bridge - Draw and paint callbacks
//

#ifndef QURAN_RENDERER_HB_SKIA_CANVAS_H
#define QURAN_RENDERER_HB_SKIA_CANVAS_H

#include <hb.h>
#include "SkCanvas.h"
#include "SkPath.h"

typedef struct
{
    SkCanvas *canvas;
    SkPath path;
    SkPaint * paint;
    hb_color_t foreground;          // Foreground color for text
    bool use_foreground_override;   // If true, keep foreground fixed (do not update per glyph)
    bool dark_mode;                 // If true, remap near-black palette colors to foreground
} skia_context_t;

void hb_skia_paint_glyph (hb_font_t *font,
                          hb_codepoint_t glyph, void *paint_data,
                          unsigned int palette_index,
                          hb_color_t foreground);

void hb_skia_render_glyph (hb_font_t *font, hb_codepoint_t glyph, void *draw_data);

hb_draw_funcs_t * hb_skia_draw_get_funcs ();
hb_paint_funcs_t * hb_skia_paint_get_funcs ();

#endif //QURAN_RENDERER_HB_SKIA_CANVAS_H
