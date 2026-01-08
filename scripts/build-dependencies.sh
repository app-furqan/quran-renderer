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
# NOTE: This function is DEPRECATED - use scripts/build-android-skia.sh instead
# That script uses the SAME configuration as iOS/macOS for consistent rendering
build_skia_abi() {
    local cpu="$1"
    local abi="$2"
    
    log_warn "build_skia_abi() is deprecated - redirecting to build-android-skia.sh"
    log_info "This ensures consistent Skia configuration across all platforms"
    
    # Map cpu to ABI name for the new script
    local abi_name=""
    case $cpu in
        arm64) abi_name="arm64-v8a" ;;
        arm) abi_name="armeabi-v7a" ;;
        x64) abi_name="x86_64" ;;
        *)
            log_error "Unknown CPU: $cpu"
            return 1
            ;;
    esac
    
    # Set environment variables and call the unified build script
    export ANDROID_NDK_HOME="$NDK_PATH"
    "$SCRIPT_DIR/build-android-skia.sh" "$abi_name"
    
    log_success "Built Skia for $abi using unified configuration"
}
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
    echo "Step 1/6: Cloning/updating HarfBuzz (DigitalKhatt fork)"
    echo "--------------------------------------------------------"
    clone_or_update "https://github.com/DigitalKhatt/harfbuzz.git" "$DEPS_DIR/harfbuzz" "justification"
    
    echo ""
    echo "Step 2/6: Cloning/updating VisualMetaFont (Quran text data)"
    echo "------------------------------------------------------------"
    clone_or_update "https://github.com/DigitalKhatt/visualmetafont.git" "$DEPS_DIR/visualmetafont"
    log_info "Quran text source: $DEPS_DIR/visualmetafont/src/qurantext/quran.cpp"
    
    echo ""
    echo "Step 3/6: Downloading DigitalKhatt fonts"
    echo "-----------------------------------------"
    FONTS_DIR="$DEPS_DIR/fonts"
    mkdir -p "$FONTS_DIR"
    
    # Download Madina font (new style)
    if [[ ! -f "$FONTS_DIR/madina.otf" ]]; then
        log_info "Downloading Madina font..."
        curl -L -o "$FONTS_DIR/madina.otf" \
            "https://github.com/DigitalKhatt/madinafont/raw/main/madina.otf" 2>/dev/null || \
            log_warn "Could not download madina.otf - you may need to build it from source"
    else
        log_info "Madina font already exists"
    fi
    
    # Download Old Madina font
    log_info "Checking Old Madina font..."
    if [[ ! -f "$FONTS_DIR/oldmadina.otf" ]]; then
        # Try to get from releases first
        OLDMADINA_URL=$(curl -s "https://api.github.com/repos/DigitalKhatt/oldmadinafont/releases/latest" | \
            grep "browser_download_url.*\.otf" | cut -d '"' -f 4 || echo "")
        if [[ -n "$OLDMADINA_URL" ]]; then
            log_info "Downloading Old Madina font from release..."
            curl -L -o "$FONTS_DIR/oldmadina.otf" "$OLDMADINA_URL" 2>/dev/null
        else
            log_warn "Old Madina font not available as pre-built - needs to be built from source"
            log_warn "See: https://github.com/DigitalKhatt/oldmadinafont"
        fi
    else
        log_info "Old Madina font already exists"
    fi
    
    # Download IndoPak font
    log_info "Checking IndoPak font..."
    if [[ ! -f "$FONTS_DIR/indopak.otf" ]]; then
        INDOPAK_URL=$(curl -s "https://api.github.com/repos/DigitalKhatt/indopakfont/releases/latest" | \
            grep "browser_download_url.*\.otf" | cut -d '"' -f 4 || echo "")
        if [[ -n "$INDOPAK_URL" ]]; then
            log_info "Downloading IndoPak font from release..."
            curl -L -o "$FONTS_DIR/indopak.otf" "$INDOPAK_URL" 2>/dev/null
        else
            log_warn "IndoPak font not available as pre-built - needs to be built from source"
            log_warn "See: https://github.com/DigitalKhatt/indopakfont"
        fi
    else
        log_info "IndoPak font already exists"
    fi
    
    log_success "Fonts downloaded to: $FONTS_DIR"
    ls -lh "$FONTS_DIR"/*.otf 2>/dev/null || true
    
    if [[ "$SKIP_SKIA" == true ]]; then
        log_warn "Skipping Skia build (--skip-skia specified)"
    else
        echo ""
        echo "Step 4/6: Setting up Skia build tools"
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
        echo "Step 5/6: Building Skia for all ABIs"
        echo "-------------------------------------"
        
        # Build for all ABIs
        build_skia_abi "arm64" "arm64-v8a"
        build_skia_abi "arm" "armeabi-v7a"
        build_skia_abi "x64" "x86_64"
    fi
    
    echo ""
    echo "Step 6/6: Organizing output"
    echo "----------------------------"
    
    # Create output directory structure
    SKIA_OUT="$DEPS_DIR/skia-build"
    mkdir -p "$SKIA_OUT"/{arm64-v8a,armeabi-v7a,x86_64,include_root}
    
    if [[ "$SKIP_SKIA" != true ]]; then
        # Copy static libraries (using new naming convention from build-android-skia.sh)
        cp "$DEPS_DIR/skia/out/android-arm64-v8a/libskia.a" "$SKIA_OUT/arm64-v8a/"
        cp "$DEPS_DIR/skia/out/android-armeabi-v7a/libskia.a" "$SKIA_OUT/armeabi-v7a/"
        cp "$DEPS_DIR/skia/out/android-x86_64/libskia.a" "$SKIA_OUT/x86_64/"
        
        # Copy headers
        rm -rf "$SKIA_OUT/include_root/include"
        cp -r "$DEPS_DIR/skia/include" "$SKIA_OUT/include_root/"
        
        log_success "Skia libraries organized in $SKIA_OUT"
        log_info "Note: Skia is built with consistent configuration (CPU-only, matches iOS/macOS)"
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

# DigitalKhatt fonts directory (contains madina.otf, oldmadina.otf, indopak.otf)
fonts.dir=$DEPS_DIR/fonts
EOF
    
    log_success "Generated $LOCAL_PROPS"
    
    # Copy default font to assets if not exists
    ASSETS_FONTS="$PROJECT_DIR/android/src/main/assets/fonts"
    if [[ -f "$DEPS_DIR/fonts/madina.otf" ]] && [[ ! -f "$ASSETS_FONTS/digitalkhatt.otf" ]]; then
        log_info "Copying Madina font to assets as default..."
        mkdir -p "$ASSETS_FONTS"
        cp "$DEPS_DIR/fonts/madina.otf" "$ASSETS_FONTS/digitalkhatt.otf"
    fi
    
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
    echo "  │   └── src/qurantext/quran.cpp  <- Quran text source (604 pages)"
    echo "  ├── fonts/             (DigitalKhatt fonts)"
    echo "  │   ├── madina.otf     (New Madina style - default)"
    echo "  │   ├── oldmadina.otf  (Old Madina style)"
    echo "  │   └── indopak.otf    (IndoPak 13-line style)"
    echo "  ├── skia/              (Skia source)"
    echo "  ├── skia-build/        (Organized output)"
    echo "  │   ├── arm64-v8a/libskia.a"
    echo "  │   ├── armeabi-v7a/libskia.a"
    echo "  │   ├── x86_64/libskia.a"
    echo "  │   └── include_root/include/"
    echo "  └── depot_tools/"
    echo ""
    
    echo "Available fonts:"
    ls -lh "$DEPS_DIR/fonts"/*.otf 2>/dev/null | awk '{print "  " $9 ": " $5}' || echo "  (none downloaded)"
    echo ""
    
    if [[ "$SKIP_SKIA" != true ]]; then
        echo "Skia library sizes:"
        ls -lh "$SKIA_OUT"/*/libskia.a 2>/dev/null | awk '{print "  " $9 ": " $5}'
        echo ""
    fi
    
    echo "To use a different font, copy it to:"
    echo "  $PROJECT_DIR/android/src/main/assets/fonts/digitalkhatt.otf"
    echo ""
    echo "Next steps:"
    echo "  1. cd $PROJECT_DIR"
    echo "  2. ./gradlew :android:assembleRelease"
    echo "  3. Find AAR at: android/build/outputs/aar/android-release.aar"
    echo ""
}

# Run main
main "$@"