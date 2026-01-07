#!/bin/bash
#
# build-apple.sh
#
# Build quran-renderer for iOS and macOS
# Creates XCFramework containing:
#   - iOS device (arm64)
#   - iOS simulator (arm64) [x86_64 optional]
#   - macOS (arm64) [x86_64 optional]
#
# Also packages (if present):
#   - Android (arm64-v8a, armeabi-v7a) [x86_64 optional]
#
# Usage:
#   ./scripts/build-apple.sh [OPTIONS]
#
# Options:
#   --deps-dir PATH    Directory containing dependencies (default: ../quran-deps)
#   --output-dir PATH  Output directory (default: build/apple)
#   --ios-only         Build only iOS
#   --macos-only       Build only macOS
#   --skip-deps        Skip building dependencies
#   --clean            Clean build directory before building
#   --include-x86_64    Also include x86_64 artifacts (Apple simulator + macOS; Android in release zip)
#   --help             Show this help message
#
# Requirements:
#   - macOS with Xcode installed
#   - Xcode Command Line Tools
#   - CMake, Ninja, Python3
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Default values
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="${PROJECT_DIR}/../quran-deps"
OUTPUT_DIR="${PROJECT_DIR}/build/apple"
IOS_ONLY=false
MACOS_ONLY=false
SKIP_DEPS=false
CLEAN_BUILD=false
INCLUDE_X86_64=false

# Minimum deployment targets
IOS_DEPLOYMENT_TARGET="13.0"
MACOS_DEPLOYMENT_TARGET="11.0"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --deps-dir)
            DEPS_DIR="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --ios-only)
            IOS_ONLY=true
            shift
            ;;
        --macos-only)
            MACOS_ONLY=true
            shift
            ;;
        --skip-deps)
            SKIP_DEPS=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --include-x86_64)
            INCLUDE_X86_64=true
            shift
            ;;
        --help)
            head -30 "$0" | tail -25
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check for macOS
check_platform() {
    if [[ "$(uname)" != "Darwin" ]]; then
        log_error "This script must be run on macOS"
        exit 1
    fi
}

# Check for required tools
check_requirements() {
    log_info "Checking requirements..."
    
    for cmd in git cmake ninja python3 xcodebuild xcrun; do
        if ! command -v $cmd &> /dev/null; then
            log_error "$cmd is required but not installed"
            exit 1
        fi
    done
    
    log_success "All requirements satisfied"
}

# Clone or update repository - returns 0 if updated/cloned, 1 if no changes
clone_or_update() {
    local repo_url="$1"
    local target_dir="$2"
    local branch="${3:-}"
    
    if [[ -d "$target_dir/.git" ]]; then
        cd "$target_dir"
        git fetch origin --quiet
        
        local LOCAL=$(git rev-parse HEAD)
        local REMOTE
        if [[ -n "$branch" ]]; then
            REMOTE=$(git rev-parse origin/$branch 2>/dev/null || git rev-parse FETCH_HEAD)
        else
            REMOTE=$(git rev-parse origin/master 2>/dev/null || git rev-parse origin/main 2>/dev/null || git rev-parse FETCH_HEAD)
        fi
        
        if [[ "$LOCAL" != "$REMOTE" ]]; then
            log_info "$(basename "$target_dir") has updates, pulling..."
            if [[ -n "$branch" ]]; then
                git checkout "$branch"
                git pull origin "$branch" || true
            else
                git pull || true
            fi
            return 0  # Updated
        else
            log_info "$(basename "$target_dir") is up to date"
            return 1  # No changes
        fi
    else
        log_info "Cloning $(basename "$target_dir")..."
        if [[ -n "$branch" ]]; then
            git clone -b "$branch" "$repo_url" "$target_dir"
        else
            git clone "$repo_url" "$target_dir"
        fi
        return 0  # Fresh clone
    fi
}

