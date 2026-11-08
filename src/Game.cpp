#include "Game.h"

#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "Graphics.h"
#include "Character.h"
#include "Collision.h"
#include "Navigation.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>  // GetAsyncKeyState: reliable Shift state
#endif

using namespace gfx;

namespace {
const ui::Items MAIN_ITEMS{"Start Game", "Scoreboard", "Instructions", "Exit"};
const ui::Items NAME_ITEMS{"Let's Go", "Back"};
const ui::Items BRIEF_ITEMS{"Back to campus"};
const ui::Items TALK_ITEMS{"Back to campus"};
const ui::Items INSTR_ITEMS{"Back"};
const ui::Items PAUSE_ITEMS{"Resume", "Restart Level", "Main Menu"};
const ui::Items CAUGHT_ITEMS{"Restart Level", "Main Menu"};
const ui::Items COMPLETE_ITEMS{"Continue", "Main Menu"};
const ui::Items VICTORY_ITEMS{"Scoreboard", "Main Menu"};
const ui::Items NO_ITEMS{};

std::pair<Vec2, Vec2> routePose(const std::vector<Vec2>& route, float distance) {
    float length = 0;
    for (size_t i = 1; i < route.size(); ++i) length += (route[i] - route[i - 1]).length();
    if (length < 0.01f) return {route.front(), {0, -1}};
    distance = std::fmod(distance, length * 2);
    bool reverse = distance > length;
    if (reverse) distance = length * 2 - distance;
    for (size_t i = 1; i < route.size(); ++i) {
        Vec2 delta = route[i] - route[i - 1];
        float segment = delta.length();
        if (segment > 0 && distance <= segment)
            return {route[i - 1] + delta * (distance / segment), reverse ? delta * -1 : delta};
        distance -= segment;
    }
    return {route.back(), {0, -1}};
}

int letterToDir(unsigned char key) {
    switch (key) {
        case 'w': case 'W': return 0;
        case 's': case 'S': return 1;
        case 'a': case 'A': return 2;
        case 'd': case 'D': return 3;
        default: return -1;
    }
}
int arrowToDir(int key) {
    switch (key) {
        case GLUT_KEY_UP: return 0;
        case GLUT_KEY_DOWN: return 1;
        case GLUT_KEY_LEFT: return 2;
        case GLUT_KEY_RIGHT: return 3;
        default: return -1;
    }
}
}  // namespace

// ===========================================================================
// State / flow
// ===========================================================================
void Game::init() {
    audio_.init();
    scoreboard_.load();
    levelIndex_ = 0;
    restartLevel();  // the menu shows level 1's campus in the background
    setScreen(Screen::MainMenu);
}

void Game::setScreen(Screen s) {
    // Trigger on entry, not on every update of the frozen caught screen.
    if (screen_ != s) {
        if (screen_ == Screen::Caught) audio_.stop();
        if (s == Screen::Caught) audio_.playCaught();
    }
    player_.stop();
    screen_ = s;
    selected_ = 0;
    screenTime_ = 0.0f;
    std::fill(std::begin(letterHeld_), std::end(letterHeld_), false);
    std::fill(std::begin(arrowHeld_), std::end(arrowHeld_), false);
    std::fill(std::begin(shiftHeld_), std::end(shiftHeld_), false);
    spaceHeld_ = false;
    refreshInput();
}

void Game::startLevel(int index) {
    levelIndex_ = std::clamp(index, 0, levelCount() - 1);
    if (levelIndex_ == 0) {  // a new run
        totalScore_ = 0;
        totalTime_ = 0.0f;
    }
    retries_ = 0;
    restartLevel();
    setScreen(Screen::Briefing);
}

// Score = 1000 for clearing + time bonus + stealth bonus - retry penalty.
void Game::finishLevel() {
    audio_.playCue(SoundCue::Complete);
    ui::LevelResult r;
    r.time = levelTime_;
    r.timeBonus = static_cast<int>(600 * remainingTime() / level_.timeLimit);
    r.stealthBonus = !spotted_ ? 500 : (!alarmed_ ? 250 : 0);
    r.retryPenalty = retries_ * 100;
    r.levelScore = std::max(0, 1000 + r.timeBonus + r.stealthBonus - r.retryPenalty);
    totalScore_ += r.levelScore;
    totalTime_ += levelTime_;
    r.totalScore = totalScore_;
    r.totalTime = totalTime_;
    result_ = r;
    if (levelIndex_ + 1 == levelCount()) {
        if (rankedRun_) {
            bool saved = scoreboard_.record({playerName_, totalScore_, totalTime_});
            scoreStatus_ = saved ? "Run saved. The best 10 runs appear on the scoreboard."
                                 : "Could not save to disk. Scores are kept for this session.";
        } else scoreStatus_ = "Practice run - not entered on the scoreboard.";
    }
    setScreen(levelIndex_ + 1 < levelCount() ? Screen::LevelComplete : Screen::Victory);
}

