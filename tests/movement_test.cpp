// movement_test.cpp - automated checks for student movement and level
// completability. Runs the real Player/Collision/Level code without a window.
#include <cmath>
#include <cstdio>
#include <queue>
#include <vector>

#include "../src/Collision.h"
#include "../src/Level.h"
#include "../src/Player.h"

static int g_failures = 0;
#define CHECK(cond, msg)                                         \
    do {                                                         \
        if (cond) std::printf("  PASS  %s\n", msg);              \
        else { std::printf("  FAIL  %s\n", msg); ++g_failures; } \
    } while (0)

static void simulate(Player& p, const InputState& in, const Map& m, float seconds) {
    const float dt = 1.0f / 120.0f;
    for (float t = 0; t < seconds; t += dt) p.update(dt, in, m);
}

static float measureSpeed(const InputState& in, const Map& m, Vec2 start) {
    Player p;
    p.reset(start);
    simulate(p, in, m, 0.5f);  // let acceleration settle
    Vec2 a = p.position();
    simulate(p, in, m, 0.25f);
    return (p.position() - a).length() / 0.25f;
}

// Flood fill over a 5-unit grid of positions where the student's collision
// circle fits: is the goal reachable from the spawn?
static bool reachable(const Map& m, Vec2 from, const Rect& goal) {
    const float cell = 5.0f;
    const int W = static_cast<int>(WORLD_W / cell), H = static_cast<int>(WORLD_H / cell);
    std::vector<char> seen(static_cast<size_t>(W) * H, 0);
    auto isFree = [&](int x, int y) {
        Vec2 c{(x + 0.5f) * cell, (y + 0.5f) * cell};
        return c.x > Player::RADIUS && c.y > Player::RADIUS && c.x < WORLD_W - Player::RADIUS &&
               c.y < WORLD_H - Player::RADIUS && !collision::overlapsSolid(c, Player::RADIUS, m);
    };
    std::queue<std::pair<int, int>> q;
    int sx = static_cast<int>(from.x / cell), sy = static_cast<int>(from.y / cell);
    q.push({sx, sy});
    seen[sy * W + sx] = 1;
    Rect target = goal.expanded(Player::RADIUS);
    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        if (target.contains({(x + 0.5f) * cell, (y + 0.5f) * cell})) return true;
        const int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
        for (int k = 0; k < 4; ++k) {
            int nx = x + dx[k], ny = y + dy[k];
            if (nx < 0 || ny < 0 || nx >= W || ny >= H || seen[ny * W + nx] || !isFree(nx, ny)) continue;
            seen[ny * W + nx] = 1;
            q.push({nx, ny});
        }
    }
    return false;
}

int main() {
    std::printf("Movement tests\n");

    // A small test map: one building and the four boundary walls.
    Map m;
    m.buildings.push_back({{600, 400, 300, 150}, BuildingKind::Academic, "", 750});
    m.walls = {{0, 0, WORLD_W, 14}, {0, WORLD_H - 14, WORLD_W, 14}, {0, 0, 14, WORLD_H}, {WORLD_W - 14, 0, 14, WORLD_H}};
    m.finalize();

    InputState right; right.right = true;
    Player responsive;
    responsive.reset({200, 200});
    simulate(responsive, right, m, 0.5f);
    Vec2 released = responsive.position();
    simulate(responsive, {}, m, 0.2f);
    CHECK((responsive.position() - released).length() < 0.001f, "releasing movement stops immediately without sliding");
    InputState diag; diag.right = true; diag.up = true;
    float sStraight = measureSpeed(right, m, {200, 200});
    float sDiag = measureSpeed(diag, m, {200, 200});
    std::printf("        straight %.1f u/s, diagonal %.1f u/s\n", sStraight, sDiag);
    CHECK(std::fabs(sStraight - Player::SPEED) < 1.0f, "straight speed matches SPEED");
    CHECK(std::fabs(sDiag - sStraight) < 1.0f, "diagonal speed is normalised");

    // Sprint: faster, drains stamina, recovers when walking.
    InputState run = right;
    run.sprint = true;
    float sRun = measureSpeed(run, m, {200, 200});
    std::printf("        sprint %.1f u/s\n", sRun);
    CHECK(std::fabs(sRun - Player::SPEED * Player::SPRINT_MULT) < 1.5f, "sprint speed = SPEED x SPRINT_MULT");
    Player r;
    r.reset({200, 200});
    simulate(r, run, m, 2.7f);  // 1/0.4 = 2.5 s of sprint
    CHECK(r.stamina() < 0.06f && !r.isSprinting(), "stamina runs out and sprinting stops");
    simulate(r, right, m, 2.0f);
    CHECK(r.stamina() > 0.3f, "stamina recovers while walking");

    Player p;
    p.reset({700, 330});
    InputState up; up.up = true;
    simulate(p, up, m, 3.0f);
    CHECK(p.position().y <= 400.0f - Player::RADIUS + 0.01f, "blocked by a building wall");
    CHECK(!collision::overlapsSolid(p.position(), Player::RADIUS, m), "never inside an obstacle after pushing");

    p.reset({700, 380});
    simulate(p, diag, m, 3.0f);
    CHECK(p.position().x > 860.0f, "slides along the building when moving diagonally");

    p.reset({60, 330});
    InputState left; left.left = true;
    simulate(p, left, m, 3.0f);
    CHECK(p.position().x >= 14.0f + Player::RADIUS - 0.01f, "blocked by boundary wall");

    // Real levels.
    for (int i = 0; i < levelCount(); ++i) {
        Level L = buildLevel(i);
        std::printf("  %s\n", L.name.c_str());
        CHECK(!collision::overlapsSolid(L.spawn, Player::RADIUS, L.map), "spawn does not overlap any obstacle");
        CHECK(reachable(L.map, L.spawn, L.goal), "goal reachable from spawn");
        for (Vec2 checkpoint : L.checkpoints) {
            CHECK(!collision::overlapsSolid(checkpoint, Player::RADIUS, L.map), "pass is clear of obstacles");
            CHECK(reachable(L.map, L.spawn, {checkpoint.x - 15, checkpoint.y - 15, 30, 30}), "pass reachable from spawn");
        }
        bool routesClear = true;
        for (const GuardConfig& g : L.guards)
            for (const Waypoint& w : g.route)
                if (collision::overlapsSolid(w.p, 9.0f, L.map)) routesClear = false;
        CHECK(routesClear, "guard waypoints are not inside obstacles");
        p.reset(L.spawn);
        InputState down; down.down = true;
        simulate(p, down, L.map, 3.0f);
        CHECK(p.position().y >= Player::RADIUS - 0.01f, "cannot leave the map through the gate");
    }

    std::printf(g_failures ? "%d FAILED\n" : "ALL PASSED\n", g_failures);
    return g_failures ? 1 : 0;
}
