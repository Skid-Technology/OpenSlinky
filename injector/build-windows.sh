#!/bin/sh
# Cross-build slinky-inject.exe with MinGW-w64.
set -e
cd "$(dirname "$0")"
cmake -S . -B build-windows \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows -j"$(nproc)"
echo "built: $(pwd)/build-windows/slinky-inject.exe"
