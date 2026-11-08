#!/usr/bin/env bash
# Build and run the automated (window-less) tests:   ./test.sh
set -e
cd "$(dirname "$0")"
FLAGS="-std=c++20 -O2 -Wall -Wextra -DFREEGLUT_STATIC"
LIBS="-static -lfreeglut -lopengl32 -lglu32 -lgdi32 -lwinmm"
CORE="src/Player.cpp src/Guard.cpp src/Character.cpp src/Collision.cpp src/Level.cpp src/Map.cpp src/Graphics.cpp src/Navigation.cpp"
mkdir -p build
g++ $FLAGS tests/movement_test.cpp $CORE -o build/movement_test.exe $LIBS
g++ $FLAGS tests/vision_test.cpp $CORE -o build/vision_test.exe $LIBS
g++ $FLAGS tests/audio_events_test.cpp src/Game.cpp src/UI.cpp src/Scoreboard.cpp src/Campus.cpp $CORE -o build/audio_events_test.exe $LIBS
g++ $FLAGS tests/gameplay_test.cpp src/Game.cpp src/UI.cpp src/Audio.cpp src/Scoreboard.cpp src/Campus.cpp $CORE -o build/gameplay_test.exe $LIBS
./build/movement_test.exe
./build/vision_test.exe
./build/audio_events_test.exe
./build/gameplay_test.exe
