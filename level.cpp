// ============================================================
// level.cpp
// The four museum levels: buildings, paths, trees, guards,
// evidence stands, spawn point and exit.
// ============================================================

#include "game.h"

// ============================================================
// HELPERS TO FILL THE MAP
// ============================================================

Rect makeRect(float x, float y, float w, float h)
{
    Rect r = {x, y, w, h};
    return r;
}

void addPath(float x, float y, float w, float h, int kind)
{
    map.paths[map.pathCount].r = makeRect(x, y, w, h);
    map.paths[map.pathCount].kind = kind;
    map.pathCount++;
}

void addCrossing(float x, float y, float w, float h)
{
    map.crossings[map.crossingCount++] = makeRect(x, y, w, h);
}

void addBuilding(float x, float y, float w, float h, int kind, const char *name, float doorX)
{
    Building &b = map.buildings[map.buildingCount++];
    b.r = makeRect(x, y, w, h);
    b.kind = kind;
    b.name = name;
    b.doorX = doorX;
}

void addWall(float x, float y, float w, float h)
{
    map.walls[map.wallCount++] = makeRect(x, y, w, h);
}

void addTree(float x, float y, float radius, int kind)
{
    Tree &t = map.trees[map.treeCount++];
    t.p = makeVec(x, y);
    t.radius = radius;
    t.kind = kind;
}

void addBush(float x, float y, float radius)
{
    map.bushes[map.bushCount].p = makeVec(x, y);
    map.bushes[map.bushCount].radius = radius;
    map.bushCount++;
}

void addFlowerBed(float x, float y, float w, float h)
{
    map.flowerBeds[map.flowerBedCount++] = makeRect(x, y, w, h);
}

void addBench(float x, float y, bool vertical)
{
    map.benches[map.benchCount].p = makeVec(x, y);
    map.benches[map.benchCount].vertical = vertical;
    map.benchCount++;
}

void addLamp(float x, float y)
{
    map.lamps[map.lampCount++] = makeVec(x, y);
}

// The outer wall around the museum grounds (optionally with a gate)
void addBoundary(bool hasGate)
{
    float t = 14.0f;
    map.hasGate = hasGate;
    if (hasGate)
    {
        map.gateX0 = 80;
        map.gateX1 = 200;
        addWall(0, 0, map.gateX0 - 16, t);
        addWall(map.gateX1 + 16, 0, WORLD_W - (map.gateX1 + 16), t);
    }
    else
    {
        addWall(0, 0, WORLD_W, t);
    }
    addWall(0, WORLD_H - t, WORLD_W, t);
    addWall(0, t, t, WORLD_H - 2 * t);
    addWall(WORLD_W - t, t, t, WORLD_H - 2 * t);
}

// The gold exit sits just below a building's front door
Rect doorGoal(float doorX, float buildingY)
{
    return makeRect(doorX - 20, buildingY - 14, 40, 14);
}

// A guard walking between waypoints (waiting 'wait' seconds at each)
Guard &addGuard(float x1, float y1, float x2, float y2, float wait, float speed, float startAngle)
{
    Guard &g = guards[guardCount++];
    g.routeCount = 2;
    g.route[0].p = makeVec(x1, y1);
    g.route[0].searchTime = wait;
    g.route[1].p = makeVec(x2, y2);
    g.route[1].searchTime = wait;
    g.speed = speed;
    g.startAngle = startAngle;
    g.fov = 80;
    g.range = 240;
    g.searchSweep = 70;
    g.detectionTimeScale = 1.35f;
    g.chaseSpeed = maxf(130, speed * 1.6f);
    g.hearingRadius = 170;
    g.forgetTime = 7;
    return g;
}

// ============================================================
// SOLIDS AND SIGHT BLOCKERS
// ============================================================
// Lakes stop your feet but not the guards' eyes.
// Buildings, walls and the stone sign stop both.

void addSolid(Rect r) { map.solids[map.solidCount++] = r; }
void addBlocker(Rect r) { map.blockers[map.blockerCount++] = r; }

void addSolidCircle(Vec2 p, float r)
{
    map.circlePos[map.circleCount] = p;
    map.circleR[map.circleCount] = r;
    map.circleCount++;
}

