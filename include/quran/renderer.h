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

#ifdef __cplusplus
}
#endif

#endif // QURAN_RENDERER_H