void Game::restartLevel() {
    level_ = buildLevel(levelIndex_);
    player_.reset(level_.spawn);
    guards_.clear();
    for (const GuardConfig& gc : level_.guards) guards_.emplace_back(gc);
    catcher_ = -1;
    levelTime_ = 0.0f;
    awaitingDeparture_ = true;
    spotted_ = alarmed_ = false;
    collected_.assign(level_.checkpoints.size(), false);
    pickupTime_ = petTime_ = alertCueCooldown_ = 0;
    petCat_ = -1;
    distractions_ = 2;
    distractionCooldown_ = noiseTime_ = radioCooldown_ = 0;
    message_ = level_.objective + ". J: view your case file. F: collect evidence.";
    messageTime_ = 5;
    crowdRoutes_.clear();
    const Vec2 endpoints[][2] = {
        {{100, 200}, {900, 200}},
        {{200, 150}, {800, 150}},
        {{150, 300}, {850, 300}},
        {{300, 100}, {700, 100}},
        {{400, 350}, {600, 350}},
        {{500, 240}, {500, 100}},
    };
    for (const auto& route : endpoints) {
        auto path = navigation::findPath(route[0], route[1], Player::RADIUS, level_.map);
        if (!path.empty()) { path.insert(path.begin(), route[0]); crowdRoutes_.push_back(std::move(path)); }
    }
    catRoutes_.clear();
    catClocks_.clear();
    catAffection_.clear();
    const Vec2 catEndpoints[][2] = {
        {{150, 300}, {350, 310}},
        {{600, 150}, {600, 100}},
        {{400, 220}, {450, 180}},
        {{750, 300}, {800, 280}}
    };
    for (const auto& route : catEndpoints) {
        auto path = navigation::findPath(route[0], route[1], 6, level_.map);
        if (!path.empty()) { path.insert(path.begin(), route[0]); catRoutes_.push_back(std::move(path)); }
    }
    npcs_ = campusCharacters(levelIndex_);
    for (size_t i = 0; i < catRoutes_.size(); ++i) {
        catClocks_.push_back(float(i) * 2.7f);
        catAffection_.push_back(0);
    }
    setScreen(Screen::Playing);
    updateCamera(0.0f, true);
}

// Another attempt at the same level (counts against the level score).
void Game::retryLevel() {
    ++retries_;
    totalTime_ += levelTime_;
    restartLevel();
}

void Game::goToMenu() {
    levelIndex_ = 0;
    restartLevel();
    setScreen(Screen::MainMenu);
}

const ui::Items& Game::menuItems() const {
    switch (screen_) {
        case Screen::MainMenu: return MAIN_ITEMS;
        case Screen::NameEntry: return NAME_ITEMS;
        case Screen::Scoreboard: return INSTR_ITEMS;
        case Screen::Instructions: return INSTR_ITEMS;
        case Screen::Paused: return PAUSE_ITEMS;
        case Screen::Briefing: return BRIEF_ITEMS;
        case Screen::Dialogue: return TALK_ITEMS;
        case Screen::Caught: return CAUGHT_ITEMS;
        case Screen::TimedOut: return CAUGHT_ITEMS;
        case Screen::LevelComplete: return COMPLETE_ITEMS;
        case Screen::Victory: return VICTORY_ITEMS;
        default: return NO_ITEMS;
    }
}

void Game::activate(int item) {
    switch (screen_) {
        case Screen::MainMenu:
            if (item == 0) { nameDraft_ = playerName_; setScreen(Screen::NameEntry); }
            else if (item == 1) setScreen(Screen::Scoreboard);
            else if (item == 2) setScreen(Screen::Instructions);
            else quit_ = true;
            break;
        case Screen::NameEntry:
            if (item == 1) setScreen(Screen::MainMenu);
            else if (!Scoreboard::cleanName(nameDraft_).empty()) {
                playerName_ = Scoreboard::cleanName(nameDraft_);
                rankedRun_ = true;
                scoreStatus_.clear();
                startLevel(0);
            }
            break;
        case Screen::Scoreboard: goToMenu(); break;
        case Screen::Instructions: setScreen(Screen::MainMenu); break;
        case Screen::Briefing: setScreen(Screen::Playing); break;
        case Screen::Dialogue: advanceDialogue(); break;
        case Screen::Paused:
            if (item == 0) setScreen(Screen::Playing);
            else if (item == 1) retryLevel();
            else goToMenu();
            break;
        case Screen::Caught:
        case Screen::TimedOut:
            if (item == 0) retryLevel();
            else goToMenu();
            break;
        case Screen::LevelComplete:
            if (item == 0) startLevel(levelIndex_ + 1);
            else goToMenu();
            break;
        case Screen::Victory:
            if (item == 0) setScreen(Screen::Scoreboard);
            else goToMenu();
            break;
        case Screen::Playing: break;
    }
}

// ===========================================================================
// Update
// ===========================================================================
void Game::update(float dt) {
    uiTime_ += dt;
    screenTime_ += dt;
    switch (screen_) {
        case Screen::MainMenu:
        case Screen::NameEntry:
        case Screen::Scoreboard:
        case Screen::Instructions: {
            // Living background: guards patrol, the camera slowly pans.
            time_ += dt;
            for (Guard& g : guards_) g.update(dt, {-9999.0f, -9999.0f}, level_.map);
            camera_ = {(0.5f + 0.5f * std::sin(uiTime_ * 0.06f)) * (WORLD_W - VIEW_W),
                       (0.5f + 0.5f * std::sin(uiTime_ * 0.045f)) * (WORLD_H - VIEW_H)};
            break;
        }
        case Screen::Playing: updatePlaying(dt); break;
        case Screen::Dialogue: dialogueTime_ += dt; break;
        default: break;  // paused / result screens freeze the world
    }
}

