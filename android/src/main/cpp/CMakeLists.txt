cmake_minimum_required(VERSION 3.22.1)

project("quranrenderer")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path variables - these will be set via local.properties or build.gradle
# HARFBUZZ_DIR: Path to HarfBuzz source (justification branch)
# SKIA_DIR: Path to Skia source
# VMF_DIR: Path to VisualMetaFont source (for quran.cpp)

# HarfBuzz source files
set(HARFBUZZ_FILES
    ${HARFBUZZ_DIR}/src/hb-unicode.cc
    ${HARFBUZZ_DIR}/src/hb-aat-layout.cc
    ${HARFBUZZ_DIR}/src/hb-aat-map.cc
    ${HARFBUZZ_DIR}/src/hb-blob.cc
    ${HARFBUZZ_DIR}/src/hb-buffer.cc
    ${HARFBUZZ_DIR}/src/hb-common.cc
    ${HARFBUZZ_DIR}/src/hb-face.cc
    ${HARFBUZZ_DIR}/src/hb-fallback-shape.cc
    ${HARFBUZZ_DIR}/src/hb-font.cc
    ${HARFBUZZ_DIR}/src/hb-ft.cc
    ${HARFBUZZ_DIR}/src/hb-ot-cff1-table.cc
    ${HARFBUZZ_DIR}/src/hb-ot-cff2-table.cc
    ${HARFBUZZ_DIR}/src/hb-ot-face.cc
    ${HARFBUZZ_DIR}/src/hb-ot-font.cc
    ${HARFBUZZ_DIR}/src/hb-ot-layout.cc
    ${HARFBUZZ_DIR}/src/hb-ot-map.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-arabic.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-default.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-hangul.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-hebrew.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-indic-table.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-indic.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-khmer.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-myanmar.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-thai.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-use.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-syllabic.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shaper-vowel-constraints.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shape-fallback.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shape-normalize.cc
    ${HARFBUZZ_DIR}/src/hb-ot-shape.cc
    ${HARFBUZZ_DIR}/src/hb-ot-tag.cc
    ${HARFBUZZ_DIR}/src/hb-ot-var.cc
    ${HARFBUZZ_DIR}/src/hb-set.cc
    ${HARFBUZZ_DIR}/src/hb-shape-plan.cc
    ${HARFBUZZ_DIR}/src/hb-shape.cc
    ${HARFBUZZ_DIR}/src/hb-shaper.cc
    ${HARFBUZZ_DIR}/src/hb-static.cc
    ${HARFBUZZ_DIR}/src/hb-ucd.cc
    ${HARFBUZZ_DIR}/src/hb-ot-metrics.cc
    ${HARFBUZZ_DIR}/src/hb-number.cc
    ${HARFBUZZ_DIR}/src/hb-ot-name.cc
    ${HARFBUZZ_DIR}/src/hb-ot-layout-jtst-context.cc
    ${HARFBUZZ_DIR}/src/hb-ot-color.cc
    ${HARFBUZZ_DIR}/src/hb-paint.cc
    ${HARFBUZZ_DIR}/src/hb-paint-extents.cc
    ${HARFBUZZ_DIR}/src/hb-draw.cc
    ${HARFBUZZ_DIR}/src/hb-outline.cc
    ${HARFBUZZ_DIR}/src/hb-buffer-verify.cc
    ${HARFBUZZ_DIR}/src/hb-buffer-serialize.cc
)

# Quran text data
set(VMF_FILES
    ${VMF_DIR}/src/qurantext/quran.cpp
)

# Core library files
set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/core)
set(CORE_FILES
    ${CORE_DIR}/quran_renderer.cpp
    ${CORE_DIR}/hb_skia_canvas.cpp
)

# Android JNI wrapper
set(ANDROID_FILES
    quran_renderer_jni.cpp
)

# Create shared library
add_library(quranrenderer SHARED
    ${HARFBUZZ_FILES}
    ${VMF_FILES}
    ${CORE_FILES}
    ${ANDROID_FILES}
)

# Skia built from source (matches iOS configuration for consistent rendering)
# Expected location: ${SKIA_DIR}/out/android-${ANDROID_ABI}/libskia.a
# Note: build-android-skia.sh creates out/android-armeabi-v7a, out/android-arm64-v8a, etc.
add_library(skia STATIC IMPORTED)
set_target_properties(skia PROPERTIES
    IMPORTED_LOCATION "${SKIA_DIR}/out/android-${ANDROID_ABI}/libskia.a"
)

# Verify Skia static library exists
if(NOT EXISTS "${SKIA_DIR}/out/android-${ANDROID_ABI}/libskia.a")
    message(FATAL_ERROR
        "Skia static library not found at: ${SKIA_DIR}/out/android-${ANDROID_ABI}/libskia.a\n"
        "Please run: ./scripts/build-android-skia.sh ${ANDROID_ABI} to build Skia from source.\n"
        "This ensures consistent rendering with iOS/macOS."
    )
endif()

# Include paths for Skia headers
target_include_directories(skia INTERFACE
    ${SKIA_DIR}
    ${SKIA_DIR}/include/core
    ${SKIA_DIR}/include/config
    ${SKIA_DIR}/include/private
    ${SKIA_DIR}/include/private/base
)

message(STATUS "Android ${ANDROID_ABI}: Using Skia built from source")
message(STATUS "  Skia library: ${SKIA_DIR}/out/android-${ANDROID_ABI}/libskia.a")
message(STATUS "  This matches iOS configuration for consistent rendering")

# Compiler definitions
target_compile_definitions(quranrenderer PUBLIC 
    HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR
)

message(STATUS "Android ${ANDROID_ABI}: Building quranrenderer with:")
message(STATUS "  - HarfBuzz sources from: ${HARFBUZZ_DIR}/src")
message(STATUS "  - Skia static library: ${SKIA_DIR}/${ANDROID_ABI}/libskia.a")
message(STATUS "  - Quran text data: ${VMF_DIR}/src/qurantext")

# Include directories
target_include_directories(quranrenderer PRIVATE
    ${HARFBUZZ_DIR}/src
    ${VMF_DIR}/src/qurantext
    ${CORE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
)

# Find Android libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)
find_library(GLESv2-lib GLESv2)
find_library(EGL-lib EGL)
find_library(z-lib z)

# Link libraries
# Note: skia must come before GL libs since it depends on them
target_link_libraries(quranrenderer
    ${log-lib}
    ${android-lib}
    ${jnigraphics-lib}
    skia
    ${GLESv2-lib}
    ${EGL-lib}
    ${z-lib}
)