void finishMap()
{
    map.solidCount = 0;
    map.blockerCount = 0;
    map.circleCount = 0;

    for (int i = 0; i < map.lakeCount; i++)
        addSolid(map.lakes[i]);
    for (int i = 0; i < map.buildingCount; i++)
    {
        addSolid(map.buildings[i].r);
        addBlocker(map.buildings[i].r);
    }
    for (int i = 0; i < map.wallCount; i++)
    {
        addSolid(map.walls[i]);
        addBlocker(map.walls[i]);
    }
    if (map.hasGate)
    {
        addSolid(makeRect(map.gateX0 - 17, 0, 18, 20)); // gate pillars
        addSolid(makeRect(map.gateX1 - 1, 0, 18, 20));
    }
    for (int i = 0; i < map.signCount; i++)
    {
        addSolid(rectExpand(map.signs[i], 4));
        addBlocker(map.signs[i]);
    }
    for (int i = 0; i < map.benchCount; i++)
    {
        Vec2 p = map.benches[i].p;
        if (map.benches[i].vertical)
            addSolid(makeRect(p.x - 7, p.y - 18, 14, 36));
        else
            addSolid(makeRect(p.x - 18, p.y - 7, 36, 14));
    }
    for (int i = 0; i < map.railingCount; i++)
    {
        Railing rl = map.railings[i];
        addSolid(makeRect(minf(rl.a.x, rl.b.x) - 1, minf(rl.a.y, rl.b.y) - 1,
                          fabsf(rl.b.x - rl.a.x) + 2, fabsf(rl.b.y - rl.a.y) + 4));
    }
    for (int i = 0; i < map.treeCount; i++)
        addSolidCircle(map.trees[i].p, 7);
    for (int i = 0; i < map.lampCount; i++)
        addSolidCircle(map.lamps[i], 4);
    for (int i = 0; i < map.flagCount; i++)
        addSolidCircle(map.flags[i], 6);
    for (int i = 0; i < map.tableCount; i++)
        addSolidCircle(map.tables[i].p, 12);
}

// ============================================================
// LEVEL 1 - MUSEUM ENTRANCE
// ============================================================

void loadLevel1()
{
    level.name = "Level 1 - Museum Entrance";
    level.objective = "Collect the entrance security log";
    level.briefing = "A priceless artifact vanished last night. Sneak into the museum and gather the security log from the entrance desk.";
    level.reward = "ENTRANCE LOG SECURED";
    level.passNames[0] = "Security Log";
    level.passNames[1] = "Guard Roster";
    level.passCount = 2;
    level.timeLimit = 90;
    level.spawn = makeVec(140, 35);
    level.goal = doorGoal(230, 350);
    level.checkpoints[0] = makeVec(230, 150);
    level.checkpoints[1] = makeVec(560, 250);

    addBoundary(true);

    // roads and walkways
    addPath(95, 0, 90, 130, ROAD);
    addPath(205, 120, 50, 235, PAVEMENT);
    addPath(50, 120, 160, 40, PAVEMENT);
    addPath(255, 120, 270, 40, PAVEMENT);
    addPath(500, 210, 240, 100, PAVEMENT);
    addCrossing(205, 120, 50, 40);

    // buildings
    addBuilding(50, 350, 340, 115, OFFICE, "CURATOR'S OFFICE", 230);
    addBuilding(14, 20, 48, 44, BOOTH, "SECURITY DESK", 38);

    // pond
    map.lakes[map.lakeCount++] = makeRect(480, 80, 240, 130);

    // cover walls
    addWall(330, 180, 70, 14);
    addWall(500, 225, 14, 65);

    addTree(380, 140, 24, RAIN_TREE);
    addTree(130, 230, 22, RAIN_TREE);
    addTree(440, 220, 22, FLAME_TREE);
    addTree(80, 290, 22, MANGO_TREE);
    addBush(200, 210, 10);
    addBush(270, 210, 10);
    addBush(470, 75, 10);
    addFlowerBed(60, 325, 140, 16);
    addFlowerBed(240, 95, 140, 14);
    addBench(140, 170, false);
    addBench(440, 120, false);
    addLamp(195, 160);
    addLamp(265, 160);
    addLamp(265, 290);

    // one slow guard with short sight: the tutorial level
    Guard &g = addGuard(410, 240, 235, 260, 2, 50, 90);
    g.routeCount = 3;
    g.route[2].p = makeVec(380, 212);
    g.route[2].searchTime = 2;
    g.range = 160;
    g.fov = 65;
    g.searchSweep = 45;
    g.detectionTimeScale = 2.4f;
    g.chaseSpeed = 125;
    g.hearingRadius = 115;
    g.forgetTime = 5;
}

