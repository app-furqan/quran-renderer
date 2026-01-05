/**
 * QuranRenderer - Cross-platform Quran text rendering library
 * 
 * Uses custom HarfBuzz fork with justification support and Skia for rendering.
 * Supports tajweed coloring and Arabic kashida justification.
 */

#ifndef QURAN_RENDERER_H
#define QURAN_RENDERER_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Pixel format enumeration
 */
typedef enum {
    QURAN_PIXEL_FORMAT_RGBA8888 = 0,
    QURAN_PIXEL_FORMAT_BGRA8888 = 1,
} QuranPixelFormat;

/**
 * Platform-agnostic pixel buffer for rendering
 */
typedef struct {
    void* pixels;           // Pointer to pixel data
    int width;              // Width in pixels
    int height;             // Height in pixels
    int stride;             // Bytes per row
    QuranPixelFormat format; // Pixel format
} QuranPixelBuffer;

/**
 * Font data container
 */
typedef struct {
    const uint8_t* data;    // Font file data
    size_t size;            // Size in bytes
} QuranFontData;

/**
 * Renderer configuration
 */
typedef struct {
    bool tajweed;       // Enable tajweed coloring
    bool justify;       // Enable line justification
    float fontScale;    // Font scale factor (1.0 = default, 0.8-1.2 recommended) - DEPRECATED, use fontSize
    uint32_t backgroundColor; // Background color in RGBA format (0xRRGGBBAA, default: 0xFFFFFFFF for white)
    int fontSize;       // Font size in pixels (0 = auto, calculated as (width/17)*0.9 to match mushaf-android)
    bool useForeground; // If true, use foreground color when COLR requests use_foreground
    float lineHeightDivisor;  // Line height = height / lineHeightDivisor (0 = auto: 10.0 for regular pages, 7.5 for Fatiha)
    float topMarginLines;     // Top margin in line-heights for Fatiha pages 1&2 (0 = auto: 3.5 for Fatiha, 0 for others)
} QuranRenderConfig;

/**
 * Opaque renderer handle
 */
typedef struct QuranRendererImpl* QuranRendererHandle;

/**
 * Initialize the renderer with font data
 * 
 * @param fontData Font file data (must remain valid for renderer lifetime)
 * @return Renderer handle, or NULL on failure
 */
QuranRendererHandle quran_renderer_create(const QuranFontData* fontData);

/**
 * Destroy the renderer and free resources
 */
void quran_renderer_destroy(QuranRendererHandle renderer);

/**
 * Render a page to a pixel buffer
 * 
 * @param renderer Renderer handle
 * @param buffer Pixel buffer to render into
 * @param pageIndex Page index (0-603)
 * @param config Render configuration
 */
void quran_renderer_draw_page(
    QuranRendererHandle renderer,
    QuranPixelBuffer* buffer,
    int pageIndex,
    const QuranRenderConfig* config
);

/**
 * Get the total number of pages
 */
int quran_renderer_get_page_count(QuranRendererHandle renderer);

/* ============================================================================
 * Surah/Ayah API
 * ============================================================================ */

/**
 * Surah information structure
 */
typedef struct {
    int number;             // Surah number (1-114)
    int ayahCount;          // Number of ayahs in this surah
    int startAyah;          // Starting ayah index (0-based cumulative)
    const char* nameArabic; // Arabic name (UTF-8)
    const char* nameTrans;  // Transliterated name
    const char* nameEnglish;// English name
    const char* type;       // "Meccan" or "Medinan"
    int revelationOrder;    // Order of revelation (1-114)
    int rukuCount;          // Number of rukus
} QuranSurahInfo;

/**
 * Ayah location structure
 */
typedef struct {
    int surahNumber;        // Surah number (1-114)
    int ayahNumber;         // Ayah number within surah (1-based)
    int pageIndex;          // Page index (0-603)
} QuranAyahLocation;

/**
 * Get the total number of surahs (always 114)
 */
int quran_renderer_get_surah_count(void);

/**
 * Get the total number of ayahs (always 6236)
 */
int quran_renderer_get_total_ayah_count(void);

/**
 * Get information about a surah
 * 
 * @param surahNumber Surah number (1-114)
 * @param info Output structure to fill
 * @return true on success, false if surahNumber is invalid
 */
bool quran_renderer_get_surah_info(int surahNumber, QuranSurahInfo* info);

/**
 * Get the page index where a surah starts
 * 
 * @param surahNumber Surah number (1-114)
 * @return Page index (0-603), or -1 if invalid
 */
int quran_renderer_get_surah_start_page(int surahNumber);

/**
 * Get the page index for a specific ayah
 * 
 * @param surahNumber Surah number (1-114)
 * @param ayahNumber Ayah number within surah (1-based)
 * @return Page index (0-603), or -1 if invalid
 */
int quran_renderer_get_ayah_page(int surahNumber, int ayahNumber);

/**
 * Get surah and ayah number from a page index
 * 
 * @param pageIndex Page index (0-603)
 * @param location Output structure with surah/ayah at start of page
 * @return true on success, false if pageIndex is invalid
 */
bool quran_renderer_get_page_location(int pageIndex, QuranAyahLocation* location);

/**
 * Get the number of ayahs in a surah
 * 
 * @param surahNumber Surah number (1-114)
 * @return Number of ayahs, or -1 if invalid
 */
int quran_renderer_get_ayah_count(int surahNumber);

/* ============================================================================
 * Generic Arabic Text Rendering API
 * ============================================================================ */

/**
 * Special values for auto-detection in QuranTextConfig
 */
