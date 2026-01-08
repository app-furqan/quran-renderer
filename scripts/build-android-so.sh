#!/bin/bash
#
# build-android-so.sh
#
# Build quran-renderer for Android with Skia from source
# Ensures consistent rendering with iOS/macOS
#
# Usage:
#   ./scripts/build-android-so.sh [OPTIONS]
#
# Options:
#   --abi ABI         Build for specific ABI (armeabi-v7a, arm64-v8a, x86_64, all)
#   --skip-skia       Skip building Skia (use existing builds)
#   --skip-harfbuzz   Skip building HarfBuzz (use existing builds)
#   --clean           Clean build directories
#   --help            Show this help
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

# Default options
TARGET_ABI="all"
SKIP_SKIA=false
SKIP_HARFBUZZ=false
CLEAN_BUILD=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --abi)
            TARGET_ABI="$2"
            shift 2
            ;;
        --skip-skia)
            SKIP_SKIA=true
            shift
            ;;
        --skip-harfbuzz)
            SKIP_HARFBUZZ=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --help)
            head -15 "$0" | tail -13
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check Android NDK
if [[ -z "$ANDROID_NDK_HOME" ]] && [[ -z "$ANDROID_NDK" ]]; then
    log_error "ANDROID_NDK_HOME or ANDROID_NDK must be set"
    log_info "Example: export ANDROID_NDK_HOME=\$HOME/Android/Sdk/ndk/25.2.9519653"
    exit 1
fi

NDK_PATH="${ANDROID_NDK_HOME:-$ANDROID_NDK}"
log_info "Using Android NDK: $NDK_PATH"

# Check for required tools
for cmd in git cmake ninja python3; do
    if ! command -v $cmd &> /dev/null; then
        log_error "$cmd is required but not installed"
        exit 1
    fi
done

# Build HarfBuzz for Android ABI
build_harfbuzz_android() {
    local ABI=$1
    local HB_DIR="$DEPS_DIR/harfbuzz"
    local HB_BUILD="$DEPS_DIR/harfbuzz-build-android-$ABI"
    local HB_LIB="$HB_BUILD/libharfbuzz.a"
    
    if [[ -f "$HB_LIB" ]]; then
        log_success "HarfBuzz for $ABI already built (cached)"
        return 0
    fi
    
    log_info "Building HarfBuzz for Android $ABI..."
    
    # Clone/update HarfBuzz
    if [[ ! -d "$HB_DIR/.git" ]]; then
        log_info "Cloning HarfBuzz (justification branch)..."
        git clone -b justification https://github.com/DigitalKhatt/harfbuzz.git "$HB_DIR"
    fi
    
    mkdir -p "$HB_BUILD"
    cd "$HB_BUILD"
    
    # Map ABI to Android ABI name for toolchain
    local ANDROID_ABI_NAME=""
    case $ABI in
        armeabi-v7a)
            ANDROID_ABI_NAME="armeabi-v7a"
            ;;
        arm64-v8a)
            ANDROID_ABI_NAME="arm64-v8a"
            ;;
        x86_64)
            ANDROID_ABI_NAME="x86_64"
            ;;
        *)
            log_error "Unknown ABI: $ABI"
            exit 1
            ;;
    esac
    
    cmake "$HB_DIR" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -DHB_HAVE_FREETYPE=OFF \
        -DHB_HAVE_GLIB=OFF \
        -DHB_HAVE_ICU=OFF \
        -DHB_HAVE_GOBJECT=OFF \
        -DHB_BUILD_SUBSET=OFF \
        -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ANDROID_ABI_NAME" \
        -DANDROID_PLATFORM=android-21 \
        -DANDROID_STL=c++_shared
    
    ninja
    
    log_success "HarfBuzz built for Android $ABI"
}

# Setup VisualMetaFont
setup_visualmetafont() {
    local VMF_DIR="$DEPS_DIR/visualmetafont"
    
    if [[ -d "$VMF_DIR/.git" ]]; then
        log_success "VisualMetaFont already set up (cached)"
        return 0
    fi
    
    log_info "Cloning VisualMetaFont..."
    git clone https://github.com/DigitalKhatt/visualmetafont.git "$VMF_DIR"
    log_success "VisualMetaFont ready"
}