# Build HarfBuzz for a platform
build_harfbuzz_platform() {
    local PLATFORM=$1  # macos, ios, ios-simulator
    local ARCH=$2      # arm64, x86_64
    
    local HB_DIR="$DEPS_DIR/harfbuzz"
    local HB_BUILD="$DEPS_DIR/harfbuzz-build-$PLATFORM-$ARCH"
    local HB_LIB="$HB_BUILD/libharfbuzz.a"
    
    # Check for updates first
    local HB_UPDATED=false
    if clone_or_update "https://github.com/DigitalKhatt/harfbuzz.git" "$HB_DIR" "justification"; then
        HB_UPDATED=true
    fi
    
    # Check if already built and no updates
    if [[ -f "$HB_LIB" ]] && [[ "$HB_UPDATED" != true ]]; then
        log_success "HarfBuzz for $PLATFORM-$ARCH already built (cached, up to date)"
        return 0
    fi
    
    log_info "Building HarfBuzz for $PLATFORM-$ARCH..."
    
    mkdir -p "$HB_BUILD"
    cd "$HB_BUILD"
    
    local CMAKE_ARGS=(
        -G Ninja
        -DCMAKE_BUILD_TYPE=Release
        -DBUILD_SHARED_LIBS=OFF
        -DHB_HAVE_FREETYPE=OFF
        -DHB_HAVE_GLIB=OFF
        -DHB_HAVE_ICU=OFF
        -DHB_HAVE_GOBJECT=OFF
        -DHB_BUILD_SUBSET=OFF
        -DCMAKE_OSX_ARCHITECTURES=$ARCH
    )
    
    case $PLATFORM in
        macos)
            CMAKE_ARGS+=(-DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOS_DEPLOYMENT_TARGET)
            ;;
        ios)
            CMAKE_ARGS+=(
                -DCMAKE_SYSTEM_NAME=iOS
                -DCMAKE_OSX_DEPLOYMENT_TARGET=$IOS_DEPLOYMENT_TARGET
                -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphoneos --show-sdk-path)
            )
            ;;
        ios-simulator)
            CMAKE_ARGS+=(
                -DCMAKE_SYSTEM_NAME=iOS
                -DCMAKE_OSX_DEPLOYMENT_TARGET=$IOS_DEPLOYMENT_TARGET
                -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphonesimulator --show-sdk-path)
            )
            ;;
    esac
    
    cmake "$HB_DIR" "${CMAKE_ARGS[@]}"
    ninja
    
    log_success "HarfBuzz built for $PLATFORM-$ARCH"
}

