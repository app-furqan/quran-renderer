/**
 * QuranRenderer - Core rendering implementation
 * Platform-agnostic Quran text rendering using HarfBuzz and Skia
 */

#include "quran/renderer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

// Suppress Skia header warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Wdouble-promotion"

#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkPath.h"
#include "SkPathBuilder.h"

#pragma GCC diagnostic pop

// Suppress HarfBuzz internal header warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wdouble-promotion"

#include <hb.h>
#include <hb-buffer.hh>
#include <hb-font.hh>

#pragma GCC diagnostic pop

#include "hb_skia_canvas.h"
#include "quran.h"
#include "quran_metadata.h"

#include <string>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <vector>

namespace {

enum class LineType {
    Line = 0,
    Sura = 1,
    Bism = 2
};

enum class JustType {
    just = 0,
    center = 1,
};

struct QuranLine {
    std::string text;
    LineType line_type = LineType::Line;
    JustType just_type = JustType::just;
};

} // anonymous namespace

struct QuranRendererImpl {
    hb_face_t* face = nullptr;
    hb_font_t* font = nullptr;
    hb_draw_funcs_t* draw_funcs = nullptr;
    hb_paint_funcs_t* paint_funcs = nullptr;
    hb_language_t ar_language;
    unsigned int upem = 0;
    
    std::vector<std::vector<QuranLine>> pages;
    std::unordered_map<int, float> lineWidths;
    
    bool tajweed = true;
    int tajweedcolorindex = 152;
    hb_feature_t features[1];
    int coords[2];
    
    // Font data kept alive
    const uint8_t* fontDataPtr = nullptr;
    
    QuranRendererImpl() {
        // Initialize special line widths for certain pages
        lineWidths = {
            { 15 * 585 + 0, 0.81f},
            { 15 * 592 + 1, 0.81f},
            { 15 * 593 + 4, 0.63f},
            { 15 * 599 + 9, 0.63f },
            { 15 * 601 + 4, 0.63f },
            { 15 * 601 + 10, 0.9f },
            { 15 * 601 + 14, 0.53f },
            { 15 * 602 + 9, 0.66f },
            { 15 * 602 + 14, 0.60f },
            { 15 * 603 + 3, 0.55f },
            { 15 * 603 + 8, 0.55f },
            { 15 * 603 + 13, 0.675f },
            { 15 * 603 + 14, 0.5f },
        };
        
        features[0] = { HB_TAG('t', 'j', 'w', 'd'), 1, 0, (unsigned int)-1 };
    }
    
    ~QuranRendererImpl() {
        if (font) hb_font_destroy(font);
        if (face) hb_face_destroy(face);
        if (draw_funcs) hb_draw_funcs_destroy(draw_funcs);
        if (paint_funcs) hb_paint_funcs_destroy(paint_funcs);
    }
    
    bool initialize(const uint8_t* fontData, size_t fontDataSize) {
        fontDataPtr = fontData;
        
        auto blob = hb_blob_create_or_fail(
            reinterpret_cast<const char*>(fontData), 
            fontDataSize, 
            HB_MEMORY_MODE_READONLY, 
            nullptr, nullptr
        );
        if (!blob) return false;
        
        face = hb_face_create(blob, 0);
        hb_blob_destroy(blob);
        
        if (!face) return false;
        
        // Parse Quran text
        parseQuranText();
        
        ar_language = hb_language_from_string("ar", 2);
        upem = hb_face_get_upem(face);
        
        font = hb_font_create(face);
        hb_font_set_scale(font, upem, upem);
        
        draw_funcs = hb_skia_draw_get_funcs();
        paint_funcs = hb_skia_paint_get_funcs();
        
        return true;
    }
    
