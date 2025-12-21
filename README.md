# Quran Renderer Library

A cross-platform, high-quality Quran text rendering library using custom HarfBuzz (with Arabic justification) and Skia.

## Features

- **Professional Arabic Typography**: Uses custom HarfBuzz fork with line justification via variable font tatweel extension
- **Tajweed Coloring**: Per-glyph color output from OpenType lookup execution
- **Variable Font Support**: Per-glyph axis control for kashida/tatweel stretching
- **High-Quality Rendering**: Skia-based rendering with anti-aliasing
- **Cross-Platform**: Core C++ library with platform wrappers

## Architecture

```
quran-renderer-android/
├── include/quran/          # Public C API headers
│   └── renderer.h
├── src/
│   └── core/               # Platform-agnostic core
│       ├── quran_renderer.cpp
│       ├── hb_skia_canvas.cpp/h
│       └── quran.h
├── android/                # Android library module (.aar)
│   └── src/main/
│       ├── cpp/           # JNI wrapper
│       └── java/          # Kotlin API
└── CMakeLists.txt         # Cross-platform CMake
```

## Dependencies

This library requires:

1. **HarfBuzz (justification branch)**: https://github.com/DigitalKhatt/harfbuzz
   - Branch: `justification`
   - Custom features: `hb_buffer_set_justify()`, tatweel extension fields

2. **Skia**: https://skia.org/
   - Pre-built for target platform
   
3. **VisualMetaFont**: https://github.com/DigitalKhatt/visualmetafont
   - Provides `quran.cpp` with text data

4. **Quran Font**: Custom OpenType variable font with tajweed lookups

## Building for Android

### Prerequisites

1. Android Studio with NDK 27.0.12077973 or later
2. CMake 3.22.1 or later
3. Pre-built Skia for arm64-v8a

### Configuration

1. Copy `local.properties.template` to `local.properties`
2. Set the paths to dependencies:

```properties
harfbuzz.dir=/path/to/harfbuzz
skia.dir=/path/to/skia
vmf.dir=/path/to/visualmetafont
sdk.dir=/path/to/Android/Sdk
```

### Build AAR

```bash
./gradlew :android:assembleRelease
```

Output: `android/build/outputs/aar/android-release.aar`

## Usage

### Android (Kotlin)

```kotlin
import org.digitalkhatt.quran.renderer.QuranRenderer
import android.graphics.Bitmap

// Initialize
val renderer = QuranRenderer.getInstance()
renderer.initialize(assets, "fonts/quran.otf")

// Render a page
val bitmap = renderer.renderPage(
    width = 1080,
    height = 1920,
    pageIndex = 0,  // First page (Al-Fatiha)
    tajweed = true,
    justify = true
)

// Or render into existing bitmap
renderer.drawPage(existingBitmap, pageIndex = 1)

// Cleanup when done
renderer.destroy()
```

### Flutter Integration (via Platform Views)

```dart
// In your Flutter plugin, create a platform view that uses QuranRenderer
class QuranPageView extends StatelessWidget {
  final int pageIndex;
  
  @override
  Widget build(BuildContext context) {
    return AndroidView(
      viewType: 'org.digitalkhatt.quran/page_view',
      creationParams: {'pageIndex': pageIndex},
      creationParamsCodec: const StandardMessageCodec(),
    );
  }
}
```

## C API

```c
#include <quran/renderer.h>

// Create renderer with font data
QuranFontData fontData = { fontBytes, fontSize };
QuranRendererHandle renderer = quran_renderer_create(&fontData);

// Render a page
QuranPixelBuffer buffer = { pixels, width, height, stride, QURAN_PIXEL_FORMAT_RGBA8888 };
QuranRenderConfig config = { true, true };  // tajweed, justify
quran_renderer_draw_page(renderer, &buffer, pageIndex, &config);

// Cleanup
quran_renderer_destroy(renderer);
```

## License

AGPL-3.0 (same as DigitalKhatt)

## Credits

Based on [DigitalKhatt](https://digitalkhatt.org/) by Amine Anane.