# Build Skia for a platform
build_skia_platform() {
    local PLATFORM=$1
    local ARCH=$2
    
    local SKIA_DIR="$DEPS_DIR/skia"
    local OUT_DIR="out/$PLATFORM-$ARCH"
    local SKIA_LIB="$SKIA_DIR/$OUT_DIR/libskia.a"
    local NEEDS_BUILD=false
    
    # Check if Skia repo exists and for updates
    if [[ -d "$SKIA_DIR/.git" ]]; then
        cd "$SKIA_DIR"
        git fetch origin --quiet
        local LOCAL=$(git rev-parse HEAD)
        local REMOTE=$(git rev-parse origin/main 2>/dev/null || git rev-parse FETCH_HEAD)
        
        if [[ "$LOCAL" != "$REMOTE" ]]; then
            log_info "Skia has updates available, pulling..."
            git pull origin main
            NEEDS_BUILD=true
        elif [[ -f "$SKIA_LIB" ]]; then
            log_success "Skia for $PLATFORM-$ARCH already built (cached, up to date)"
            return 0
        else
            NEEDS_BUILD=true
        fi
    else
        log_info "Cloning Skia..."
        git clone https://skia.googlesource.com/skia.git "$SKIA_DIR"
        NEEDS_BUILD=true
    fi
    
    if [[ "$NEEDS_BUILD" != true ]] && [[ -f "$SKIA_LIB" ]]; then
        log_success "Skia for $PLATFORM-$ARCH already built (cached)"
        return 0
    fi
    
    log_info "Building Skia for $PLATFORM-$ARCH..."
    cd "$SKIA_DIR"
    
    # Only sync deps if not already synced
    if [[ ! -d "$SKIA_DIR/third_party/externals/freetype" ]]; then
        log_info "Syncing Skia dependencies (first time only)..."
        python3 tools/git-sync-deps
    else
        log_success "Skia dependencies already synced (cached)"
    fi
    
    local TARGET_OS=""
    local EXTRA_ARGS=""
    
    case $PLATFORM in
        macos)
            TARGET_OS="mac"
            EXTRA_ARGS="extra_cflags=[\"-mmacosx-version-min=$MACOS_DEPLOYMENT_TARGET\"]"
            ;;
        ios)
            TARGET_OS="ios"
            EXTRA_ARGS="ios_min_target=\"$IOS_DEPLOYMENT_TARGET\""
            ;;
        ios-simulator)
            TARGET_OS="ios"
            EXTRA_ARGS="ios_min_target=\"$IOS_DEPLOYMENT_TARGET\" ios_use_simulator=true"
            ;;
    esac
    
    bin/gn gen "$OUT_DIR" --args="
        is_official_build=true
        is_component_build=false
        is_debug=false
        target_os=\"$TARGET_OS\"
        target_cpu=\"$ARCH\"
        skia_use_system_expat=false
        skia_use_system_freetype2=false
        skia_use_system_harfbuzz=false
        skia_use_system_icu=false
        skia_use_system_libjpeg_turbo=false
        skia_use_system_libpng=false
        skia_use_system_libwebp=false
        skia_use_system_zlib=false
        skia_enable_gpu=false
        skia_enable_skottie=false
        skia_enable_pdf=false
        skia_enable_skshaper=false
        skia_enable_svg=false
        skia_use_gl=false
        skia_use_vulkan=false
        skia_use_metal=false
        skia_use_dawn=false
        skia_use_dng_sdk=false
        skia_use_piex=false
        skia_use_sfntly=false
        skia_use_libjpeg_turbo_decode=false
        skia_use_libjpeg_turbo_encode=false
        skia_use_wuffs=true
        extra_cflags_cc=[\"-frtti\"]
        $EXTRA_ARGS
    "
    
    ninja -C "$OUT_DIR" :skia
    
    log_success "Skia built for $PLATFORM-$ARCH"
}

# Setup VisualMetaFont
setup_visualmetafont() {
    local VMF_DIR="$DEPS_DIR/visualmetafont"
    
    # Check if already cloned and for updates
    if [[ -d "$VMF_DIR/.git" ]]; then
        cd "$VMF_DIR"
        git fetch origin --quiet
        local LOCAL=$(git rev-parse HEAD)
        local REMOTE=$(git rev-parse origin/master 2>/dev/null || git rev-parse origin/main 2>/dev/null || git rev-parse FETCH_HEAD)
        
        if [[ "$LOCAL" != "$REMOTE" ]]; then
            log_info "VisualMetaFont has updates, pulling..."
            git pull
            log_success "VisualMetaFont updated"
        else
            log_success "VisualMetaFont already set up (cached, up to date)"
        fi
        return 0
    fi
    
    log_info "Setting up VisualMetaFont..."
    clone_or_update "https://github.com/DigitalKhatt/visualmetafont.git" "$VMF_DIR"
    log_success "VisualMetaFont ready"
}

# Download fonts
download_fonts() {
    local FONTS_DIR="$DEPS_DIR/fonts"
    local VMF_FONT="$DEPS_DIR/visualmetafont/examples/testvarfont/digitalkhatt-cff2.otf"
    
    # Check if already downloaded
    if [[ -f "$FONTS_DIR/digitalkhatt.otf" ]]; then
        log_success "Fonts already downloaded (cached)"
        return 0
    fi
    
    log_info "Setting up fonts..."
    mkdir -p "$FONTS_DIR"
    
    # Prefer the CFF2 variable font with embedded tajweed (172 GPOS lookups)
    # from visualmetafont if available, otherwise download the older version
    if [[ -f "$VMF_FONT" ]]; then
        cp "$VMF_FONT" "$FONTS_DIR/digitalkhatt.otf"
        log_success "Using CFF2 font with embedded tajweed from visualmetafont"
    else
        log_info "CFF2 font not found, downloading standard font..."
        curl -L -o "$FONTS_DIR/digitalkhatt.otf" \
            "https://github.com/DigitalKhatt/mushaf-android/raw/main/app/src/main/assets/fonts/digitalkhatt.otf"
        log_success "Fonts downloaded"
    fi
}

