#!/bin/bash
set -e

VISUALMETAFONT_DIR="/home/ali/Desktop/Projects/mushaf-android/libs/visualmetafont/src"
QURAN_TEXT_DIR="$VISUALMETAFONT_DIR/qurantext"

# Clean and create build directory
rm -rf build/linux
mkdir -p build/linux
cd build/linux

# Run cmake with quran text sources added directly
cmake ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON

# Now we need to manually add the quran text sources
# Create a modified CMakeLists to include them
cd ../..

# Build with the qurantext sources
cmake --build build/linux --target quran_renderer -- VERBOSE=1

echo "Done. Check build/linux for libquranrenderer.so"
