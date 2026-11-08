// Level.h - level definitions (map layout, spawn, goal).
#pragma once

#include <string>
#include <vector>

#include "Guard.h"
#include "Map.h"

struct Level {
    std::string name;
    std::string objective;
    std::string briefing, reward;
    std::vector<std::string> passNames;
    Map map;
    Vec2 spawn;
    Rect goal;
    std::vector<Vec2> checkpoints;
    float timeLimit = 70.0f;
    std::vector<GuardConfig> guards;
};

// Number of levels currently implemented.
int levelCount();
// Builds level `index` (0-based) from scratch, so restarting is just rebuilding.
Level buildLevel(int index);