# Build quran-renderer for a platform
build_library_platform() {
    local PLATFORM=$1
    local ARCH=$2
    
    log_info "Building quran-renderer for $PLATFORM-$ARCH..."
    
    local BUILD_DIR="$OUTPUT_DIR/build-$PLATFORM-$ARCH"
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    local CMAKE_ARGS=(
        -G Ninja
        -DCMAKE_BUILD_TYPE=Release
        -DBUILD_SHARED_LIBS=OFF
        -DHARFBUZZ_INCLUDE_DIR="$DEPS_DIR/harfbuzz/src"
        -DHARFBUZZ_LIBRARY_DIR="$DEPS_DIR/harfbuzz-build-$PLATFORM-$ARCH"
        -DSKIA_INCLUDE_DIR="$DEPS_DIR/skia"
        -DSKIA_LIBRARY_DIR="$DEPS_DIR/skia/out/$PLATFORM-$ARCH"
        -DQURAN_TEXT_DIR="$DEPS_DIR/visualmetafont/src/qurantext"
        -DCMAKE_OSX_ARCHITECTURES=$ARCH
    )
    
    case $PLATFORM in
        macos)
            CMAKE_ARGS+=(-DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOS_DEPLOYMENT_TARGET)
            ;;
        ios)
            CMAKE_ARGS+=(
                -DCMAKE_SYSTEM_NAME=iOS
                -DCMAKE_OSX_DEPLOYMENT_TARGET=$IOS_DEPLOYMENT_TARGET
                -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphoneos --show-sdk-path)
            )
            ;;
        ios-simulator)
            CMAKE_ARGS+=(
                -DCMAKE_SYSTEM_NAME=iOS
                -DCMAKE_OSX_DEPLOYMENT_TARGET=$IOS_DEPLOYMENT_TARGET
                -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphonesimulator --show-sdk-path)
            )
            ;;
    esac
    
    cmake "$PROJECT_DIR" "${CMAKE_ARGS[@]}"
    ninja
    
    # Merge all static libraries (quran_renderer + HarfBuzz + Skia) into one
    # This is critical - without this, the output .a only contains quran_renderer
    # and has undefined symbols for HarfBuzz/Skia
    log_info "Merging static libraries for $PLATFORM-$ARCH..."
    
    local HARFBUZZ_LIB="$DEPS_DIR/harfbuzz-build-$PLATFORM-$ARCH/libharfbuzz.a"
    local SKIA_LIB="$DEPS_DIR/skia/out/$PLATFORM-$ARCH/libskia.a"
    local RENDERER_LIB="$BUILD_DIR/libquran_renderer.a"
    local MERGED_LIB="$BUILD_DIR/libquranrenderer_merged.a"
    
    # CMake outputs libquran_renderer.a (with underscore) - rename to final name
    if [[ -f "$BUILD_DIR/libquranrenderer.a" ]]; then
        RENDERER_LIB="$BUILD_DIR/libquranrenderer.a"
    fi
    
    # Use libtool to merge static libraries (preserves all object files)
    libtool -static -o "$MERGED_LIB" "$RENDERER_LIB" "$HARFBUZZ_LIB" "$SKIA_LIB"
    
    # Replace the original with merged version
    mv "$MERGED_LIB" "$BUILD_DIR/libquranrenderer.a"
    
    log_info "Merged library size: $(du -h "$BUILD_DIR/libquranrenderer.a" | cut -f1)"
    
    # Copy individual arch build to platform directory for packaging
    if [[ "$PLATFORM" == "macos" ]]; then
        mkdir -p "$OUTPUT_DIR/macos/lib"
        cp libquranrenderer.a "$OUTPUT_DIR/macos/lib/" 2>/dev/null || true
    fi
    
    log_success "Built quran-renderer for $PLATFORM-$ARCH"
}

