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
    float fontScale;    // Font scale factor (1.0 = default, 0.8-1.2 recommended)
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

#ifdef __cplusplus
}
#endif

#endif // QURAN_RENDERER_H