    void parseQuranText() {
        const char* bism1 = "بِسْمِ ٱللَّهِ ٱلرَّحْمَٰنِ ٱلرَّحِيمِ";
        const char* bism2 = "بِسْمِ ٱللَّهِ ٱلرَّحْمَٰنِ ٱلرَّحِيمِ";
        std::regex begin_sura("^(سُورَة.*)$");
        
        const int pageWidth = 17000;
        
        pages.clear();
        pages.reserve(604);
        
        for (int pageIndex = 0; pageIndex < 604; pageIndex++) {
            auto& page = pages.emplace_back();
            const char* pagetext = qurantext[pageIndex] + 1;
            std::stringstream ss(pagetext);
            QuranLine line;
            int lineIndex = -1;
            
            while (std::getline(ss, line.text, '\n')) {
                lineIndex++;
                
                if (line.text == bism1 || line.text == bism2) {
                    line.line_type = LineType::Bism;
                    line.just_type = JustType::center;
                } else if (regex_match(line.text, begin_sura)) {
                    line.line_type = LineType::Sura;
                    line.just_type = JustType::center;
                } else {
                    line.line_type = LineType::Line;
                    line.just_type = JustType::just;
                }
                
                // Special handling for first two pages (Fatiha)
                if (pageIndex == 0 || pageIndex == 1) {
                    if (lineIndex > 0) {
                        line.just_type = JustType::just;
                        line.line_type = LineType::Line;
                        double ratio = 0.9;
                        double diameter = pageWidth * ratio;
                        double startangle = 30;
                        double endangle = 22.5;
                        double degree = (startangle + (lineIndex - 1) * (180 - (startangle + endangle)) / 6) * M_PI / 180;
                        double lineWidth = diameter * std::sin(degree);
                        lineWidths[15 * pageIndex + lineIndex] = static_cast<float>(lineWidth / pageWidth);
                    } else {
                        line.just_type = JustType::center;
                    }
                }
                
                page.push_back(line);
            }
        }
    }
    
    void drawLine(QuranLine& lineText, skia_context_t* context, double lineWidth, bool justify, double scale) {
        unsigned count = 0;
        hb_glyph_info_t* glyph_info = nullptr;
        hb_glyph_position_t* glyph_pos = nullptr;
        const int spaceCodePoint = 3;
        int currentLineWidth;
        double spaceWidth;
        
        auto canvas = context->canvas;
        
        hb_buffer_t* buffer = hb_buffer_create();
        hb_buffer_set_direction(buffer, HB_DIRECTION_RTL);
        hb_buffer_set_script(buffer, HB_SCRIPT_ARABIC);
        hb_buffer_set_language(buffer, ar_language);
        
        hb_buffer_add_utf8(buffer, lineText.text.c_str(), lineText.text.size(), 0, lineText.text.size());
        
        if (justify && lineText.just_type == JustType::just) {
            hb_buffer_set_justify(buffer, lineWidth);
        }
        
        features[0].value = tajweed ? 1 : 0;
        hb_shape(font, buffer, features, 1);
        
        glyph_info = hb_buffer_get_glyph_infos(buffer, &count);
        glyph_pos = hb_buffer_get_glyph_positions(buffer, &count);
        
        int textWidth = 0;
        int nbSpaces = 0;
        currentLineWidth = 0;
        
        for (int i = count - 1; i >= 0; i--) {
            if (glyph_info[i].codepoint == spaceCodePoint) {
                nbSpaces++;
            } else {
                textWidth += glyph_pos[i].x_advance;
            }
            currentLineWidth += glyph_pos[i].x_advance;
        }
        
        bool applySpaceWidth = false;
        bool changeSize = true;
        
        if (currentLineWidth > lineWidth) {
            if (changeSize) {
                double ratio = (double)lineWidth / currentLineWidth;
                canvas->scale(ratio, ratio);
                scale = scale / ratio;
                currentLineWidth = lineWidth;
                textWidth = textWidth * ratio;
            }
        } else if (textWidth < lineWidth) {
            spaceWidth = (lineWidth - textWidth) / (double)nbSpaces;
            applySpaceWidth = true;
        }
        
        if (lineText.just_type == JustType::center) {
            auto pad = (lineWidth - currentLineWidth) / 2;
            canvas->translate(-pad, 0);
        }
        
        for (int i = count - 1; i >= 0; i--) {
            auto glyph_index = glyph_info[i].codepoint;
            bool extend = false;
            
            if (glyph_info[i].lefttatweel != 0 || glyph_info[i].righttatweel != 0) {
                extend = true;
                coords[0] = roundf(glyph_info[i].lefttatweel * 16384.f);
                coords[1] = roundf(glyph_info[i].righttatweel * 16384.f);
                font->num_coords = 2;
                font->coords = &coords[0];
            }
            
            if (glyph_info[i].codepoint == spaceCodePoint && lineText.just_type == JustType::just && applySpaceWidth) {
                canvas->translate(-spaceWidth, 0);
            } else {
                canvas->translate(-glyph_pos[i].x_advance, 0);
            }
            
            canvas->translate(glyph_pos[i].x_offset, glyph_pos[i].y_offset);
            
            auto color = HB_COLOR(0, 0, 0, 255);
            if (tajweed && glyph_pos[i].lookup_index >= tajweedcolorindex) {
                color = HB_COLOR(
                    (glyph_pos[i].base_codepoint >> 8) & 0xff,
                    (glyph_pos[i].base_codepoint >> 16) & 0xff,
                    (glyph_pos[i].base_codepoint >> 24) & 0xff,
                    255
                );
            }
            hb_font_paint_glyph(font, glyph_index, paint_funcs, context, 0, color);
            
            canvas->translate(-glyph_pos[i].x_offset, -glyph_pos[i].y_offset);
            
            if (extend) {
                font->num_coords = 0;
                font->coords = nullptr;
            }
        }
        
        hb_buffer_destroy(buffer);
    }
    
