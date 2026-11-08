// vision_test.cpp - automated checks for guard vision (Phase 3).
// Uses a hand-built map: one building between x=400..500, y=250..350.
#include <cmath>
#include <cstdio>

#include "../src/Collision.h"
#include "../src/Guard.h"
#include "../src/Map.h"

static int g_failures = 0;
#define CHECK(cond, msg)                                         \
    do {                                                         \
        if (cond) std::printf("  PASS  %s\n", msg);              \
        else { std::printf("  FAIL  %s\n", msg); ++g_failures; } \
    } while (0)

static void run(Guard& g, Vec2 player, const Map& m, float seconds) {
    const float dt = 1.0f / 120.0f;
    for (float t = 0; t < seconds; t += dt) g.update(dt, player, m);
}

int main() {
    Map m;
    m.buildings.push_back({{400, 250, 100, 100}, BuildingKind::Academic, "", 450});
    m.finalize();
    std::printf("Phase 3 vision tests\n");

    // Sentry at (200,300) looking east (no sweep), 70 deg cone, range 400.
    GuardConfig cfg;
    cfg.route = {{{200, 300}, 100.0f}};
    cfg.fov = 70.0f;
    cfg.range = 400.0f;
    cfg.searchSweep = 0.0f;
    cfg.startAngle = 0.0f;
    Guard g(cfg);

    CHECK(g.canSee({300, 300}, m), "sees a student straight ahead");
    CHECK(!g.canSee({100, 300}, m), "does not see behind");
    CHECK(!g.canSee({300, 450}, m), "does not see outside the cone angle (56 deg)");
    CHECK(g.canSee({300, 360}, m), "sees inside the cone angle (31 deg)");
    CHECK(!g.canSee({700, 300}, m), "does not see beyond range");
    CHECK(!g.canSee({550, 300}, m), "does not see through a building");
    CHECK(!g.canSee({550, 380}, m), "does not see through a building corner");
    CHECK(g.canSee({550, 420}, m), "sees past the building corner");

    // Ray clipping used by the drawn cone matches the obstacle.
    float hit = collision::raycast({200, 300}, {1, 0}, 400.0f, m);
    std::printf("        ray east hits at %.2f (building face at 200)\n", hit);
    CHECK(std::fabs(hit - 200.0f) < 0.01f, "cone ray stops exactly at the building face");
    CHECK(std::fabs(collision::raycast({200, 300}, {-1, 0}, 150.0f, m) - 150.0f) < 0.01f, "free ray reaches full range");

    // Line intersection primitives.
    CHECK(collision::segmentsIntersect({0, 0}, {10, 10}, {0, 10}, {10, 0}), "crossing segments intersect");
    CHECK(!collision::segmentsIntersect({0, 0}, {10, 0}, {0, 5}, {10, 5}), "parallel segments do not intersect");

    // Detection meter: rises while visible, decays while hidden.
    g.reset();
    run(g, {300, 300}, m, 0.3f);
    float afterSeen = g.detection();
    CHECK(afterSeen > 0.05f && !g.hasCaught(), "meter rises while visible");
    run(g, {550, 300}, m, 0.5f);  // student steps behind the building
    CHECK(g.detection() < afterSeen, "meter decays while hidden");
    run(g, {300, 300}, m, 5.0f);
    CHECK(g.hasCaught(), "continuous visibility fills the detection meter and causes capture");

    Guard distant(cfg);
    run(distant, {350, 300}, m, 0.35f);
    float warning = distant.detection();
    CHECK(warning > 0 && !distant.hasCaught(), "brief sighting provides a warning before capture");
    run(distant, {550, 300}, m, 0.5f);
    CHECK(distant.detection() < warning, "breaking sight reduces the detection meter");
    distant.reset();
    run(distant, {350, 300}, m, 5);
    CHECK(distant.hasCaught(), "remaining in sight fills the meter and causes capture");

    // A hidden student is never caught, however long they wait.
    g.reset();
    run(g, {550, 300}, m, 10.0f);
    CHECK(g.detection() == 0.0f, "never detected through a wall");

    // Closer students are detected faster.
    Guard nearG(cfg), farG(cfg);
    run(nearG, {240, 300}, m, 0.3f);
    run(farG, {390, 300}, m, 0.3f);
    CHECK(nearG.detection() > farG.detection(), "closer = faster detection");

    // Search: at a waypoint the guard looks left, then right, then centre.
    GuardConfig sc;
    sc.route = {{{200, 300}, 4.0f}};
    sc.searchSweep = 60.0f;
    sc.startAngle = 0.0f;
    Guard s(sc);
    run(s, {1900, 1100}, m, 1.0f);  // u = 0.25 -> full left
    float left = s.facingAngle();
    run(s, {1900, 1100}, m, 1.4f);  // u = 0.60 -> full right
    std::printf("        search angles %.1f -> %.1f\n", left, s.facingAngle());
    CHECK(s.state() == GuardState::Searching, "guard searches at a waypoint");
    CHECK(std::fabs(left - 60.0f) < 1.0f && std::fabs(s.facingAngle() + 60.0f) < 1.0f, "looks left then right");

    // Patrol: walks between waypoints, pauses to search, continues.
    GuardConfig pc;
    pc.route = {{{100, 100}, 1.0f}, {{300, 100}, 1.0f}};
    pc.speed = 50.0f;
    Guard p(pc);
    run(p, {1900, 1100}, m, 3.0f);  // search 1 s, walk 2 s (100 units)
    std::printf("        patrol x after 3 s: %.1f\n", p.position().x);
    CHECK(p.state() == GuardState::Patrolling && std::fabs(p.position().x - 200.0f) < 3.0f, "walks the route at its speed");
    run(p, {1900, 1100}, m, 2.5f);
    CHECK(p.state() == GuardState::Searching && std::fabs(p.position().x - 300.0f) < 0.5f, "stops to search at the next point");
    run(p, {1900, 1100}, m, 2.0f);
    CHECK(p.state() == GuardState::Patrolling && p.position().x < 300.0f, "continues back along the route");

    // Alert: a brief sighting makes a patrolling guard stop and turn.
    Guard a(pc);
    run(a, {1900, 1100}, m, 1.2f);      // finished search, now walking east
    run(a, {170, 100}, m, 0.3f);  // student appears right in front
    Vec2 stopAt = a.position();
    CHECK(a.state() == GuardState::Alert, "brief sighting -> ALERT");
    run(a, {170, 400}, m, 0.5f);  // student slips away out of sight
    CHECK((a.position() - stopAt).length() < 0.01f, "alert guard stops and stares first");
    run(a, {170, 400}, m, 2.0f);
    std::printf("        investigating guard at (%.1f, %.1f)\n", a.position().x, a.position().y);
    CHECK(a.state() == GuardState::Alert && (a.position() - Vec2{170, 100}).length() < 16.0f,
          "then walks to where the student was last seen");
    run(a, {170, 400}, m, 8.0f);
    CHECK(a.state() == GuardState::Patrolling || a.state() == GuardState::Searching,
          "returns to its route after losing the student");

    // Hearing: sprinting nearby (even behind a wall) makes the guard turn.
    Guard h(cfg);  // faces east
    run(h, {200, 220}, m, 0.1f);  // south of him, outside the cone, walking quietly
    CHECK(h.state() != GuardState::Alert, "quiet student behind a guard is not noticed");
    const float dt = 1.0f / 120.0f;
    for (float t = 0; t < 0.6f; t += dt) h.update(dt, {200, 220}, m, true);
    std::printf("        guard turned to %.1f deg\n", h.facingAngle());
    CHECK(h.state() == GuardState::Alert && std::fabs(h.facingAngle() + 90.0f) < 5.0f, "hears sprinting and turns towards it");

    std::printf(g_failures ? "%d FAILED\n" : "ALL PASSED\n", g_failures);
    return g_failures ? 1 : 0;
}
