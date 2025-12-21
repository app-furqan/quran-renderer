#!/bin/bash
#
# build-dependencies.sh
# 
# Automated script to build all dependencies for quran-renderer-android
# This script clones/updates and builds:
#   - HarfBuzz (DigitalKhatt fork with justification support)
#   - VisualMetaFont (Quran text data)
#   - Skia (static library for all Android ABIs)
#
# Usage:
#   ./scripts/build-dependencies.sh [OPTIONS]
#
# Options:
#   --ndk-path PATH    Path to Android NDK (default: auto-detect from ANDROID_HOME)
#   --deps-dir PATH    Directory to store dependencies (default: ../quran-deps)
#   --skip-skia        Skip Skia build (useful if already built)
#   --clean            Clean and rebuild everything
#   --help             Show this help message
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="${PROJECT_DIR}/../quran-deps"
NDK_VERSION="28.2.13676358"
NDK_PATH=""
SKIP_SKIA=false
CLEAN_BUILD=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --ndk-path)
            NDK_PATH="$2"
            shift 2
            ;;
        --deps-dir)
            DEPS_DIR="$2"
            shift 2
            ;;
        --skip-skia)
            SKIP_SKIA=true
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

# Functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect NDK path
detect_ndk() {
    if [[ -n "$NDK_PATH" ]]; then
        return
    fi
    
    # Try ANDROID_HOME
    if [[ -n "$ANDROID_HOME" ]]; then
        NDK_PATH="$ANDROID_HOME/ndk/$NDK_VERSION"
        if [[ -d "$NDK_PATH" ]]; then
            return
        fi
    fi
    
    # Try ANDROID_SDK_ROOT
    if [[ -n "$ANDROID_SDK_ROOT" ]]; then
        NDK_PATH="$ANDROID_SDK_ROOT/ndk/$NDK_VERSION"
        if [[ -d "$NDK_PATH" ]]; then
            return
        fi
    fi
    
    # Try common locations
    for SDK_PATH in "$HOME/Android/Sdk" "$HOME/Library/Android/sdk" "/opt/android-sdk"; do
        NDK_PATH="$SDK_PATH/ndk/$NDK_VERSION"
        if [[ -d "$NDK_PATH" ]]; then
            return
        fi
    done
    
    log_error "Could not find Android NDK $NDK_VERSION"
    log_error "Please install NDK $NDK_VERSION via Android Studio SDK Manager"
    log_error "Or specify path with --ndk-path /path/to/ndk/$NDK_VERSION"
    exit 1
}

# Clone or update a git repository
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
            git pull origin "$branch"
        else
            git pull
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

# Build Skia for a specific ABI
build_skia_abi() {
    local cpu="$1"
    local abi="$2"
    local out_dir="out/android-$cpu"
    
    log_info "Building Skia for $abi (cpu=$cpu)..."
    
    if [[ "$CLEAN_BUILD" == true ]] && [[ -d "$out_dir" ]]; then
        rm -rf "$out_dir"
    fi
    
    bin/gn gen "$out_dir" --args="
        target_os=\"android\"
        target_cpu=\"$cpu\"
        ndk=\"$NDK_PATH\"
        is_official_build=true
        is_component_build=false
        is_debug=false
        skia_use_system_freetype2=false
        skia_use_freetype=true
        skia_enable_fontmgr_android=true
        skia_use_gl=true
    "
    
    ninja -C "$out_dir" skia
    
    log_success "Built Skia for $abi"
}