    void drawPage(void* pixels, int width, int height, int stride, int pageIndex, bool justify, float fontScale = 1.0f) {
        SkImageInfo imageInfo = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kPremul_SkAlphaType); // was Make(width, height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        auto canvas = SkCanvas::MakeRasterDirect(imageInfo, pixels, stride);
        
        canvas->drawColor(SK_ColorWHITE);
        
        SkPaint paint;
        paint.setColor(SK_ColorBLACK);
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kFill_Style);
        
        skia_context_t context{};
        context.canvas = canvas.get();
        context.paint = &paint;
        
        auto& pageText = pages[pageIndex];
        
        // Apply font scale factor (clamp to reasonable range)
        float clampedScale = std::max(0.5f, std::min(2.0f, fontScale));
        
        int char_height = (width / 17) * 0.9 * clampedScale;
        int inter_line = height / 15;
        int y_start = inter_line * 0.72 + (inter_line * (1.0f - clampedScale) * 0.3f);
        int x_padding = width / 42.5;
        
        double scale = (double)char_height / upem;
        int x_start = width - x_padding;
        double pageWidth = (width - 2 * x_padding) / scale;
        
        if (pageIndex == 0 || pageIndex == 1) {
            y_start = y_start + 3.5 * inter_line;
        }
        
        for (size_t lineIndex = 0; lineIndex < pageText.size(); lineIndex++) {
            canvas->resetMatrix();
            
            auto lineWidth = pageWidth;
            
            auto specialWidth = lineWidths.find(pageIndex * 15 + lineIndex);
            if (specialWidth != lineWidths.end()) {
                lineWidth = pageWidth * specialWidth->second;
                float xxstart = (pageWidth - lineWidth) / 2;
                canvas->translate(x_start - xxstart * scale, y_start + lineIndex * inter_line);
            } else {
                canvas->translate(x_start, y_start + lineIndex * inter_line);
            }
            canvas->scale(scale, -scale);
            
            auto& linetext = pageText[lineIndex];
            drawLine(linetext, &context, lineWidth, justify, scale);
        }
    }
    
    void setTajweed(bool enabled) {
        tajweed = enabled;
    }
};

// C API Implementation

