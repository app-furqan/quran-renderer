#!/bin/bash
#
# build-android-skia.sh
#
# Build Skia for Android with identical configuration to iOS
# This ensures consistent rendering across platforms
#
# Usage:
#   ./scripts/build-android-skia.sh [ABI]
#
# ABI options: armeabi-v7a, arm64-v8a, x86_64, all (default)
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="${PROJECT_DIR}/../quran-deps"
SKIA_DIR="$DEPS_DIR/skia"

# Target ABI (default: all)
TARGET_ABI="${1:-all}"

# Android NDK detection
if [[ -z "$ANDROID_NDK_HOME" ]] && [[ -z "$ANDROID_NDK" ]]; then
    log_error "ANDROID_NDK_HOME or ANDROID_NDK must be set"
    log_info "Example: export ANDROID_NDK_HOME=\$HOME/Android/Sdk/ndk/25.2.9519653"
    exit 1
fi

NDK_PATH="${ANDROID_NDK_HOME:-$ANDROID_NDK}"
log_info "Using Android NDK: $NDK_PATH"

# Minimum Android API level
ANDROID_API_LEVEL=21

# Clone or update Skia
clone_or_update_skia() {
    if [[ -d "$SKIA_DIR/.git" ]]; then
        log_info "Skia repository exists, checking for updates..."
        cd "$SKIA_DIR"
        git fetch origin --quiet
        local LOCAL=$(git rev-parse HEAD)
        local REMOTE=$(git rev-parse origin/main 2>/dev/null || git rev-parse FETCH_HEAD)
        
        if [[ "$LOCAL" != "$REMOTE" ]]; then
            log_info "Skia has updates, pulling..."
            git pull origin main
        else
            log_success "Skia is up to date"
        fi
    else
        log_info "Cloning Skia..."
        git clone https://skia.googlesource.com/skia.git "$SKIA_DIR"
        cd "$SKIA_DIR"
    fi
    
    # Sync dependencies (first time or if outdated)
    if [[ ! -d "$SKIA_DIR/third_party/externals/freetype" ]]; then
        log_info "Syncing Skia dependencies (this may take a while)..."
        python3 tools/git-sync-deps
    else
        log_success "Skia dependencies already synced"
    fi
}

# Build Skia for a specific Android ABI
build_skia_android() {
    local ABI=$1
    local OUT_DIR="out/android-$ABI"
    local SKIA_LIB="$SKIA_DIR/$OUT_DIR/libskia.a"
    
    # Check if already built
    if [[ -f "$SKIA_LIB" ]]; then
        log_warn "Skia for $ABI already exists. Delete $OUT_DIR to rebuild."
        return 0
    fi
    
    log_info "Building Skia for Android $ABI..."
    cd "$SKIA_DIR"
    
    # Map ABI to target_cpu
    local TARGET_CPU=""
    case $ABI in
        armeabi-v7a)
            TARGET_CPU="arm"
            ;;
        arm64-v8a)
            TARGET_CPU="arm64"
            ;;
        x86_64)
            TARGET_CPU="x64"
            ;;
        *)
            log_error "Unknown ABI: $ABI"
            exit 1
            ;;
    esac
    
    # Build configuration - MATCHES iOS configuration for consistency
    # See scripts/build-release.sh lines 299-330 for reference
    bin/gn gen "$OUT_DIR" --args="
        is_official_build=true
        is_component_build=false
        is_debug=false
        target_os=\"android\"
        target_cpu=\"$TARGET_CPU\"
        ndk=\"$NDK_PATH\"
        
        # System dependencies - all disabled (use Skia's bundled versions)
        skia_use_system_expat=false
        skia_use_system_freetype2=false
        skia_use_system_harfbuzz=false
        skia_use_system_icu=false
        skia_use_system_libjpeg_turbo=false
        skia_use_system_libpng=false
        skia_use_system_libwebp=false
        skia_use_system_zlib=false
        
        # GPU/Graphics backends - DISABLED for CPU-only rendering
        # This matches iOS configuration for consistent rendering
        skia_enable_gpu=false
        skia_use_gl=false
        skia_use_vulkan=false
        skia_use_metal=false
        skia_use_dawn=false
        
        # Features - minimal set for text rendering
        skia_enable_skottie=false
        skia_enable_pdf=false
        skia_enable_skshaper=false
        skia_enable_svg=false
        skia_use_dng_sdk=false
        skia_use_piex=false
        skia_use_sfntly=false
        skia_use_libjpeg_turbo_decode=false
        skia_use_libjpeg_turbo_encode=false
        skia_use_wuffs=true
        
        # C++ RTTI required for HarfBuzz integration
        extra_cflags_cc=[\"-frtti\"]
    "
    
    # Build
    ninja -C "$OUT_DIR" :skia
    
    log_success "Skia built for Android $ABI: $SKIA_LIB"
    log_info "  Size: $(du -h "$SKIA_LIB" | cut -f1)"
}

# Main
main() {
    log_info "==========================================="
    log_info "  Android Skia Build Script              "
    log_info "==========================================="
    
    mkdir -p "$DEPS_DIR"
    
    clone_or_update_skia
    
    if [[ "$TARGET_ABI" == "all" ]]; then
        log_info "Building Skia for all Android ABIs..."
        build_skia_android "armeabi-v7a"
        build_skia_android "arm64-v8a"
        log_success "All Android Skia builds complete"
    else
        build_skia_android "$TARGET_ABI"
    fi
    
    log_success "==========================================="
    log_success "  Android Skia Build Complete!          "
    log_success "==========================================="
    log_info ""
    log_info "Skia libraries:"
    ls -lh "$SKIA_DIR/out/android-"*/libskia.a 2>/dev/null || log_warn "No builds found"
    log_info ""
    log_info "Next steps:"
    log_info "  1. Update android/src/main/cpp/CMakeLists.txt to use these builds"
    log_info "  2. Run: ./gradlew :android:assembleRelease"
}

main "$@"
