#!/usr/bin/env bash
# Build "ID Card Kothay?" from an MSYS2 UCRT64 shell:   ./build.sh
# FreeGLUT and the C++ runtime are linked statically (no extra DLLs).
# Keep assets/ beside the EXE for the guard voice recording.
set -e
cd "$(dirname "$0")"
g++ -std=c++20 -O2 -Wall -Wextra -DFREEGLUT_STATIC \
    src/*.cpp -o IDCardKothay.exe \
    -static -lfreeglut -lopengl32 -lglu32 -lgdi32 -lwinmm -mwindows
echo "Built IDCardKothay.exe"
