/**
 * QuranRenderer - Core rendering implementation
 * Platform-agnostic Quran text rendering using HarfBuzz and Skia
 */

#include "quran/renderer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>

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
#include <hb-ot.h>

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

// Calculate relative luminance of a color (0.0 = black, 1.0 = white)
// Uses sRGB luminance formula: https://www.w3.org/TR/WCAG20/#relativeluminancedef
inline float calculateLuminance(uint8_t r, uint8_t g, uint8_t b) {
    return (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
}

// Determine if background is dark (luminance < 0.5)
inline bool isDarkBackground(uint32_t backgroundColor) {
    uint8_t r = (backgroundColor >> 24) & 0xFF;
    uint8_t g = (backgroundColor >> 16) & 0xFF;
    uint8_t b = (backgroundColor >> 8) & 0xFF;
    return calculateLuminance(r, g, b) < 0.5f;
}

// Get appropriate text color for background
inline hb_color_t getTextColorForBackground(uint32_t backgroundColor) {
    return isDarkBackground(backgroundColor) 
        ? HB_COLOR(255, 255, 255, 255)  // White text for dark backgrounds
        : HB_COLOR(0, 0, 0, 255);        // Black text for light backgrounds
}

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
    unsigned int tajweedcolorindex = 0xFFFF;
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
        
        // Tajweed color detection:
        // Check if font has tajweed color support via GPOS lookup count
        // Fonts with >150 GPOS lookups likely have tajweed-specific lookups starting around index 152
        // Note: DigitalKhattV2 only has 140 lookups and uses external regex-based tajweed coloring
        unsigned int gpos_lookup_count = hb_ot_layout_table_get_lookup_count(face, HB_OT_TAG_GPOS);
        if (gpos_lookup_count > 150) {
            // Font has embedded tajweed support
            tajweedcolorindex = 152;
        }
        // Otherwise keep default 0xFFFF (disabled, will fall back to base_codepoint check)
        
        return true;
    }
    
    // Structure to hold line extent measurements
    struct LineExtents {
        int maxAscent;   // Maximum height above baseline (positive)
        int maxDescent;  // Maximum depth below baseline (positive)
        int totalHeight; // maxAscent + maxDescent
    };
    
    // Measure the vertical extents of a single line of text
    // Returns the max ascent (above baseline) and descent (below baseline) in font units
    LineExtents measureLineExtents(const std::string& text, double lineWidth, bool justify, bool measureTajweed) {
        LineExtents extents = {0, 0, 0};
        
        hb_buffer_t* buffer = hb_buffer_create();
        hb_buffer_set_direction(buffer, HB_DIRECTION_RTL);
        hb_buffer_set_script(buffer, HB_SCRIPT_ARABIC);
        hb_buffer_set_language(buffer, ar_language);
        
        hb_buffer_add_utf8(buffer, text.c_str(), text.size(), 0, text.size());
        
        if (justify) {
            hb_buffer_set_justify(buffer, lineWidth);
        }
        
        // Shape for measurement.
        // IMPORTANT: when tajweed is enabled, glyph substitutions/marks can change extents,
        // so measurement must match the actual shaping settings to avoid overlaps.
        hb_feature_t measureFeatures[1] = {{ HB_TAG('t', 'j', 'w', 'd'), measureTajweed ? 1u : 0u, 0, (unsigned int)-1 }};
        hb_shape(font, buffer, measureFeatures, 1);
        
        unsigned int count;
        hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buffer, &count);
        hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buffer, &count);
        
        for (unsigned int i = 0; i < count; i++) {
            hb_glyph_extents_t glyph_extents;
            if (hb_font_get_glyph_extents(font, glyph_info[i].codepoint, &glyph_extents)) {
                // y_bearing is the top of the glyph relative to baseline (positive = above)
                // height is negative (extends downward from y_bearing)
                int glyphTop = glyph_extents.y_bearing + glyph_pos[i].y_offset;
                int glyphBottom = glyph_extents.y_bearing + glyph_extents.height + glyph_pos[i].y_offset;
                
                // Track maximum extent above baseline (positive values)
                if (glyphTop > extents.maxAscent) {
                    extents.maxAscent = glyphTop;
                }
                
                // Track maximum extent below baseline (negative values, convert to positive)
                if (glyphBottom < 0 && -glyphBottom > extents.maxDescent) {
                    extents.maxDescent = -glyphBottom;
                }
            }
        }
        
        extents.totalHeight = extents.maxAscent + extents.maxDescent;
        
        hb_buffer_destroy(buffer);
        return extents;
    }

    struct PageExtents {
        int maxAscent;
        int maxDescent;
        int requiredLineHeight; // maxAscent + maxDescent (worst-case across different lines)
    };

    PageExtents calculatePageExtentsUnits(int pageIndex, double pageWidth, bool justify) {
        if (pageIndex < 0 || pageIndex >= (int)pages.size()) {
            return {0, 0, 0};
        }

        auto& pageText = pages[pageIndex];

        int maxAscent = 0;
        int maxDescent = 0;
        for (size_t lineIndex = 0; lineIndex < pageText.size(); lineIndex++) {
            auto& linetext = pageText[lineIndex];

            double lineWidth = pageWidth;
            auto specialWidth = lineWidths.find(pageIndex * 15 + lineIndex);
            if (specialWidth != lineWidths.end()) {
                lineWidth = pageWidth * specialWidth->second;
            }

            bool shouldJustify = justify && (linetext.just_type == JustType::just);
            bool measureTajweed = tajweed && (linetext.line_type != LineType::Sura);
            LineExtents extents = measureLineExtents(linetext.text, lineWidth, shouldJustify, measureTajweed);

            // NOTE: max ascent and max descent can occur on different lines.
            // For safe inter-line spacing we need inter_line >= maxAscent + maxDescent.
            if (extents.maxAscent > maxAscent) maxAscent = extents.maxAscent;
            if (extents.maxDescent > maxDescent) maxDescent = extents.maxDescent;
        }

        return {maxAscent, maxDescent, maxAscent + maxDescent};
    }
    
    // Calculate the optimal line height for a page to avoid overlaps
    // Returns the minimum inter_line spacing needed in pixels
    int calculateOptimalLineHeight(int pageIndex, int width, int height, int x_padding, float fontScale = 1.0f) {
        if (pageIndex < 0 || pageIndex >= (int)pages.size()) {
            return height / 10;  // Default fallback
        }
        
        auto& pageText = pages[pageIndex];
        bool isFatihaPage = (pageIndex == 0 || pageIndex == 1);
        
        // Calculate char_height and scale (same as in drawPage)
        float clampedScale = std::max(0.5f, std::min(2.0f, fontScale));
        int char_height = static_cast<int>((width / 17.0) * 0.9 * clampedScale);
        double scale = (double)char_height / upem;
        
        // Calculate page width dynamically like mushaf-android
        // This ensures measurement calculations match rendering on all orientations
        double pageWidth = (width - 2 * x_padding) / scale;
        
        // Measure all lines and find the maximum required spacing
        int maxTotalHeight = 0;
        
        for (size_t lineIndex = 0; lineIndex < pageText.size(); lineIndex++) {
            auto& linetext = pageText[lineIndex];
            
            // Get line width for this specific line
            double lineWidth = pageWidth;
            auto specialWidth = lineWidths.find(pageIndex * 15 + lineIndex);
            if (specialWidth != lineWidths.end()) {
                lineWidth = pageWidth * specialWidth->second;
            }
            
            bool shouldJustify = (linetext.just_type == JustType::just);
            LineExtents extents = measureLineExtents(linetext.text, lineWidth, shouldJustify, false);
            
            if (extents.totalHeight > maxTotalHeight) {
                maxTotalHeight = extents.totalHeight;
            }
        }
        
        // Convert from font units to pixels
        int maxHeightPixels = static_cast<int>(maxTotalHeight * scale);
        
        // Add a tiny buffer (2%) just to prevent pixel-level touching
        int requiredLineHeight = static_cast<int>(maxHeightPixels * 1.02);
        
        // CRITICAL: Ensure 15 lines fit within the page height
        // The layout places lines at: y_start + lineIndex * inter_line
        // where y_start = inter_line * 0.72 (for first line baseline)
        // Last line baseline: 0.72 * inter_line + 14 * inter_line = 14.72 * inter_line
        // Text below baseline needs ~0.28 * inter_line, so max Y ≈ 15 * inter_line
        // To fit within page: inter_line <= height / 15
        int maxLineHeight = height / 15;
        
        // Use the smaller of the required height and max that fits
        return std::min(requiredLineHeight, maxLineHeight);
    }
    
    // Draw a decorative surah name frame
    // Approximation of ayaframe.svg from DigitalKhatt
    // The original SVG has intricate wave decorations at both ends
    void drawSurahFrame(SkCanvas* canvas, float x, float y, float width, float height, uint32_t backgroundColor) {
        // Determine if dark mode based on background luminance
        uint8_t bg_r = (backgroundColor >> 24) & 0xFF;
        uint8_t bg_g = (backgroundColor >> 16) & 0xFF;
        uint8_t bg_b = (backgroundColor >> 8) & 0xFF;
        float luminance = (0.299f * bg_r + 0.587f * bg_g + 0.114f * bg_b) / 255.0f;
        bool isDarkMode = luminance < 0.5f;
        
        // Colors from the original SVG
        // Main fill: #1c7897 (teal/blue)
        // Inner fill: #ffffff (white)
        // Stroke: #000000 (black, thin)
        SkColor outerColor = isDarkMode ? SkColorSetRGB(0x43, 0xB4, 0xE5) : SkColorSetRGB(0x1C, 0x78, 0x97);
        SkColor innerColor = isDarkMode ? SkColorSetARGB(255, 0x1A, 0x1A, 0x1A) : SK_ColorWHITE;
        SkColor strokeColor = isDarkMode ? SkColorSetRGB(0x43, 0xB4, 0xE5) : SK_ColorBLACK;
        
        // The SVG viewBox is 84.74 x 10.67, so aspect ratio is about 7.94:1
        // Scale factors for the ornate ends
        float endWidth = height * 1.2f;  // Width of ornate section at each end
        float centerStart = x + endWidth;
        float centerEnd = x + width - endWidth;
        float centerWidth = centerEnd - centerStart;
        
        // Outer fill paint
        SkPaint outerPaint;
        outerPaint.setAntiAlias(true);
        outerPaint.setStyle(SkPaint::kFill_Style);
        outerPaint.setColor(outerColor);
        
        // Inner fill paint (white background for the text)
        SkPaint innerPaint;
        innerPaint.setAntiAlias(true);
        innerPaint.setStyle(SkPaint::kFill_Style);
        innerPaint.setColor(innerColor);
        
        // Stroke paint
        SkPaint strokePaint;
        strokePaint.setAntiAlias(true);
        strokePaint.setStyle(SkPaint::kStroke_Style);
        strokePaint.setStrokeWidth(height * 0.01f);
        strokePaint.setColor(strokeColor);
        
        // Build the outer frame shape with wave decorations
        SkPathBuilder outer;
        
        // The frame shape: rectangular center with ornate wave ends
        // Start at top-left of center section
        float cy = y + height * 0.5f;  // Center Y
        float topY = y + height * 0.1f;
        float bottomY = y + height * 0.9f;
        float waveHeight = height * 0.35f;
        
        // Right end ornate waves (more elaborate)
        float rx = x + width;  // Right edge
        outer.moveTo(centerEnd, topY);
        
        // Wave pattern going right - multiple curves creating ornate design
        outer.cubicTo(centerEnd + endWidth * 0.2f, topY,
                      centerEnd + endWidth * 0.3f, y,
                      centerEnd + endWidth * 0.5f, y);
        outer.cubicTo(centerEnd + endWidth * 0.7f, y,
                      centerEnd + endWidth * 0.8f, cy - waveHeight,
                      rx - endWidth * 0.3f, cy - waveHeight * 0.5f);
        outer.cubicTo(rx - endWidth * 0.1f, cy - waveHeight * 0.3f,
                      rx, cy - waveHeight * 0.2f,
                      rx, cy);
        outer.cubicTo(rx, cy + waveHeight * 0.2f,
                      rx - endWidth * 0.1f, cy + waveHeight * 0.3f,
                      rx - endWidth * 0.3f, cy + waveHeight * 0.5f);
        outer.cubicTo(centerEnd + endWidth * 0.8f, cy + waveHeight,
                      centerEnd + endWidth * 0.7f, y + height,
                      centerEnd + endWidth * 0.5f, y + height);
        outer.cubicTo(centerEnd + endWidth * 0.3f, y + height,
                      centerEnd + endWidth * 0.2f, bottomY,
                      centerEnd, bottomY);
        
        // Bottom edge (straight)
        outer.lineTo(centerStart, bottomY);
        
        // Left end ornate waves (mirror of right)
        float lx = x;  // Left edge
        outer.cubicTo(centerStart - endWidth * 0.2f, bottomY,
                      centerStart - endWidth * 0.3f, y + height,
                      centerStart - endWidth * 0.5f, y + height);
        outer.cubicTo(centerStart - endWidth * 0.7f, y + height,
                      centerStart - endWidth * 0.8f, cy + waveHeight,
                      lx + endWidth * 0.3f, cy + waveHeight * 0.5f);
        outer.cubicTo(lx + endWidth * 0.1f, cy + waveHeight * 0.3f,
                      lx, cy + waveHeight * 0.2f,
                      lx, cy);
        outer.cubicTo(lx, cy - waveHeight * 0.2f,
                      lx + endWidth * 0.1f, cy - waveHeight * 0.3f,
                      lx + endWidth * 0.3f, cy - waveHeight * 0.5f);
        outer.cubicTo(centerStart - endWidth * 0.8f, cy - waveHeight,
                      centerStart - endWidth * 0.7f, y,
                      centerStart - endWidth * 0.5f, y);
        outer.cubicTo(centerStart - endWidth * 0.3f, y,
                      centerStart - endWidth * 0.2f, topY,
                      centerStart, topY);
        
        // Top edge back to start
        outer.close();
        
        // Draw outer fill
        canvas->drawPath(outer.detach(), outerPaint);
        
        // Build inner white area (slightly inset)
        float inset = height * 0.08f;
        SkPathBuilder inner;
        float innerTopY = topY + inset;
        float innerBottomY = bottomY - inset;
        float innerCenterStart = centerStart + inset * 0.5f;
        float innerCenterEnd = centerEnd - inset * 0.5f;
        float innerEndWidth = endWidth - inset;
        float innerWaveHeight = waveHeight * 0.8f;
        
        inner.moveTo(innerCenterEnd, innerTopY);
        
        // Simplified inner curves (less elaborate)
        inner.cubicTo(innerCenterEnd + innerEndWidth * 0.3f, innerTopY,
                      innerCenterEnd + innerEndWidth * 0.5f, y + inset,
                      innerCenterEnd + innerEndWidth * 0.6f, cy);
        inner.cubicTo(innerCenterEnd + innerEndWidth * 0.5f, y + height - inset,
                      innerCenterEnd + innerEndWidth * 0.3f, innerBottomY,
                      innerCenterEnd, innerBottomY);
        
        inner.lineTo(innerCenterStart, innerBottomY);
        
        inner.cubicTo(innerCenterStart - innerEndWidth * 0.3f, innerBottomY,
                      innerCenterStart - innerEndWidth * 0.5f, y + height - inset,
                      innerCenterStart - innerEndWidth * 0.6f, cy);
        inner.cubicTo(innerCenterStart - innerEndWidth * 0.5f, y + inset,
                      innerCenterStart - innerEndWidth * 0.3f, innerTopY,
                      innerCenterStart, innerTopY);
        
        inner.close();
        
        // Draw inner fill
        canvas->drawPath(inner.detach(), innerPaint);
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
    
    void drawLine(QuranLine& lineText, skia_context_t* context, double lineWidth, bool justify, double scale, hb_color_t defaultTextColor = HB_COLOR(0, 0, 0, 255), bool disableTajweed = false) {
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
        
        // Disable tajweed for surah name lines - they should be plain black text
        bool useTajweed = tajweed && !disableTajweed;
        features[0].value = useTajweed ? 1 : 0;
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
            // Only apply space-stretching if gap is significant (>1% of line width).
            // When kashida justification is active, small gaps are acceptable.
            double gap = lineWidth - currentLineWidth;
            if (gap > lineWidth * 0.01 && nbSpaces > 0) {
                spaceWidth = (lineWidth - textWidth) / (double)nbSpaces;
                applySpaceWidth = true;
            }
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
            
            // Apply glyph offset for proper vowel mark positioning (hamza, harakat, etc.)
            // Match mushaf-android: use y_offset directly without negation
            canvas->translate(glyph_pos[i].x_offset, glyph_pos[i].y_offset);
            
            // Tajweed color handling:
            // DigitalKhatt fonts can encode tajweed colors in two ways:
            // 1. Embedded in base_codepoint during GPOS processing (older fonts)
            // 2. External application-level logic via regex analysis (DigitalKhattV2 and web implementation)
            //
            // This implementation handles method #1. For DigitalKhattV2, tajweed colors
            // are determined by JavaScript regex in tajweed.service.ts on the web, not embedded in the font.
            // The color categories are: green (idgham/ikhfa), tafkim (dark blue), lgray (silent letters),
            // lkalkala (light blue), red1-4 (various madd counts).
            //
            // If using DigitalKhattV2 and need tajweed colors, implement the regex patterns from:
            // https://github.com/DigitalKhatt/digitalkhatt.org/blob/master/ClientApp/src/app/services/tajweed.service.ts
            
            auto color = defaultTextColor; // Use computed text color based on background
            
            // Tajweed color check: lookup_index >= tajweedcolorindex indicates a tajweed lookup was applied
            // and base_codepoint contains the RGB color encoded by HarfBuzz during GPOS processing
            if (useTajweed && glyph_pos[i].lookup_index >= tajweedcolorindex) {
                color = HB_COLOR(
                    (glyph_pos[i].base_codepoint >> 8) & 0xff,
                    (glyph_pos[i].base_codepoint >> 16) & 0xff,
                    (glyph_pos[i].base_codepoint >> 24) & 0xff,
                    255
                );
            }
            // Update context foreground before painting so COLR use_foreground layers
            // can access it.
            context->foreground = color;
            hb_font_paint_glyph(font, glyph_index, paint_funcs, context, 0, color);
            
            // Reverse the offset translation (negate both x and y)
            canvas->translate(-glyph_pos[i].x_offset, -glyph_pos[i].y_offset);
            
            if (extend) {
                font->num_coords = 0;
                font->coords = nullptr;
            }
        }
        
        hb_buffer_destroy(buffer);
    }
    
    void drawPage(void* pixels, int width, int height, int stride, int pageIndex, bool justify, float fontScale = 1.0f, uint32_t backgroundColor = 0xFFFFFFFF, int fontSize = 0, bool useForeground = false, float lineHeightDivisor = 0.0f, float topMarginLines = -1.0f, QuranPixelFormat format = QURAN_PIXEL_FORMAT_RGBA8888) {
        // Respect the pixel format - critical for cross-platform compatibility
        // Android uses RGBA8888, iOS/macOS may use BGRA8888
        SkColorType colorType = (format == QURAN_PIXEL_FORMAT_BGRA8888)
            ? kBGRA_8888_SkColorType
            : kRGBA_8888_SkColorType;
        SkImageInfo imageInfo = SkImageInfo::Make(width, height, colorType, kPremul_SkAlphaType);
        auto canvas = SkCanvas::MakeRasterDirect(imageInfo, pixels, stride);
        
        // Extract RGBA components from backgroundColor (0xRRGGBBAA format)
        uint8_t bg_r = (backgroundColor >> 24) & 0xFF;
        uint8_t bg_g = (backgroundColor >> 16) & 0xFF;
        uint8_t bg_b = (backgroundColor >> 8) & 0xFF;
        uint8_t bg_a = backgroundColor & 0xFF;
        canvas->drawColor(SkColorSetARGB(bg_a, bg_r, bg_g, bg_b));
        
        SkPaint paint;
        paint.setColor(SK_ColorBLACK);
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kFill_Style);
        
        skia_context_t context{};
        context.canvas = canvas.get();
        context.paint = &paint;
        context.foreground = HB_COLOR(0, 0, 0, 255);
        context.backgroundColor = HB_COLOR(bg_r, bg_g, bg_b, bg_a);
        context.use_foreground_override = useForeground;
        
        auto& pageText = pages[pageIndex];
        
        // Determine if this is a Fatiha page (special layout)
        bool isFatihaPage = (pageIndex == 0 || pageIndex == 1);
        
        // Font size and line height calculation - EXACTLY match mushaf-android
        // Lines 239-242 of mushaf-android/app/src/main/cpp/text-rendering.cpp:
        //     char_height = (dstInfo.width / 17 ) * 0.9;
        //     inter_line = dstInfo.height / 15;
        //     y_start = inter_line * 0.72;
        //     int x_padding = dstInfo.width / 42.5;
        int char_height = static_cast<int>((width / 17.0) * 0.9);
        int inter_line = height / 15;
        int x_padding = width / 42.5;
        
        // Apply fontScale to char_height if specified
        float clampedScale = std::max(0.5f, std::min(2.0f, fontScale));
        char_height = static_cast<int>(char_height * clampedScale);
        
        // Override with explicit fontSize if provided
        if (fontSize > 0) {
            char_height = fontSize;
        }
        
        // Calculate render scale from char_height
        double renderScale = (double)char_height / upem;
        
        // CRITICAL FIX: Calculate page width dynamically like mushaf-android
        // This ensures text adjusts properly on orientation changes
        // mushaf-android: double pageWidth = (dstInfo.width - 2*x_padding) / scale;
        double pageWidth = (width - 2 * x_padding) / renderScale;
        // y_start positions the first line's baseline.
        // Arabic text needs room above the baseline for marks (fatha, damma, shadda, etc.)
        // Following mushaf-android: baseline at ~72% down from line slot top
        // This leaves ~28% of line height for marks above, ~72% for base + marks below
        int y_start = static_cast<int>(inter_line * 0.72);
        
        int x_start = width - x_padding;
        
        // CRITICAL: Use a fixed reference page width for HarfBuzz justification.
        // HarfBuzz kashida justification and glyph positioning are calculated based on
        // the target line width in font units. If this varies with font size, the glyph
        // offsets (x_offset, y_offset) change, causing vowel marks to shift.
        // 
        // mushafuses a fixed pageWidth of 17000 units. We calculate the actual
        // rendering area based on screen dimensions and derive a consistent reference.
        // The key is that the RATIO of lineWidth to pageWidth stays consistent.
        // Use reference page width for HarfBuzz justification (consistent glyph positioning)
        // Render scale is computed above (constrained by width + height).
        
        // Top margin: user-specified or auto
        // Auto: 0 for regular pages (start at top), 0 for Fatiha too (removed old 3.5 offset)
        // User can set topMarginLines to add spacing at top if desired
        float effectiveTopMargin = topMarginLines;
        if (effectiveTopMargin < 0.0f) {
            // Auto mode: no extra top margin - start from the top of the page
            effectiveTopMargin = 0.0f;
        }
        
        if (effectiveTopMargin > 0.0f) {
            y_start = y_start + static_cast<int>(effectiveTopMargin * inter_line);
        }
        
        // Compute text color based on background luminance
        hb_color_t textColor = getTextColorForBackground(backgroundColor);

        // Ensure foreground color follows the computed text color.
        // This matters when COLR/painted glyphs request "use_foreground"; in that case
        // hb_skia_paint_color may use context.foreground (when override is enabled).
        context.foreground = textColor;
        paint.setColor(SkColorSetARGB(
            hb_color_get_alpha(textColor),
            hb_color_get_red(textColor),
            hb_color_get_green(textColor),
            hb_color_get_blue(textColor)
        ));
        
        for (size_t lineIndex = 0; lineIndex < pageText.size(); lineIndex++) {
            canvas->resetMatrix();
            
            auto lineWidth = pageWidth;
            
            auto specialWidth = lineWidths.find(pageIndex * 15 + lineIndex);
            if (specialWidth != lineWidths.end()) {
                lineWidth = pageWidth * specialWidth->second;
                float xxstart = (pageWidth - lineWidth) / 2;
                canvas->translate(x_start - xxstart * renderScale, y_start + lineIndex * inter_line);
            } else {
                canvas->translate(x_start, y_start + lineIndex * inter_line);
            }
            
            auto& linetext = pageText[lineIndex];
            
            // Draw surah name frame for Sura lines
            if (linetext.line_type == LineType::Sura) {
                canvas->resetMatrix();
                // Frame dimensions - spans most of the page width
                float frameWidth = (width - 2 * x_padding) * 0.85f;
                float frameHeight = inter_line * 0.7f;
                float frameX = x_padding + (width - 2 * x_padding - frameWidth) / 2;
                float frameY = y_start + lineIndex * inter_line - inter_line * 0.55f;
                
                drawSurahFrame(canvas.get(), frameX, frameY, frameWidth, frameHeight, backgroundColor);
                
                // Reset canvas for text drawing
                canvas->resetMatrix();
                canvas->translate(x_start, y_start + lineIndex * inter_line);
            }
            
            canvas->scale(renderScale, -renderScale);
            
            // Disable tajweed coloring for surah name lines - they should be plain text
            bool disableTajweed = (linetext.line_type == LineType::Sura);
            drawLine(linetext, &context, lineWidth, justify, renderScale, textColor, disableTajweed);
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
        config ? config->fontScale : 1.0f,
        config ? config->backgroundColor : 0xFFFFFFFF,
        config ? config->fontSize : 0,
        config ? config->useForeground : false,
        config ? config->lineHeightDivisor : 0.0f,
        config ? config->topMarginLines : -1.0f,
        buffer->format  // Pass pixel format through to renderer
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

// ============================================================================
// Generic Arabic Text Rendering Implementation
// ============================================================================

int quran_renderer_draw_text(
    QuranRendererHandle renderer,
    QuranPixelBuffer* buffer,
    const char* text,
    int textLength,
    const QuranTextConfig* config
) {
    if (!renderer || !buffer || !buffer->pixels || !text) {
        return -1;
    }
    
    // Handle null-terminated strings
    size_t len = (textLength < 0) ? strlen(text) : static_cast<size_t>(textLength);
    if (len == 0) {
        return 0;
    }
    
    // Extract configuration with defaults for auto (0) values
    uint32_t bgColor = config ? config->backgroundColor : 0xFFFFFFFF;
    bool justify = config ? config->justify : false;
    float targetWidth = config ? config->lineWidth : 0;
    
    // Auto line width: use buffer width minus padding
    if (targetWidth <= 0) {
        targetWidth = buffer->width - 20.0f;  // 10px padding on each side
    }
    
    // Auto font size: calculate based on buffer width (matches mushaf-android)
    int fontSize = config ? config->fontSize : 0;
    if (fontSize <= 0) {
        // Auto-calculate: (width / 17) * 0.9 - same formula as page rendering
        fontSize = static_cast<int>((buffer->width / 17.0f) * 0.9f);
        if (fontSize < 12) fontSize = 12;  // Minimum readable size
    }
    
    // Auto-detect text color if not specified (0 means auto)
    uint32_t textColor;
    if (config && config->textColor != 0) {
        textColor = config->textColor;
    } else {
        // Auto: white text for dark backgrounds, black for light
        textColor = isDarkBackground(bgColor) ? 0xFFFFFFFF : 0x000000FF;
    }
    
    // Set up Skia canvas with correct pixel format
    SkColorType colorType = (buffer->format == QURAN_PIXEL_FORMAT_BGRA8888)
        ? kBGRA_8888_SkColorType
        : kRGBA_8888_SkColorType;
    SkImageInfo imageInfo = SkImageInfo::Make(
        buffer->width, buffer->height,
        colorType, kPremul_SkAlphaType
    );
    auto canvas = SkCanvas::MakeRasterDirect(imageInfo, buffer->pixels, buffer->stride);
    
    // Clear with background color
    uint8_t bg_r = (bgColor >> 24) & 0xFF;
    uint8_t bg_g = (bgColor >> 16) & 0xFF;
    uint8_t bg_b = (bgColor >> 8) & 0xFF;
    uint8_t bg_a = bgColor & 0xFF;
    canvas->drawColor(SkColorSetARGB(bg_a, bg_r, bg_g, bg_b));
    
    // Set up paint
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    
    // Set up rendering context
    skia_context_t context{};
    context.canvas = canvas.get();
    context.paint = &paint;
    context.backgroundColor = HB_COLOR(bg_r, bg_g, bg_b, bg_a);
    
    // Extract text color
    uint8_t txt_r = (textColor >> 24) & 0xFF;
    uint8_t txt_g = (textColor >> 16) & 0xFF;
    uint8_t txt_b = (textColor >> 8) & 0xFF;
    hb_color_t hbTextColor = HB_COLOR(txt_r, txt_g, txt_b, 255);
    context.foreground = hbTextColor;
    
    // Tajweed handling: when tajweed is enabled, allow font colors through (use_foreground_override=false)
    // When disabled, force all glyphs to use foreground color (use_foreground_override=true)
    bool useTajweed = config ? config->tajweed : true;
    context.use_foreground_override = !useTajweed;  // false when tajweed enabled = allow font colors
    
    // Create and shape the text
    hb_buffer_t* hbBuffer = hb_buffer_create();
    hb_buffer_set_direction(hbBuffer, HB_DIRECTION_RTL);
    hb_buffer_set_script(hbBuffer, HB_SCRIPT_ARABIC);
    hb_buffer_set_language(hbBuffer, renderer->ar_language);
    
    hb_buffer_add_utf8(hbBuffer, text, len, 0, len);
    
    // Calculate line width in font units
    double scale = static_cast<double>(fontSize) / renderer->upem;
    double lineWidth = (targetWidth > 0) ? targetWidth / scale : (buffer->width - 20) / scale;
    
    if (justify) {
        hb_buffer_set_justify(hbBuffer, lineWidth);
    }
    
    // Shape with tajweed based on config (useTajweed already set earlier)
    renderer->features[0].value = useTajweed ? 1 : 0;
    hb_shape(renderer->font, hbBuffer, renderer->features, 1);
    
    // Get glyph data
    unsigned int count;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(hbBuffer, &count);
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(hbBuffer, &count);
    
    // Calculate text width
    int totalWidth = 0;
    for (unsigned int i = 0; i < count; i++) {
        totalWidth += glyph_pos[i].x_advance;
    }
    
    // Calculate margins for positioning
    float marginRight = config ? config->marginRight : -1.0f;
    float marginLeft = config ? config->marginLeft : -1.0f;
    
    // Auto margin: 5% of buffer width, minimum 10px
    if (marginRight < 0) {
        marginRight = std::max(10.0f, buffer->width * 0.05f);
    }
    if (marginLeft < 0) {
        marginLeft = std::max(10.0f, buffer->width * 0.05f);
    }
    
    // Position text (RTL: start from right edge minus right margin)
    int x_start = static_cast<int>(buffer->width - marginRight);
    int y_start = fontSize + 10;      // Baseline position
    
    canvas->resetMatrix();
    canvas->translate(x_start, y_start);
    canvas->scale(scale, -scale);
    
    // Render glyphs
    for (int i = count - 1; i >= 0; i--) {
        auto glyph_index = glyph_info[i].codepoint;
        
        // Handle variable font coordinates for kashida
        if (glyph_info[i].lefttatweel != 0 || glyph_info[i].righttatweel != 0) {
            renderer->coords[0] = roundf(glyph_info[i].lefttatweel * 16384.f);
            renderer->coords[1] = roundf(glyph_info[i].righttatweel * 16384.f);
            renderer->font->num_coords = 2;
            renderer->font->coords = &renderer->coords[0];
        }
        
        canvas->translate(-glyph_pos[i].x_advance, 0);
        
        // Apply glyph offset for proper vowel mark positioning (hamza, harakat, etc.)
        // Must match drawLine(): use y_offset directly (canvas Y is already flipped by -scale)
        canvas->translate(glyph_pos[i].x_offset, glyph_pos[i].y_offset);
        
        // Handle tajweed colors: lookup_index >= tajweedcolorindex indicates tajweed lookup was applied
        hb_color_t glyphColor = hbTextColor;
        if (useTajweed && glyph_pos[i].lookup_index >= renderer->tajweedcolorindex) {
            glyphColor = HB_COLOR(
                (glyph_pos[i].base_codepoint >> 8) & 0xff,
                (glyph_pos[i].base_codepoint >> 16) & 0xff,
                (glyph_pos[i].base_codepoint >> 24) & 0xff,
                255
            );
        }
        
        // Update context foreground before painting so COLR use_foreground layers can access it
        context.foreground = glyphColor;
        hb_font_paint_glyph(renderer->font, glyph_index, renderer->paint_funcs, &context, 0, glyphColor);
        
        canvas->translate(-glyph_pos[i].x_offset, -glyph_pos[i].y_offset);
        
        // Reset variable font coords
        if (glyph_info[i].lefttatweel != 0 || glyph_info[i].righttatweel != 0) {
            renderer->font->num_coords = 0;
            renderer->font->coords = nullptr;
        }
    }
    
    hb_buffer_destroy(hbBuffer);
    
    return static_cast<int>(totalWidth * scale);
}

bool quran_renderer_measure_text(
    QuranRendererHandle renderer,
    const char* text,
    int textLength,
    int fontSize,
    int* outWidth,
    int* outHeight
) {
    if (!renderer || !text) {
        return false;
    }
    
    size_t len = (textLength < 0) ? strlen(text) : static_cast<size_t>(textLength);
    if (len == 0) {
        if (outWidth) *outWidth = 0;
        if (outHeight) *outHeight = fontSize;
        return true;
    }
    
    // Create and shape the text
    hb_buffer_t* hbBuffer = hb_buffer_create();
    hb_buffer_set_direction(hbBuffer, HB_DIRECTION_RTL);
    hb_buffer_set_script(hbBuffer, HB_SCRIPT_ARABIC);
    hb_buffer_set_language(hbBuffer, renderer->ar_language);
    
    hb_buffer_add_utf8(hbBuffer, text, len, 0, len);
    
    // Tajweed doesn't affect measurement, but keep consistent
    renderer->features[0].value = 1;
    hb_shape(renderer->font, hbBuffer, renderer->features, 1);
    
    // Get glyph data
    unsigned int count;
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(hbBuffer, &count);
    
    // Calculate width
    int totalWidth = 0;
    for (unsigned int i = 0; i < count; i++) {
        totalWidth += glyph_pos[i].x_advance;
    }
    
    double scale = static_cast<double>(fontSize) / renderer->upem;
    
    if (outWidth) *outWidth = static_cast<int>(totalWidth * scale);
    if (outHeight) *outHeight = fontSize;
    
    hb_buffer_destroy(hbBuffer);
    
    return true;
}

int quran_renderer_draw_multiline_text(
    QuranRendererHandle renderer,
    QuranPixelBuffer* buffer,
    const char* text,
    int textLength,
    const QuranTextConfig* config,
    float lineSpacing
) {
    if (!renderer || !buffer || !buffer->pixels || !text) {
        return -1;
    }
    
    size_t len = (textLength < 0) ? strlen(text) : static_cast<size_t>(textLength);
    if (len == 0) {
        return 0;
    }
    
    // Extract configuration with defaults for auto (0) values
    uint32_t bgColor = config ? config->backgroundColor : 0xFFFFFFFF;
    
    // Auto font size: calculate based on buffer width (matches mushaf-android)
    int fontSize = config ? config->fontSize : 0;
    if (fontSize <= 0) {
        // Auto-calculate: (width / 17) * 0.9 - same formula as page rendering
        fontSize = static_cast<int>((buffer->width / 17.0f) * 0.9f);
        if (fontSize < 12) fontSize = 12;  // Minimum readable size
    }
    
    // Auto line spacing (0 = 1.5x default)
    float spacing = (lineSpacing > 0) ? lineSpacing : 1.5f;
    
    // Set up Skia canvas and clear background
    SkImageInfo imageInfo = SkImageInfo::Make(
        buffer->width, buffer->height, 
        kRGBA_8888_SkColorType, kPremul_SkAlphaType
    );
    auto canvas = SkCanvas::MakeRasterDirect(imageInfo, buffer->pixels, buffer->stride);
    
    uint8_t bg_r = (bgColor >> 24) & 0xFF;
    uint8_t bg_g = (bgColor >> 16) & 0xFF;
    uint8_t bg_b = (bgColor >> 8) & 0xFF;
    uint8_t bg_a = bgColor & 0xFF;
    canvas->drawColor(SkColorSetARGB(bg_a, bg_r, bg_g, bg_b));
    
    // Split text by newlines
    std::string fullText(text, len);
    std::vector<std::string> lines;
    std::istringstream stream(fullText);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    // Create sub-config for each line - preserve auto values so draw_text handles them
    QuranTextConfig lineConfig = config ? *config : QuranTextConfig{};
    lineConfig.fontSize = fontSize;  // Use resolved font size
    lineConfig.backgroundColor = 0x00000000; // Transparent (background already cleared)
    // Keep textColor as-is (0 = auto will be resolved in draw_text based on original bgColor)
    if (config && config->textColor == 0) {
        // For auto text color, resolve it here based on the actual background
        lineConfig.textColor = isDarkBackground(bgColor) ? 0xFFFFFFFF : 0x000000FF;
    }
        // Line height - match mushafapproach: simple multiplier on font size
        int lineHeight = static_cast<int>(fontSize * spacing);
        
        // Calculate margins - match mushaf rendering (width / 42.5)
        float marginLeft = config ? config->marginLeft : -1.0f;
        float marginRight = config ? config->marginRight : -1.0f;
        
        // Auto margin: match mushaf rendering (width / 42.5)
        if (marginLeft < 0) {
            marginLeft = std::max(10.0f, static_cast<float>(buffer->width) / 42.5f);
        }
        if (marginRight < 0) {
            marginRight = std::max(10.0f, static_cast<float>(buffer->width) / 42.5f);
        }
        
        int yOffset = static_cast<int>(marginLeft);  // Use left margin for top
    for (size_t i = 0; i < lines.size(); i++) {
        if (lines[i].empty()) {
            yOffset += lineHeight;
            continue;
        }
        
        // Create a sub-buffer for this line (share the same pixel data)
        // Allocate enough height for marks above and below
        int neededHeight = static_cast<int>(fontSize * 2.0f);
        QuranPixelBuffer lineBuffer = *buffer;
        lineBuffer.pixels = static_cast<uint8_t*>(buffer->pixels) + (yOffset * buffer->stride);
        lineBuffer.height = std::min(neededHeight, buffer->height - yOffset);
        
        if (lineBuffer.height <= 0) break;
        
        quran_renderer_draw_text(renderer, &lineBuffer, lines[i].c_str(), -1, &lineConfig);
        
        yOffset += lineHeight;
    }
    
    return static_cast<int>(lines.size());
}

// Helper: split UTF-8 string by spaces while preserving Arabic text integrity
static std::vector<std::string> splitIntoWords(const char* text, size_t len) {
    std::vector<std::string> words;
    std::string current;
    
    for (size_t i = 0; i < len; ) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        
        // Check for space (ASCII space or Arabic space)
        if (c == ' ' || c == '\t') {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
            i++;
        } else {
            // UTF-8 character: determine byte count
            int charBytes = 1;
            if ((c & 0xF8) == 0xF0) charBytes = 4;      // 4-byte UTF-8
            else if ((c & 0xF0) == 0xE0) charBytes = 3; // 3-byte UTF-8
            else if ((c & 0xE0) == 0xC0) charBytes = 2; // 2-byte UTF-8
            
            // Append full UTF-8 character
            for (int j = 0; j < charBytes && i + j < len; j++) {
                current += text[i + j];
            }
            i += charBytes;
        }
    }
    
    if (!current.empty()) {
        words.push_back(current);
    }
    
    return words;
}

