#!/bin/bash
#
# deploy-android-libs.sh
#
# Deploy built Android native libraries to the appropriate location
# for packaging with the Android app
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
BUILD_DIR="$PROJECT_DIR/build/android"
JNILIBS_DIR="$PROJECT_DIR/android/src/main/jniLibs"

log_info "Deploying Android native libraries..."

# Check if build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    log_error "Build directory not found: $BUILD_DIR"
    log_info "Please run: ./scripts/build-android-so.sh first"
    exit 1
fi

# Create jniLibs directory if it doesn't exist
mkdir -p "$JNILIBS_DIR"

# Deploy each ABI
for abi_dir in "$BUILD_DIR"/*; do
    if [[ -d "$abi_dir" ]]; then
        ABI=$(basename "$abi_dir")
        SO_FILE="$abi_dir/libquranrenderer.so"
        
        if [[ -f "$SO_FILE" ]]; then
            log_info "Deploying $ABI..."
            mkdir -p "$JNILIBS_DIR/$ABI"
            cp "$SO_FILE" "$JNILIBS_DIR/$ABI/"
            
            # Show file info
            SIZE=$(du -h "$SO_FILE" | cut -f1)
            if command -v md5sum &> /dev/null; then
                MD5=$(md5sum "$SO_FILE" | cut -d' ' -f1)
            elif command -v md5 &> /dev/null; then
                MD5=$(md5 -q "$SO_FILE")
            else
                MD5="(unavailable)"
            fi
            
            log_success "  Deployed: $JNILIBS_DIR/$ABI/libquranrenderer.so"
            log_info "  Size: $SIZE - MD5: $MD5"
        fi
    fi
done

log_success "==========================================="
log_success "  Deployment Complete!                   "
log_success "==========================================="
log_info ""
log_info "Libraries deployed to: $JNILIBS_DIR"
log_info ""
log_info "Next steps:"
log_info "  1. Rebuild your Android app"
log_info "  2. Reinstall on device/emulator"
log_info ""
log_info "Example:"
log_info "  ./gradlew :android:assembleDebug"
log_info "  adb install -r android/build/outputs/apk/debug/android-debug.apk"