# Create fat library from multiple architectures
create_fat_library() {
    local PLATFORM=$1
    shift
    local ARCHS=("$@")
    
    log_info "Creating fat library for $PLATFORM..."
    
    local FAT_DIR="$OUTPUT_DIR/$PLATFORM"
    mkdir -p "$FAT_DIR/lib"
    
    local LIPO_ARGS=()
    for ARCH in "${ARCHS[@]}"; do
        LIPO_ARGS+=("$OUTPUT_DIR/build-$PLATFORM-$ARCH/libquranrenderer.a")
    done
    
    lipo -create "${LIPO_ARGS[@]}" -output "$FAT_DIR/lib/libquranrenderer.a"
    
    log_success "Fat library created: $FAT_DIR/lib/libquranrenderer.a"
}

# Create XCFramework
create_xcframework() {
    log_info "Creating XCFramework..."
    
    local XCFRAMEWORK_DIR="$OUTPUT_DIR/QuranRenderer.xcframework"
    
    rm -rf "$XCFRAMEWORK_DIR"
    
    local FRAMEWORK_ARGS=()
    
    # iOS device
    if [[ -f "$OUTPUT_DIR/ios/lib/libquranrenderer.a" ]]; then
        FRAMEWORK_ARGS+=(-library "$OUTPUT_DIR/ios/lib/libquranrenderer.a")
        FRAMEWORK_ARGS+=(-headers "$PROJECT_DIR/include")
    fi
    
    # iOS simulator
    if [[ -f "$OUTPUT_DIR/ios-simulator/lib/libquranrenderer.a" ]]; then
        FRAMEWORK_ARGS+=(-library "$OUTPUT_DIR/ios-simulator/lib/libquranrenderer.a")
        FRAMEWORK_ARGS+=(-headers "$PROJECT_DIR/include")
    fi
    
    # macOS
    if [[ -f "$OUTPUT_DIR/macos/lib/libquranrenderer.a" ]]; then
        FRAMEWORK_ARGS+=(-library "$OUTPUT_DIR/macos/lib/libquranrenderer.a")
        FRAMEWORK_ARGS+=(-headers "$PROJECT_DIR/include")
    fi
    
    xcodebuild -create-xcframework "${FRAMEWORK_ARGS[@]}" -output "$XCFRAMEWORK_DIR"
    
    log_success "XCFramework created: $XCFRAMEWORK_DIR"
}

# Build all dependencies
build_all_deps() {
    setup_visualmetafont
    download_fonts
    
    if [[ "$MACOS_ONLY" != true ]]; then
        # iOS
        build_harfbuzz_platform ios arm64
        build_skia_platform ios arm64
        
        # iOS Simulator
        build_harfbuzz_platform ios-simulator arm64
        build_skia_platform ios-simulator arm64
        if [[ "$INCLUDE_X86_64" == true ]]; then
            build_harfbuzz_platform ios-simulator x86_64
            build_skia_platform ios-simulator x86_64
        fi
    fi
    
    if [[ "$IOS_ONLY" != true ]]; then
        # macOS
        build_harfbuzz_platform macos arm64
        build_skia_platform macos arm64
        if [[ "$INCLUDE_X86_64" == true ]]; then
            build_harfbuzz_platform macos x86_64
            build_skia_platform macos x86_64
        fi
    fi
}

