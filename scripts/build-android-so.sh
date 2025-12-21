#!/bin/bash
#
# build-android-so.sh
#
# Build quran-renderer shared library (.so) for Android
# This builds a standalone .so file (not AAR) for use with Flutter FFI, etc.
#
# Usage:
#   ./scripts/build-android-so.sh [OPTIONS]
#
# Options:
#   --abi ABI          Target ABI: arm64-v8a, armeabi-v7a, x86_64 (required)
#   --deps-dir PATH    Directory containing dependencies (default: ../quran-deps)
#   --output-dir PATH  Output directory (default: build/android/<abi>)
#   --ndk-path PATH    Path to Android NDK (default: $ANDROID_NDK_HOME)
#   --skip-deps        Skip building dependencies
#   --clean            Clean build directory before building
#   --help             Show this help message
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
TARGET_ABI=""
NDK_PATH="${ANDROID_NDK_HOME:-${ANDROID_HOME}/ndk/28.0.12674087}"
SKIP_DEPS=false
CLEAN_BUILD=false
API_LEVEL=24

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --abi)
            TARGET_ABI="$2"
            shift 2
            ;;
        --deps-dir)
            DEPS_DIR="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --ndk-path)
            NDK_PATH="$2"
            shift 2
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
            head -20 "$0" | tail -15
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Set output dir based on ABI if not specified
OUTPUT_DIR="${OUTPUT_DIR:-${PROJECT_DIR}/build/android/${TARGET_ABI}}"

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Validate ABI
validate_abi() {
    case "$TARGET_ABI" in
        arm64-v8a|armeabi-v7a|x86_64|x86)
            log_info "Target ABI: $TARGET_ABI"
            ;;
        "")
            log_error "ABI is required. Use --abi <arm64-v8a|armeabi-v7a|x86_64>"
            exit 1
            ;;
        *)
            log_error "Invalid ABI: $TARGET_ABI"
            log_error "Valid ABIs: arm64-v8a, armeabi-v7a, x86_64, x86"
            exit 1
            ;;
    esac
}

# Check requirements
check_requirements() {
    log_info "Checking requirements..."
    
    for cmd in git cmake ninja python3; do
        if ! command -v $cmd &> /dev/null; then
            log_error "$cmd is required but not installed"
            exit 1
        fi
    done
    
    if [[ ! -d "$NDK_PATH" ]]; then
        log_error "Android NDK not found at: $NDK_PATH"
        log_error "Set ANDROID_NDK_HOME or use --ndk-path"
        exit 1
    fi
    
    log_success "All requirements satisfied"
    log_info "Using NDK: $NDK_PATH"
}

# Get architecture-specific settings
get_arch_settings() {
    case "$TARGET_ABI" in
        arm64-v8a)
            ARCH="aarch64"
            SKIA_CPU="arm64"
            ;;
        armeabi-v7a)
            ARCH="armv7a"
            SKIA_CPU="arm"
            ;;
        x86_64)
            ARCH="x86_64"
            SKIA_CPU="x64"
            ;;
        x86)
            ARCH="i686"
            SKIA_CPU="x86"
            ;;
    esac
}

# Clone or update repository
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
            REMOTE=$(git rev-parse origin/$branch 2>/dev/null)
        else
            REMOTE=$(git rev-parse FETCH_HEAD 2>/dev/null)
        fi
        
        if [[ "$LOCAL" == "$REMOTE" ]]; then
            log_success "$(basename "$target_dir") is up to date"
            return 1
        fi
        
        log_info "Updating $(basename "$target_dir")..."
        if [[ -n "$branch" ]]; then
            git checkout "$branch"
            git pull origin "$branch"
        else
            git pull
        fi
        return 0
    else
        log_info "Cloning $(basename "$target_dir")..."
        if [[ -n "$branch" ]]; then
            git clone -b "$branch" "$repo_url" "$target_dir"
        else
            git clone "$repo_url" "$target_dir"
        fi
        return 0
    fi
}

# Build HarfBuzz for Android
build_harfbuzz() {
    local HB_DIR="$DEPS_DIR/harfbuzz"
    local HB_BUILD="$DEPS_DIR/harfbuzz-build-android-${TARGET_ABI}"
    local HB_LIB="$HB_BUILD/libharfbuzz.a"
    
    clone_or_update "https://github.com/DigitalKhatt/harfbuzz.git" "$HB_DIR" "justification" || true
    
    if [[ -f "$HB_LIB" && "$SKIP_DEPS" == "true" ]]; then
        log_success "HarfBuzz already built for $TARGET_ABI (cached)"
        return 0
    fi
    
    log_info "Building HarfBuzz for Android ($TARGET_ABI)..."
    
    mkdir -p "$HB_BUILD"
    cd "$HB_BUILD"
    
    cmake "$HB_DIR" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$TARGET_ABI" \
        -DANDROID_PLATFORM=android-$API_LEVEL \
        -DANDROID_STL=c++_static \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DHB_HAVE_FREETYPE=OFF \
        -DHB_HAVE_GLIB=OFF \
        -DHB_HAVE_ICU=OFF \
        -DHB_HAVE_GOBJECT=OFF \
        -DHB_BUILD_SUBSET=OFF
    
    ninja
    
    log_success "HarfBuzz built successfully for $TARGET_ABI"
}

