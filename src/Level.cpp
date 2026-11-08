#include "Level.h"

#include <utility>
#include <algorithm>

namespace {

void addBoundary(Map& m, bool hasGate = false) {
    const float t = 14.0f;
    m.hasGate = hasGate;
    if (hasGate) {
        m.gate = {80.0f, 200.0f};
        m.walls.push_back({0, 0, m.gate.x0 - 16, t});
        m.walls.push_back({m.gate.x1 + 16, 0, WORLD_W - (m.gate.x1 + 16), t});
    } else {
        m.walls.push_back({0, 0, WORLD_W, t});
    }
    m.walls.push_back({0, WORLD_H - t, WORLD_W, t});
    m.walls.push_back({0, t, t, WORLD_H - 2 * t});
    m.walls.push_back({WORLD_W - t, t, t, WORLD_H - 2 * t});
}

Rect doorGoal(float doorX, float buildingY) {
    return {doorX - 20.0f, buildingY - 14.0f, 40.0f, 14.0f};
}

GuardConfig patrol(std::vector<Waypoint> route, float speed, float startAngle, float range = 240.0f) {
    GuardConfig g;
    g.route = std::move(route);
    g.speed = speed;
    g.startAngle = startAngle;
    g.range = range;
    g.chaseSpeed = std::max(130.0f, speed * 1.6f);
    return g;
}

// ---------------------------------------------------------------------------
// Level 1 - Museum Entrance: Main gate at bottom, Curator's office at top edge,
// pond nearby. Pure museum entrance scene only.
// ---------------------------------------------------------------------------
Level level1() {
    Level L;
    L.name = "Level 1 - Museum Entrance";
    L.objective = "Collect the entrance security log";
    L.briefing = "A priceless artifact vanished last night. Sneak into the museum and gather the security log from the entrance desk.";
    L.reward = "ENTRANCE LOG SECURED";
    L.passNames = {"Security Log", "Guard Roster"};
    L.timeLimit = 90.0f;
    L.spawn = {140, 35};
    L.goal = doorGoal(230, 350);

    Map& m = L.map;
    addBoundary(m, true);

    // Roads & Walkways
    m.paths.push_back({{95, 0, 90, 130}, PathKind::Road});
    m.paths.push_back({{205, 120, 50, 235}, PathKind::Pavement});
    m.paths.push_back({{50, 120, 160, 40}, PathKind::Pavement});
    m.paths.push_back({{255, 120, 270, 40}, PathKind::Pavement});
    m.paths.push_back({{500, 210, 240, 100}, PathKind::Pavement});
    m.crossings = {{205, 120, 50, 40}};

    // Buildings
    m.buildings.push_back({{50, 350, 340, 115}, BuildingKind::Admin, "CURATOR'S OFFICE", 230});
    m.buildings.push_back({{14, 20, 48, 44}, BuildingKind::GuardBooth, "SECURITY DESK", 38});

    // Pond & Monuments brought closer
    m.lakes.push_back({480, 80, 240, 130});

    // Cover walls
    m.walls.push_back({330, 180, 70, 14});
    m.walls.push_back({500, 225, 14, 65});

    // Trees
    using TK = TreeKind;
    m.trees = {
        {{380, 140}, 24, TK::Rain},
        {{130, 230}, 22, TK::Rain},
        {{440, 220}, 22, TK::Krishnachura},
        {{80, 290}, 22, TK::Mango}
    };
    m.bushes = {{{200, 210}, 10}, {{270, 210}, 10}, {{470, 75}, 10}};
    m.flowerBeds = {{{60, 325, 140, 16}}, {{240, 95, 140, 14}}};
    m.benches = {{{140, 170}, false}, {{440, 120}, false}};
    m.lamps = {{{195, 160}}, {{265, 160}}, {{265, 290}}};

    // Checkpoints
    L.checkpoints = {{230, 150}, {560, 250}};

    // Guards (starts at 410,240 which is > 300 away from spawn 140,35)
    auto g = patrol({{{410, 240}, 2.0f}, {{235, 260}, 2.0f}, {{380, 180}, 2.0f}}, 50.0f, 90.0f, 160.0f);
    g.fov = 65.0f;
    g.searchSweep = 45.0f;
    g.detectionTimeScale = 2.4f;
    g.chaseSpeed = 125.0f;
    g.hearingRadius = 115.0f;
    g.forgetTime = 5.0f;
    L.guards.push_back(g);

    m.finalize();
    return L;
}

// ---------------------------------------------------------------------------
// Level 2 - Ancient Artifacts Wing: Artifacts gallery entrance
// ---------------------------------------------------------------------------
Level level2() {
    Level L;
    L.name = "Level 2 - Ancient Artifacts Wing";
    L.objective = "Locate the artifact registry";
    L.briefing = "The security office confirmed your access. Now find the artifact registry before the night patrol rounds the corner.";
    L.reward = "ARTIFACT REGISTRY LOCATED";
    L.passNames = {"Case File", "Evidence Tag"};
    L.timeLimit = 85.0f;
    L.spawn = {230, 335};
    L.goal = doorGoal(720, 350);

    Map& m = L.map;
    addBoundary(m, false);

    // Buildings
    m.buildings.push_back({{500, 350, 440, 115}, BuildingKind::Academic, "ANCIENT ARTIFACTS WING", 720});
    m.buildings.push_back({{60, 350, 200, 115}, BuildingKind::Admin, "RESTORATION LAB", 160});

    // Walkways
    m.paths.push_back({{60, 290, 880, 50}, PathKind::Pavement});
    m.paths.push_back({{60, 70, 880, 50}, PathKind::Pavement});
    m.paths.push_back({{420, 70, 50, 270}, PathKind::Pavement});
    m.paths.push_back({{700, 70, 50, 270}, PathKind::Pavement});
    m.crossings = {{420, 290, 50, 50}, {420, 70, 50, 50}};

    // Cover walls
    m.walls.push_back({300, 180, 80, 14});
    m.walls.push_back({580, 180, 14, 80});

    using TK = TreeKind;
    m.trees = {
        {{360, 230}, 24, TK::Krishnachura},
        {{520, 230}, 24, TK::Rain},
        {{640, 230}, 22, TK::Mango},
        {{800, 140}, 24, TK::Rain},
        {{180, 140}, 22, TK::Mango}
    };
    m.flowerBeds = {{{270, 320, 140, 16}}, {{520, 320, 160, 16}}};
    m.benches = {{{360, 130}, false}, {{640, 130}, false}, {{800, 200}, true}};
    m.lamps = {{{410, 150}}, {{480, 150}}, {{690, 150}}, {{760, 150}}};

    // Checkpoints
    L.checkpoints = {{445, 180}, {725, 140}};

    // Guards (starts at 445,120 and 880,260, both > 300 away from spawn 230,335)
    {
        auto g = patrol({{{445, 120}, 2.0f}, {{445, 270}, 2.0f}}, 55.0f, 90.0f);
        g.fov = 65.0f;  // torch light cone slightly narrower
        L.guards.push_back(g);
    }
    {
        auto g = patrol({{{880, 260}, 2.0f}, {{725, 260}, 2.0f}}, 65.0f, 0.0f);
        g.fov = 65.0f;  // torch light cone slightly narrower
        L.guards.push_back(g);
    }

    m.finalize();
    return L;
}

// ---------------------------------------------------------------------------
// Level 3 - Patrol Timing: Paintings Gallery & Curator's witness statement
// ---------------------------------------------------------------------------
Level level3() {
    Level L;
    L.name = "Level 3 - Paintings Gallery";
    L.objective = "Retrieve the curator's statement";
    L.briefing = "The night curator holds a signed witness statement. Find her in the gallery, but watch for the roaming guards.";
    L.reward = "CURATOR'S STATEMENT ACQUIRED";
    L.passNames = {"Curator Note", "Witness Log", "Gallery Key"};
    L.timeLimit = 100.0f;
    L.spawn = {720, 335};
    L.goal = doorGoal(250, 350);

    Map& m = L.map;
    addBoundary(m, false);

    // Buildings
    m.buildings.push_back({{60, 350, 380, 115}, BuildingKind::Canteen, "PAINTINGS GALLERY", 250});
    m.buildings.push_back({{550, 350, 390, 115}, BuildingKind::Admin, "CURATOR'S OFFICE", 720});

    // Football pitch & umbrella tables
    m.footballCourts.push_back({460, 60, 380, 180});
    m.tables.push_back({{260, 210}, 0});
    m.tables.push_back({{370, 200}, 1});

    // Walkways
    m.paths.push_back({{60, 290, 400, 50}, PathKind::Pavement});
    m.paths.push_back({{60, 60, 380, 50}, PathKind::Pavement});
    m.paths.push_back({{410, 60, 45, 280}, PathKind::Pavement});
    m.paths.push_back({{455, 290, 480, 50}, PathKind::Pavement});

    // Railings & Cover
    m.railings.push_back({{450, 60}, {450, 240}});
    m.walls.push_back({360, 260, 14, 75});
    m.walls.push_back({220, 140, 80, 14});
    m.benches = {{{400, 270}, false}, {{180, 100}, false}};
    m.lamps = {{{240, 280}}, {{380, 280}}, {{470, 280}}};

    using TK = TreeKind;
    m.trees = {
        {{280, 140}, 24, TK::Rain},
        {{410, 220}, 22, TK::Krishnachura},
        {{880, 270}, 24, TK::Mango}
    };

    // Checkpoints
    L.checkpoints = {{120, 240}, {420, 120}, {310, 210}};

    // Guards (starts at 120,270; 220,90; 390,270, all > 300 away from spawn 720,335)
    {
        auto g = patrol({{{120, 270}, 2.0f}, {{250, 270}, 2.0f}}, 50.0f, 180.0f);
        g.fov = 65.0f;  // torch light cone slightly narrower
        L.guards.push_back(g);
    }
    {
        auto g = patrol({{{220, 90}, 1.5f}, {{400, 90}, 1.5f}}, 75.0f, 0.0f);
        g.fov = 65.0f;  // torch light cone slightly narrower
        L.guards.push_back(g);
    }
    {
        auto g = patrol({{{390, 270}, 2.0f}, {{520, 160}, 2.0f}}, 60.0f, 270.0f);
        g.fov = 65.0f;  // torch light cone slightly narrower
        L.guards.push_back(g);
    }

    m.finalize();
    return L;
}

// ---------------------------------------------------------------------------
// Level 4 - The Main Vault: Grand Museum Gallery
// ---------------------------------------------------------------------------
Level level4() {
    Level L;
    L.name = "Level 4 - The Main Vault";
    L.objective = "Recover the stolen ledger";
    L.briefing = "Final step: the theft ledger is in the vault. Grab it and exit before the chief guard completes his sweep.";
    L.reward = "CASE FILE COMPLETE";
    L.passNames = {"Vault Log", "Shift Roster", "Evidence Ledger"};
    L.timeLimit = 110.0f;
    L.spawn = {250, 335};
    L.goal = doorGoal(680, 345);

    Map& m = L.map;
    addBoundary(m, false);

    // Buildings
    m.buildings.push_back({{380, 345, 560, 120}, BuildingKind::Academic, "MAIN VAULT", 680});
    m.buildings.push_back({{60, 340, 240, 125}, BuildingKind::Academic, "SECURITY OFFICE", 180});

    // Grand Plaza
    m.paths.push_back({{350, 90, 590, 255}, PathKind::Plaza});
    m.paths.push_back({{60, 90, 300, 45}, PathKind::Pavement});
    m.paths.push_back({{160, 135, 45, 205}, PathKind::Pavement});

    // Monuments & Props
    m.signs.push_back({230, 45, 140, 40});
    m.flags.push_back({160, 240});
    m.flowerBeds = {{{400, 315, 160, 20}}, {{720, 315, 160, 20}}};

    // Cover walls
    m.walls.push_back({480, 220, 80, 14});
    m.walls.push_back({660, 180, 14, 80});
    m.walls.push_back({320, 160, 14, 80});

    using TK = TreeKind;
    m.trees = {
        {{320, 130}, 24, TK::Rain},
        {{100, 210}, 22, TK::Krishnachura},
        {{280, 280}, 22, TK::Mango},
        {{880, 130}, 24, TK::Krishnachura}
    };
    m.benches = {{{440, 100}, false}, {{780, 100}, false}, {{900, 220}, true}};
    m.lamps = {{{360, 180}}, {{550, 180}}, {{750, 180}}, {{600, 280}}, {{760, 280}}};

    // Checkpoints
    L.checkpoints = {{180, 200}, {520, 250}, {820, 180}};

    // Guards (starts at 820,120; 850,160; 650,280; 780,240, all > 300 away from spawn 250,335)
    L.guards.push_back(patrol({{{820, 120}, 2.0f}, {{480, 120}, 2.0f}}, 75.0f, 180.0f));
    L.guards.push_back(patrol({{{850, 160}, 2.0f}, {{850, 280}, 2.0f}}, 60.0f, 90.0f));
    L.guards.push_back(patrol({{{650, 280}, 2.0f}, {{520, 280}, 2.0f}}, 55.0f, 180.0f));
    L.guards.push_back(patrol({{{780, 240}, 1.5f}, {{680, 240}, 1.5f}}, 70.0f, 180.0f));

    m.finalize();
    return L;
}

}  // namespace

int levelCount() { return 4; }

Level buildLevel(int index) {
    switch (index) {
        case 1: return level2();
        case 2: return level3();
        case 3: return level4();
        default: return level1();
    }
}
