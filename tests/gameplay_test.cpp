#include "../src/Game.h"
#include "../src/Navigation.h"
#include "../src/Collision.h"
#include <cstdio>
#include <fstream>
#include <limits>

struct GameplayTestAccess {
    static void safe(Game& g) { g.guards_.clear(); }
    static void expire(Game& g) { g.levelTime_ = g.level_.timeLimit - 0.01f; }
    static int score(const Game& g) { return g.totalScore_; }
    static float totalTime(const Game& g) { return g.totalTime_; }
    static void board(Game& g, const std::filesystem::path& p) { g.scoreboard_ = Scoreboard(p); }
    static const Scoreboard& board(const Game& g) { return g.scoreboard_; }
    static Vec2 position(const Game& g) { return g.player_.position(); }
    static Vec2 aim(const Game& g) { return g.distractionTarget(); }
    static Vec2 noise(const Game& g) { return g.noisePosition_; }
    static Vec2 camera(const Game& g) { return g.camera_; }
    static size_t crowds(const Game& g) { return g.crowdRoutes_.size(); }
    static bool spotted(const Game& g) { return g.spotted_; }
    static const std::vector<Guard>& guards(const Game& g) { return g.guards_; }
    static const std::vector<std::string>& dialogue(const Game& g) { return g.dialogue_; }
    static void collectFirst(Game& g) { g.collected_[0] = true; }
    static size_t page(const Game& g) { return g.dialoguePage_; }
    static float reveal(const Game& g) { return g.dialogueTime_; }
    static CampusCatPose cat(const Game& g, size_t i) { return g.catPose(i); }
    static size_t cats(const Game& g) { return g.catRoutes_.size(); }
};
int failures = 0;
void check(bool result, const char* label) {
    std::printf("  %s %s\n", result ? "PASS" : "FAIL", label);
    if (!result) ++failures;
}
void run(Game& g, float seconds) {
    for (int i = 0; i < int(seconds * 120); ++i) g.update(1.0f / 120);
}