# Build all libraries
build_all_libraries() {
    if [[ "$MACOS_ONLY" != true ]]; then
        # iOS device
        build_library_platform ios arm64
        create_fat_library ios arm64
        
        # iOS simulator
        build_library_platform ios-simulator arm64
        if [[ "$INCLUDE_X86_64" == true ]]; then
            build_library_platform ios-simulator x86_64
            create_fat_library ios-simulator arm64 x86_64
        else
            create_fat_library ios-simulator arm64
        fi
    fi
    
    if [[ "$IOS_ONLY" != true ]]; then
        # macOS
        build_library_platform macos arm64
        if [[ "$INCLUDE_X86_64" == true ]]; then
            build_library_platform macos x86_64
            create_fat_library macos arm64 x86_64
        else
            create_fat_library macos arm64
        fi
    fi
    
    create_xcframework
}

# Build macOS dynamic library (dylib)
build_macos_dylib() {
    log_info "Building macOS dylib..."
    
    mkdir -p "$OUTPUT_DIR/macos-dylib"
    
    # Build arm64 dylib
    local ARM64_BUILD_DIR="$OUTPUT_DIR/build-macos-dylib-arm64"
    mkdir -p "$ARM64_BUILD_DIR"
    cd "$ARM64_BUILD_DIR"
    
    cmake "$PROJECT_DIR" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON \
        -DHARFBUZZ_INCLUDE_DIR="$DEPS_DIR/harfbuzz/src" \
        -DHARFBUZZ_LIBRARY_DIR="$DEPS_DIR/harfbuzz-build-macos-arm64" \
        -DSKIA_INCLUDE_DIR="$DEPS_DIR/skia" \
        -DSKIA_LIBRARY_DIR="$DEPS_DIR/skia/out/macos-arm64" \
        -DQURAN_TEXT_DIR="$DEPS_DIR/visualmetafont/src/qurantext" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOS_DEPLOYMENT_TARGET \
        -DCMAKE_OSX_ARCHITECTURES=arm64
    
    ninja
    
    if [[ "$INCLUDE_X86_64" == true ]]; then
        # Build x86_64 dylib
        local X64_BUILD_DIR="$OUTPUT_DIR/build-macos-dylib-x86_64"
        mkdir -p "$X64_BUILD_DIR"
        cd "$X64_BUILD_DIR"
        
        cmake "$PROJECT_DIR" \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=ON \
            -DHARFBUZZ_INCLUDE_DIR="$DEPS_DIR/harfbuzz/src" \
            -DHARFBUZZ_LIBRARY_DIR="$DEPS_DIR/harfbuzz-build-macos-x86_64" \
            -DSKIA_INCLUDE_DIR="$DEPS_DIR/skia" \
            -DSKIA_LIBRARY_DIR="$DEPS_DIR/skia/out/macos-x86_64" \
            -DQURAN_TEXT_DIR="$DEPS_DIR/visualmetafont/src/qurantext" \
            -DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOS_DEPLOYMENT_TARGET \
            -DCMAKE_OSX_ARCHITECTURES=x86_64
        
        ninja
        
        # Create universal dylib using lipo
        log_info "Creating universal dylib..."
        lipo -create \
            "$ARM64_BUILD_DIR/libquranrenderer.dylib" \
            "$X64_BUILD_DIR/libquranrenderer.dylib" \
            -output "$OUTPUT_DIR/macos-dylib/libquranrenderer.dylib"
    else
        log_info "Creating arm64 dylib (x86_64 excluded)..."
        cp "$ARM64_BUILD_DIR/libquranrenderer.dylib" "$OUTPUT_DIR/macos-dylib/libquranrenderer.dylib"
    fi
    
    # Strip debug symbols for release
    log_info "Stripping debug symbols..."
    strip -x "$OUTPUT_DIR/macos-dylib/libquranrenderer.dylib"
    
    # Code sign the dylib (ad-hoc signing for distribution)
    log_info "Code signing dylib..."
    codesign --force --sign - --timestamp=none "$OUTPUT_DIR/macos-dylib/libquranrenderer.dylib"
    
    # Copy header
    mkdir -p "$OUTPUT_DIR/macos-dylib/include"
    cp -r "$PROJECT_DIR/include/quran" "$OUTPUT_DIR/macos-dylib/include/"
    
    log_success "macOS dylib created: $OUTPUT_DIR/macos-dylib/libquranrenderer.dylib"
    log_info "  Architectures: $(lipo -info "$OUTPUT_DIR/macos-dylib/libquranrenderer.dylib")"
    log_info "  Code signature: $(codesign -dv "$OUTPUT_DIR/macos-dylib/libquranrenderer.dylib" 2>&1 | grep Signature)"
}