// ============================================================
// LEVEL 2 - ANCIENT ARTIFACTS WING
// ============================================================

void loadLevel2()
{
    level.name = "Level 2 - Ancient Artifacts Wing";
    level.objective = "Locate the artifact registry";
    level.briefing = "The security office confirmed your access. Now find the artifact registry before the night patrol rounds the corner.";
    level.reward = "ARTIFACT REGISTRY LOCATED";
    level.passNames[0] = "Case File";
    level.passNames[1] = "Evidence Tag";
    level.passCount = 2;
    level.timeLimit = 85;
    level.spawn = makeVec(230, 335);
    level.goal = doorGoal(720, 350);
    level.checkpoints[0] = makeVec(445, 180);
    level.checkpoints[1] = makeVec(725, 140);

    addBoundary(false);

    addBuilding(500, 350, 440, 115, HALL, "ANCIENT ARTIFACTS WING", 720);
    addBuilding(60, 350, 200, 115, OFFICE, "RESTORATION LAB", 160);

    addPath(60, 290, 880, 50, PAVEMENT);
    addPath(60, 70, 880, 50, PAVEMENT);
    addPath(420, 70, 50, 270, PAVEMENT);
    addPath(700, 70, 50, 270, PAVEMENT);
    addCrossing(420, 290, 50, 50);
    addCrossing(420, 70, 50, 50);

    addWall(300, 180, 80, 14);
    addWall(580, 180, 14, 80);

    addTree(360, 230, 24, FLAME_TREE);
    addTree(520, 230, 24, RAIN_TREE);
    addTree(640, 230, 22, MANGO_TREE);
    addTree(800, 140, 24, RAIN_TREE);
    addTree(180, 140, 22, MANGO_TREE);
    addFlowerBed(270, 320, 140, 16);
    addFlowerBed(520, 320, 160, 16);
    addBench(360, 130, false);
    addBench(640, 130, false);
    addBench(800, 200, true);
    addLamp(410, 150);
    addLamp(480, 150);
    addLamp(690, 150);
    addLamp(760, 150);

    // turns at y = 190, so he can't see the arrival point
    Guard &g1 = addGuard(445, 120, 445, 190, 2, 55, 90);
    g1.fov = 65;
    Guard &g2 = addGuard(880, 260, 725, 260, 2, 65, 0);
    g2.fov = 65;
}

// ============================================================
// LEVEL 3 - PAINTINGS GALLERY
// ============================================================

void loadLevel3()
{
    level.name = "Level 3 - Paintings Gallery";
    level.objective = "Retrieve the curator's statement";
    level.briefing = "The night curator holds a signed witness statement. Find her in the gallery, but watch for the roaming guards.";
    level.reward = "CURATOR'S STATEMENT ACQUIRED";
    level.passNames[0] = "Curator Note";
    level.passNames[1] = "Witness Log";
    level.passNames[2] = "Gallery Key";
    level.passCount = 3;
    level.timeLimit = 100;
    level.spawn = makeVec(720, 335);
    level.goal = doorGoal(250, 350);
    level.checkpoints[0] = makeVec(120, 240);
    level.checkpoints[1] = makeVec(420, 120);
    level.checkpoints[2] = makeVec(310, 210);

    addBoundary(false);

    addBuilding(60, 350, 380, 115, GALLERY, "PAINTINGS GALLERY", 250);
    addBuilding(550, 350, 390, 115, OFFICE, "CURATOR'S OFFICE", 720);

    // sports lawn and cafe tables
    map.fields[map.fieldCount++] = makeRect(460, 60, 380, 180);
    map.tables[0].p = makeVec(260, 210);
    map.tables[0].colorIndex = 0;
    map.tables[1].p = makeVec(370, 200);
    map.tables[1].colorIndex = 1;
    map.tableCount = 2;

    addPath(60, 290, 400, 50, PAVEMENT);
    addPath(60, 60, 380, 50, PAVEMENT);
    addPath(410, 60, 45, 280, PAVEMENT);
    addPath(455, 290, 480, 50, PAVEMENT);

    map.railings[0].a = makeVec(450, 60);
    map.railings[0].b = makeVec(450, 240);
    map.railingCount = 1;
    addWall(360, 260, 14, 75);
    addWall(220, 140, 80, 14);
    addBench(400, 270, false);
    addBench(180, 100, false);
    addLamp(240, 280);
    addLamp(380, 280);
    addLamp(470, 280);

    addTree(280, 140, 24, RAIN_TREE);
    addTree(410, 220, 22, FLAME_TREE);
    addTree(880, 270, 24, MANGO_TREE);

    Guard &g1 = addGuard(120, 270, 250, 270, 2, 50, 180);
    g1.fov = 65;
    Guard &g2 = addGuard(220, 90, 400, 90, 1.5f, 75, 0);
    g2.fov = 65;
    Guard &g3 = addGuard(390, 250, 520, 160, 2, 60, 270);
    g3.fov = 65;
}

