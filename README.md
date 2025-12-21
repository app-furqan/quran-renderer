# Quran Renderer

A high-quality, cross-platform Quran text rendering library using the **[DigitalKhatt](https://digitalkhatt.org)** Quran font. Features professional Arabic typography with tajweed coloring and automatic line justification using kashida stretching.

**Supported Platforms:**
- **Android** - AAR library with Kotlin API
- **Linux** - Shared library (.so)
- **iOS/macOS** - XCFramework with C API

This library renders Quran pages using:
- **DigitalKhatt Fonts** - Variable OpenType fonts specifically designed for Quran typography with COLR-based tajweed highlighting
- **HarfBuzz (DigitalKhatt fork)** - Custom text shaping engine with Arabic text justification support
- **VisualMetaFont (DigitalKhatt)** - Quran text data ([quran.cpp](https://github.com/DigitalKhatt/visualmetafont/blob/main/src/qurantext/quran.cpp)) containing all 604 pages
- **Skia Graphics Engine** - High-quality 2D rendering

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

## Features

- **Cross-Platform**: Android, Linux, iOS, and macOS support
- **Professional Arabic Typography**: Custom HarfBuzz fork with kashida/tatweel-based line justification
- **Tajweed Coloring**: Per-glyph color output from OpenType COLR table (Madina Quranic font)
- **Variable Font Support**: Per-glyph axis control for dynamic kashida stretching
- **Font Size Scaling**: Adjustable text size (0.5x to 2.0x)
- **Four Font Styles Included**: Madina Quranic (with tajweed), New Madina, Old Madina, and IndoPak
- **High-Quality Rendering**: Skia-based rendering with anti-aliasing and subpixel positioning

## Architecture

```
quran-renderer/
├── include/quran/              # Public C API headers
│   └── renderer.h
├── src/
│   └── core/                   # Platform-agnostic C++ core
│       ├── quran_renderer.cpp  # Main rendering logic
│       ├── hb_skia_canvas.cpp  # HarfBuzz-Skia bridge
│       ├── hb_skia_canvas.h
│       └── quran.h
├── android/                    # Android library module
│   ├── build.gradle
│   └── src/main/
│       ├── cpp/                # JNI wrapper
│       │   ├── CMakeLists.txt
│       │   └── quran_renderer_jni.cpp
│       ├── java/org/digitalkhatt/quran/renderer/
│       │   └── QuranRenderer.kt   # Kotlin API
│       └── assets/fonts/       # Font files
├── scripts/
│   ├── build-dependencies.sh   # Android dependencies builder
│   ├── build-linux.sh          # Linux build script
│   └── build-apple.sh          # iOS/macOS build script
├── CMakeLists.txt              # Cross-platform CMake config
└── local.properties.template
```

## Requirements

### Build Requirements

- Android Studio Arctic Fox or later
- Android NDK 28.2.13676358
- CMake 3.22.1+
- JDK 17+

### Dependencies (Source)

1. **HarfBuzz (DigitalKhatt fork)**: https://github.com/DigitalKhatt/harfbuzz
   - Branch: `justification`
   - Features: `hb_buffer_set_justify()`, tatweel extension fields for Arabic text justification
   - This fork adds kashida-based line justification support not available in upstream HarfBuzz

2. **VisualMetaFont (DigitalKhatt)**: https://github.com/DigitalKhatt/visualmetafont
   - **Quran Text Source**: [`src/qurantext/quran.cpp`](https://github.com/DigitalKhatt/visualmetafont/blob/main/src/qurantext/quran.cpp) - Contains all 604 pages of Quran text
   - ⚠️ **Hard Dependency**: This file is compiled into the library and cannot be changed at runtime
   - Pre-formatted with surah names, ayah markers (۝١, ۝٢, etc.), and line breaks
   - Also includes `qurancomplex.cpp` for King Fahd Complex text variant

3. **Skia (static build)**: Built from https://skia.googlesource.com/skia
   - Pre-built static libraries for Android ABIs
   - Used for high-quality 2D rendering with anti-aliasing

4. **DigitalKhatt Quran Fonts**: Variable OpenType fonts with kashida stretching
   
   All four font styles are **bundled with the AAR** in `assets/fonts/`:
   
   | Font | File | Style | Tajweed Colors |
   |------|------|-------|----------------|
   | **Madina Quranic** | `digitalkhatt.otf` | `QuranFont.MADINA_QURANIC` | ✅ Yes (COLR/CPAL) |
   | **New Madina** | `madina.otf` | `QuranFont.MADINA` | ❌ No |
   | **Old Madina** | `oldmadina.otf` | `QuranFont.OLD_MADINA` | ❌ No |
   | **IndoPak** | `indopak.otf` | `QuranFont.INDOPAK` | ❌ No |
   
   > **Note:** Only `MADINA_QURANIC` includes tajweed coloring. Other fonts render in black.
   
   **Source Repositories:**
   - Madina Quranic: [DigitalKhatt/mushaf-android](https://github.com/DigitalKhatt/mushaf-android)
   - New Madina: [DigitalKhatt/madinafont](https://github.com/DigitalKhatt/madinafont)
   - Old Madina: [DigitalKhatt/oldmadinafont](https://github.com/DigitalKhatt/oldmadinafont)
   - IndoPak: [DigitalKhatt/indopakfont](https://github.com/DigitalKhatt/indopakfont)
   
   Visit https://digitalkhatt.org for live demos and more information.

---

## Installation

### Option 1: Pre-built AAR (Recommended)

Download the latest release AAR from the [Releases](../../releases) page.

#### Gradle (Kotlin DSL)

```kotlin
// app/build.gradle.kts
dependencies {
    implementation(files("libs/android-release.aar"))
}
```

#### Gradle (Groovy)

```groovy
// app/build.gradle
dependencies {
    implementation files('libs/android-release.aar')
}
```

### Option 2: Build from Source (Automated)

The easiest way to build from source is using the automated build script:

```bash
# Clone the repository
git clone https://github.com/hussainak/quran-renderer.git
cd quran-renderer

# Run the automated dependency builder
# This clones HarfBuzz, VisualMetaFont, builds Skia for all ABIs,
# and generates local.properties automatically
./scripts/build-dependencies.sh

# Build the AAR
./gradlew :android:assembleRelease
```

**Script Options:**
```bash
./scripts/build-dependencies.sh --help

Options:
  --ndk-path PATH    Path to Android NDK (default: auto-detect)
  --deps-dir PATH    Directory for dependencies (default: ../quran-deps)
  --skip-skia        Skip Skia build (if already built)
  --clean            Clean and rebuild everything
```

> **Note:** Building Skia takes 10-30 minutes depending on your machine. The script builds for arm64-v8a, armeabi-v7a, and x86_64.

### Option 3: Build from Source (Manual)

1. Clone the repository:
   ```bash
   git clone https://github.com/hussainak/quran-renderer.git
   cd quran-renderer
   ```

2. Configure dependencies in `local.properties`:
   ```properties
   # Path to HarfBuzz source (justification branch)
   harfbuzz.dir=/path/to/harfbuzz

   # Path to Skia build output (with libskia.a in <ABI>/ folders)
   skia.dir=/path/to/skia-build/out

   # Path to VisualMetaFont source
   vmf.dir=/path/to/visualmetafont

   # Android SDK
   sdk.dir=/path/to/Android/Sdk
   ```

3. Build:
   ```bash
   # Debug build
   ./gradlew :android:assembleDebug

   # Release build (for production)
   ./gradlew :android:assembleRelease
   ```

4. Output AAR files:
   - `android/build/outputs/aar/android-debug.aar`
   - `android/build/outputs/aar/android-release.aar`

---

## Linux Build

Build a shared library (`.so`) for Linux:

```bash
# Clone the repository
git clone https://github.com/hussainak/quran-renderer.git
cd quran-renderer

# Run the Linux build script
./scripts/build-linux.sh
```

**Output Files:**
```
build/linux/
├── lib/
│   ├── libquranrenderer.so          # Symlink → libquranrenderer.so.1
│   ├── libquranrenderer.so.1        # Symlink → libquranrenderer.so.1.0.0
│   └── libquranrenderer.so.1.0.0    # Shared library (~7 MB)
├── include/
│   └── quran/
│       └── renderer.h               # C API header
└── fonts/
    └── digitalkhatt.otf             # Quran font with tajweed colors
```

**Build Options:**
```bash
./scripts/build-linux.sh --help

Options:
  --deps-dir PATH    Directory for dependencies (default: ../quran-deps)
  --output-dir PATH  Output directory (default: build/linux)
  --skip-deps        Skip building dependencies
  --clean            Clean build directory
```

**Caching:**
The build script automatically caches dependencies in `../quran-deps/`. Subsequent builds check for upstream updates and only rebuild if there are new commits:
- First build: ~10-20 minutes (downloads and builds Skia, HarfBuzz, etc.)
- Subsequent builds: ~3 seconds (uses cached artifacts)

**Requirements:**
- CMake 3.18+
- Ninja
- Python 3
- GCC/Clang with C++17 support

---

## iOS/macOS Build

Build an XCFramework for iOS and macOS (requires macOS with Xcode):

```bash
# Clone the repository
git clone https://github.com/hussainak/quran-renderer.git
cd quran-renderer

# Run the Apple build script
./scripts/build-apple.sh
```

**Output:**
- `build/apple/QuranRenderer.xcframework` - Universal framework
  - iOS device (arm64)
  - iOS Simulator (arm64, x86_64)
  - macOS (arm64, x86_64)

**Build Options:**
```bash
./scripts/build-apple.sh --help

Options:
  --deps-dir PATH    Directory for dependencies (default: ../quran-deps)
  --output-dir PATH  Output directory (default: build/apple)
  --ios-only         Build only iOS
  --macos-only       Build only macOS
  --skip-deps        Skip building dependencies
  --clean            Clean build directory
```

**Requirements:**
- macOS with Xcode
- Xcode Command Line Tools
- CMake, Ninja, Python 3

**Xcode Integration:**
1. Drag `QuranRenderer.xcframework` into your Xcode project
2. Add to "Frameworks, Libraries, and Embedded Content"
3. `#include <quran/renderer.h>`

---

## Usage

### Android (Kotlin)

#### Basic Usage

```kotlin
import org.digitalkhatt.quran.renderer.QuranRenderer
import org.digitalkhatt.quran.renderer.QuranFont
import android.graphics.Bitmap

class QuranActivity : AppCompatActivity() {
    private lateinit var renderer: QuranRenderer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Get singleton instance
        renderer = QuranRenderer.getInstance()
        
        // Initialize with Madina Quranic font (has tajweed coloring)
        val success = renderer.initialize(assets, QuranFont.MADINA_QURANIC)
        
        // Or choose a different font style (no tajweed colors):
        // renderer.initialize(assets, QuranFont.MADINA)      // New Madina style
        // renderer.initialize(assets, QuranFont.OLD_MADINA)  // Older Madina Mushaf
        // renderer.initialize(assets, QuranFont.INDOPAK)     // 13-line IndoPak style
        
        if (success) {
            // Render page 1 (Al-Fatiha)
            val bitmap = renderer.renderPage(
                width = 1080,
                height = 1920,
                pageIndex = 0,
                tajweed = true,    // Enable tajweed coloring
                justify = true     // Enable line justification
            )
            
            imageView.setImageBitmap(bitmap)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        renderer.destroy()
    }
}
```

#### Rendering into Existing Bitmap

```kotlin
// Create a reusable bitmap
val bitmap = Bitmap.createBitmap(1080, 1920, Bitmap.Config.ARGB_8888)

// Render different pages into the same bitmap
for (page in 0 until renderer.pageCount) {
    renderer.drawPage(bitmap, page, tajweed = true, justify = true)
    // Use bitmap...
}
```

#### Switching Fonts at Runtime

```kotlin
import org.digitalkhatt.quran.renderer.QuranFont

// Switch to a different font style
renderer.setFont(assets, QuranFont.INDOPAK)

// Check current font
val currentFont = renderer.getCurrentFont()
Log.d("Quran", "Current font: ${currentFont?.displayName}")
Log.d("Quran", "Has tajweed: ${currentFont?.hasTajweed}")

// Available font styles:
// - QuranFont.MADINA_QURANIC -> "fonts/digitalkhatt.otf" (with tajweed colors)
// - QuranFont.MADINA         -> "fonts/madina.otf" (no tajweed)
// - QuranFont.OLD_MADINA     -> "fonts/oldmadina.otf" (no tajweed)
// - QuranFont.INDOPAK        -> "fonts/indopak.otf" (no tajweed)
```

#### Font Selection UI Example

```kotlin
// Create a font selection dialog
val fontOptions = QuranFont.values()
val fontNames = fontOptions.map { "${it.displayName}${if (it.hasTajweed) " ✓" else ""}" }.toTypedArray()

AlertDialog.Builder(this)
    .setTitle("Select Font Style")
    .setItems(fontNames) { _, which ->
        val selectedFont = fontOptions[which]
        if (renderer.setFont(assets, selectedFont)) {
            // Re-render current page with new font
            invalidate()
        }
    }
    .show()
```

#### Adjusting Font Size

```kotlin
// Render with larger text (1.2x scale)
renderer.drawPage(bitmap, pageIndex, tajweed = true, justify = true, fontScale = 1.2f)

// Render with smaller text (0.8x scale) 
renderer.drawPage(bitmap, pageIndex, tajweed = true, justify = true, fontScale = 0.8f)

// Font scale range: 0.5 to 2.0 (clamped internally)
// Default: 1.0

// Example: Font size slider in settings
val fontScale = seekBar.progress / 100f + 0.5f  // Map 0-100 to 0.5-1.5
renderer.renderPage(width, height, pageIndex, fontScale = fontScale)
```

#### Surah/Ayah API (Kotlin)

Access surah and ayah metadata without rendering:

```kotlin
import org.digitalkhatt.quran.renderer.QuranRenderer
import org.digitalkhatt.quran.renderer.SurahInfo
import org.digitalkhatt.quran.renderer.AyahLocation

val renderer = QuranRenderer.getInstance()

// Basic counts (no initialization required)
val surahCount = renderer.surahCount       // 114
val totalAyahs = renderer.totalAyahCount   // 6236

// Get surah information
val surah: SurahInfo? = renderer.getSurahInfo(1)  // Al-Fatiha
surah?.let {
    Log.d("Quran", "Surah ${it.number}: ${it.nameTrans} (${it.nameEnglish})")
    Log.d("Quran", "Arabic: ${it.nameArabic}")
    Log.d("Quran", "Ayahs: ${it.ayahCount}, Type: ${it.type}")
    Log.d("Quran", "Revelation Order: ${it.revelationOrder}")
}

// Get ayah count for a surah
val ayahs = renderer.getAyahCount(2)  // Al-Baqara = 286

// Find which page a surah starts on
val yaseenPage = renderer.getSurahStartPage(36)  // Yaseen

// Find which page contains a specific ayah
val ayatKursiPage = renderer.getAyahPage(2, 255)  // Ayat Al-Kursi

// Get the surah/ayah that starts a page
val location: AyahLocation? = renderer.getPageLocation(0)
location?.let {
    Log.d("Quran", "Page 0 starts with Surah ${it.surahNumber}, Ayah ${it.ayahNumber}")
}

// Build a surah list
val surahList = (1..114).mapNotNull { renderer.getSurahInfo(it) }
```

---

### C API (Linux/iOS/macOS)

The C API can be used from C, C++, Swift, or any language with C FFI support.

#### Basic Usage (C/C++)

```c
#include <quran/renderer.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Load font file
    FILE* f = fopen("fonts/digitalkhatt.otf", "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* fontData = malloc(size);
    fread(fontData, 1, size, f);
    fclose(f);
    
    // Create renderer
    QuranFontData font = { .data = fontData, .size = size };
    QuranRendererHandle renderer = quran_renderer_create(&font);
    
    if (!renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        return 1;
    }
    
    // Create pixel buffer (RGBA)
    int width = 1080, height = 1920;
    uint8_t* pixels = malloc(width * height * 4);
    
    QuranPixelBuffer buffer = {
        .pixels = pixels,
        .width = width,
        .height = height,
        .stride = width * 4,
        .format = QURAN_PIXEL_FORMAT_RGBA8888
    };
    
    // Render page
    QuranRenderConfig config = {
        .tajweed = true,
        .justify = true,
        .fontScale = 1.0f
    };
    
    quran_renderer_draw_page(renderer, &buffer, 0, &config);  // Page 0 = Al-Fatiha
    
    // pixels now contains the rendered page
    // Save to PNG, display, etc.
    
    // Cleanup
    quran_renderer_destroy(renderer);
    free(pixels);
    free(fontData);
    
    return 0;
}
```

#### Rendering Surah by Surah (C API)

```c
#include <quran/renderer.h>
#include <stdio.h>
#include <stdlib.h>

// Render all pages of a specific surah
void render_surah(QuranRendererHandle renderer, QuranPixelBuffer* buffer, int surahNumber) {
    // Get surah information
    QuranSurahInfo info;
    if (!quran_renderer_get_surah_info(surahNumber, &info)) {
        fprintf(stderr, "Invalid surah number: %d\n", surahNumber);
        return;
    }
    
    printf("Rendering Surah %d: %s (%s)\n", info.number, info.nameEnglish, info.nameArabic);
    printf("  Type: %s, Ayahs: %d, Revelation Order: %d\n", 
           info.type, info.ayahCount, info.revelationOrder);
    
    // Find start and end pages for this surah
    int startPage = quran_renderer_get_surah_start_page(surahNumber);
    int endPage;
    
    if (surahNumber < 114) {
        // Not the last surah - end page is just before next surah starts
        endPage = quran_renderer_get_surah_start_page(surahNumber + 1) - 1;
    } else {
        // Last surah (An-Nas) - goes to page 603 (last page)
        endPage = 603;
    }
    
    printf("  Pages: %d to %d (%d pages total)\n", 
           startPage + 1, endPage + 1, endPage - startPage + 1);
    
    // Render each page
    QuranRenderConfig config = { .tajweed = true, .justify = true, .fontScale = 1.0f };
    
    for (int page = startPage; page <= endPage; page++) {
        quran_renderer_draw_page(renderer, buffer, page, &config);
        
        // Save or process the rendered page
        printf("  Rendered page %d\n", page + 1);
        
        // Example: Save to file
        char filename[256];
        snprintf(filename, sizeof(filename), "surah_%d_page_%d.rgba", surahNumber, page + 1);
        FILE* out = fopen(filename, "wb");
        fwrite(buffer->pixels, 1, buffer->height * buffer->stride, out);
        fclose(out);
    }
    
    printf("✅ Surah %d complete!\n\n", surahNumber);
}

// Example: Render multiple surahs
int main() {
    // ... (initialize renderer as shown above)
    QuranRendererHandle renderer = /* ... */;
    QuranPixelBuffer buffer = /* ... */;
    
    // Render Surah Al-Fatiha (1)
    render_surah(renderer, &buffer, 1);
    
    // Render Surah Yaseen (36)
    render_surah(renderer, &buffer, 36);
    
    // Render Surah Al-Mulk (67)
    render_surah(renderer, &buffer, 67);
    
    // Render all surahs
    for (int surah = 1; surah <= 114; surah++) {
        render_surah(renderer, &buffer, surah);
    }
    
    quran_renderer_destroy(renderer);
    return 0;
}
```

#### Rendering Ayah by Ayah (C API)

```c
#include <quran/renderer.h>
#include <stdio.h>
#include <stdlib.h>

// Render page containing a specific ayah
void render_ayah(QuranRendererHandle renderer, QuranPixelBuffer* buffer, 
                 int surahNumber, int ayahNumber) {
    // Get the page containing this ayah
    int pageIndex = quran_renderer_get_ayah_page(surahNumber, ayahNumber);
    
    if (pageIndex < 0) {
        fprintf(stderr, "Invalid ayah: Surah %d, Ayah %d\n", surahNumber, ayahNumber);
        return;
    }
    
    // Get surah info for context
    QuranSurahInfo info;
    quran_renderer_get_surah_info(surahNumber, &info);
    
    printf("Rendering Surah %d (%s), Ayah %d\n", surahNumber, info.nameEnglish, ayahNumber);
    printf("  Located on page %d\n", pageIndex + 1);
    
    // Render the page
    QuranRenderConfig config = { .tajweed = true, .justify = true, .fontScale = 1.0f };
    quran_renderer_draw_page(renderer, buffer, pageIndex, &config);
    
    // Save or display the page
    char filename[256];
    snprintf(filename, sizeof(filename), "surah_%d_ayah_%d_page_%d.rgba", 
             surahNumber, ayahNumber, pageIndex + 1);
    FILE* out = fopen(filename, "wb");
    fwrite(buffer->pixels, 1, buffer->height * buffer->stride, out);
    fclose(out);
    
    printf("✅ Rendered!\n\n");
}

// Render all ayahs of a surah (one page per ayah location)
void render_surah_ayahs(QuranRendererHandle renderer, QuranPixelBuffer* buffer, int surahNumber) {
    int ayahCount = quran_renderer_get_ayah_count(surahNumber);
    
    if (ayahCount < 0) {
        fprintf(stderr, "Invalid surah: %d\n", surahNumber);
        return;
    }
    
    QuranSurahInfo info;
    quran_renderer_get_surah_info(surahNumber, &info);
    
    printf("Rendering all ayahs of Surah %d: %s\n", surahNumber, info.nameEnglish);
    printf("  Total ayahs: %d\n", ayahCount);
    
    int lastPage = -1;  // Track to avoid rendering same page multiple times
    
    for (int ayah = 1; ayah <= ayahCount; ayah++) {
        int pageIndex = quran_renderer_get_ayah_page(surahNumber, ayah);
        
        // Only render if we've moved to a new page
        if (pageIndex != lastPage) {
            printf("  Ayah %d on page %d\n", ayah, pageIndex + 1);
            
            QuranRenderConfig config = { .tajweed = true, .justify = true, .fontScale = 1.0f };
            quran_renderer_draw_page(renderer, buffer, pageIndex, &config);
            
            // Save page
            char filename[256];
            snprintf(filename, sizeof(filename), "surah_%d_page_%d.rgba", 
                     surahNumber, pageIndex + 1);
            FILE* out = fopen(filename, "wb");
            fwrite(buffer->pixels, 1, buffer->height * buffer->stride, out);
            fclose(out);
            
            lastPage = pageIndex;
        }
    }
    
    printf("✅ Complete!\n\n");
}

// Example usage
int main() {
    // ... (initialize renderer)
    QuranRendererHandle renderer = /* ... */;
    QuranPixelBuffer buffer = /* ... */;
    
    // Render specific ayahs
    render_ayah(renderer, &buffer, 2, 255);   // Ayat al-Kursi
    render_ayah(renderer, &buffer, 18, 10);   // Surah Al-Kahf, Ayah 10
    render_ayah(renderer, &buffer, 36, 1);    // Surah Yaseen, start
    
    // Render all ayahs of a surah (generates one page per unique page location)
    render_surah_ayahs(renderer, &buffer, 1);   // Al-Fatiha
    render_surah_ayahs(renderer, &buffer, 112); // Al-Ikhlas
    
    quran_renderer_destroy(renderer);
    return 0;
}
```

#### Complete Navigation Example (C API)

```c
#include <quran/renderer.h>
#include <stdio.h>

void explore_quran() {
    // Get basic statistics
    int totalSurahs = quran_renderer_get_surah_count();     // 114
    int totalAyahs = quran_renderer_get_total_ayah_count(); // 6236
    int totalPages = 604;
    
    printf("=== Quran Statistics ===\n");
    printf("Surahs: %d\n", totalSurahs);
    printf("Ayahs: %d\n", totalAyahs);
    printf("Pages: %d\n\n", totalPages);
    
    // List all surahs with their locations
    printf("=== All Surahs ===\n");
    for (int i = 1; i <= totalSurahs; i++) {
        QuranSurahInfo info;
        if (quran_renderer_get_surah_info(i, &info)) {
            int startPage = quran_renderer_get_surah_start_page(i);
            printf("%3d. %-20s %-30s (%-7s) - %3d ayahs, page %3d\n",
                   info.number, info.nameEnglish, info.nameArabic, 
                   info.type, info.ayahCount, startPage + 1);
        }
    }
    
    // Find famous ayahs
    printf("\n=== Famous Ayahs ===\n");
    
    struct { int surah; int ayah; const char* name; } famous[] = {
        {2, 255, "Ayat al-Kursi"},
        {18, 10, "Cave Sleepers"},
        {36, 1, "Yaseen Opening"},
        {55, 13, "Which favors"},
        {112, 1, "Surah Al-Ikhlas"}
    };
    
    for (int i = 0; i < 5; i++) {
        int page = quran_renderer_get_ayah_page(famous[i].surah, famous[i].ayah);
        printf("%-20s - Surah %3d, Ayah %3d → Page %3d\n", 
               famous[i].name, famous[i].surah, famous[i].ayah, page + 1);
    }
    
    // Show what's on specific pages
    printf("\n=== Page Locations ===\n");
    int pages[] = {1, 50, 100, 200, 300, 400, 500, 604};
    for (int i = 0; i < 8; i++) {
        QuranAyahLocation loc;
        if (quran_renderer_get_page_location(pages[i] - 1, &loc)) {
            QuranSurahInfo info;
            quran_renderer_get_surah_info(loc.surahNumber, &info);
            printf("Page %3d starts at: Surah %3d (%s), Ayah %3d\n",
                   pages[i], loc.surahNumber, info.nameEnglish, loc.ayahNumber);
        }
    }
}

int main() {
    explore_quran();
    return 0;
}
```

#### Swift Usage (iOS/macOS)

```swift
import Foundation

class QuranRenderer {
    private var handle: QuranRendererHandle?
    private var fontData: Data?
    
    init?(fontPath: String) {
        guard let data = FileManager.default.contents(atPath: fontPath) else {
            return nil
        }
        
        fontData = data
        
        var font = QuranFontData()
        fontData?.withUnsafeBytes { ptr in
            font.data = ptr.baseAddress?.assumingMemoryBound(to: UInt8.self)
            font.size = data.count
        }
        
        handle = quran_renderer_create(&font)
        guard handle != nil else { return nil }
    }
    
    deinit {
        if let handle = handle {
            quran_renderer_destroy(handle)
        }
    }
    
    func renderPage(pageIndex: Int, width: Int, height: Int, 
                    tajweed: Bool = true, fontScale: Float = 1.0) -> Data? {
        guard let handle = handle else { return nil }
        
        var pixels = [UInt8](repeating: 0, count: width * height * 4)
        
        var buffer = QuranPixelBuffer()
        buffer.width = Int32(width)
        buffer.height = Int32(height)
        buffer.stride = Int32(width * 4)
        buffer.format = QURAN_PIXEL_FORMAT_RGBA8888
        
        pixels.withUnsafeMutableBytes { ptr in
            buffer.pixels = ptr.baseAddress
            
            var config = QuranRenderConfig()
            config.tajweed = tajweed
            config.justify = true
            config.fontScale = fontScale
            
            quran_renderer_draw_page(handle, &buffer, Int32(pageIndex), &config)
        }
        
        return Data(pixels)
    }
    
    var pageCount: Int {
        guard let handle = handle else { return 0 }
        return Int(quran_renderer_get_page_count(handle))
    }
    
    // Surah/Ayah API (no handle required)
    
    static var surahCount: Int {
        return Int(quran_renderer_get_surah_count())
    }
    
    static var totalAyahCount: Int {
        return Int(quran_renderer_get_total_ayah_count())
    }
    
    static func getSurahInfo(_ surahNumber: Int) -> SurahInfo? {
        var info = QuranSurahInfo()
        guard quran_renderer_get_surah_info(Int32(surahNumber), &info) else { return nil }
        return SurahInfo(
            number: Int(info.number),
            ayahCount: Int(info.ayahCount),
            startAyah: Int(info.startAyah),
            nameArabic: String(cString: info.nameArabic),
            nameTrans: String(cString: info.nameTrans),
            nameEnglish: String(cString: info.nameEnglish),
            type: String(cString: info.type),
            revelationOrder: Int(info.revelationOrder),
            rukuCount: Int(info.rukuCount)
        )
    }
    
    static func getAyahCount(_ surahNumber: Int) -> Int {
        return Int(quran_renderer_get_ayah_count(Int32(surahNumber)))
    }
    
    static func getSurahStartPage(_ surahNumber: Int) -> Int {
        return Int(quran_renderer_get_surah_start_page(Int32(surahNumber)))
    }
    
    static func getAyahPage(surah: Int, ayah: Int) -> Int {
        return Int(quran_renderer_get_ayah_page(Int32(surah), Int32(ayah)))
    }
    
    static func getPageLocation(_ pageIndex: Int) -> AyahLocation? {
        var loc = QuranAyahLocation()
        guard quran_renderer_get_page_location(Int32(pageIndex), &loc) else { return nil }
        return AyahLocation(
            surahNumber: Int(loc.surahNumber),
            ayahNumber: Int(loc.ayahNumber),
            pageIndex: Int(loc.pageIndex)
        )
    }
}

// Swift data structures
struct SurahInfo {
    let number: Int
    let ayahCount: Int
    let startAyah: Int
    let nameArabic: String
    let nameTrans: String
    let nameEnglish: String
    let type: String
    let revelationOrder: Int
    let rukuCount: Int
}

struct AyahLocation {
    let surahNumber: Int
    let ayahNumber: Int
    let pageIndex: Int
}

// Usage
if let renderer = QuranRenderer(fontPath: Bundle.main.path(forResource: "digitalkhatt", ofType: "otf")!) {
    if let imageData = renderer.renderPage(pageIndex: 0, width: 1080, height: 1920) {
        // Create UIImage/NSImage from imageData
    }
}

// Surah/Ayah API usage (static methods, no renderer needed)
print("Total surahs: \(QuranRenderer.surahCount)")  // 114
print("Total ayahs: \(QuranRenderer.totalAyahCount)")  // 6236

if let surah = QuranRenderer.getSurahInfo(36) {  // Yaseen
    print("Surah \(surah.number): \(surah.nameTrans)")
    print("Ayahs: \(surah.ayahCount), Type: \(surah.type)")
}

let yaseenPage = QuranRenderer.getSurahStartPage(36)
let ayatKursiPage = QuranRenderer.getAyahPage(surah: 2, ayah: 255)
```

---

#### Surah/Ayah API

The library provides metadata APIs for accessing surah and ayah information without needing to render pages.

**C/C++ Usage:**

```c
#include <quran/renderer.h>
#include <stdio.h>

int main() {
    // Get basic counts (no renderer needed)
    printf("Total Surahs: %d\n", quran_renderer_get_surah_count());  // 114
    printf("Total Ayahs: %d\n", quran_renderer_get_total_ayah_count());  // 6236
    
    // Get surah information
    QuranSurahInfo info;
    if (quran_renderer_get_surah_info(1, &info)) {
        printf("Surah %d: %s (%s)\n", info.number, info.nameTrans, info.nameEnglish);
        printf("  Arabic: %s\n", info.nameArabic);
        printf("  Ayahs: %d, Type: %s\n", info.ayahCount, info.type);
        printf("  Revelation Order: %d\n", info.revelationOrder);
    }
    
    // Get ayah count for a surah
    int ayahs = quran_renderer_get_ayah_count(2);  // Al-Baqara = 286
    
    // Find which page a surah starts on
    int page = quran_renderer_get_surah_start_page(36);  // Yaseen
    
    // Find which page a specific ayah is on
    page = quran_renderer_get_ayah_page(2, 255);  // Ayat Al-Kursi
    
    // Get the surah/ayah that starts a page
    QuranAyahLocation loc;
    if (quran_renderer_get_page_location(0, &loc)) {
        printf("Page 0 starts with Surah %d, Ayah %d\n", 
               loc.surahNumber, loc.ayahNumber);  // Surah 1, Ayah 1
    }
    
    return 0;
}
```

**API Reference:**

| Function | Description |
|----------|-------------|
| `quran_renderer_get_surah_count()` | Returns 114 (total surahs) |
| `quran_renderer_get_total_ayah_count()` | Returns 6236 (total ayahs) |
| `quran_renderer_get_surah_info(surah, &info)` | Get detailed surah information |
| `quran_renderer_get_ayah_count(surah)` | Get number of ayahs in a surah |
| `quran_renderer_get_surah_start_page(surah)` | Get page index where surah starts |
| `quran_renderer_get_ayah_page(surah, ayah)` | Get page index for specific ayah |
| `quran_renderer_get_page_location(page, &loc)` | Get surah/ayah at start of page |

**QuranSurahInfo Structure:**

```c
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
```

---

#### Custom View Example

```kotlin
class QuranPageView(context: Context, attrs: AttributeSet?) : View(context, attrs) {
    private var pageIndex: Int = 0
    private var tajweed: Boolean = true
    private var bitmap: Bitmap? = null
    private val renderer = QuranRenderer.getInstance()

    fun setPage(page: Int) {
        pageIndex = page
        bitmap = null
        invalidate()
    }

    fun setTajweed(enabled: Boolean) {
        tajweed = enabled
        bitmap = null
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if (width <= 0 || height <= 0) return

        if (bitmap == null) {
            bitmap = renderer.renderPage(width, height, pageIndex, tajweed, true)
        }
        bitmap?.let { canvas.drawBitmap(it, 0f, 0f, null) }
    }
}
```

---

### Flutter Integration

#### Step 1: Add AAR to Flutter Project

```bash
# Create libs directory
mkdir -p android/app/libs

# Copy AAR files
cp /path/to/android-debug.aar android/app/libs/
cp /path/to/android-release.aar android/app/libs/
```

#### Step 2: Update `android/app/build.gradle.kts`

```kotlin
dependencies {
    debugImplementation(files("libs/android-debug.aar"))
    releaseImplementation(files("libs/android-release.aar"))
}
```

#### Step 3: Create Native Android View

Create `android/app/src/main/kotlin/.../QuranPageView.kt`:

```kotlin
package com.example.yourapp

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.view.View
import org.digitalkhatt.quran.renderer.QuranRenderer

class QuranPageView(context: Context) : View(context) {
    private var pageIndex: Int = 0
    private var tajweed: Boolean = true
    private var bitmap: Bitmap? = null
    private val renderer = QuranRenderer.getInstance()

    fun setPage(page: Int) {
        if (pageIndex != page) {
            pageIndex = page
            bitmap = null
            invalidate()
        }
    }

    fun setTajweed(enabled: Boolean) {
        if (tajweed != enabled) {
            tajweed = enabled
            bitmap = null
            invalidate()
        }
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        bitmap = null
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if (width <= 0 || height <= 0) return

        if (bitmap == null || bitmap!!.width != width || bitmap!!.height != height) {
            bitmap?.recycle()
            bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
            renderer.drawPage(bitmap!!, pageIndex, tajweed, true)
        }
        bitmap?.let { canvas.drawBitmap(it, 0f, 0f, null) }
    }
}
```

#### Step 4: Create Platform View Factory

Create `android/app/src/main/kotlin/.../QuranPageViewFactory.kt`:

```kotlin
package com.example.yourapp

import android.content.Context
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.StandardMessageCodec
import io.flutter.plugin.platform.PlatformView
import io.flutter.plugin.platform.PlatformViewFactory

class QuranPageViewFactory(
    private val messenger: BinaryMessenger
) : PlatformViewFactory(StandardMessageCodec.INSTANCE) {
    
    override fun create(context: Context, viewId: Int, args: Any?): PlatformView {
        val creationParams = args as? Map<String, Any?>
        return QuranPlatformView(context, viewId, creationParams, messenger)
    }
}

class QuranPlatformView(
    context: Context,
    private val viewId: Int,
    creationParams: Map<String, Any?>?,
    messenger: BinaryMessenger
) : PlatformView, MethodChannel.MethodCallHandler {
    
    private val quranView = QuranPageView(context)
    private val methodChannel = MethodChannel(messenger, "quran_page_view_$viewId")

    init {
        methodChannel.setMethodCallHandler(this)
        creationParams?.let {
            (it["pageIndex"] as? Int)?.let { page -> quranView.setPage(page) }
            (it["tajweed"] as? Boolean)?.let { tajweed -> quranView.setTajweed(tajweed) }
        }
    }

    override fun getView() = quranView
    override fun dispose() { methodChannel.setMethodCallHandler(null) }

    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "setPage" -> {
                quranView.setPage(call.argument<Int>("page") ?: 0)
                result.success(null)
            }
            "setTajweed" -> {
                quranView.setTajweed(call.argument<Boolean>("enabled") ?: true)
                result.success(null)
            }
            else -> result.notImplemented()
        }
    }
}
```

#### Step 5: Register in MainActivity

Update `android/app/src/main/kotlin/.../MainActivity.kt`:

```kotlin
package com.example.yourapp

import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import org.digitalkhatt.quran.renderer.QuranRenderer

class MainActivity : FlutterActivity() {
    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)

        // Initialize renderer with font from assets
        QuranRenderer.getInstance().initialize(assets, "fonts/digitalkhatt.otf")

        // Register platform view
        flutterEngine.platformViewsController.registry
            .registerViewFactory(
                "quran-page-view",
                QuranPageViewFactory(flutterEngine.dartExecutor.binaryMessenger)
            )
    }
}
```

#### Step 6: Create Flutter Widget

Create `lib/quran_page_widget.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class QuranPageWidget extends StatefulWidget {
  final int pageIndex;
  final bool tajweed;

  const QuranPageWidget({
    super.key,
    required this.pageIndex,
    this.tajweed = true,
  });

  @override
  State<QuranPageWidget> createState() => _QuranPageWidgetState();
}

class _QuranPageWidgetState extends State<QuranPageWidget> {
  MethodChannel? _channel;

  @override
  void didUpdateWidget(QuranPageWidget oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (_channel != null) {
      if (oldWidget.pageIndex != widget.pageIndex) {
        _channel!.invokeMethod('setPage', {'page': widget.pageIndex});
      }
      if (oldWidget.tajweed != widget.tajweed) {
        _channel!.invokeMethod('setTajweed', {'enabled': widget.tajweed});
      }
    }
  }

  void _onPlatformViewCreated(int viewId) {
    _channel = MethodChannel('quran_page_view_$viewId');
  }

  @override
  Widget build(BuildContext context) {
    return AndroidView(
      viewType: 'quran-page-view',
      creationParams: {
        'pageIndex': widget.pageIndex,
        'tajweed': widget.tajweed,
      },
      creationParamsCodec: const StandardMessageCodec(),
      onPlatformViewCreated: _onPlatformViewCreated,
    );
  }
}
```

#### Step 7: Use in Your App

```dart
import 'package:flutter/material.dart';
import 'quran_page_widget.dart';

class QuranReaderPage extends StatefulWidget {
  @override
  State<QuranReaderPage> createState() => _QuranReaderPageState();
}

class _QuranReaderPageState extends State<QuranReaderPage> {
  int _currentPage = 0;
  bool _tajweedEnabled = true;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Page ${_currentPage + 1} of 604'),
        actions: [
          Row(
            children: [
              const Text('Tajweed'),
              Switch(
                value: _tajweedEnabled,
                onChanged: (value) => setState(() => _tajweedEnabled = value),
              ),
            ],
          ),
        ],
      ),
      body: PageView.builder(
        itemCount: 604,
        reverse: true, // RTL for Arabic
        onPageChanged: (index) => setState(() => _currentPage = index),
        itemBuilder: (context, index) => QuranPageWidget(
          pageIndex: index,
          tajweed: _tajweedEnabled,
        ),
      ),
    );
  }
}
```

#### Flutter Surah/Ayah API via Method Channel

To access the surah/ayah metadata from Flutter, add a method channel:

**Android side** - Update `MainActivity.kt`:

```kotlin
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {
    private val CHANNEL = "org.digitalkhatt.quran/metadata"
    
    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        
        val renderer = QuranRenderer.getInstance()
        renderer.initialize(assets)
        
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, CHANNEL).setMethodCallHandler { call, result ->
            when (call.method) {
                "getSurahCount" -> result.success(renderer.surahCount)
                "getTotalAyahCount" -> result.success(renderer.totalAyahCount)
                "getSurahInfo" -> {
                    val surahNumber = call.argument<Int>("surahNumber") ?: 1
                    val info = renderer.getSurahInfo(surahNumber)
                    result.success(info?.let {
                        mapOf(
                            "number" to it.number,
                            "ayahCount" to it.ayahCount,
                            "nameArabic" to it.nameArabic,
                            "nameTrans" to it.nameTrans,
                            "nameEnglish" to it.nameEnglish,
                            "type" to it.type,
                            "revelationOrder" to it.revelationOrder,
                            "rukuCount" to it.rukuCount
                        )
                    })
                }
                "getAyahCount" -> {
                    val surahNumber = call.argument<Int>("surahNumber") ?: 1
                    result.success(renderer.getAyahCount(surahNumber))
                }
                "getSurahStartPage" -> {
                    val surahNumber = call.argument<Int>("surahNumber") ?: 1
                    result.success(renderer.getSurahStartPage(surahNumber))
                }
                "getAyahPage" -> {
                    val surahNumber = call.argument<Int>("surahNumber") ?: 1
                    val ayahNumber = call.argument<Int>("ayahNumber") ?: 1
                    result.success(renderer.getAyahPage(surahNumber, ayahNumber))
                }
                "getPageLocation" -> {
                    val pageIndex = call.argument<Int>("pageIndex") ?: 0
                    val loc = renderer.getPageLocation(pageIndex)
                    result.success(loc?.let {
                        mapOf("surahNumber" to it.surahNumber, "ayahNumber" to it.ayahNumber)
                    })
                }
                else -> result.notImplemented()
            }
        }
    }
}
```

**Flutter side** - Create `lib/quran_metadata.dart`:

```dart
import 'package:flutter/services.dart';

class SurahInfo {
  final int number;
  final int ayahCount;
  final String nameArabic;
  final String nameTrans;
  final String nameEnglish;
  final String type;
  final int revelationOrder;
  final int rukuCount;

  SurahInfo.fromMap(Map<dynamic, dynamic> map)
      : number = map['number'],
        ayahCount = map['ayahCount'],
        nameArabic = map['nameArabic'],
        nameTrans = map['nameTrans'],
        nameEnglish = map['nameEnglish'],
        type = map['type'],
        revelationOrder = map['revelationOrder'],
        rukuCount = map['rukuCount'];
}

class QuranMetadata {
  static const _channel = MethodChannel('org.digitalkhatt.quran/metadata');

  static Future<int> get surahCount => _channel.invokeMethod<int>('getSurahCount').then((v) => v ?? 114);
  static Future<int> get totalAyahCount => _channel.invokeMethod<int>('getTotalAyahCount').then((v) => v ?? 6236);

  static Future<SurahInfo?> getSurahInfo(int surahNumber) async {
    final map = await _channel.invokeMethod<Map>('getSurahInfo', {'surahNumber': surahNumber});
    return map != null ? SurahInfo.fromMap(map) : null;
  }

  static Future<int> getAyahCount(int surahNumber) =>
      _channel.invokeMethod<int>('getAyahCount', {'surahNumber': surahNumber}).then((v) => v ?? -1);

  static Future<int> getSurahStartPage(int surahNumber) =>
      _channel.invokeMethod<int>('getSurahStartPage', {'surahNumber': surahNumber}).then((v) => v ?? -1);

  static Future<int> getAyahPage(int surahNumber, int ayahNumber) =>
      _channel.invokeMethod<int>('getAyahPage', {'surahNumber': surahNumber, 'ayahNumber': ayahNumber}).then((v) => v ?? -1);

  static Future<({int surahNumber, int ayahNumber})?> getPageLocation(int pageIndex) async {
    final map = await _channel.invokeMethod<Map>('getPageLocation', {'pageIndex': pageIndex});
    return map != null ? (surahNumber: map['surahNumber'] as int, ayahNumber: map['ayahNumber'] as int) : null;
  }
  
  static Future<List<SurahInfo>> getAllSurahs() async {
    final surahs = <SurahInfo>[];
    for (int i = 1; i <= 114; i++) {
      final info = await getSurahInfo(i);
      if (info != null) surahs.add(info);
    }
    return surahs;
  }
}

// Usage example:
// final surah = await QuranMetadata.getSurahInfo(1);
// print('${surah?.nameTrans}: ${surah?.ayahCount} ayahs');
// 
// final page = await QuranMetadata.getAyahPage(2, 255);  // Ayat Al-Kursi
// print('Ayat Al-Kursi is on page $page');
```

---

## API Reference

### QuranRenderer (Kotlin)

```kotlin
class QuranRenderer {
    companion object {
        fun getInstance(): QuranRenderer
    }

    // Initialize with font from assets
    fun initialize(assetManager: AssetManager, fontPath: String): Boolean

    // Check initialization status
    fun isInitialized(): Boolean

    // Get total page count (604 for standard Mushaf)
    val pageCount: Int

    // Render page to new bitmap
    fun renderPage(
        width: Int,
        height: Int,
        pageIndex: Int,          // 0-603
        tajweed: Boolean = true, // Enable tajweed colors
        justify: Boolean = true  // Enable line justification
    ): Bitmap

    // Render page into existing bitmap (must be ARGB_8888)
    fun drawPage(
        bitmap: Bitmap,
        pageIndex: Int,
        tajweed: Boolean = true,
        justify: Boolean = true
    )

    // Release native resources
    fun destroy()
}
```

### Page Indices

| Page Index | Content |
|------------|---------|
| 0 | Al-Fatiha |
| 1 | Al-Baqarah starts |
| 603 | An-Nas (last page) |

---

## Building Dependencies from Source

### Automated Build Script (Recommended)

The easiest way to build all dependencies is using the provided script:

```bash
# From the project root
./scripts/build-dependencies.sh
```

This script will:
1. Clone/update HarfBuzz from https://github.com/DigitalKhatt/harfbuzz (justification branch)
2. Clone/update VisualMetaFont from https://github.com/DigitalKhatt/visualmetafont
3. Clone Skia and build static libraries for all 3 ABIs
4. Organize output files in the correct structure
5. Generate `local.properties` automatically

**Script options:**
```bash
./scripts/build-dependencies.sh --ndk-path /path/to/ndk  # Specify NDK path
./scripts/build-dependencies.sh --deps-dir /path/to/deps # Custom deps location
./scripts/build-dependencies.sh --skip-skia              # Skip Skia (already built)
./scripts/build-dependencies.sh --clean                  # Clean rebuild
```

---

### Manual Build Instructions

If you prefer to build manually, follow these steps:

#### 1. Clone All Dependencies

```bash
# Create a working directory
mkdir -p ~/quran-renderer-deps && cd ~/quran-renderer-deps

# Clone HarfBuzz (DigitalKhatt fork with justification support)
git clone -b justification https://github.com/DigitalKhatt/harfbuzz.git

# Clone VisualMetaFont (Quran text data)
git clone https://github.com/DigitalKhatt/visualmetafont.git

# Clone Skia (for building static library)
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$PWD/depot_tools:$PATH"
git clone https://skia.googlesource.com/skia.git
```

#### 2. Build Skia for Android

```bash
cd ~/quran-renderer-deps/skia
python3 tools/git-sync-deps

# Build for all Android ABIs
# arm64-v8a
bin/gn gen out/android-arm64 --args='
  target_os="android"
  target_cpu="arm64"
  ndk="/path/to/Android/Sdk/ndk/28.2.13676358"
  is_official_build=true
  is_component_build=false
  is_debug=false
  skia_use_system_freetype2=false
  skia_use_freetype=true
  skia_enable_fontmgr_android=true
  skia_use_gl=true
'
ninja -C out/android-arm64 skia

# armeabi-v7a
bin/gn gen out/android-arm --args='
  target_os="android"
  target_cpu="arm"
  ndk="/path/to/Android/Sdk/ndk/28.2.13676358"
  is_official_build=true
  is_component_build=false
  is_debug=false
  skia_use_system_freetype2=false
  skia_use_freetype=true
  skia_enable_fontmgr_android=true
  skia_use_gl=true
'
ninja -C out/android-arm skia

# x86_64 (for emulators)
bin/gn gen out/android-x64 --args='
  target_os="android"
  target_cpu="x64"
  ndk="/path/to/Android/Sdk/ndk/28.2.13676358"
  is_official_build=true
  is_component_build=false
  is_debug=false
  skia_use_system_freetype2=false
  skia_use_freetype=true
  skia_enable_fontmgr_android=true
  skia_use_gl=true
'
ninja -C out/android-x64 skia
```

### 3. Organize Skia Output

```bash
# Create output directory structure
mkdir -p ~/quran-renderer-deps/skia-build/out/{arm64-v8a,armeabi-v7a,x86_64,include_root}

# Copy static libraries
cp ~/quran-renderer-deps/skia/out/android-arm64/libskia.a ~/quran-renderer-deps/skia-build/out/arm64-v8a/
cp ~/quran-renderer-deps/skia/out/android-arm/libskia.a ~/quran-renderer-deps/skia-build/out/armeabi-v7a/
cp ~/quran-renderer-deps/skia/out/android-x64/libskia.a ~/quran-renderer-deps/skia-build/out/x86_64/

# Copy headers (maintaining structure for includes like "include/core/SkCanvas.h")
cp -r ~/quran-renderer-deps/skia/include ~/quran-renderer-deps/skia-build/out/include_root/
```

### 4. Configure local.properties

```properties
# In your quran-renderer-android/local.properties:
sdk.dir=/path/to/Android/Sdk
harfbuzz.dir=/home/user/quran-renderer-deps/harfbuzz
vmf.dir=/home/user/quran-renderer-deps/visualmetafont
skia.dir=/home/user/quran-renderer-deps/skia-build/out
```

### 5. Build the AAR

```bash
cd /path/to/quran-renderer-android
./gradlew :android:assembleRelease
```

Output: `android/build/outputs/aar/android-release.aar`

---

### Updating Dependencies

To update to newer versions:

```bash
# Update HarfBuzz
cd ~/quran-renderer-deps/harfbuzz
git pull origin justification

# Update VisualMetaFont  
cd ~/quran-renderer-deps/visualmetafont
git pull origin main

# Update Skia (then rebuild)
cd ~/quran-renderer-deps/skia
git pull
python3 tools/git-sync-deps
# Rebuild for all ABIs as shown above
```

---

## Legacy: Building Skia Only

If you only need to rebuild Skia:

```bash
# Clone depot_tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$PWD/depot_tools:$PATH"

# Clone Skia
git clone https://skia.googlesource.com/skia.git
cd skia
python3 tools/git-sync-deps

# Build for Android arm64
bin/gn gen out/android-arm64 --args='
  target_os="android"
  target_cpu="arm64"
  ndk="/path/to/Android/Sdk/ndk/28.2.13676358"
  is_official_build=true
  is_component_build=false
  is_debug=false
  skia_use_system_freetype2=false
  skia_use_freetype=true
  skia_enable_fontmgr_android=true
  skia_use_gl=true
'
ninja -C out/android-arm64 skia

# Repeat for arm, x64 ABIs with target_cpu="arm" and target_cpu="x64"
```

---

## Troubleshooting

### Common Issues

**1. Font not loading**
- Ensure font file is in `assets/fonts/` directory
- Check font path matches exactly (case-sensitive)

**2. Black/empty pages**
- Verify `initialize()` returned `true`
- Check page index is within `0..<pageCount`

**3. Crash on bitmap rendering**
- Bitmap must be `ARGB_8888` config
- Bitmap dimensions must be > 0

**4. NDK version mismatch**
- Ensure NDK 28.2.13676358 is installed
- Set `ndkVersion` in `build.gradle` if needed

---

## License

This project is licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0)**, consistent with the DigitalKhatt project.

See [LICENSE](LICENSE) for full text.

---

## Credits

- **DigitalKhatt**: Original Quran rendering engine by Amine Anane
  - Website: https://digitalkhatt.org
  - HarfBuzz fork: https://github.com/DigitalKhatt/harfbuzz
  - VisualMetaFont: https://github.com/DigitalKhatt/visualmetafont

- **Skia**: 2D graphics library by Google
  - https://skia.org

---

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

---

## Support

For issues and questions:
- Open an issue on GitHub