extern "C" {

QuranRendererHandle quran_renderer_create(const QuranFontData* fontData) {
    if (!fontData || !fontData->data || fontData->size == 0) {
        return nullptr;
    }
    
    auto renderer = new QuranRendererImpl();
    if (!renderer->initialize(fontData->data, fontData->size)) {
        delete renderer;
        return nullptr;
    }
    
    return renderer;
}

void quran_renderer_destroy(QuranRendererHandle renderer) {
    delete renderer;
}

void quran_renderer_draw_page(
    QuranRendererHandle renderer,
    QuranPixelBuffer* buffer,
    int pageIndex,
    const QuranRenderConfig* config
) {
    if (!renderer || !buffer || !buffer->pixels) return;
    if (pageIndex < 0 || pageIndex >= 604) return;
    
    renderer->setTajweed(config ? config->tajweed : true);
    renderer->drawPage(
        buffer->pixels,
        buffer->width,
        buffer->height,
        buffer->stride,
        pageIndex,
        config ? config->justify : true,
        config ? config->fontScale : 1.0f
    );
}

int quran_renderer_get_page_count(QuranRendererHandle renderer) {
    return renderer ? 604 : 0;
}

// ============================================================================
// Surah/Ayah API Implementation
// ============================================================================

int quran_renderer_get_surah_count(void) {
    return QURAN_SURAH_COUNT;
}

int quran_renderer_get_total_ayah_count(void) {
    return QURAN_TOTAL_AYAHS;
}

bool quran_renderer_get_surah_info(int surahNumber, QuranSurahInfo* info) {
    if (surahNumber < 1 || surahNumber > 114 || !info) {
        return false;
    }
    
    const SurahData& surah = SURAH_DATA[surahNumber];
    info->number = surah.number;
    info->ayahCount = surah.ayahCount;
    info->startAyah = surah.startAyah;
    info->nameArabic = surah.nameArabic;
    info->nameTrans = surah.nameTrans;
    info->nameEnglish = surah.nameEnglish;
    info->type = surah.type;
    info->revelationOrder = surah.revelationOrder;
    info->rukuCount = surah.rukuCount;
    
    return true;
}

int quran_renderer_get_surah_start_page(int surahNumber) {
    if (surahNumber < 1 || surahNumber > 114) {
        return -1;
    }
    
    // Find the first page where this surah starts
    for (int i = 0; i < QURAN_PAGE_COUNT; i++) {
        if (PAGE_LOCATIONS[i].surahNumber == surahNumber && PAGE_LOCATIONS[i].ayahNumber == 1) {
            return i;
        }
        // Also check if we're past this surah already (it must have started before)
        if (PAGE_LOCATIONS[i].surahNumber > surahNumber) {
            // Go back to find where it started
            for (int j = i - 1; j >= 0; j--) {
                if (PAGE_LOCATIONS[j].surahNumber == surahNumber) {
                    return j;
                }
                if (PAGE_LOCATIONS[j].surahNumber < surahNumber) {
                    return j + 1;
                }
            }
        }
    }
    
    // Surah not found in page locations (shouldn't happen)
    return -1;
}

int quran_renderer_get_ayah_page(int surahNumber, int ayahNumber) {
    if (surahNumber < 1 || surahNumber > 114) {
        return -1;
    }
    
    const SurahData& surah = SURAH_DATA[surahNumber];
    if (ayahNumber < 1 || ayahNumber > surah.ayahCount) {
        return -1;
    }
    
    // Find the page containing this ayah
    // We need to find the last page where the starting location is <= our target ayah
    for (int i = QURAN_PAGE_COUNT - 1; i >= 0; i--) {
        const PageLocation& loc = PAGE_LOCATIONS[i];
        if (loc.surahNumber < surahNumber) {
            return i + 1;  // Our ayah is on the next page
        }
        if (loc.surahNumber == surahNumber && loc.ayahNumber <= ayahNumber) {
            return i;
        }
    }
    
    return 0;  // Default to first page
}

bool quran_renderer_get_page_location(int pageIndex, QuranAyahLocation* location) {
    if (pageIndex < 0 || pageIndex >= QURAN_PAGE_COUNT || !location) {
        return false;
    }
    
    const PageLocation& loc = PAGE_LOCATIONS[pageIndex];
    location->surahNumber = loc.surahNumber;
    location->ayahNumber = loc.ayahNumber;
    location->pageIndex = pageIndex;
    
    return true;
}

int quran_renderer_get_ayah_count(int surahNumber) {
    if (surahNumber < 1 || surahNumber > 114) {
        return -1;
    }
    
    return SURAH_DATA[surahNumber].ayahCount;
}

} // extern "C"
