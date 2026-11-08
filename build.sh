#!/bin/sh
# Compile every module and verify it against the original slinky2 image.
# Original image path: tools/paths.py (override with SLINKY_ORIG=...).
set -e
cd "$(dirname "$0")"
python3 tools/verify.py
python3 tools/verify_logic.py
python3 tools/verify_engine.py --quiet
for f in src/engine/protected/*.cpp; do
    clang-cl --target=x86_64-pc-windows-msvc -O2 -Iinclude -c "$f" -o "build/obj/protected_$(basename "$f" .cpp).obj"
done
echo "protected (readable, compile-checked): $(ls src/engine/protected/*.cpp | wc -l) files"
for f in src/engine/lib/*.cpp; do
    clang-cl --target=x86_64-pc-windows-msvc -O2 -Iinclude -c "$f" -o "build/obj/lib_$(basename "$f" .cpp).obj"
done
echo "lib (translated from the listing, compile-checked): $(ls src/engine/lib/*.cpp | wc -l) files"
echo "full DLL: cmake -S . -B build/cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-windows-clang-cl.cmake && cmake --build build/cmake"
python3 tools/verify_modules.py
python3 tools/report_modules.py