# Main
main() {
    log_info "==========================================="
    log_info "  Quran Renderer - Apple Build Script     "
    log_info "==========================================="
    
    check_platform
    check_requirements
    
    mkdir -p "$DEPS_DIR"
    mkdir -p "$OUTPUT_DIR"
    
    if [[ "$CLEAN_BUILD" == true ]]; then
        rm -rf "$OUTPUT_DIR"
        mkdir -p "$OUTPUT_DIR"
    fi
    
    if [[ "$SKIP_DEPS" != true ]]; then
        build_all_deps
    fi
    
    build_all_libraries
    
    # Build macOS dylib (in addition to static libraries)
    if [[ "$IOS_ONLY" != true ]]; then
        build_macos_dylib
    fi
    
    # Copy fonts
    mkdir -p "$OUTPUT_DIR/fonts"
    cp "$DEPS_DIR/fonts/digitalkhatt.otf" "$OUTPUT_DIR/fonts/"
    
    log_success "==========================================="
    log_success "  Build Complete!"
    log_success "==========================================="
    log_info ""
    log_info "Output: $OUTPUT_DIR"
    log_info ""
    log_info "XCFramework: $OUTPUT_DIR/QuranRenderer.xcframework"
    log_info "macOS dylib: $OUTPUT_DIR/macos-dylib/"
    log_info ""
    log_info "Usage in Xcode:"
    log_info "  1. Drag QuranRenderer.xcframework to your project"
    log_info "  2. Add to 'Frameworks, Libraries, and Embedded Content'"
    log_info "  3. #include <quran/renderer.h>"
    log_info ""
    if [[ -d "$OUTPUT_DIR/QuranRenderer.xcframework" ]]; then
        log_info "XCFramework contents:"
        ls -la "$OUTPUT_DIR/QuranRenderer.xcframework/"
    fi
    
    # Create release zip (now includes macos-dylib and android if available)
    local ZIP_PATH="${PROJECT_DIR}/build/quran-renderer-release.zip"
    log_info ""
    log_info "Creating release zip..."
    cd "$OUTPUT_DIR"
    rm -f "$ZIP_PATH"
    
    # Start with Apple artifacts
    local ZIP_CONTENTS="QuranRenderer.xcframework ios ios-simulator macos macos-dylib fonts"
    
    # Add Android libs if they exist
    if [[ -d "${PROJECT_DIR}/build/android" ]]; then
        log_info "Including Android libraries in release..."
        rm -rf "$OUTPUT_DIR/android"
        mkdir -p "$OUTPUT_DIR/android"

        # Always include device ABIs
        if [[ -d "${PROJECT_DIR}/build/android/armeabi-v7a" ]]; then
            cp -R "${PROJECT_DIR}/build/android/armeabi-v7a" "$OUTPUT_DIR/android/"
        fi
        if [[ -d "${PROJECT_DIR}/build/android/arm64-v8a" ]]; then
            cp -R "${PROJECT_DIR}/build/android/arm64-v8a" "$OUTPUT_DIR/android/"
        fi

        # x86_64 is opt-in
        if [[ "$INCLUDE_X86_64" == true ]] && [[ -d "${PROJECT_DIR}/build/android/x86_64" ]]; then
            cp -R "${PROJECT_DIR}/build/android/x86_64" "$OUTPUT_DIR/android/"
        else
            log_info "Android x86_64 excluded (use --include-x86_64 to include)"
        fi

        ZIP_CONTENTS="$ZIP_CONTENTS android"
    fi
    
    zip -r "$ZIP_PATH" $ZIP_CONTENTS
    log_success "Release zip: $ZIP_PATH"
}

main "$@"