int quran_renderer_draw_wrapped_text(
    QuranRendererHandle renderer,
    QuranPixelBuffer* buffer,
    const char* text,
    int textLength,
    const QuranTextConfig* config,
    float lineSpacing
) {
    if (!renderer || !buffer || !buffer->pixels || !text) {
        return -1;
    }
    
    size_t len = (textLength < 0) ? strlen(text) : static_cast<size_t>(textLength);
    if (len == 0) {
        return 0;
    }
    
    // Extract configuration
    uint32_t bgColor = config ? config->backgroundColor : 0xFFFFFFFF;
    int fontSize = config ? config->fontSize : 0;
    if (fontSize <= 0) fontSize = 48;
    
    // Calculate margins (auto = ~5% of buffer width)
    float marginLeft = config ? config->marginLeft : -1.0f;
    float marginRight = config ? config->marginRight : -1.0f;
    
    // Auto margin: 5% of buffer width, minimum 10px
    if (marginLeft < 0) {
        marginLeft = std::max(10.0f, buffer->width * 0.05f);
    }
    if (marginRight < 0) {
        marginRight = std::max(10.0f, buffer->width * 0.05f);
    }
    
    // Calculate effective line width considering margins
    float maxLineWidth = config ? config->lineWidth : 0;
    if (maxLineWidth <= 0) {
        // Auto: use buffer width minus margins
        maxLineWidth = buffer->width - marginLeft - marginRight;
    } else {
        // User specified lineWidth - ensure it fits within buffer with margins
        float availableWidth = buffer->width - marginLeft - marginRight;
        if (maxLineWidth > availableWidth) {
            maxLineWidth = availableWidth;
        }
    }
    
    // Ensure we have positive width
    if (maxLineWidth <= 0) {
        maxLineWidth = buffer->width * 0.9f;  // 90% of buffer as fallback
    }
    
    float spacing = (lineSpacing > 0) ? lineSpacing : 1.5f;
    
    // Set up Skia canvas and clear background
    SkImageInfo imageInfo = SkImageInfo::Make(
        buffer->width, buffer->height, 
        kRGBA_8888_SkColorType, kPremul_SkAlphaType
    );
    auto canvas = SkCanvas::MakeRasterDirect(imageInfo, buffer->pixels, buffer->stride);
    
    uint8_t bg_r = (bgColor >> 24) & 0xFF;
    uint8_t bg_g = (bgColor >> 16) & 0xFF;
    uint8_t bg_b = (bgColor >> 8) & 0xFF;
    uint8_t bg_a = bgColor & 0xFF;
    canvas->drawColor(SkColorSetARGB(bg_a, bg_r, bg_g, bg_b));
    
    // Split text into words (only at whitespace boundaries - never break mid-word)
    std::vector<std::string> words = splitIntoWords(text, len);
    
    // Build lines by measuring and wrapping at word boundaries
    std::vector<std::string> lines;
    std::string currentLine;
    int currentLineWidth = 0;
    
    // Calculate space width using HarfBuzz measurement
    int spaceWidth = 0;
    {
        int withSpace = 0, withoutSpace = 0;
        quran_renderer_measure_text(renderer, "ا ب", -1, fontSize, &withSpace, nullptr);
        quran_renderer_measure_text(renderer, "اب", -1, fontSize, &withoutSpace, nullptr);
        spaceWidth = withSpace - withoutSpace;
        if (spaceWidth <= 0) {
            // Fallback: estimate space as ~25% of fontSize
            spaceWidth = fontSize / 4;
        }
    }
    
    for (const auto& word : words) {
        int wordWidth = 0;
        quran_renderer_measure_text(renderer, word.c_str(), word.length(), fontSize, &wordWidth, nullptr);
        
        if (currentLine.empty()) {
            // First word on line
            // Handle very long words that exceed maxLineWidth
            if (wordWidth > maxLineWidth) {
                // Word is too long - still add it (will be clipped, but don't break mid-word)
                // For Arabic, breaking mid-word would disconnect letters incorrectly
                currentLine = word;
                currentLineWidth = wordWidth;
            } else {
                currentLine = word;
                currentLineWidth = wordWidth;
            }
        } else {
            // Check if word fits on current line (with space between)
            int newWidth = currentLineWidth + spaceWidth + wordWidth;
            if (newWidth <= maxLineWidth) {
                // Fits - add with space
                currentLine += " " + word;
                currentLineWidth = newWidth;
            } else {
                // Doesn't fit - push current line and start new line with this word
                lines.push_back(currentLine);
                currentLine = word;
                currentLineWidth = wordWidth;
            }
        }
    }
    
    // Don't forget last line
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    // Create sub-config for rendering each line
    QuranTextConfig lineConfig = config ? *config : QuranTextConfig{};
    lineConfig.fontSize = fontSize;
    lineConfig.backgroundColor = 0x00000000; // Transparent (background already cleared)
    lineConfig.lineWidth = maxLineWidth;     // Use calculated line width
    // Note: margins will be set per-line in the loop below
    
    // Resolve auto text color
    if (config && config->textColor == 0) {
        lineConfig.textColor = isDarkBackground(bgColor) ? 0xFFFFFFFF : 0x000000FF;
    }
    
    // Line height must accommodate Arabic marks above and below
    // Reduced from 1.5x to 1.2x for tighter spacing
    int baseLineHeight = static_cast<int>(fontSize * 1.2f);
    int lineHeight = static_cast<int>(baseLineHeight * spacing);
    
    // Start Y position with top margin
    int yOffset = static_cast<int>(marginLeft);
    
    for (size_t i = 0; i < lines.size(); i++) {
        // Ensure sub-buffer has enough height for the text plus marks
        // Arabic text needs extra height for marks above (fatha, damma, shadda, etc.)
        int neededHeight = static_cast<int>(fontSize * 2.0f);
        if (buffer->height - yOffset < neededHeight) break;
        
        // Create a sub-buffer positioned with margins
        // For RTL text, we need to offset the buffer so text starts at the right margin
        QuranPixelBuffer lineBuffer;
        lineBuffer.width = buffer->width;
        lineBuffer.height = std::min(neededHeight, buffer->height - yOffset);
        lineBuffer.stride = buffer->stride;
        lineBuffer.pixels = static_cast<uint8_t*>(buffer->pixels) + (yOffset * buffer->stride);
        
        // Create a config with explicit margins for this line
        // The right margin positions where RTL text starts
        // The left margin limits how far left the text can extend
        QuranTextConfig marginConfig = lineConfig;
        marginConfig.lineWidth = maxLineWidth;
        marginConfig.marginRight = marginRight;  // Use calculated right margin
        marginConfig.marginLeft = marginLeft;    // Use calculated left margin
        
        quran_renderer_draw_text(renderer, &lineBuffer, lines[i].c_str(), -1, &marginConfig);
        
        yOffset += lineHeight;
    }
    
    return static_cast<int>(lines.size());
}

} // extern "C"