void Game::updatePlaying(float dt) {
    time_ += dt;
    // Arrival is a ready state, not timed invulnerability: no patrol or clock
    // advances until the player deliberately starts this attempt.
    if (awaitingDeparture_) {
        if (input_.axis().length() == 0) return;
        awaitingDeparture_ = false;
    }
    levelTime_ += dt;
    distractionCooldown_ = std::max(0.0f, distractionCooldown_ - dt);
    noiseTime_ = std::max(0.0f, noiseTime_ - dt);
    radioCooldown_ -= dt;
    messageTime_ = std::max(0.0f, messageTime_ - dt);
#ifdef _WIN32
    // GLUT does not always report Shift on its own, so ask Windows directly.
    bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    input_.sprint = shiftDown || shiftHeld_[0] || shiftHeld_[1] || spaceHeld_;
#endif
    pickupTime_ = std::max(0.0f, pickupTime_ - dt);
    petTime_ = std::max(0.0f, petTime_ - dt);
    alertCueCooldown_ = std::max(0.0f, alertCueCooldown_ - dt);
    if (input_.axis().length() > 0) petTime_ = 0; // moving always cancels petting
    player_.update(dt, input_, level_.map);
    for (size_t i = 0; i < catRoutes_.size(); ++i) {
        if (catAffection_[i] <= 1.5f) catClocks_[i] += dt;
        catAffection_[i] = std::max(0.0f, catAffection_[i] - dt);
    }
    for (size_t i = 0; i < guards_.size(); ++i) {
        bool wasSeeing = guards_[i].isSeeing();
        guards_[i].update(dt, player_.position(), level_.map, player_.isSprinting());
        if (!wasSeeing && guards_[i].isSeeing() && alertCueCooldown_ <= 0) {
            audio_.playCue(SoundCue::Alert); alertCueCooldown_ = 4;
        }
        if (guards_[i].hasCaught() && catcher_ < 0) catcher_ = static_cast<int>(i);
        if (guards_[i].state() == GuardState::Alert && guards_[i].isSeeing()) spotted_ = true;
        if (guards_[i].detection() >= 0.6f) alarmed_ = true;
    }
    // Only an actual sighting is shared; hidden players are never tracked by radio.
    if (radioCooldown_ <= 0) for (const Guard& source : guards_) {
        if (!source.isSeeing() || source.detection() < 0.4f) continue;
        for (Guard& receiver : guards_)
            if ((source.position() - receiver.position()).length() < 480)
                receiver.receiveReport(player_.position());
        radioCooldown_ = 2.5f;
        message_ = "Security radio: detective spotted! Break sight and change direction.";
        messageTime_ = 2.5f;
        break;
    }
    updateCamera(dt, false);
    if (catcher_ >= 0) {
        setScreen(Screen::Caught);
        return;
    }
    if (remainingTime() <= 0) { setScreen(Screen::TimedOut); return; }
    if (level_.goal.expanded(Player::RADIUS).contains(player_.position())) {
        if (collected() == int(collected_.size())) finishLevel();
        else { message_ = "Case incomplete. Collect the remaining evidence. J: checklist."; messageTime_ = 1; }
    }
}

int Game::nearbyPass() const {
    for (size_t i = 0; i < collected_.size(); ++i)
        if (!collected_[i] && (level_.checkpoints[i] - player_.position()).length() < 28 &&
            collision::lineOfSight(player_.position(), level_.checkpoints[i], level_.map)) return int(i);
    return -1;
}

void Game::interact() {
    int pass = nearbyPass();
    if (pass >= 0) {
        awaitingDeparture_ = false;
        collected_[pass] = true;
        pickupPosition_ = level_.checkpoints[pass];
        pickupTime_ = 1.0f;
        audio_.playCue(SoundCue::Pickup);
        message_ = level_.passNames[pass] + " collected. " +
            (collected() == int(collected_.size()) ? "Case file ready! Follow the gold arrow." : "Check J for the remaining evidence.");
        messageTime_ = 4;
        return;
    }
    talk();
}

int Game::collected() const { return int(std::count(collected_.begin(), collected_.end(), true)); }

Vec2 Game::nextObjective() const {
    Vec2 target = level_.goal.center();
    float nearest = 1e9f;
    for (size_t i = 0; i < collected_.size(); ++i) if (!collected_[i]) {
        float distance = (level_.checkpoints[i] - player_.position()).length();
        if (distance < nearest) { nearest = distance; target = level_.checkpoints[i]; }
    }
    return target;
}

bool Game::mouseAimValid() const {
    if (!mouseKnown_) return false;
    Vec2 view = windowToView(mouseX_, mouseY_);
    return view.x >= 0 && view.x < VIEW_W && view.y >= PLAY_BOTTOM && view.y < PLAY_TOP;
}