# Main script
main() {
    echo ""
    echo "=========================================="
    echo " Quran Renderer - Dependency Builder"
    echo "=========================================="
    echo ""
    
    # Detect NDK
    detect_ndk
    log_info "Using NDK: $NDK_PATH"
    
    # Create deps directory
    mkdir -p "$DEPS_DIR"
    DEPS_DIR="$(cd "$DEPS_DIR" && pwd)"
    log_info "Dependencies directory: $DEPS_DIR"
    
    # Clean if requested
    if [[ "$CLEAN_BUILD" == true ]]; then
        log_warn "Clean build requested - removing existing dependencies"
        rm -rf "$DEPS_DIR/harfbuzz" "$DEPS_DIR/visualmetafont" "$DEPS_DIR/skia" "$DEPS_DIR/depot_tools"
    fi
    
    echo ""
    echo "Step 1/5: Cloning/updating HarfBuzz (DigitalKhatt fork)"
    echo "--------------------------------------------------------"
    clone_or_update "https://github.com/DigitalKhatt/harfbuzz.git" "$DEPS_DIR/harfbuzz" "justification"
    
    echo ""
    echo "Step 2/5: Cloning/updating VisualMetaFont"
    echo "------------------------------------------"
    clone_or_update "https://github.com/DigitalKhatt/visualmetafont.git" "$DEPS_DIR/visualmetafont"
    
    if [[ "$SKIP_SKIA" == true ]]; then
        log_warn "Skipping Skia build (--skip-skia specified)"
    else
        echo ""
        echo "Step 3/5: Setting up Skia build tools"
        echo "--------------------------------------"
        
        # Clone depot_tools if needed
        if [[ ! -d "$DEPS_DIR/depot_tools" ]]; then
            log_info "Cloning depot_tools..."
            git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$DEPS_DIR/depot_tools"
        fi
        export PATH="$DEPS_DIR/depot_tools:$PATH"
        
        # Clone or update Skia
        clone_or_update "https://skia.googlesource.com/skia.git" "$DEPS_DIR/skia"
        
        cd "$DEPS_DIR/skia"
        log_info "Syncing Skia dependencies..."
        python3 tools/git-sync-deps
        
        echo ""
        echo "Step 4/5: Building Skia for all ABIs"
        echo "-------------------------------------"
        
        # Build for all ABIs
        build_skia_abi "arm64" "arm64-v8a"
        build_skia_abi "arm" "armeabi-v7a"
        build_skia_abi "x64" "x86_64"
    fi
    
    echo ""
    echo "Step 5/5: Organizing output"
    echo "----------------------------"
    
    # Create output directory structure
    SKIA_OUT="$DEPS_DIR/skia-build"
    mkdir -p "$SKIA_OUT"/{arm64-v8a,armeabi-v7a,x86_64,include_root}
    
    if [[ "$SKIP_SKIA" != true ]]; then
        # Copy static libraries
        cp "$DEPS_DIR/skia/out/android-arm64/libskia.a" "$SKIA_OUT/arm64-v8a/"
        cp "$DEPS_DIR/skia/out/android-arm/libskia.a" "$SKIA_OUT/armeabi-v7a/"
        cp "$DEPS_DIR/skia/out/android-x64/libskia.a" "$SKIA_OUT/x86_64/"
        
        # Copy headers
        rm -rf "$SKIA_OUT/include_root/include"
        cp -r "$DEPS_DIR/skia/include" "$SKIA_OUT/include_root/"
        
        log_success "Skia libraries organized in $SKIA_OUT"
    fi
    
    # Generate local.properties
    LOCAL_PROPS="$PROJECT_DIR/local.properties"
    log_info "Generating local.properties..."
    
    # Preserve sdk.dir if it exists
    SDK_DIR=""
    if [[ -f "$LOCAL_PROPS" ]]; then
        SDK_DIR=$(grep "^sdk.dir=" "$LOCAL_PROPS" 2>/dev/null | cut -d= -f2 || echo "")
    fi
    
    if [[ -z "$SDK_DIR" ]]; then
        # Try to detect SDK
        for path in "$ANDROID_HOME" "$ANDROID_SDK_ROOT" "$HOME/Android/Sdk" "$HOME/Library/Android/sdk"; do
            if [[ -d "$path" ]]; then
                SDK_DIR="$path"
                break
            fi
        done
    fi
    
    cat > "$LOCAL_PROPS" << EOF
# Auto-generated by build-dependencies.sh
# Generated on: $(date)

# Android SDK
sdk.dir=$SDK_DIR

# HarfBuzz (DigitalKhatt fork with justification support)
harfbuzz.dir=$DEPS_DIR/harfbuzz

# VisualMetaFont (Quran text data)
vmf.dir=$DEPS_DIR/visualmetafont

# Skia static libraries
skia.dir=$SKIA_OUT
EOF
    
    log_success "Generated $LOCAL_PROPS"
    
    # Print summary
    echo ""
    echo "=========================================="
    echo " Build Complete!"
    echo "=========================================="
    echo ""
    echo "Dependencies location: $DEPS_DIR"
    echo ""
    echo "Directory structure:"
    echo "  $DEPS_DIR/"
    echo "  ├── harfbuzz/          (DigitalKhatt fork)"
    echo "  ├── visualmetafont/    (Quran text data)"
    echo "  ├── skia/              (Skia source)"
    echo "  ├── skia-build/        (Organized output)"
    echo "  │   ├── arm64-v8a/libskia.a"
    echo "  │   ├── armeabi-v7a/libskia.a"
    echo "  │   ├── x86_64/libskia.a"
    echo "  │   └── include_root/include/"
    echo "  └── depot_tools/"
    echo ""
    
    if [[ "$SKIP_SKIA" != true ]]; then
        echo "Library sizes:"
        ls -lh "$SKIA_OUT"/*/libskia.a 2>/dev/null | awk '{print "  " $9 ": " $5}'
        echo ""
    fi
    
    echo "Next steps:"
    echo "  1. cd $PROJECT_DIR"
    echo "  2. ./gradlew :android:assembleRelease"
    echo "  3. Find AAR at: android/build/outputs/aar/android-release.aar"
    echo ""
}

# Run main
main "$@"