#define QURAN_TEXT_COLOR_AUTO       0           // Auto-detect text color based on background luminance
#define QURAN_FONT_SIZE_AUTO        0           // Auto-calculate font size to fit line width
#define QURAN_LINE_WIDTH_AUTO       0.0f        // Use buffer width as line width
#define QURAN_LINE_SPACING_AUTO     0.0f        // Use default 1.5x line spacing

/**
 * Configuration for rendering arbitrary Arabic text
 * 
 * All fields support "auto" values:
 * - fontSize: 0 = auto-calculate based on lineWidth (default: 48 if both are auto)
 * - textColor: 0 = auto-detect based on background luminance (white on dark, black on light)
 * - backgroundColor: Set explicitly (no auto, default: white 0xFFFFFFFF)
 * - justify: Explicit true/false (no auto)
 * - lineWidth: 0 = use buffer width minus padding
 * - rightToLeft: Explicit true/false (default: true for Arabic)
 */
typedef struct {
    int fontSize;             // Font size in pixels (0 = QURAN_FONT_SIZE_AUTO)
    uint32_t textColor;       // Text color 0xRRGGBBAA (0 = QURAN_TEXT_COLOR_AUTO)
    uint32_t backgroundColor; // Background color 0xRRGGBBAA (default: 0xFFFFFFFF white)
    bool justify;             // Enable kashida justification to fill lineWidth
    float lineWidth;          // Target line width in pixels (0 = QURAN_LINE_WIDTH_AUTO)
    bool rightToLeft;         // Text direction (true for Arabic, default: true)
    bool tajweed;             // Enable tajweed coloring (default: true)
} QuranTextConfig;

/**
 * Create a default QuranTextConfig with sensible auto values
 * 
 * Returns a config with:
 * - fontSize: 0 (auto)
 * - textColor: 0 (auto - white on dark, black on light)
 * - backgroundColor: 0xFFFFFFFF (white)
 * - justify: false
 * - lineWidth: 0 (auto - use buffer width)
 * - rightToLeft: true
 * - tajweed: true (enabled by default)
 */
static inline QuranTextConfig quran_text_config_default(void) {
    QuranTextConfig config = {0};
    config.fontSize = QURAN_FONT_SIZE_AUTO;
    config.textColor = QURAN_TEXT_COLOR_AUTO;
    config.backgroundColor = 0xFFFFFFFF;  // White background
    config.justify = false;
    config.lineWidth = QURAN_LINE_WIDTH_AUTO;
    config.rightToLeft = true;
    config.tajweed = true;  // Tajweed coloring enabled by default
    return config;
}

/**
 * Render arbitrary Arabic text to a pixel buffer
 * 
 * This function uses the same HarfBuzz + Skia rendering pipeline as the
 * Quran page renderer, but accepts any UTF-8 Arabic text string.
 * 
 * @param renderer Renderer handle (must be initialized with font data)
 * @param buffer Pixel buffer to render into
 * @param text UTF-8 encoded Arabic text to render
 * @param textLength Length of text in bytes (or -1 for null-terminated)
 * @param config Text rendering configuration
 * @return Width of rendered text in pixels, or -1 on error
 */
int quran_renderer_draw_text(
    QuranRendererHandle renderer,
    QuranPixelBuffer* buffer,
    const char* text,
    int textLength,
    const QuranTextConfig* config
);

/**
 * Measure Arabic text without rendering
 * 
 * @param renderer Renderer handle
 * @param text UTF-8 encoded Arabic text
 * @param textLength Length of text in bytes (or -1 for null-terminated)
 * @param fontSize Font size in pixels
 * @param outWidth Output: text width in pixels
 * @param outHeight Output: text height in pixels
 * @return true on success
 */
bool quran_renderer_measure_text(
    QuranRendererHandle renderer,
    const char* text,
    int textLength,
    int fontSize,
    int* outWidth,
    int* outHeight
);

/**
 * Render multi-line Arabic text with automatic line breaking
 * 
 * @param renderer Renderer handle
 * @param buffer Pixel buffer to render into
 * @param text UTF-8 encoded Arabic text (can contain newlines)
 * @param textLength Length of text in bytes (or -1 for null-terminated)
 * @param config Text rendering configuration
 * @param lineSpacing Line spacing multiplier (1.0 = single space, 1.5 = 1.5x, etc.)
 * @return Number of lines rendered, or -1 on error
 */
int quran_renderer_draw_multiline_text(
    QuranRendererHandle renderer,
    QuranPixelBuffer* buffer,
    const char* text,
    int textLength,
    const QuranTextConfig* config,
    float lineSpacing
);

/**
 * Render Arabic text with automatic word-wrapping
 * 
 * Automatically breaks text at word boundaries to fit within lineWidth.
 * Words are determined by spaces. Long text flows onto multiple visual lines.
 * 
 * @param renderer Renderer handle
 * @param buffer Pixel buffer to render into
 * @param text UTF-8 encoded Arabic text
 * @param textLength Length of text in bytes (or -1 for null-terminated)
 * @param config Text rendering configuration (lineWidth controls wrap width)
 * @param lineSpacing Line spacing multiplier (0 = auto 1.5x, 1.0 = single, etc.)
 * @return Number of lines rendered, or -1 on error
 */
int quran_renderer_draw_wrapped_text(
    QuranRendererHandle renderer,
    QuranPixelBuffer* buffer,
    const char* text,
    int textLength,
    const QuranTextConfig* config,
    float lineSpacing
);

#ifdef __cplusplus
}
#endif

#endif // QURAN_RENDERER_H
