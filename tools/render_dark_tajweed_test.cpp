#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "quran/renderer.h"

static bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    in.seekg(0, std::ios::end);
    std::streamsize size = in.tellg();
    if (size <= 0) return false;
    in.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    if (!in.read(reinterpret_cast<char*>(out.data()), size)) return false;
    return true;
}

static bool writePPM_RGBA8888(const std::string& path, const uint8_t* rgba, int width, int height, int strideBytes) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    out << "P6\n" << width << " " << height << "\n255\n";
    for (int y = 0; y < height; ++y) {
        const uint8_t* row = rgba + y * strideBytes;
        for (int x = 0; x < width; ++x) {
            const uint8_t* px = row + x * 4;
            // RGBA8888
            out.put(static_cast<char>(px[0]));
            out.put(static_cast<char>(px[1]));
            out.put(static_cast<char>(px[2]));
        }
    }
    return true;
}

static inline bool nearColor(uint8_t r, uint8_t g, uint8_t b, uint8_t rr, uint8_t gg, uint8_t bb, int tol) {
    return std::abs(int(r) - int(rr)) <= tol && std::abs(int(g) - int(gg)) <= tol && std::abs(int(b) - int(bb)) <= tol;
}

struct ColorStats {
    size_t pureBlack = 0;
    size_t tajweedLike = 0;
};

static ColorStats analyzePixels(const uint8_t* rgba, int width, int height, int strideBytes,
                                uint8_t bgR, uint8_t bgG, uint8_t bgB,
                                uint8_t fgR, uint8_t fgG, uint8_t fgB) {
    ColorStats stats;
    for (int y = 0; y < height; ++y) {
        const uint8_t* row = rgba + y * strideBytes;
        for (int x = 0; x < width; ++x) {
            const uint8_t* px = row + x * 4;
            const uint8_t r = px[0];
            const uint8_t g = px[1];
            const uint8_t b = px[2];

            if (r == 0 && g == 0 && b == 0) {
                stats.pureBlack++;
            }

            // Heuristic for "tajweed-like" colored pixels:
            // not close to background, not close to foreground, and not a gray-ish shade.
            const bool isBg = nearColor(r, g, b, bgR, bgG, bgB, 6);
            const bool isFg = nearColor(r, g, b, fgR, fgG, fgB, 18);
            const int maxc = std::max({int(r), int(g), int(b)});
            const int minc = std::min({int(r), int(g), int(b)});
            const bool isGrayish = (maxc - minc) <= 18;
            if (!isBg && !isFg && !isGrayish) {
                stats.tajweedLike++;
            }
        }
    }
    return stats;
}

int main(int argc, char** argv) {
    // Defaults
    std::string fontPath = "android/src/main/assets/fonts/digitalkhatt.otf";
    std::string outPath = "build/dark-tajweed.ppm";
    int pageIndex = 2;
    int width = 1200;
    int height = 1800;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--font") == 0 && i + 1 < argc) {
            fontPath = argv[++i];
        } else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outPath = argv[++i];
        } else if (std::strcmp(argv[i], "--page") == 0 && i + 1 < argc) {
            pageIndex = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            width = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            height = std::atoi(argv[++i]);
        } else {
            std::cerr << "Usage: " << argv[0]
                      << " [--font <path>] [--out <path>] [--page <index>]"
                      << " [--width <px>] [--height <px>]\n";
            return 2;
        }
    }

    std::vector<uint8_t> fontBytes;
    if (!readFile(fontPath, fontBytes)) {
        std::cerr << "Failed to read font: " << fontPath << "\n";
        return 2;
    }

    QuranFontData fontData;
    fontData.data = fontBytes.data();
    fontData.size = static_cast<size_t>(fontBytes.size());

    QuranRendererHandle renderer = quran_renderer_create(&fontData);
    if (!renderer) {
        std::cerr << "Failed to create renderer\n";
        return 2;
    }

    const int stride = width * 4;
    std::vector<uint8_t> pixels(static_cast<size_t>(stride * height));

    QuranPixelBuffer buffer;
    buffer.width = width;
    buffer.height = height;
    buffer.stride = stride;
    buffer.pixels = pixels.data();

    auto renderAndCheck = [&](uint32_t bg, const std::string& ppmPath, uint8_t fgR, uint8_t fgG, uint8_t fgB) -> int {
        std::fill(pixels.begin(), pixels.end(), 0);
        QuranRenderConfig cfg;
        cfg.tajweed = true;
        cfg.justify = true;
        cfg.fontScale = 1.0f;
        cfg.backgroundColor = bg;
        cfg.fontSize = 0;
        cfg.useForeground = false;
        cfg.lineHeightDivisor = 0.0f;  // Auto: 10.0 for regular pages, 7.5 for Fatiha
        cfg.topMarginLines = -1.0f;    // Auto: no extra top margin

        quran_renderer_draw_page(renderer, &buffer, pageIndex, &cfg);

        if (!writePPM_RGBA8888(ppmPath, pixels.data(), width, height, stride)) {
            std::cerr << "Failed to write PPM: " << ppmPath << "\n";
            return 2;
        }

        const uint8_t bgR = (bg >> 24) & 0xFF;
        const uint8_t bgG = (bg >> 16) & 0xFF;
        const uint8_t bgB = (bg >> 8) & 0xFF;
        ColorStats stats = analyzePixels(pixels.data(), width, height, stride, bgR, bgG, bgB, fgR, fgG, fgB);

        const double blackPct = 100.0 * double(stats.pureBlack) / double(width * height);
        const double tajPct = 100.0 * double(stats.tajweedLike) / double(width * height);
        std::cout << "Wrote " << ppmPath << "\n";
        std::cout << "Pure-black pixels: " << stats.pureBlack << " (" << blackPct << "%)\n";
        std::cout << "Tajweed-like colored pixels: " << stats.tajweedLike << " (" << tajPct << "%)\n";

        // On dark backgrounds, some black is expected from ayah number digits 
        // which stay black per mushaf-android design. Allow up to 1% for these.
        if (bgR < 64 && bgG < 64 && bgB < 64 && blackPct > 1.0) {
            std::cerr << "Too many pure-black pixels on dark background; likely lingering black text.\n";
            return 1;
        }

        // Tajweed should create at least some non-foreground colors.
        if (tajPct < 0.002) {
            std::cerr << "Tajweed colors look missing (too few non-foreground colored pixels).\n";
            return 1;
        }

        return 0;
    };

    // Dark background: expect white foreground.
    int rcDark = renderAndCheck(0x1E1E1EFF, outPath, 255, 255, 255);
    // Light background: expect black foreground.
    int rcLight = renderAndCheck(0xFFFFFFFF, "build/light-tajweed.ppm", 0, 0, 0);

    quran_renderer_destroy(renderer);

    return (rcDark != 0 || rcLight != 0) ? 1 : 0;
}
