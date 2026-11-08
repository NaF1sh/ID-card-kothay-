// Exercise real game transitions without opening an audio device or GL window.
#include <cstdio>
#include "../src/Game.h"

namespace {
int plays = 0, stops = 0, failures = 0;
void check(bool condition, const char* message) {
    std::printf("  %s  %s\n", condition ? "PASS" : "FAIL", message);
    if (!condition) ++failures;
}
void run(Game& game, float seconds) {
    for (int i = 0; i < static_cast<int>(seconds * 120); ++i) game.update(1.0f / 120);
}
}

// Substitute only the audio device; Game and guard detection are unchanged.
bool Audio::init() { return true; }
bool Audio::playCaught() { ++plays; return true; }
void Audio::playCue(SoundCue) {}
void Audio::stop() { ++stops; }
Audio::~Audio() = default;

int main() {
    Game game;
    game.init();
    run(game, 2);
    check(plays == 0, "menu does not play the guard voice");
    game.startAtLevel(0);
    run(game, 1);
    check(plays == 0, "safe gameplay is silent");
    game.placePlayer({260, 620});
    run(game, 3);
    check(plays == 1, "capture plays the line once");
    run(game, 10);
    check(plays == 1, "remaining on caught screen does not repeat the line");
    game.keyDown('r');
    check(stops == 1, "restart stops the line");
    game.placePlayer({260, 620});
    run(game, 3);
    check(plays == 2, "another capture plays the line again");
    game.keyDown(27);
    check(stops == 2, "returning to menu stops the line");
    run(game, 4);
    check(plays == 2, "background guards do not trigger speech");
    return failures ? 1 : 0;
}
