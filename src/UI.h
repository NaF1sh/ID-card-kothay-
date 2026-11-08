// UI.h - menus, HUD and result screens. All drawn in screen space
// (VIEW_W x VIEW_H) from primitives + GLUT text; no game logic here.
#pragma once

#include <string>
#include <vector>

#include "Types.h"
#include "Scoreboard.h"
#include "Campus.h"
#include "Level.h"

namespace ui {

using Items = std::vector<std::string>;

struct HudInfo {
    std::string levelName, objective;
    int levelNumber = 1, levelCount = 4;
    float detection = 0.0f;  // highest guard detection 0..1
    float stamina = 1.0f;
    bool sprinting = false;
    bool anyAlert = false;
    float levelTime = 0.0f;  // seconds
    int score = 0;           // total score so far
    std::string playerName, message, talkPrompt;
    int passes = 0, passesNeeded = 0, distractions = 0;
    float timeLeft = 0;
};

// Score breakdown for a finished level.
struct LevelResult {
    float time = 0.0f;
    int timeBonus = 0, stealthBonus = 0, retryPenalty = 0, levelScore = 0, totalScore = 0;
    float totalTime = 0.0f;  // all levels so far
};

std::string formatTime(float seconds);  // "m:ss"

// Buttons drawn this frame are remembered so the mouse can hit-test them.
void beginFrame();
int buttonAt(Vec2 viewPos);  // item index under the point, or -1

void drawMainMenu(const Items& items, int selected, float t);
void drawNameEntry(const std::string& name, const Items& items, int selected, float t);
void drawScoreboard(const std::vector<ScoreEntry>& entries, const std::string& status, const Items& items, int selected, float t);
void drawDialogue(const std::string& name, const std::string& role, const std::string& page,
                  size_t pageIndex, size_t pageCount, size_t revealed, const CampusNpc* npc, int cat, float t);
void drawTimeout(const Items& items, int selected, float t);
void drawInstructions(const Items& items, int selected, float t);
void drawHud(const HudInfo& h, float t);
void drawBriefing(const Level& level, int stage, const std::vector<bool>& collected, const Items& items, float t);
void drawPauseMenu(const Items& items, int selected, float t);
void drawGameOver(const Items& items, int selected, float t);
void drawLevelComplete(const std::string& levelName, const LevelResult& r, const Items& items, int selected, float t,
                       const std::string& reward, const std::string& next);
void drawVictory(const LevelResult& r, const Items& items, int selected, float t, const std::string& status = "");

}  // namespace ui
