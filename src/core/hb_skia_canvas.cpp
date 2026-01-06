//
// HarfBuzz-Skia bridge - Draw and paint callbacks
//

// Suppress HarfBuzz internal header warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wdouble-promotion"

#include <hb.h>
#include <hb-machinery.hh>

#pragma GCC diagnostic pop

// Suppress Skia header warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"

#include "SkPath.h"
#include "SkPathBuilder.h"
#include "SkCanvas.h"

#pragma GCC diagnostic pop

#include "hb_skia_canvas.h"

static void
hb_skia_canvas_move_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
                        void *data,
                        hb_draw_state_t *st,
                        float to_x, float to_y,
                        void *user_data HB_UNUSED)
{
    SkPathBuilder *c = (SkPathBuilder *) data;
    c->moveTo (to_x, to_y);
}

static void
hb_skia_canvas_line_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
                        void *data,
                        hb_draw_state_t *st,
                        float to_x, float to_y,
                        void *user_data HB_UNUSED)
{
    SkPathBuilder *c = (SkPathBuilder *) data;
    c->lineTo(to_x, to_y);
}

static void
hb_skia_canvas_quadratic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
                             void *data,
                             hb_draw_state_t *st,
                             float control_x, float control_y,
                             float to_x, float to_y,
                             void *user_data HB_UNUSED)
{
    SkPathBuilder *c = (SkPathBuilder *) data;
    c->quadTo(control_x, control_y, to_x, to_y);
}

static void
hb_skia_canvas_cubic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
                         void *data,
                         hb_draw_state_t *st,
                         float control1_x, float control1_y,
                         float control2_x, float control2_y,
                         float to_x, float to_y,
                         void *user_data HB_UNUSED)
{
    SkPathBuilder *c = (SkPathBuilder *) data;
    c->cubicTo(control1_x, control1_y, control2_x, control2_y, to_x, to_y);
}

static void
hb_skia_canvas_close_path (hb_draw_funcs_t *dfuncs HB_UNUSED,
                           void *data,
                           hb_draw_state_t *st,
                           void *user_data HB_UNUSED)
{
    SkPathBuilder *c = (SkPathBuilder *) data;
    c->close();
}

static inline void free_static_skia_canvas_funcs();

static struct hb_skia_canvas_funcs_lazy_loader_t : hb_draw_funcs_lazy_loader_t<hb_skia_canvas_funcs_lazy_loader_t>
{
    static hb_draw_funcs_t *create ()
    {
        hb_draw_funcs_t *funcs = hb_draw_funcs_create ();

        hb_draw_funcs_set_move_to_func (funcs, hb_skia_canvas_move_to, nullptr, nullptr);
        hb_draw_funcs_set_line_to_func (funcs, hb_skia_canvas_line_to, nullptr, nullptr);
        hb_draw_funcs_set_quadratic_to_func (funcs, hb_skia_canvas_quadratic_to, nullptr, nullptr);
        hb_draw_funcs_set_cubic_to_func (funcs, hb_skia_canvas_cubic_to, nullptr, nullptr);
        hb_draw_funcs_set_close_path_func (funcs, hb_skia_canvas_close_path, nullptr, nullptr);

        hb_draw_funcs_make_immutable (funcs);

        hb_atexit (free_static_skia_canvas_funcs);

        return funcs;
    }
} static_skia_canvas_funcs;

static inline
void free_static_skia_canvas_funcs ()
{
    static_skia_canvas_funcs.free_instance ();
}

hb_draw_funcs_t *
hb_skia_draw_get_funcs ()
{
    return static_skia_canvas_funcs.get_unconst ();
}

static void
hb_skia_push_clip_glyph (hb_paint_funcs_t *pfuncs HB_UNUSED,
                          void *paint_data,
                          hb_codepoint_t glyph,
                          hb_font_t *font,
                          void *user_data HB_UNUSED)
{
    skia_context_t *c = (skia_context_t *) paint_data;

    SkPathBuilder pathBuilder;
    hb_font_draw_glyph (font, glyph, hb_skia_draw_get_funcs (), &pathBuilder);
    c->path = pathBuilder.detach();
}

static void
hb_skia_paint_color (hb_paint_funcs_t *pfuncs HB_UNUSED,
                      void *paint_data,
                      hb_bool_t use_foreground,
                      hb_color_t color,
                      void *user_data HB_UNUSED)
{
    skia_context_t *c = (skia_context_t *) paint_data;
    
    // Determine which color to use:
    // - If use_foreground_override is set (tajweed OFF), always use the foreground color
    // - If HarfBuzz says use_foreground, use the foreground color
    // - Otherwise use the embedded font color (for tajweed and COLR glyphs)
    //
    // Note: COLR glyphs like ayah numbers are self-contained with their own
    // internal background (white) and foreground (black digits, blue decorations).
    // We render them exactly as designed - they have built-in contrast.
    hb_color_t finalColor;
    if (c->use_foreground_override || use_foreground) {
        finalColor = c->foreground;
    } else {
        finalColor = color;
    }
    
    c->paint->setColor(SkColorSetARGB(
        hb_color_get_alpha(finalColor), 
        hb_color_get_red(finalColor), 
        hb_color_get_green(finalColor), 
        hb_color_get_blue(finalColor)
    ));
    c->canvas->drawPath(c->path, *c->paint);
}

static void
hb_skia_pop_clip (hb_paint_funcs_t *pfuncs HB_UNUSED,
                   void *paint_data,
                   void *user_data HB_UNUSED)
{
    // Empty - no action needed for pop_clip in this implementation
}

static inline void free_static_skia_paint_funcs ();

static struct hb_skia_paint_funcs_lazy_loader_t : hb_paint_funcs_lazy_loader_t<hb_skia_paint_funcs_lazy_loader_t>
{
    static hb_paint_funcs_t *create ()
    {
        hb_paint_funcs_t *paint_funcs = hb_paint_funcs_create ();

        hb_paint_funcs_set_push_clip_glyph_func (paint_funcs, hb_skia_push_clip_glyph, nullptr, nullptr);
        hb_paint_funcs_set_color_func (paint_funcs, hb_skia_paint_color, nullptr, nullptr);
        hb_paint_funcs_set_pop_clip_func (paint_funcs, hb_skia_pop_clip, nullptr, nullptr);

        hb_paint_funcs_make_immutable (paint_funcs);

        hb_atexit (free_static_skia_paint_funcs);

        return paint_funcs;
    }
} static_skia_paint_funcs;

static inline
void free_static_skia_paint_funcs ()
{
    static_skia_paint_funcs.free_instance ();
}

hb_paint_funcs_t *
hb_skia_paint_get_funcs ()
{
    return static_skia_paint_funcs.get_unconst ();
}

void hb_skia_paint_glyph (hb_font_t *font,
                          hb_codepoint_t glyph, 
                          void *paint_data,
                          unsigned int palette_index,
                          hb_color_t foreground)
{
    // Keep HarfBuzz foreground color in sync per glyph.
    // This is required to preserve tajweed colors when the font paints using
    // use_foreground layers.
    skia_context_t *c = (skia_context_t *) paint_data;
    c->foreground = foreground;
    hb_font_paint_glyph (font, glyph, hb_skia_paint_get_funcs(), paint_data, palette_index, foreground);
}

void hb_skia_render_glyph (hb_font_t *font, hb_codepoint_t glyph, void *draw_data)
{
    hb_font_draw_glyph (font, glyph, hb_skia_draw_get_funcs(), draw_data);
}