# Build for specific ABI
build_android_abi() {
    local ABI=$1
    
    log_info "Building quran-renderer for Android $ABI..."
    
    local BUILD_DIR="$PROJECT_DIR/build/android-build-$ABI"
    local OUTPUT_DIR="$PROJECT_DIR/build/android/$ABI"
    local OUTPUT_LIB="$OUTPUT_DIR/libquranrenderer.so"
    local SKIA_LIB="$DEPS_DIR/skia/out/android-$ABI/libskia.a"
    
    # Check if we need to rebuild by comparing timestamps
    local NEEDS_BUILD=false
    
    if [[ ! -f "$OUTPUT_LIB" ]]; then
        NEEDS_BUILD=true
        log_info "No existing .so file found, building from scratch..."
    elif [[ -f "$SKIA_LIB" ]] && [[ "$SKIA_LIB" -nt "$OUTPUT_LIB" ]]; then
        NEEDS_BUILD=true
        log_warn "Skia library is newer than existing .so, forcing rebuild..."
        rm -rf "$BUILD_DIR"
    elif [[ "$CLEAN_BUILD" == true ]]; then
        NEEDS_BUILD=true
        log_info "Clean build requested, removing old build..."
        rm -rf "$BUILD_DIR"
    fi
    
    # Also check if source files are newer
    if [[ "$NEEDS_BUILD" == false ]]; then
        for src_file in "$PROJECT_DIR/src/core/"*.cpp "$PROJECT_DIR/android/src/main/cpp/"*.cpp; do
            if [[ -f "$src_file" ]] && [[ "$src_file" -nt "$OUTPUT_LIB" ]]; then
                NEEDS_BUILD=true
                log_warn "Source file $(basename "$src_file") is newer, forcing rebuild..."
                rm -rf "$BUILD_DIR"
                break
            fi
        done
    fi
    
    if [[ "$NEEDS_BUILD" == false ]]; then
        log_success "Android $ABI library is up to date (no rebuild needed)"
        log_info "  Output: $OUTPUT_LIB"
        log_info "  Size: $(du -h "$OUTPUT_LIB" | cut -f1)"
        if command -v md5sum &> /dev/null; then
            MD5=$(md5sum "$OUTPUT_LIB" | cut -d' ' -f1)
        elif command -v md5 &> /dev/null; then
            MD5=$(md5 -q "$OUTPUT_LIB")
        else
            MD5="(md5 tool not available)"
        fi
        log_info "  MD5: $MD5"
        return 0
    fi
    
    mkdir -p "$BUILD_DIR"
    mkdir -p "$OUTPUT_DIR"
    
    cd "$BUILD_DIR"
    
    # Map ABI to Android ABI name
    local ANDROID_ABI_NAME=""
    case $ABI in
        armeabi-v7a)
            ANDROID_ABI_NAME="armeabi-v7a"
            ;;
        arm64-v8a)
            ANDROID_ABI_NAME="arm64-v8a"
            ;;
        x86_64)
            ANDROID_ABI_NAME="x86_64"
            ;;
        *)
            log_error "Unknown ABI: $ABI"
            exit 1
            ;;
    esac
    
    # Configure with CMake
    cmake "$PROJECT_DIR/android/src/main/cpp" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ANDROID_ABI_NAME" \
        -DANDROID_PLATFORM=android-21 \
        -DANDROID_STL=c++_shared \
        -DHARFBUZZ_DIR="$DEPS_DIR/harfbuzz" \
        -DSKIA_DIR="$DEPS_DIR/skia" \
        -DVMF_DIR="$DEPS_DIR/visualmetafont"
    
    # Build
    ninja
    
    # Copy output
    cp libquranrenderer.so "$OUTPUT_DIR/"
    
    # Generate MD5 checksum for verification
    if command -v md5sum &> /dev/null; then
        MD5=$(md5sum "$OUTPUT_DIR/libquranrenderer.so" | cut -d' ' -f1)
    elif command -v md5 &> /dev/null; then
        MD5=$(md5 -q "$OUTPUT_DIR/libquranrenderer.so")
    else
        MD5="(md5 tool not available)"
    fi
    
    log_success "Built quran-renderer for Android $ABI"
    log_info "  Output: $OUTPUT_DIR/libquranrenderer.so"
    log_info "  Size: $(du -h "$OUTPUT_DIR/libquranrenderer.so" | cut -f1)"
    log_info "  MD5: $MD5"
}
# Main
main() {
    log_info "==========================================="
    log_info "  Android Build Script (Skia from source) "
    log_info "==========================================="
    log_info ""
    log_info "This script builds Skia with the SAME configuration"
    log_info "as iOS/macOS for consistent rendering across platforms:"
    log_info "  - CPU-only rendering (no GPU backends)"
    log_info "  - Bundled dependencies (not system libraries)"
    log_info "  - Identical feature flags"
    log_info ""
    
    mkdir -p "$DEPS_DIR"
    
    if [[ "$CLEAN_BUILD" == true ]]; then
        log_warn "Cleaning build directories..."
        rm -rf "$PROJECT_DIR/build/android"
        rm -rf "$PROJECT_DIR/build/android-build-"*
    fi
    
    # Setup dependencies
    setup_visualmetafont
    
    # Determine ABIs to build
    local ABIS=()
    if [[ "$TARGET_ABI" == "all" ]]; then
        ABIS=("armeabi-v7a" "arm64-v8a")
    else
        ABIS=("$TARGET_ABI")
    fi
    
    # Build Skia if needed
    if [[ "$SKIP_SKIA" != true ]]; then
        log_info "Building Skia from source (this takes a while)..."
        log_info "Using build-android-skia.sh for consistent configuration..."
        for abi in "${ABIS[@]}"; do
            "$SCRIPT_DIR/build-android-skia.sh" "$abi"
        done
    else
        log_warn "Skipping Skia build (using existing builds)"
    fi
    
    # Build HarfBuzz if needed
    if [[ "$SKIP_HARFBUZZ" != true ]]; then
        for abi in "${ABIS[@]}"; do
            build_harfbuzz_android "$abi"
        done
    else
        log_warn "Skipping HarfBuzz build (using existing builds)"
    fi
    
    # Build quran-renderer for each ABI
    for abi in "${ABIS[@]}"; do
        build_android_abi "$abi"
    done
    
    # Deploy to jniLibs automatically
    log_info ""
    log_info "Deploying libraries to jniLibs..."
    local JNILIBS_DIR="$PROJECT_DIR/android/src/main/jniLibs"
    mkdir -p "$JNILIBS_DIR"
    
    for lib in "$PROJECT_DIR/build/android/"*"/libquranrenderer.so"; do
        if [[ -f "$lib" ]]; then
            local ABI=$(basename $(dirname "$lib"))
            mkdir -p "$JNILIBS_DIR/$ABI"
            cp "$lib" "$JNILIBS_DIR/$ABI/"
            log_info "  Deployed: $JNILIBS_DIR/$ABI/libquranrenderer.so"
        fi
    done
    
    log_success "==========================================="
    log_success "  Android Build Complete!                "
    log_success "==========================================="
    log_info ""
    log_info "Output libraries with MD5 checksums:"
    
    for lib in "$PROJECT_DIR/build/android/"*"/libquranrenderer.so"; do
        if [[ -f "$lib" ]]; then
            local SIZE=$(du -h "$lib" | cut -f1)
            local MD5=""
            if command -v md5sum &> /dev/null; then
                MD5=$(md5sum "$lib" | cut -d' ' -f1)
            elif command -v md5 &> /dev/null; then
                MD5=$(md5 -q "$lib")
            else
                MD5="(unavailable)"
            fi
            log_info "  $(basename $(dirname "$lib")): $SIZE - MD5: $MD5"
        fi
    done
    
    log_info ""
    log_info "Next steps:"
    log_info "  1. Copy libs to android/src/main/jniLibs/"
    log_info "  2. Run: ./gradlew :android:assembleRelease"
    log_info ""
    log_info "Note: Skia is now built from source with the same"
    log_info "configuration as iOS for consistent rendering!"
    log_info "MD5 checksums verify libraries use unified Skia build."
}

main "$@"
