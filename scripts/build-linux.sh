#!/bin/bash
#
# build-linux.sh
#
# Build quran-renderer shared library (.so) for Linux
#
# Usage:
#   ./scripts/build-linux.sh [OPTIONS]
#
# Options:
#   --deps-dir PATH    Directory containing dependencies (default: ../quran-deps)
#   --output-dir PATH  Output directory for built library (default: build/linux)
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
OUTPUT_DIR="${PROJECT_DIR}/build/linux"
SKIP_DEPS=false
CLEAN_BUILD=false

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

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check for required tools
check_requirements() {
    log_info "Checking requirements..."
    
    for cmd in git cmake ninja python3 curl; do
        if ! command -v $cmd &> /dev/null; then
            log_error "$cmd is required but not installed"
            exit 1
        fi
    done
    
    log_success "All requirements satisfied"
}

# Clone or update repository
clone_or_update() {
    local repo_url="$1"
    local target_dir="$2"
    local branch="${3:-}"
    
    if [[ -d "$target_dir/.git" ]]; then
        log_info "Updating $(basename "$target_dir")..."
        cd "$target_dir"
        git fetch origin
        if [[ -n "$branch" ]]; then
            git checkout "$branch"
            git pull origin "$branch" || true
        fi
    else
        log_info "Cloning $(basename "$target_dir")..."
        if [[ -n "$branch" ]]; then
            git clone -b "$branch" "$repo_url" "$target_dir"
        else
            git clone "$repo_url" "$target_dir"
        fi
    fi
}

# Build HarfBuzz for Linux
build_harfbuzz() {
    log_info "Building HarfBuzz for Linux..."
    
    local HB_DIR="$DEPS_DIR/harfbuzz"
    local HB_BUILD="$DEPS_DIR/harfbuzz-build-linux"
    
    clone_or_update "https://github.com/DigitalKhatt/harfbuzz.git" "$HB_DIR" "justification"
    
    mkdir -p "$HB_BUILD"
    cd "$HB_BUILD"
    
    cmake "$HB_DIR" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -DHB_HAVE_FREETYPE=OFF \
        -DHB_HAVE_GLIB=OFF \
        -DHB_HAVE_ICU=OFF \
        -DHB_HAVE_GOBJECT=OFF \
        -DHB_BUILD_SUBSET=OFF
    
    ninja
    
    log_success "HarfBuzz built successfully"
}

# Build Skia for Linux
build_skia() {
    log_info "Building Skia for Linux..."
    
    local SKIA_DIR="$DEPS_DIR/skia"
    
    if [[ ! -d "$SKIA_DIR" ]]; then
        log_info "Cloning Skia..."
        git clone https://skia.googlesource.com/skia.git "$SKIA_DIR"
    fi
    
    cd "$SKIA_DIR"
    
    # Sync dependencies
    python3 tools/git-sync-deps
    
    # Configure for Linux
    bin/gn gen out/linux-release --args='
        is_official_build=true
        is_component_build=false
        is_debug=false
        target_os="linux"
        target_cpu="x64"
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
        extra_cflags_cc=["-frtti"]
    '
    
    ninja -C out/linux-release
    
    log_success "Skia built successfully"
}

# Download VisualMetaFont for Quran text
setup_visualmetafont() {
    log_info "Setting up VisualMetaFont..."
    
    local VMF_DIR="$DEPS_DIR/visualmetafont"
    clone_or_update "https://github.com/DigitalKhatt/visualmetafont.git" "$VMF_DIR"
    
    log_success "VisualMetaFont ready"
}

# Download fonts
download_fonts() {
    log_info "Downloading fonts..."
    
    local FONTS_DIR="$DEPS_DIR/fonts"
    mkdir -p "$FONTS_DIR"
    
    # Madina Quranic (with tajweed)
    if [[ ! -f "$FONTS_DIR/digitalkhatt.otf" ]]; then
        curl -L -o "$FONTS_DIR/digitalkhatt.otf" \
            "https://github.com/DigitalKhatt/mushaf-android/raw/main/app/src/main/assets/fonts/digitalkhatt.otf"
    fi
    
    log_success "Fonts downloaded"
}

# Build quran-renderer
build_library() {
    log_info "Building quran-renderer for Linux..."
    
    local BUILD_DIR="$OUTPUT_DIR"
    
    if [[ "$CLEAN_BUILD" == true ]] && [[ -d "$BUILD_DIR" ]]; then
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake "$PROJECT_DIR" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON \
        -DHARFBUZZ_INCLUDE_DIR="$DEPS_DIR/harfbuzz/src" \
        -DHARFBUZZ_LIBRARY_DIR="$DEPS_DIR/harfbuzz-build-linux/src" \
        -DSKIA_INCLUDE_DIR="$DEPS_DIR/skia" \
        -DSKIA_LIBRARY_DIR="$DEPS_DIR/skia/out/linux-release" \
        -DQURAN_TEXT_DIR="$DEPS_DIR/visualmetafont/src/qurantext"
    
    ninja
    
    # Copy output
    mkdir -p "$OUTPUT_DIR/lib"
    mkdir -p "$OUTPUT_DIR/include/quran"
    
    cp libquranrenderer.so* "$OUTPUT_DIR/lib/" 2>/dev/null || cp libquranrenderer.so "$OUTPUT_DIR/lib/"
    cp "$PROJECT_DIR/include/quran/renderer.h" "$OUTPUT_DIR/include/quran/"
    
    # Copy font for testing
    mkdir -p "$OUTPUT_DIR/fonts"
    cp "$DEPS_DIR/fonts/digitalkhatt.otf" "$OUTPUT_DIR/fonts/"
    
    log_success "Library built: $OUTPUT_DIR/lib/libquranrenderer.so"
}

# Main
main() {
    log_info "========================================"
    log_info "  Quran Renderer - Linux Build Script  "
    log_info "========================================"
    
    check_requirements
    
    mkdir -p "$DEPS_DIR"
    
    if [[ "$SKIP_DEPS" != true ]]; then
        build_harfbuzz
        build_skia
        setup_visualmetafont
        download_fonts
    fi
    
    build_library
    
    log_success "========================================"
    log_success "  Build Complete!"
    log_success "========================================"
    log_info "Output: $OUTPUT_DIR"
    log_info ""
    log_info "Files:"
    ls -la "$OUTPUT_DIR/lib/"
    log_info ""
    log_info "Usage:"
    log_info "  #include <quran/renderer.h>"
    log_info "  Link with: -lquranrenderer -L$OUTPUT_DIR/lib"
}

main "$@"