Vec2 Game::distractionTarget() const {
    Vec2 direction;
    float reach = 178;  // preserve the keyboard-only throw distance
    switch (player_.facing()) {
        case Direction::Up: direction = {0, 1}; break;
        case Direction::Down: direction = {0, -1}; break;
        case Direction::Left: direction = {-1, 0}; break;
        case Direction::Right: direction = {1, 0}; break;
    }
    if (mouseAimValid()) {
        // Keep the pointer in window coordinates so scrolling and resizing do
        // not leave a stale world target. The HUD is outside the world viewport.
        Vec2 view = windowToView(mouseX_, mouseY_);
        Vec2 delta = camera_ + Vec2{view.x, view.y - PLAY_BOTTOM} - player_.position();
        if (delta.length() < 0.001f) return player_.position();
        direction = delta.normalized();
        reach = std::min(190.0f, delta.length());
    }
    float distance = std::max(0.0f, std::min(reach,
        collision::raycast(player_.position(), direction, reach + 12, level_.map) - 12));
    Vec2 target = collision::clampToWorld(player_.position() + direction * distance, 12);
    while (distance > 0 && collision::overlapsSolid(target, Guard::RADIUS, level_.map)) {
        distance = std::max(0.0f, distance - 10);
        target = collision::clampToWorld(player_.position() + direction * distance, 12);
    }
    return target;
}

void Game::throwDistraction() {
    if (screen_ != Screen::Playing || distractions_ <= 0 || distractionCooldown_ > 0) return;
    if (mouseKnown_ && !mouseAimValid()) return;
    awaitingDeparture_ = false;
    audio_.playCue(SoundCue::Distraction);
    --distractions_;
    noisePosition_ = distractionTarget();
    noiseTime_ = 2.5f;
    distractionCooldown_ = 1;
    for (Guard& guard : guards_) guard.hearNoise(noisePosition_, 310);
    message_ = "Clink! Nearby guards investigate the sound. Slip past while they turn.";
    messageTime_ = 2.5f;
}

int Game::nearbyCharacter() const {
    Vec2 facing;
    switch (player_.facing()) {
        case Direction::Up: facing = {0, 1}; break;
        case Direction::Down: facing = {0, -1}; break;
        case Direction::Left: facing = {-1, 0}; break;
        case Direction::Right: facing = {1, 0}; break;
    }
    int found = -1;
    float nearest = 65;
    auto consider = [&](Vec2 position, int index) {
        Vec2 delta = position - player_.position();
        float distance = delta.length();
        if (distance < nearest && (index < int(npcs_.size()) || distance < 38) && (distance < 15 || dot(delta.normalized(), facing) > 0.35f)
            && collision::lineOfSight(player_.position(), position, level_.map)) { nearest = distance; found = index; }
    };
    for (size_t i = 0; i < npcs_.size(); ++i) consider(npcs_[i].position, int(i));
    for (size_t i = 0; i < catRoutes_.size(); ++i)
        consider(catPose(i).position, int(npcs_.size() + i));
    return found;
}

void Game::talk() {
    int nearby = nearbyCharacter();
    if (nearby < 0) return;
    awaitingDeparture_ = false;
    dialogueCharacter_ = nearby;
    dialoguePage_ = 0;
    dialogueTime_ = 0;
    if (nearby < int(npcs_.size())) {
        CampusNpc& npc = npcs_[nearby];
        speaker_ = npc.name;
        speakerRole_ = npc.role;
        dialogue_.clear();
        const CampusRole contacts[] = {CampusRole::Topper, CampusRole::Professor, CampusRole::Seller};
        if (levelIndex_ < 3 && npc.kind == contacts[levelIndex_]) {
            std::string task = collected() == int(collected_.size())
                ? "You have everything. Take the evidence to the gold exit to close this case."
                : "I can help with your case file. Find ";
            if (collected() != int(collected_.size())) {
                bool first = true;
                for (size_t i = 0; i < collected_.size(); ++i) if (!collected_[i]) {
                    if (!first) task += " and ";
                    task += level_.passNames[i]; first = false;
                }
                task += ". Press F at each evidence stand.";
            }
            // A changed task gets its own interaction, never a preface to
            // several unrelated speeches.
            if (task != npc.lastTask) {
                dialogue_ = {task};
                npc.lastTask = task;
            }
        }
        if (dialogue_.empty()) {
            size_t count = 0;
            for (const auto& conversation : npc.conversations) count += conversation.size();
            if (count == 0) return;
            size_t line = size_t(npc.visits++) % count;
            for (const auto& conversation : npc.conversations) {
                if (line < conversation.size()) { dialogue_ = {conversation[line]}; break; }
                line -= conversation.size();
            }
        }
    } else {
        static const char* names[] = {"Biscuit", "Shadow", "Mishti", "Professor Meow"};
        int cat = nearby - int(npcs_.size());
        if (catAffection_[cat] > 1.5f) return;
        catAffection_[cat] = 3;
        petCat_ = cat;
        petTime_ = 0.85f;
        player_.stop();
        message_ = std::string(names[cat % 4]) + " leans into your hand. Prrrr...";
        messageTime_ = 3;
        audio_.playCue(SoundCue::Pet);
        return;
    }
    setScreen(Screen::Dialogue);
}

