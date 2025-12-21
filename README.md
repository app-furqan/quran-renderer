# Quran Renderer Android

A high-quality Quran text rendering library for Android using the **[DigitalKhatt](https://digitalkhatt.org)** Quran font. Features professional Arabic typography with tajweed coloring and automatic line justification using kashida stretching.

This library renders Quran pages using:
- **DigitalKhatt Font** - A variable OpenType font specifically designed for Quran typography with COLR-based tajweed highlighting
- **HarfBuzz (DigitalKhatt fork)** - Custom text shaping engine with Arabic text justification support
- **VisualMetaFont (DigitalKhatt)** - Quran text data and page layout information
- **Skia Graphics Engine** - High-quality 2D rendering

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

## Features

- **Professional Arabic Typography**: Custom HarfBuzz fork with kashida/tatweel-based line justification
- **Tajweed Coloring**: Per-glyph color output from OpenType COLR table lookups
- **Variable Font Support**: Per-glyph axis control for dynamic kashida stretching
- **High-Quality Rendering**: Skia-based rendering with anti-aliasing and subpixel positioning
- **Multi-ABI Support**: Pre-built for `arm64-v8a`, `armeabi-v7a`, and `x86_64`
- **Small Footprint**: Static Skia linking (~7.6 MB release AAR)

## Architecture

```
quran-renderer-android/
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
├── build.gradle
├── settings.gradle
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
   - Provides Quran text data and page layout (`quran.cpp`)
   - Contains the text content for all 604 pages of the Quran

3. **Skia (static build)**: Built from https://skia.googlesource.com/skia
   - Pre-built static libraries for Android ABIs
   - Used for high-quality 2D rendering with anti-aliasing

4. **DigitalKhatt Quran Font**: Variable OpenType font with:
   - Tajweed coloring via COLR table lookups
   - Variable axis for kashida stretching
   - Visit https://digitalkhatt.org for more information

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

### Option 2: Build from Source

1. Clone the repository:
   ```bash
   git clone https://github.com/AliQasimzade/quran-renderer-android.git
   cd quran-renderer-android
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

## Usage

### Android (Kotlin)

#### Basic Usage

```kotlin
import org.digitalkhatt.quran.renderer.QuranRenderer
import android.graphics.Bitmap

class QuranActivity : AppCompatActivity() {
    private lateinit var renderer: QuranRenderer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Get singleton instance and initialize with font
        renderer = QuranRenderer.getInstance()
        val success = renderer.initialize(assets, "fonts/digitalkhatt.otf")
        
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

## Building Skia from Source

If you need to build Skia yourself:

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
