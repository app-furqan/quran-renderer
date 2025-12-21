#!/bin/bash
#
# build-apple.sh
#
# Build quran-renderer for iOS and macOS
# Creates XCFramework containing:
#   - iOS device (arm64)
#   - iOS simulator (arm64, x86_64)
#   - macOS (arm64, x86_64)
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
        skia_use_wuffs=true
        extra_cflags_cc=[\"-frtti\"]
        $EXTRA_ARGS
    "
    
    ninja -C "$OUT_DIR"
    
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
    
    # Check if already downloaded
    if [[ -f "$FONTS_DIR/digitalkhatt.otf" ]]; then
        log_success "Fonts already downloaded (cached)"
        return 0
    fi
    
    log_info "Downloading fonts..."
    mkdir -p "$FONTS_DIR"
    
    curl -L -o "$FONTS_DIR/digitalkhatt.otf" \
        "https://github.com/DigitalKhatt/mushaf-android/raw/main/app/src/main/assets/fonts/digitalkhatt.otf"
    
    log_success "Fonts downloaded"
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
        -DHARFBUZZ_LIBRARY_DIR="$DEPS_DIR/harfbuzz-build-$PLATFORM-$ARCH/src"
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
    
    log_success "Built quran-renderer for $PLATFORM-$ARCH"
}

# Create fat library from multiple architectures
create_fat_library() {
    local PLATFORM=$1
    shift
    local ARCHS=("$@")
    
    log_info "Creating fat library for $PLATFORM..."
    
    local FAT_DIR="$OUTPUT_DIR/$PLATFORM"
    mkdir -p "$FAT_DIR"
    
    local LIPO_ARGS=()
    for ARCH in "${ARCHS[@]}"; do
        LIPO_ARGS+=("$OUTPUT_DIR/build-$PLATFORM-$ARCH/libquranrenderer.a")
    done
    
    lipo -create "${LIPO_ARGS[@]}" -output "$FAT_DIR/libquranrenderer.a"
    
    log_success "Fat library created: $FAT_DIR/libquranrenderer.a"
}

# Create XCFramework
create_xcframework() {
    log_info "Creating XCFramework..."
    
    local XCFRAMEWORK_DIR="$OUTPUT_DIR/QuranRenderer.xcframework"
    
    rm -rf "$XCFRAMEWORK_DIR"
    
    local FRAMEWORK_ARGS=()
    
    # iOS device
    if [[ -f "$OUTPUT_DIR/ios/libquranrenderer.a" ]]; then
        FRAMEWORK_ARGS+=(-library "$OUTPUT_DIR/ios/libquranrenderer.a")
        FRAMEWORK_ARGS+=(-headers "$PROJECT_DIR/include")
    fi
    
    # iOS simulator
    if [[ -f "$OUTPUT_DIR/ios-simulator/libquranrenderer.a" ]]; then
        FRAMEWORK_ARGS+=(-library "$OUTPUT_DIR/ios-simulator/libquranrenderer.a")
        FRAMEWORK_ARGS+=(-headers "$PROJECT_DIR/include")
    fi
    
    # macOS
    if [[ -f "$OUTPUT_DIR/macos/libquranrenderer.a" ]]; then
        FRAMEWORK_ARGS+=(-library "$OUTPUT_DIR/macos/libquranrenderer.a")
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
        build_harfbuzz_platform ios-simulator x86_64
        build_skia_platform ios-simulator arm64
        build_skia_platform ios-simulator x86_64
    fi
    
    if [[ "$IOS_ONLY" != true ]]; then
        # macOS
        build_harfbuzz_platform macos arm64
        build_harfbuzz_platform macos x86_64
        build_skia_platform macos arm64
        build_skia_platform macos x86_64
    fi
}

# Build all libraries
build_all_libraries() {
    if [[ "$MACOS_ONLY" != true ]]; then
        # iOS device
        build_library_platform ios arm64
        create_fat_library ios arm64
        
        # iOS simulator (universal)
        build_library_platform ios-simulator arm64
        build_library_platform ios-simulator x86_64
        create_fat_library ios-simulator arm64 x86_64
    fi
    
    if [[ "$IOS_ONLY" != true ]]; then
        # macOS (universal)
        build_library_platform macos arm64
        build_library_platform macos x86_64
        create_fat_library macos arm64 x86_64
    fi
    
    create_xcframework
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
}

main "$@"
