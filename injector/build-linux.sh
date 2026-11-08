#!/bin/sh
# Build slinky-inject for the host Linux.
set -e
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
echo "built: $(pwd)/build/slinky-inject"