void Game::advanceDialogue() {
    if (dialogue_.empty()) { setScreen(Screen::Playing); return; }
    if (dialogueTime_ * 42 < dialogue_[dialoguePage_].size()) {
        dialogueTime_ = float(dialogue_[dialoguePage_].size()) / 42 + 1;
    } else if (dialoguePage_ + 1 < dialogue_.size()) {
        ++dialoguePage_;
        dialogueTime_ = 0;
    } else setScreen(Screen::Playing);
}

CampusCatPose Game::catPose(size_t index) const {
    float clock = catClocks_[index];
    float phase = std::fmod(clock, 12.0f);
    float distance = (std::floor(clock / 12) * 8 + std::min(phase, 8.0f)) * 18;
    auto [position, direction] = routePose(catRoutes_[index], distance);
    bool petting = catAffection_[index] > 1.5f;
    return {position, petting ? player_.position() - position : direction,
            phase < 8 && !petting, catAffection_[index] > 0};
}

// The camera follows the detective smoothly and never shows outside the museum.
void Game::updateCamera(float dt, bool snap) {
    Vec2 p = player_.position();
    Vec2 target{std::clamp(p.x - VIEW_W * 0.5f, 0.0f, WORLD_W - VIEW_W),
                std::clamp(p.y - PLAY_H * 0.45f, 0.0f, WORLD_H - PLAY_H)};
    if (snap) camera_ = target;
    else camera_ += (target - camera_) * std::min(1.0f, dt * 6.0f);
}

void Game::reshape(int w, int h) { setupProjection(w, h); }

// ===========================================================================
// Render
// ===========================================================================
void Game::renderWorld(bool showPlayer) {
    // World space: the camera is a translation of the whole scene.
    pushTransform(-camera_.x, -camera_.y);

    level_.map.drawGround(time_);
    if (showPlayer) drawGoalMarker();
    for (const Guard& g : guards_) g.drawVisionCone(level_.map);  // clipped by walls

    // Upright objects + characters sorted back (high y) to front (low y).
    std::vector<DepthItem> items;
    level_.map.collectDepthItems(time_, items);
    for (const Rect& field : level_.map.footballCourts) {
        Vec2 ball = field.center() + Vec2{std::sin(time_ * 0.65f) * 115, std::cos(time_ * 0.9f) * 45};
        items.push_back({ball.y, [ball] {
            drawCharacterShadow(ball, 0.35f);
            setColor(hex(0xffffff)); circle(ball.x, ball.y + 4, 5);
            setColor(hex(0x293544)); circle(ball.x, ball.y + 4, 2);
            circle(ball.x - 3, ball.y + 6, 1); circle(ball.x + 3, ball.y + 2, 1);
        }});
        for (int i = 0; i < 4; ++i) {
            Vec2 position = field.center() + Vec2{std::sin(time_ * 0.65f + i * 0.5f) * 100,
                                                  std::cos(time_ * 0.9f + i * 1.7f) * 55};
            items.push_back({position.y, [this, position, ball, i] {
                CharacterStyle style = studentStyle();
                style.backpack = false;
                style.shirt = i % 2 ? hex(0xc74f49) : hex(0xf0d573);
                style.shirtShade = darker(style.shirt, 0.2f);
                drawCharacterShadow(position, 0.9f);
                pushTransform(position.x, position.y, 0, 0.95f, 0.95f);
                Vec2 facing = ball - position;
                drawCharacter(style, directionFromAngle(radToDeg(std::atan2(facing.y, facing.x))), time_ * 11 + i, 0.9f);
                popTransform();
            }});
        }
    }
    for (size_t i = 0; i < crowdRoutes_.size(); ++i) {
        auto [position, direction] = routePose(crowdRoutes_[i], time_ * (36 + (i % 3) * 7) + i * 137);
        items.push_back({position.y, [this, position, direction, i] {
            CharacterStyle style = studentStyle();
            static const uint32_t shirts[] = {0xc57462, 0x8b9b68, 0x8b79aa, 0xd3b772, 0x67a5a1};
            style.shirt = hex(shirts[i % 5]); style.shirtShade = darker(style.shirt, 0.2f);
            style.backpack = i % 2 == 0;
            drawCharacterShadow(position, 0.9f);
            pushTransform(position.x, position.y, 0, 0.92f, 0.92f);
            drawCharacter(style, directionFromAngle(radToDeg(std::atan2(direction.y, direction.x))), time_ * 7 + i, 0.7f);
            popTransform();
        }});
    }
    for (const CampusNpc& npc : npcs_) items.push_back({npc.position.y, [this, &npc] { drawCampusNpc(npc, time_); }});
    for (size_t i = 0; i < catRoutes_.size(); ++i) {
        CampusCatPose pose = catPose(i);
        items.push_back({pose.position.y, [this, pose, i] { drawCampusCat(pose, time_, int(i)); }});
    }
    if (showPlayer) items.push_back({player_.position().y, [this] {
        player_.draw(petTime_ > 0 ? std::sin(PI_F * (1 - petTime_ / 0.85f)) : 0);
        if (petTime_ > 0 && petCat_ >= 0) {
            Vec2 hand = catPose(petCat_).position + Vec2{0, 13 + std::sin(time_ * 18) * 1.5f};
            Vec2 shoulder = player_.position() + Vec2{6, 13};
            setColor(studentStyle().skin);
            thickLine(shoulder.x, shoulder.y, hand.x, hand.y, 3);
            circle(hand.x, hand.y, 2.5f);
        }
    }});
    for (const Guard& g : guards_) items.push_back({g.position().y, [&g] { g.draw(); }});
    std::stable_sort(items.begin(), items.end(), [](const DepthItem& a, const DepthItem& b) { return a.y > b.y; });
    for (const DepthItem& it : items) it.draw();

    level_.map.drawCanopies(time_);  // tree tops, lamp light, clouds

    if (showDebug_) {
        level_.map.drawCollisionDebug();
        player_.drawCollisionDebug();
        for (const Guard& g : guards_) g.drawRouteDebug();
    }
    if (showPlayer) {
        for (const CampusNpc& npc : npcs_) if ((npc.position - player_.position()).length() < 170) {
            setColor(hex(0x1d2430, 0.9f));
            float w = textWidth(npc.name, Font::Small) + 14;
            roundedRect(npc.position.x - w / 2, npc.position.y + 49, w, 20, 5);
            setColor(hex(0xffe5a0)); textCentered(npc.position.x, npc.position.y + 55, npc.name, Font::Small);
        }
        for (const Guard& g : guards_) g.drawIndicator(time_);
        if (player_.isSprinting()) {  // footstep "noise" rings
            float k = std::fmod(time_ * 2.5f, 1.0f);
            setColor(hex(0xffffff, 0.35f * (1.0f - k)));
            ellipseOutline(player_.position().x, player_.position().y, 12 + k * 30, 6 + k * 15, 1.5f);
        }
        if (noiseTime_ > 0) {
            setColor(hex(0xffd23f, noiseTime_ / 2.5f));
            circleOutline(noisePosition_.x, noisePosition_.y, 12 + (2.5f - noiseTime_) * 42, 2);
            textCentered(noisePosition_.x, noisePosition_.y + 15, "CLINK!", Font::Small);
        }
        if (distractions_ > 0 && screen_ == Screen::Playing && (!mouseKnown_ || mouseAimValid())) {
            Vec2 target = distractionTarget();
            setColor(distractionCooldown_ > 0 ? hex(0xb5b9b9, 0.6f) : hex(0xffe5a0, 0.8f));
            circleOutline(target.x, target.y, 8, 1);
            line(target.x - 12, target.y, target.x - 5, target.y, 1);
            line(target.x + 5, target.y, target.x + 12, target.y, 1);
            line(target.x, target.y - 12, target.x, target.y - 5, 1);
            line(target.x, target.y + 5, target.x, target.y + 12, 1);
            textCentered(target.x, target.y + 16, distractionCooldown_ > 0 ? "WAIT" : mouseAimValid() ? "CLICK" : "E", Font::Small);
        }
    }
    if (showPlayer && pickupTime_ > 0) {
        float k = 1 - pickupTime_;
        for (int i = 0; i < 8; ++i) {
            float a = i * PI_F / 4;
            setColor(hex(0xe6c779, pickupTime_));
            circle(pickupPosition_.x + std::cos(a) * (10 + k * 32),
                   pickupPosition_.y + 10 + std::sin(a) * (6 + k * 20), 2 * pickupTime_);
        }
        setColor(hex(0xfff0bf, pickupTime_));
        textCentered(pickupPosition_.x, pickupPosition_.y + 38 + k * 25, "COLLECTED", Font::Small);
    }
    if (catcher_ >= 0) guards_[catcher_].drawSpeechBubble(playerName_);
    popTransform();
}