# Build Skia for Android
build_skia() {
    local SKIA_DIR="$DEPS_DIR/skia"
    local SKIA_OUT="$SKIA_DIR/out/android-${TARGET_ABI}-release"
    local SKIA_LIB="$SKIA_OUT/libskia.a"
    
    if [[ ! -d "$SKIA_DIR" ]]; then
        log_info "Cloning Skia..."
        cd "$DEPS_DIR"
        git clone https://skia.googlesource.com/skia.git
        cd "$SKIA_DIR"
        python3 tools/git-sync-deps
    fi
    
    if [[ -f "$SKIA_LIB" && "$SKIP_DEPS" == "true" ]]; then
        log_success "Skia already built for $TARGET_ABI (cached)"
        return 0
    fi
    
    log_info "Building Skia for Android ($TARGET_ABI)..."
    
    cd "$SKIA_DIR"
    
    # Sync dependencies if needed
    if [[ ! -d "third_party/externals" ]]; then
        python3 tools/git-sync-deps
    fi
    
    bin/gn gen "$SKIA_OUT" --args="
        target_os=\"android\"
        target_cpu=\"$SKIA_CPU\"
        ndk=\"$NDK_PATH\"
        ndk_api=$API_LEVEL
        is_official_build=true
        is_component_build=false
        is_debug=false
        skia_use_system_harfbuzz=false
        skia_use_system_freetype2=false
        skia_use_system_libpng=false
        skia_use_system_zlib=false
        skia_use_system_expat=false
        skia_use_system_libjpeg_turbo=false
        skia_use_system_libwebp=false
        skia_use_libwebp_decode=false
        skia_use_libwebp_encode=false
        skia_use_libjpeg_turbo_decode=false
        skia_use_libjpeg_turbo_encode=false
        skia_use_xps=false
        skia_use_dng_sdk=false
        skia_use_piex=false
        skia_use_expat=false
        skia_use_icu=false
        skia_use_gl=true
        skia_use_vulkan=false
        skia_enable_gpu=true
        skia_enable_pdf=false
        skia_enable_skottie=false
        skia_enable_tools=false
        skia_enable_skshaper=false
        cc=\"$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin/clang\"
        cxx=\"$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++\"
    "
    
    ninja -C "$SKIA_OUT" :skia
    
    log_success "Skia built successfully for $TARGET_ABI"
}

# Clone VisualMetaFont for quran.cpp
setup_vmf() {
    local VMF_DIR="$DEPS_DIR/visualmetafont"
    clone_or_update "https://github.com/nicethings/visualmetafont.git" "$VMF_DIR" || true
}

# Build quran-renderer
build_quran_renderer() {
    log_info "Building quran-renderer for Android ($TARGET_ABI)..."
    
    local HB_BUILD="$DEPS_DIR/harfbuzz-build-android-${TARGET_ABI}"
    local SKIA_DIR="$DEPS_DIR/skia"
    local SKIA_OUT="$SKIA_DIR/out/android-${TARGET_ABI}-release"
    local VMF_DIR="$DEPS_DIR/visualmetafont"
    local BUILD_DIR="$PROJECT_DIR/build/android-build-${TARGET_ABI}"
    
    if [[ "$CLEAN_BUILD" == "true" && -d "$BUILD_DIR" ]]; then
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    mkdir -p "$OUTPUT_DIR"
    cd "$BUILD_DIR"
    
    cmake "$PROJECT_DIR" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$TARGET_ABI" \
        -DANDROID_PLATFORM=android-$API_LEVEL \
        -DANDROID_STL=c++_static \
        -DBUILD_SHARED_LIBS=ON \
        -DHARFBUZZ_INCLUDE_DIR="$DEPS_DIR/harfbuzz/src" \
        -DHARFBUZZ_LIBRARY_DIR="$HB_BUILD" \
        -DSKIA_INCLUDE_DIR="$SKIA_DIR" \
        -DSKIA_LIBRARY_DIR="$SKIA_OUT" \
        -DQURAN_TEXT_DIR="$VMF_DIR/src/qurantext"
    
    ninja
    
    # Copy output
    cp libquranrenderer.so "$OUTPUT_DIR/"
    
    # Strip debug symbols for smaller size
    "$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip" "$OUTPUT_DIR/libquranrenderer.so"
    
    local SIZE=$(du -h "$OUTPUT_DIR/libquranrenderer.so" | cut -f1)
    log_success "Built: $OUTPUT_DIR/libquranrenderer.so ($SIZE)"
}

# Main
main() {
    validate_abi
    get_arch_settings
    check_requirements
    
    mkdir -p "$DEPS_DIR"
    
    if [[ "$SKIP_DEPS" != "true" ]]; then
        build_harfbuzz
        build_skia
        setup_vmf
    fi
    
    build_quran_renderer
    
    log_success "Android build complete for $TARGET_ABI!"
    log_info "Output: $OUTPUT_DIR/libquranrenderer.so"
}

main