// ============================================================
// LEVEL 4 - THE MAIN VAULT
// ============================================================

void loadLevel4()
{
    level.name = "Level 4 - The Main Vault";
    level.objective = "Recover the stolen ledger";
    level.briefing = "Final step: the theft ledger is in the vault. Grab it and exit before the chief guard completes his sweep.";
    level.reward = "CASE FILE COMPLETE";
    level.passNames[0] = "Vault Log";
    level.passNames[1] = "Shift Roster";
    level.passNames[2] = "Evidence Ledger";
    level.passCount = 3;
    level.timeLimit = 110;
    level.spawn = makeVec(250, 325);
    level.goal = doorGoal(680, 345);
    level.checkpoints[0] = makeVec(180, 200);
    level.checkpoints[1] = makeVec(520, 250);
    level.checkpoints[2] = makeVec(820, 180);

    addBoundary(false);

    addBuilding(380, 345, 560, 120, HALL, "MAIN VAULT", 680);
    addBuilding(60, 340, 240, 125, HALL, "SECURITY OFFICE", 180);

    addPath(350, 90, 590, 255, PLAZA);
    addPath(60, 90, 300, 45, PAVEMENT);
    addPath(160, 135, 45, 205, PAVEMENT);

    map.signs[map.signCount++] = makeRect(200, 24, 230, 62);
    map.flags[map.flagCount++] = makeVec(160, 240);
    addFlowerBed(400, 315, 160, 20);
    addFlowerBed(720, 315, 160, 20);

    addWall(480, 220, 80, 14);
    addWall(660, 180, 14, 80);
    addWall(320, 160, 14, 80);

    addTree(320, 130, 24, RAIN_TREE);
    addTree(100, 210, 22, FLAME_TREE);
    addTree(280, 280, 22, MANGO_TREE);
    addTree(880, 130, 24, FLAME_TREE);
    addBench(440, 100, false);
    addBench(780, 100, false);
    addBench(900, 220, true);
    addLamp(360, 180);
    addLamp(550, 180);
    addLamp(750, 180);
    addLamp(600, 280);
    addLamp(760, 280);

    addGuard(820, 120, 480, 120, 2, 75, 180);
    addGuard(850, 160, 850, 280, 2, 60, 90);
    addGuard(650, 280, 520, 280, 2, 55, 180);
    addGuard(780, 240, 690, 240, 1.5f, 70, 180);
}

// ============================================================
// LOAD LEVEL
// ============================================================

void loadLevel(int index)
{
    memset(&map, 0, sizeof(map));
    memset(&level, 0, sizeof(level));
    memset(guards, 0, sizeof(guards));
    guardCount = 0;

    if (index == 0)
        loadLevel1();
    else if (index == 1)
        loadLevel2();
    else if (index == 2)
        loadLevel3();
    else
        loadLevel4();

    finishMap();
    for (int i = 0; i < guardCount; i++)
        resetGuard(guards[i]);
}

// Shown on the "level complete" screen as the next task
const char *levelObjective(int index)
{
    const char *objectives[LEVEL_COUNT] = {
        "Collect the entrance security log",
        "Locate the artifact registry",
        "Retrieve the curator's statement",
        "Recover the stolen ledger"};
    if (index < 0 || index >= LEVEL_COUNT)
        return "";
    return objectives[index];
}