void Game::render() {
    beginFrame(hex(0x1d2430));
    const bool inMenu = screen_ == Screen::MainMenu || screen_ == Screen::Instructions ||
                        screen_ == Screen::NameEntry || screen_ == Screen::Scoreboard;
    if (!inMenu) worldViewport(true);
    renderWorld(!inMenu);
    if (!inMenu) {
        if (screen_ == Screen::Playing) drawGoalArrow();
        worldViewport(false);
    }

    // Screen space (VIEW_W x VIEW_H): UI.
    if (!inMenu && screen_ != Screen::Victory) {
        ui::HudInfo h;
        h.levelName = level_.name;
        h.objective = level_.objective;
        h.levelNumber = levelIndex_ + 1;
        h.levelCount = levelCount();
        for (const Guard& g : guards_) {
            h.detection = std::max(h.detection, g.detection());
            h.anyAlert = h.anyAlert || g.state() == GuardState::Alert;
        }
        h.stamina = player_.stamina();
        h.sprinting = player_.isSprinting();
        h.levelTime = levelTime_;
        h.score = totalScore_;
        h.playerName = playerName_.empty() ? "Practice" : playerName_;
        h.timeLeft = remainingTime();
        h.passes = collected(); h.passesNeeded = int(collected_.size()); h.distractions = distractions_;
        h.message = messageTime_ > 0 ? message_ : "Keep the detection meter from filling. Sprint to escape; hide to lose pursuit. J: case file.";
        if (awaitingDeparture_) h.message = "READY WHEN YOU ARE - move or interact to begin. Patrols and the clock are paused.";
        int nearby = nearbyCharacter();
        if (nearby >= 0) h.talkPrompt = nearby < int(npcs_.size()) ? "F: Talk to " + npcs_[nearby].name : "F: Pet the cat";
        int pass = nearbyPass();
        if (pass >= 0) h.talkPrompt = "F: Take " + level_.passNames[pass];
        ui::drawHud(h, uiTime_);
    }
    const ui::Items& items = menuItems();
    ui::beginFrame();
    switch (screen_) {
        case Screen::MainMenu: ui::drawMainMenu(items, selected_, uiTime_); break;
        case Screen::NameEntry: ui::drawNameEntry(nameDraft_, items, selected_, uiTime_); break;
        case Screen::Scoreboard: ui::drawScoreboard(scoreboard_.entries(), scoreStatus_, items, selected_, uiTime_); break;
        case Screen::Instructions: ui::drawInstructions(items, selected_, uiTime_); break;
        case Screen::Briefing: ui::drawBriefing(level_, levelIndex_, collected_, items, uiTime_); break;
        case Screen::Paused: ui::drawPauseMenu(items, selected_, uiTime_); break;
        case Screen::Dialogue:
            ui::drawDialogue(speaker_, speakerRole_, dialogue_[dialoguePage_], dialoguePage_, dialogue_.size(),
                size_t(dialogueTime_ * 42), dialogueCharacter_ < int(npcs_.size()) ? &npcs_[dialogueCharacter_] : nullptr,
                dialogueCharacter_ - int(npcs_.size()), uiTime_);
            break;
        case Screen::Caught: ui::drawGameOver(items, selected_, screenTime_); break;
        case Screen::TimedOut: ui::drawTimeout(items, selected_, screenTime_); break;
        case Screen::LevelComplete: ui::drawLevelComplete(level_.name, result_, items, selected_, screenTime_, level_.reward,
            buildLevel(levelIndex_ + 1).objective); break;
        case Screen::Victory: ui::drawVictory(result_, items, selected_, screenTime_, scoreStatus_); break;
        case Screen::Playing: break;
    }
}