int main() {
    std::filesystem::path path = "build/scoreboard_test.txt";
    std::filesystem::remove(path);
    Scoreboard board(path);
    board.load();
    check(board.entries().empty(), "missing score file starts empty");
    check(board.record({"  Rafi  ", 1000, 80}), "score saves to disk");
    check(board.record({"Nila", 1000, 60}), "score file can be replaced");
    Scoreboard reloaded(path); reloaded.load();
    check(reloaded.entries().size() == 2 && reloaded.entries()[0].name == "Nila" &&
          reloaded.entries()[1].name == "Rafi", "reload preserves names and ranks ties by fastest time");
    check(!board.record({"  ", 5000, 1}), "blank name is rejected");
    check(!board.record({"Invalid", 1, std::numeric_limits<float>::infinity()}), "invalid time is rejected");
    for (int i = 0; i < 14; ++i) board.record({"Player " + std::to_string(i), 2000 + i, float(50 + i)});
    check(board.entries().size() == 10 && board.entries()[0].score == 2013, "top ten are retained in score order");
    { std::ofstream file(path, std::ios::app); file << "broken record\n-12 5 \"Bad\"\n40 -2 \"Bad\"\n"; }
    reloaded.load();
    check(reloaded.entries().size() == 10, "malformed score rows are ignored");
    Scoreboard unwritable("build/no-such-directory/scores.txt");
    check(!unwritable.record({"Player", 100, 1}) && unwritable.entries().size() == 1, "save failure keeps a session score");

    Map obstacle;
    obstacle.walls.push_back({400, 200, 120, 400}); obstacle.finalize();
    auto route = navigation::findPath({300, 400}, {620, 400}, 9, obstacle);
    bool safe = !route.empty(); Vec2 previous{300, 400}; float distance = 0;
    for (Vec2 p : route) { safe &= navigation::clearPath(previous, p, 9, obstacle); distance += (p - previous).length(); previous = p; }
    check(safe && distance > 500, "pathfinding routes around a wall with collision clearance");
    check(navigation::findPath({300, 400}, {450, 400}, 9, obstacle).empty(), "unreachable target inside a wall produces no route");
    GuardConfig cfg; cfg.route = {{{300, 400}, 0}, {{620, 400}, 0}}; cfg.speed = 100;
    Guard guard(cfg);
    bool crossed = false;
    for (int i = 0; i < 1600; ++i) {
        guard.update(1.0f / 120, {1900, 1100}, obstacle);
        crossed |= guard.position().x > 570;
        safe &= !collision::overlapsSolid(guard.position(), 9, obstacle);
    }
    check(crossed && safe, "guard actually follows a path around an obstacle");
    Guard responder(cfg); responder.receiveReport({330, 500});
    check(responder.state() == GuardState::Alert && responder.detection() == 0, "radio shares an investigation point without magical detection");
    for (int i = 0; i < 1800; ++i) responder.update(1.0f / 120, {1900, 1100}, obstacle);
    check(responder.state() != GuardState::Alert, "guard gives up a stale sighting and resumes patrol");

    Game game; game.init(); GameplayTestAccess::board(game, path);
    check(GameplayTestAccess::crowds(game) == 12 && GameplayTestAccess::cats(game) == 4, "all walking students and cats have valid routes");
    game.keyDown(13);
    check(game.screen() == Screen::NameEntry, "start asks for player name");
    game.keyDown(13);
    check(game.screen() == Screen::NameEntry, "empty player name cannot start");
    for (char c : std::string("Wasd Player")) game.keyDown(c);
    game.keyDown(13);
    check(game.screen() == Screen::Briefing && game.playerName() == "Wasd Player", "named run starts with its purpose and checklist");
    float briefTime = game.remainingTime(); run(game, 2);
    check(game.remainingTime() == briefTime, "reading the application does not consume time");
    game.keyDown(13);
    check(game.screen() == Screen::Playing, "briefing continues into gameplay");
    GameplayTestAccess::safe(game);
    Vec2 spawn = GameplayTestAccess::position(game); run(game, 0.5f);
    check((GameplayTestAccess::position(game) - spawn).length() < 0.01f, "name entry does not leave movement keys held");
    game.keyDown('e'); game.keyDown('e');
    check(game.distractions() == 1, "distraction has a cooldown");
    run(game, 1.1f); game.keyDown('e'); run(game, 1.1f); game.keyDown('e');
    check(game.distractions() == 0, "only two distractions per attempt");
    game.placePlayer({315, 855}); game.keyDown('f');
    check(game.screen() == Screen::Dialogue, "facing nearby topper opens dialogue");
    float beforeTalk = game.remainingTime(); run(game, 5);
    check(game.remainingTime() == beforeTalk, "talking pauses the countdown");
    std::string taskSpeech = GameplayTestAccess::dialogue(game).front();
    check(GameplayTestAccess::dialogue(game).size() == 1, "task advice is a single speech, without appended conversations");
    game.keyDown('f');
    check(game.screen() == Screen::Playing, "finishing one NPC speech returns directly to campus");
    game.keyDown('f');
    std::string firstSpeech = GameplayTestAccess::dialogue(game).front();
    check(firstSpeech != taskSpeech && GameplayTestAccess::dialogue(game).size() == 1, "talking again starts one new speech instead of repeating the task preface");
    game.keyDown(13);
    check(game.screen() == Screen::Dialogue && GameplayTestAccess::reveal(game) > 1, "Enter reveals the current speech without skipping it");
    game.keyDown(13);
    check(game.screen() == Screen::Playing, "Enter closes the completed speech");
    game.keyDown('f');
    check(GameplayTestAccess::dialogue(game).front() != firstSpeech && GameplayTestAccess::dialogue(game).size() == 1,
          "each fresh interaction delivers only the next speech");
    game.keyDown(' '); game.keyDown(' ');
    check(game.screen() == Screen::Playing, "Space reveals then closes one speech");
    check(game.remainingTime() == beforeTalk, "reading NPC speeches keeps the countdown paused");
    GameplayTestAccess::collectFirst(game);
    game.keyDown('f');
    check(GameplayTestAccess::dialogue(game).size() == 1 && GameplayTestAccess::dialogue(game).front() != taskSpeech &&
          GameplayTestAccess::dialogue(game).front().find("Gate application") == std::string::npos,
          "changed task progress produces one updated speech about the remaining document");
    game.keyDown(27);
    game.placePlayer({315, 945}); game.keyDown('f');
    check(game.screen() == Screen::Playing, "cannot talk while facing away");
    auto cat = GameplayTestAccess::cat(game, 0);
    game.placePlayer(cat.position + Vec2{0, -28}); run(game, 0.5f);
    check((GameplayTestAccess::cat(game, 0).position - cat.position).length() > 1, "cats keep wandering when merely approached");
    cat = GameplayTestAccess::cat(game, 0);
    game.placePlayer(cat.position + Vec2{0, -22});
    game.keyDown('f');
    check(game.screen() == Screen::Playing && GameplayTestAccess::cat(game, 0).happy, "petting reacts in the world without opening dialogue");
    Vec2 stopped = GameplayTestAccess::cat(game, 0).position;
    float beforePet = game.remainingTime(); run(game, 0.4f);
    check(game.remainingTime() < beforePet && (GameplayTestAccess::cat(game, 0).position - stopped).length() < 0.01f,
          "cat pauses for petting while gameplay keeps running");
    game.keyDown('a'); run(game, 0.15f); game.keyUp('a');
    check((GameplayTestAccess::position(game) - (cat.position + Vec2{0, -22})).length() > 5, "movement immediately cancels the pet interaction");
    game.placePlayer({140, 40}); run(game, 3);
    check((GameplayTestAccess::cat(game, 0).position - stopped).length() > 1, "cat resumes wandering after petting");
    game.keyDown('j'); float journalTime = game.remainingTime(); run(game, 2);
    check(game.screen() == Screen::Briefing && game.remainingTime() == journalTime, "J reopens the paused application checklist");
    game.keyDown(27); check(game.screen() == Screen::Playing, "Escape returns from the checklist");
    game.keyDown('p'); float paused = game.remainingTime(); run(game, 2);
    check(game.remainingTime() == paused, "pause freezes countdown"); game.keyDown('p');
    GameplayTestAccess::expire(game); run(game, 0.1f);
    check(game.screen() == Screen::TimedOut, "countdown expiration ends attempt");
    game.keyDown('r'); GameplayTestAccess::safe(game);
    check(game.screen() == Screen::Playing && game.collected() == 0 && game.distractions() == 2, "retry resets passes, clock and distractions");
    check(GameplayTestAccess::totalTime(game) >= 54, "failed attempt time remains in run total");
    for (int level = 0; level < levelCount(); ++level) {
        Level l = buildLevel(level);
        GameplayTestAccess::safe(game);
        game.placePlayer(l.goal.center()); run(game, 0.1f);
        check(game.screen() == Screen::Playing, "exit cannot bypass required passes");
        for (Vec2 p : l.checkpoints) {
            game.placePlayer(p); int before = game.collected(); run(game, 0.8f);
            check(game.collected() == before, "documents require a deliberate interaction");
            game.keyDown('f');
            check(game.collected() == before + 1, "F picks up a nearby document immediately");
            game.keyDown('f');
            check(game.collected() == before + 1, "a document cannot be picked up twice");
            if (game.screen() == Screen::Dialogue) game.keyDown(27);
        }
        check(game.collected() == int(l.checkpoints.size()), "all passes can be collected");
        game.placePlayer(l.goal.center()); run(game, 0.1f);
        int score = GameplayTestAccess::score(game); run(game, 1);
        check(GameplayTestAccess::score(game) == score, "result screen cannot award score twice");
        if (level < levelCount() - 1) {
            check(game.screen() == Screen::LevelComplete, "passes unlock level completion"); game.keyDown(13);
            check(game.screen() == Screen::Briefing, "next approval stage shows its purpose");
            check((GameplayTestAccess::position(game) - l.goal.center()).length() < 25, "next stage continues from the previous destination");
            game.keyDown(13);
        }
    }
    check(game.screen() == Screen::Victory && GameplayTestAccess::board(game).entries().size() == 1,
          "complete named run is saved exactly once");
    reloaded.load();
    check(reloaded.entries().size() == 1 && reloaded.entries()[0].name == "Wasd Player", "full run score persists across reload");
    game.keyDown(13); check(game.screen() == Screen::Scoreboard, "victory opens scoreboard");
    // Test actual arrival and retry states with live guards, including level 3.
    for (int level = 0; level < levelCount(); ++level) {
        Game arrival; arrival.startAtLevel(level);
        float fullTime = arrival.remainingTime();
        std::vector<Vec2> guardStarts;
        bool clear = true;
        for (const Guard& guard : GameplayTestAccess::guards(arrival)) {
            guardStarts.push_back(guard.position());
            clear &= (guard.position() - GameplayTestAccess::position(arrival)).length() > 300;
        }
        check(clear, "every level begins with guards well away from the arrival point");
        run(arrival, 20);
        bool waiting = arrival.screen() == Screen::Playing && arrival.remainingTime() == fullTime;
        for (size_t i = 0; i < guardStarts.size(); ++i) {
            const Guard& guard = GameplayTestAccess::guards(arrival)[i];
            waiting &= guard.detection() == 0 && (guard.position() - guardStarts[i]).length() < 0.01f;
        }
        check(waiting, "arrival remains safe with clock and guards paused until the player is ready");
        arrival.keyDown('s'); run(arrival, 0.25f); arrival.keyUp('s'); run(arrival, 5);
        bool safeDeparture = arrival.screen() == Screen::Playing && arrival.remainingTime() < fullTime - 5;
        for (const Guard& guard : GameplayTestAccess::guards(arrival)) safeDeparture &= guard.detection() == 0;
        check(safeDeparture, "first movement starts the level with a clear opening and no automatic detection");
        arrival.keyDown('r'); run(arrival, 20);
        check(arrival.screen() == Screen::Playing && arrival.remainingTime() == fullTime,
              "restarting restores the safe ready state");
        arrival.keyDown('e'); run(arrival, 0.5f);
        check(arrival.remainingTime() < fullTime && arrival.distractions() == 1,
              "a deliberate distraction also starts the attempt");
    }
    // Every named NPC speaks once per interaction, not an entire multi-page script.
    for (const auto& npc : campusCharacters()) {
        Game conversation; conversation.startAtLevel(0); GameplayTestAccess::safe(conversation);
        conversation.placePlayer(npc.position + Vec2{0, -30}); conversation.keyDown('f');
        check(conversation.screen() == Screen::Dialogue && GameplayTestAccess::dialogue(conversation).size() == 1,
              "each named NPC opens exactly one speech");
        conversation.keyDown(13); conversation.keyDown(13);
        check(conversation.screen() == Screen::Playing, "NPC interaction ends after that speech");
    }
    // Exercise mouse coordinates through the public event handlers (no GL window).
    Game mouse; mouse.startAtLevel(0); GameplayTestAccess::safe(mouse); mouse.reshape(1000, 600);
    float waitingTime = mouse.remainingTime();
    mouse.mouseMove(200, 502); // world (200,40), accounting for the bottom HUD band
    run(mouse, 2);
    check(mouse.remainingTime() == waitingTime && mouse.distractions() == 2,
          "aiming alone does not start the ready state or spend a clink");
    check((GameplayTestAccess::aim(mouse) - Vec2{200, 40}).length() < 0.1f,
          "mouse aim reaches a nearby point regardless of player facing");
    mouse.mouseClick(140, 382); // click a new point without a preceding move event
    check(mouse.distractions() == 1 && (GameplayTestAccess::noise(mouse) - Vec2{140, 160}).length() < 0.1f,
          "left click throws at its own cursor coordinates");
    mouse.mouseClick(200, 502);
    check(mouse.distractions() == 1, "rapid mouse clicks respect the throw cooldown");
    run(mouse, 1.1f); mouse.mouseMove(200, 502); mouse.keyDown('e');
    check(mouse.distractions() == 0 && (GameplayTestAccess::noise(mouse) - Vec2{200, 40}).length() < 0.1f,
          "E uses the mouse aim as an alternate throw key");
    run(mouse, 1.1f); mouse.mouseClick(140, 382);
    check(mouse.distractions() == 0, "mouse throws cannot exceed the two-clink limit");
    mouse.keyDown('r'); GameplayTestAccess::safe(mouse); waitingTime = mouse.remainingTime();
    mouse.mouseClick(150, 20); mouse.mouseClick(150, 575); mouse.keyDown('e'); run(mouse, 1);
    check(mouse.distractions() == 2 && mouse.remainingTime() == waitingTime,
          "HUD clicks and invalid mouse aim cannot throw or start the attempt");
    mouse.reshape(1200, 600); mouse.mouseClick(20, 300);
    check(mouse.distractions() == 2, "letterbox clicks do not throw");
    mouse.mouseMove(300, 502);
    check((GameplayTestAccess::aim(mouse) - Vec2{200, 40}).length() < 0.1f,
          "mouse aim accounts for horizontal letterboxing");
    mouse.reshape(1000, 800); mouse.mouseMove(200, 602);
    check((GameplayTestAccess::aim(mouse) - Vec2{200, 40}).length() < 0.1f,
          "mouse aim accounts for vertical letterboxing after a resize");
    mouse.reshape(1000, 600); mouse.mouseMove(140, 242);
    check(std::fabs((GameplayTestAccess::aim(mouse) - GameplayTestAccess::position(mouse)).length() - 190) < 0.1f,
          "distant mouse targets are clamped to throwing range");
    mouse.mouseMove(260, 472); // the campus sign is in the way
    Vec2 blockedAim = GameplayTestAccess::aim(mouse);
    check(blockedAim.x < 222 && !collision::overlapsSolid(blockedAim, Guard::RADIUS, buildLevel(0).map),
          "a throw stops before an obstacle at a clear landing point");
    mouse.placePlayer({1100, 730});
    Vec2 camera = GameplayTestAccess::camera(mouse);
    mouse.mouseMove(int(1080 - camera.x), int(600 - (730 - camera.y + PLAY_BOTTOM)));
    check((GameplayTestAccess::aim(mouse) - Vec2{1080, 730}).length() < 1.1f,
          "mouse aim follows the scrolled camera and world viewport");
    mouse.keyDown('p'); mouse.mouseClick(10, 10);
    check(mouse.distractions() == 2, "clicking while paused never throws a clink");

    Game diversion; diversion.startAtLevel(0); diversion.placePlayer({260, 560});
    diversion.keyDown('e'); run(diversion, 0.3f);
    check(!GameplayTestAccess::spotted(diversion), "a noise diversion alone does not forfeit the stealth bonus");
    // Complete the tutorial using a west-side bypass and a deliberate noise diversion.
    // Several start delays exercise different patrol phases rather than one lucky timing.
    for (float delay : {0.0f, 3.0f, 8.0f}) {
        Game tutorial; tutorial.startAtLevel(0); tutorial.placePlayer(buildLevel(0).spawn); run(tutorial, delay);
        auto walkTo = [&](Vec2 target) {
            for (int axis = 0; axis < 2; ++axis) {
                for (int step = 0; step < 2400 && tutorial.screen() == Screen::Playing; ++step) {
                    Vec2 delta = target - GameplayTestAccess::position(tutorial);
                    float amount = axis == 0 ? delta.x : delta.y;
                    if (std::fabs(amount) < 1.1f) break;
                    char key = axis == 0 ? (amount > 0 ? 'd' : 'a') : (amount > 0 ? 'w' : 's');
                    tutorial.keyDown(key); tutorial.update(1.0f / 120); tutorial.keyUp(key);
                }
            }
        };
        walkTo({140, 270}); walkTo({260, 270}); walkTo({260, 380}); tutorial.keyDown('f');
        walkTo({70, 380}); walkTo({70, 760});
        tutorial.keyDown('d'); tutorial.update(1.0f / 120); tutorial.keyUp('d');
        tutorial.keyDown('e');
        walkTo({70, 930});
        tutorial.keyDown(' '); walkTo({260, 930}); tutorial.keyUp(' '); walkTo({260, 860}); tutorial.keyDown('f'); walkTo({260, 946});
        if (tutorial.screen() != Screen::LevelComplete) {
            Vec2 p = GameplayTestAccess::position(tutorial);
            std::printf("  Tutorial delay %.0f: screen %d, position %.1f,%.1f, passes %d\n",
                        delay, int(tutorial.screen()), p.x, p.y, tutorial.collected());
        }
        check(tutorial.screen() == Screen::LevelComplete && tutorial.distractions() == 1,
              "tutorial can be completed using cover, a bypass, and one distraction");
    }
    std::filesystem::remove(path);
    return failures ? 1 : 0;
}
