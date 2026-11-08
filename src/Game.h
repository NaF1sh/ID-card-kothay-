// Game.h - top-level game object: screen state machine, level progression,
// fixed-step update and the layered render.
#pragma once

#include <vector>
#include <algorithm>

#include "Audio.h"
#include "Guard.h"
#include "Level.h"
#include "Player.h"
#include "UI.h"
#include "Scoreboard.h"
#include "Campus.h"

enum class Screen { MainMenu, NameEntry, Scoreboard, Instructions, Playing, Paused, Dialogue, Caught, TimedOut, LevelComplete, Victory, Briefing };

class Game {
public:
    void init();
    void update(float dt);  // fixed-timestep simulation step
    void render();
    void reshape(int w, int h);

    void keyDown(unsigned char key);
    void keyUp(unsigned char key);
    void specialDown(int key);
    void specialUp(int key);
    void mouseMove(int px, int py);   // aim in gameplay, hover in menus
    void mouseClick(int px, int py);  // throw in gameplay, activate in menus

    // Testing / presentation hooks (command line --level N, --start x,y).
    void startAtLevel(int index) { rankedRun_ = false; startLevel(index); setScreen(Screen::Playing); }
    void placePlayer(Vec2 p) {
        player_.reset(p);
        awaitingDeparture_ = false;  // positioned practice/test runs are already underway
        updateCamera(0.0f, true);
    }

    bool wantsQuit() const { return quit_; }
    float time() const { return uiTime_; }
    Screen screen() const { return screen_; }
    int collected() const;
    int distractions() const { return distractions_; }
    const std::string& playerName() const { return playerName_; }
    float remainingTime() const { return std::max(0.0f, level_.timeLimit - levelTime_); }

private:
    friend struct GameplayTestAccess;
    void startLevel(int index);  // load + switch to Playing
    void restartLevel();
    void retryLevel();
    void goToMenu();
    void setScreen(Screen s);
    const ui::Items& menuItems() const;
    void activate(int item);
    void updatePlaying(float dt);
    void finishLevel();
    void updateCamera(float dt, bool snap);
    void refreshInput();
    void throwDistraction();
    Vec2 distractionTarget() const;
    bool mouseAimValid() const;
    Vec2 nextObjective() const;
    int nearbyCharacter() const;
    void talk();
    void interact();
    int nearbyPass() const;
    void advanceDialogue();
    CampusCatPose catPose(size_t index) const;

    void renderWorld(bool showPlayer);
    void drawGoalMarker() const;
    void drawGoalArrow() const;

    Audio audio_;
    Level level_;
    int levelIndex_ = 0;
    Player player_;
    std::vector<Guard> guards_;
    int catcher_ = -1;  // index of the guard who caught the detective
    std::vector<bool> collected_;
    float pickupTime_ = 0, petTime_ = 0, alertCueCooldown_ = 0;
    Vec2 pickupPosition_;
    int petCat_ = -1;
    int distractions_ = 2;
    float distractionCooldown_ = 0;
    int mouseX_ = 0, mouseY_ = 0;
    bool mouseKnown_ = false;
    Vec2 noisePosition_;
    float noiseTime_ = 0;
    float radioCooldown_ = 0;
    float messageTime_ = 0;
    std::string message_;
    std::vector<std::vector<Vec2>> crowdRoutes_;
    std::vector<std::vector<Vec2>> catRoutes_;
    std::vector<float> catClocks_, catAffection_;
    std::vector<CampusNpc> npcs_;
    std::string speaker_, speakerRole_;
    std::vector<std::string> dialogue_;
    size_t dialoguePage_ = 0;
    float dialogueTime_ = 0;
    int dialogueCharacter_ = -1;
    Scoreboard scoreboard_;
    std::string playerName_, nameDraft_;
    std::string scoreStatus_;
    bool rankedRun_ = false;

    // timing + scoring
    float levelTime_ = 0.0f;
    bool awaitingDeparture_ = true;
    bool spotted_ = false;  // a guard got suspicious this attempt
    bool alarmed_ = false;  // a guard reached "!" this attempt
    int retries_ = 0;       // restarts of the current level
    int totalScore_ = 0;
    float totalTime_ = 0.0f;
    ui::LevelResult result_;

    Screen screen_ = Screen::MainMenu;
    int selected_ = 0;

    InputState input_;
    bool letterHeld_[4] = {};  // 0=up 1=down 2=left 3=right
    bool arrowHeld_[4] = {};
    bool shiftHeld_[2] = {};   // left / right shift
    bool spaceHeld_ = false;   // Space also sprints

    Vec2 camera_;              // bottom-left of the visible view, in world units
    float time_ = 0.0f;        // world animation time (frozen while paused)
    float uiTime_ = 0.0f;      // always runs (menu animations)
    float screenTime_ = 0.0f;  // time since the current screen opened
    bool showDebug_ = false;
    bool quit_ = false;
};