void Game::drawGoalMarker() const {
    for (size_t i = 0; i < collected_.size(); ++i) if (!collected_[i]) {
        Vec2 p = level_.checkpoints[i];
        setColor(hex(0x63a9b0, 0.18f)); ellipse(p.x, p.y, 22, 12);
        setColor(hex(0xb7ded4)); ellipseOutline(p.x, p.y, 22, 12, 1.5f);
        // A paper folder on a low campus document stand, grounded in the scene.
        setColor(hex(0x766552)); roundedRect(p.x - 13, p.y + 2, 26, 16, 2);
        setColor(hex(0xb69c75)); rect(p.x - 12, p.y + 6, 24, 13);
        setColor(hex(0xfff1cf)); rect(p.x - 7, p.y + 9, 14, 14);
        setColor(hex(0x568c96)); rect(p.x - 5, p.y + 18, 10, 2);
        setColor(hex(0xaaa08a)); line(p.x - 4, p.y + 15, p.x + 4, p.y + 15); line(p.x - 4, p.y + 12, p.x + 3, p.y + 12);
        if ((p - player_.position()).length() < 140) {
            std::string label = level_.passNames[i];
            float w = textWidth(label, Font::Small) + 14;
            setColor(hex(0x26353d, 0.92f)); roundedRect(p.x - w / 2, p.y + 31, w, 19, 4);
            setColor(hex(0xffedc3)); textCentered(p.x, p.y + 37, label, Font::Small);
        }
    }
    // Pulsing goal ring at the target entrance (scale animation).
    Vec2 c = level_.goal.center();
    float pulse = 1.0f + 0.15f * std::sin(time_ * 3.0f);
    pushTransform(c.x, c.y - 10.0f, 0.0f, pulse, pulse);
    softEllipse(0, 0, 30, 16, hex(0xffe066, 0.45f), hex(0xffe066, 0.0f));
    setColor(hex(0xffd23f, 0.9f));
    ellipseOutline(0, 0, 22, 11, 2.0f);
    popTransform();
    setColor(hex(0xffe5a0));
    textCentered(c.x, c.y + 27, collected() == int(collected_.size()) ? "EXIT OPEN" : "NEEDS ALL PASSES", Font::Small);
}

// When the goal is off screen, an arrow at the screen edge points to it.
void Game::drawGoalArrow() const {
    Vec2 g = nextObjective() - camera_;  // nearest uncollected pass, then exit
    const float m = 32.0f;
    if (g.x > m && g.x < VIEW_W - m && g.y > m && g.y < PLAY_H - m) return;
    Vec2 c{VIEW_W * 0.5f, PLAY_H * 0.5f};
    Vec2 d = (g - c).normalized();
    // Walk from the centre along d until hitting the inset screen border
    // (kept below the HUD panels at the top).
    float tx = d.x != 0 ? ((d.x > 0 ? VIEW_W - m : m) - c.x) / d.x : 1e9f;
    float ty = d.y != 0 ? ((d.y > 0 ? PLAY_H - m : m) - c.y) / d.y : 1e9f;
    Vec2 p = c + d * std::min(tx, ty);
    float pulse = 1.0f + 0.12f * std::sin(uiTime_ * 5.0f);
    pushTransform(p.x, p.y, radToDeg(std::atan2(d.y, d.x)), pulse, pulse);
    setColor(hex(0x1d2430, 0.35f));
    triangle({16, -3}, {-8, -15}, {-8, 9});
    setColor(collected() == int(collected_.size()) ? hex(0xffd23f) : hex(0x63efe5));
    triangle({14, 0}, {-10, -12}, {-10, 12});
    setColor(hex(0x8a6a10));
    polygonOutline({{14, 0}, {-10, -12}, {-10, 12}}, 1.5f);
    popTransform();
}

// ===========================================================================
// Input
// ===========================================================================
void Game::mouseMove(int px, int py) {
    mouseX_ = px; mouseY_ = py; mouseKnown_ = true;
    if (screen_ == Screen::Playing) return;
    int i = ui::buttonAt(windowToView(px, py));
    if (i >= 0) selected_ = i;
}

void Game::mouseClick(int px, int py) {
    mouseX_ = px; mouseY_ = py; mouseKnown_ = true;
    if (screen_ == Screen::Playing) {
        if (mouseAimValid()) throwDistraction();
        return;
    }
    if (screen_ == Screen::Dialogue) { advanceDialogue(); return; }
    int i = ui::buttonAt(windowToView(px, py));
    if (i >= 0) activate(i);
}

void Game::refreshInput() {
    input_.up = letterHeld_[0] || arrowHeld_[0];
    input_.down = letterHeld_[1] || arrowHeld_[1];
    input_.left = letterHeld_[2] || arrowHeld_[2];
    input_.right = letterHeld_[3] || arrowHeld_[3];
    input_.sprint = shiftHeld_[0] || shiftHeld_[1] || spaceHeld_;
}

void Game::keyDown(unsigned char key) {
    if (screen_ == Screen::NameEntry) {
        if (key == 27) setScreen(Screen::MainMenu);
        else if (key == 13) activate(selected_);
        else if (key == 8 && !nameDraft_.empty()) nameDraft_.pop_back();
        else if (nameDraft_.size() < 18 && ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') ||
                 (key >= '0' && key <= '9') || key == ' ' || key == '-' || key == '_')) nameDraft_ += key;
        return;
    }
    if (screen_ == Screen::Dialogue) {
        if (key == 27) setScreen(Screen::Playing);
        else if (key == 'f' || key == 'F' || key == 13 || key == ' ') advanceDialogue();
        return;
    }
    int d = letterToDir(key);
    if (d >= 0) {
        letterHeld_[d] = true;
        refreshInput();
    }
    const int n = static_cast<int>(menuItems().size());
    if (screen_ != Screen::Playing && n > 0) {
        // Menu navigation (W/S, or A/D for side-by-side buttons).
        if (d == 0 || d == 2) selected_ = (selected_ + n - 1) % n;
        if (d == 1 || d == 3) selected_ = (selected_ + 1) % n;
        if (key == 13 || (key == ' ' && screen_ != Screen::Playing)) {
            activate(selected_);
            return;
        }
    }
    switch (key) {
        case 27:
            if (screen_ == Screen::MainMenu) quit_ = true;
            else if (screen_ == Screen::Instructions) setScreen(Screen::MainMenu);
            else if (screen_ == Screen::Briefing) setScreen(Screen::Playing);
            else goToMenu();
            break;
        case 'p': case 'P':
            if (screen_ == Screen::Playing) setScreen(Screen::Paused);
            else if (screen_ == Screen::Paused) setScreen(Screen::Playing);
            break;
        case 'r': case 'R':
            if (screen_ == Screen::Playing || screen_ == Screen::Paused || screen_ == Screen::Caught || screen_ == Screen::TimedOut) retryLevel();
            break;
        case 'e': case 'E': throwDistraction(); break;
        case 'f': case 'F': if (screen_ == Screen::Playing) interact(); break;
        case 'j': case 'J':
            if (screen_ == Screen::Playing) setScreen(Screen::Briefing);
            else if (screen_ == Screen::Briefing) setScreen(Screen::Playing);
            break;
        case 'g': case 'G': showDebug_ = !showDebug_; break;
        case ' ':  // Space = alternative sprint key
            spaceHeld_ = true;
            refreshInput();
            break;
        default: break;
    }
}

void Game::keyUp(unsigned char key) {
    if (key == ' ') {
        spaceHeld_ = false;
        refreshInput();
    }
    int d = letterToDir(key);
    if (d >= 0) {
        letterHeld_[d] = false;
        refreshInput();
    }
}

void Game::specialDown(int key) {
    if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R) {
        shiftHeld_[key == GLUT_KEY_SHIFT_R] = true;
        refreshInput();
        return;
    }
    int d = arrowToDir(key);
    if (d < 0) return;
    arrowHeld_[d] = true;
    refreshInput();
    const int n = static_cast<int>(menuItems().size());
    if (screen_ != Screen::Playing && n > 0) {
        if (d == 0 || d == 2) selected_ = (selected_ + n - 1) % n;
        else selected_ = (selected_ + 1) % n;
    }
}

void Game::specialUp(int key) {
    if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R) {
        shiftHeld_[key == GLUT_KEY_SHIFT_R] = false;
        refreshInput();
        return;
    }
    int d = arrowToDir(key);
    if (d >= 0) {
        arrowHeld_[d] = false;
        refreshInput();
    }
}
