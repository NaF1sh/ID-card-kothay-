#include "game.h"

#define BORDER_WALL 14.0f
#define GATE_LEFT 80
#define GATE_RIGHT 200

static Rect makeRect(float x, float y, float w, float h)
{
    Rect r = {x, y, w, h};
    return r;
}

static void addEvidence(const char *name, float x, float y, const char *clue)
{
    int slot = level.passCount;
    level.passNames[slot] = name;
    level.passClues[slot] = clue;
    level.checkpoints[slot] = makeVec(x, y);
    level.passCount++;
}

static Rect doorGoal(float doorX, float buildingY)
{
    return makeRect(doorX - 20, buildingY - 14, 40, 14);
}

static void addPath(float x, float y, float w, float h, int kind)
{
    map.paths[map.pathCount].r = makeRect(x, y, w, h);
    map.paths[map.pathCount].kind = kind;
    map.pathCount++;
}

static void addCrossing(float x, float y, float w, float h)
{
    map.crossings[map.crossingCount] = makeRect(x, y, w, h);
    map.crossingCount++;
}

static void addBuilding(float x, float y, float w, float h, int kind, const char *name, float doorX)
{
    Building &b = map.buildings[map.buildingCount];
    b.r = makeRect(x, y, w, h);
    b.kind = kind;
    b.name = name;
    b.doorX = doorX;
    map.buildingCount++;
}

static void addWall(float x, float y, float w, float h)
{
    map.walls[map.wallCount] = makeRect(x, y, w, h);
    map.wallCount++;
}

static void addLake(float x, float y, float w, float h)
{
    map.lakes[map.lakeCount] = makeRect(x, y, w, h);
    map.lakeCount++;
}

static void addGarden(float x, float y, float w, float h)
{
    map.fields[map.fieldCount] = makeRect(x, y, w, h);
    map.fieldCount++;
}

static void addSign(float x, float y, float w, float h)
{
    map.signs[map.signCount] = makeRect(x, y, w, h);
    map.signCount++;
}

static void addFlag(float x, float y)
{
    map.flags[map.flagCount] = makeVec(x, y);
    map.flagCount++;
}

static void addTable(float x, float y, int colorIndex)
{
    map.tables[map.tableCount].p = makeVec(x, y);
    map.tables[map.tableCount].colorIndex = colorIndex;
    map.tableCount++;
}

static void addRailing(float x1, float y1, float x2, float y2)
{
    map.railings[map.railingCount].a = makeVec(x1, y1);
    map.railings[map.railingCount].b = makeVec(x2, y2);
    map.railingCount++;
}

static void addTree(float x, float y, float radius, int kind)
{
    Tree &t = map.trees[map.treeCount];
    t.p = makeVec(x, y);
    t.radius = radius;
    t.kind = kind;
    map.treeCount++;
}

static void addBush(float x, float y, float radius)
{
    map.bushes[map.bushCount].p = makeVec(x, y);
    map.bushes[map.bushCount].radius = radius;
    map.bushCount++;
}

static void addFlowerBed(float x, float y, float w, float h)
{
    map.flowerBeds[map.flowerBedCount] = makeRect(x, y, w, h);
    map.flowerBedCount++;
}

static void addBench(float x, float y, bool vertical)
{
    map.benches[map.benchCount].p = makeVec(x, y);
    map.benches[map.benchCount].vertical = vertical;
    map.benchCount++;
}

static void addLamp(float x, float y)
{
    map.lamps[map.lampCount] = makeVec(x, y);
    map.lampCount++;
}

static void addBoundary(bool hasGate)
{
    float t = BORDER_WALL;
    map.hasGate = hasGate;
    if (hasGate)
    {
        map.gateX0 = GATE_LEFT;
        map.gateX1 = GATE_RIGHT;
        float rightWallX = map.gateX1 + 16;
        addWall(0, 0, map.gateX0 - 16, t);
        addWall(rightWallX, 0, WORLD_W - rightWallX, t);
    }
    else
    {
        addWall(0, 0, WORLD_W, t);
    }
    addWall(0, WORLD_H - t, WORLD_W, t);
    addWall(0, t, t, WORLD_H - 2 * t);
    addWall(WORLD_W - t, t, t, WORLD_H - 2 * t);
}

static Guard &addGuard(float x1, float y1, float x2, float y2, float wait, float speed, float startAngle)
{
    Guard &g = guards[guardCount];
    guardCount++;
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

static void addRoutePoint(Guard &g, float x, float y, float wait)
{
    g.route[g.routeCount].p = makeVec(x, y);
    g.route[g.routeCount].searchTime = wait;
    g.routeCount++;
}

static void setGuardSight(Guard &g, float fov, float range)
{
    g.fov = fov;
    g.range = range;
}

static void addSolid(Rect r)
{
    map.solids[map.solidCount] = r;
    map.solidCount++;
}

static void addBlocker(Rect r)
{
    map.blockers[map.blockerCount] = r;
    map.blockerCount++;
}

static void addSolidCircle(Vec2 p, float r)
{
    map.circlePos[map.circleCount] = p;
    map.circleR[map.circleCount] = r;
    map.circleCount++;
}

static void addBoxSolids()
{
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
        addSolid(makeRect(map.gateX0 - 17, 0, 18, 20));
        addSolid(makeRect(map.gateX1 - 1, 0, 18, 20));
    }
    for (int i = 0; i < map.signCount; i++)
    {
        addSolid(rectExpand(map.signs[i], 4));
        addBlocker(map.signs[i]);
    }
}

static void addBenchSolids()
{
    for (int i = 0; i < map.benchCount; i++)
    {
        Vec2 p = map.benches[i].p;
        if (map.benches[i].vertical)
            addSolid(makeRect(p.x - 7, p.y - 18, 14, 36));
        else
            addSolid(makeRect(p.x - 18, p.y - 7, 36, 14));
    }
}

static void addRailingSolids()
{
    for (int i = 0; i < map.railingCount; i++)
    {
        Vec2 a = map.railings[i].a;
        Vec2 b = map.railings[i].b;
        float left = minf(a.x, b.x) - 1;
        float bottom = minf(a.y, b.y) - 1;
        float width = fabsf(b.x - a.x) + 2;
        float height = fabsf(b.y - a.y) + 4;
        addSolid(makeRect(left, bottom, width, height));
    }
}

static void addRoundSolids()
{
    for (int i = 0; i < map.bushCount; i++)
        addSolidCircle(map.bushes[i].p, map.bushes[i].radius * 0.8f);
    for (int i = 0; i < map.treeCount; i++)
        addSolidCircle(map.trees[i].p, 7);
    for (int i = 0; i < map.lampCount; i++)
        addSolidCircle(map.lamps[i], 4);
    for (int i = 0; i < map.flagCount; i++)
        addSolidCircle(map.flags[i], 6);
    for (int i = 0; i < map.tableCount; i++)
        addSolidCircle(map.tables[i].p, 12);
}

static void addStatueSolids()
{
    for (int i = 0; i < map.fieldCount; i++)
    {
        for (int statue = 0; statue < 4; statue++)
            addSolidCircle(gardenSpot(map.fields[i], statue), 11);
        addSolidCircle(gardenSpot(map.fields[i], 4), 24);
    }
}

void finishMap()
{
    map.solidCount = 0;
    map.blockerCount = 0;
    map.circleCount = 0;

    addBoxSolids();
    addBenchSolids();
    addRailingSolids();
    for (int i = 0; i < map.flowerBedCount; i++)
        addSolid(rectExpand(map.flowerBeds[i], 2));
    addRoundSolids();
    addStatueSolids();
}

static void loadLevel1()
{
    level.name = "Level 1 - Museum Entrance";
    level.objective = "Find out how your badge got here";
    level.briefing = "The Shapla Crown was stolen last night, and YOUR detective badge was found next to the empty case. Every guard thinks you are the thief. Sneak in and check the entrance records.";
    level.reward = "SOMEONE STOLE YOUR BADGE";
    level.timeLimit = 90;
    level.spawn = makeVec(140, 35);
    level.goal = doorGoal(230, 350);

    addEvidence("Badge Log", 230, 150, "Your badge was taken from the lost-and-found desk at 5 pm, the day before the theft.");
    addEvidence("CCTV Photo", 560, 250, "Camera 3: a hand in a guard's glove picks up your badge.");

    addBoundary(true);

    addPath(95, 0, 90, 130, ROAD);
    addPath(205, 120, 50, 235, PAVEMENT);
    addPath(50, 120, 160, 40, PAVEMENT);
    addPath(255, 120, 270, 40, PAVEMENT);
    addPath(500, 210, 240, 100, PAVEMENT);
    addCrossing(205, 120, 50, 40);

    addBuilding(50, 350, 340, 115, OFFICE, "CURATOR'S OFFICE", 230);
    addBuilding(14, 20, 48, 44, BOOTH, "SECURITY", 38);

    addLake(480, 80, 240, 130);

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

    Guard &g = addGuard(410, 240, 235, 260, 2, 50, 90);
    addRoutePoint(g, 380, 212, 2);
    setGuardSight(g, 65, 160);
    g.searchSweep = 45;
    g.detectionTimeScale = 2.4f;
    g.chaseSpeed = 125;
    g.hearingRadius = 115;
    g.forgetTime = 5;
}

static void loadLevel2()
{
    level.name = "Level 2 - Ancient Artifacts Wing";
    level.objective = "Prove you weren't here at 2:13 am";
    level.briefing = "The door records say your badge opened the crown room at 2:13 am. You were home asleep. Find out who was really in the Artifacts Wing that night.";
    level.reward = "YOU WEREN'T HERE AT 2:13";
    level.timeLimit = 110;
    level.spawn = makeVec(230, 335);
    level.goal = doorGoal(720, 350);

    addEvidence("Door Record", 445, 180, "Your badge opened the crown room at 2:13 am. Your phone shows you were home.");
    addEvidence("Alarm Panel", 725, 140, "The alarm was switched off at 2:10 am with a MASTER KEY. Badges can't do that.");

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

    Guard &g1 = addGuard(300, 95, 600, 95, 2, 50, 0);
    setGuardSight(g1, 60, 190);
    Guard &g2 = addGuard(880, 240, 880, 110, 2, 55, 270);
    setGuardSight(g2, 60, 170);
}

static void loadLevel3()
{
    level.name = "Level 3 - Paintings Gallery";
    level.objective = "Find who has a master key";
    level.briefing = "Someone with a master key turned off the alarm. A witness saw the thief cross the gallery. Collect the statements, and stay out of sight: the guards still think it was you.";
    level.reward = "THE THIEF WORE GUARD BOOTS";
    level.timeLimit = 125;
    level.spawn = makeVec(720, 335);
    level.goal = doorGoal(250, 350);

    addEvidence("Witness Note", 120, 240, "Arpita, watching reels at 2 am, saw someone in a detective coat and heavy guard boots.");
    addEvidence("Coat Receipt", 650, 212, "A detective coat was bought yesterday. Size: guard uniform XL.");
    addEvidence("Key List", 310, 210, "Only two master keys exist: one in the vault, one with the chief guard.");

    addBoundary(false);

    addBuilding(60, 350, 380, 115, GALLERY, "PAINTINGS GALLERY", 250);
    addBuilding(550, 350, 390, 115, OFFICE, "CURATOR'S OFFICE", 720);

    addGarden(460, 60, 380, 180);
    addTable(260, 210, 0);
    addTable(370, 200, 1);

    addPath(60, 290, 400, 50, PAVEMENT);
    addPath(60, 60, 380, 50, PAVEMENT);
    addPath(410, 60, 45, 280, PAVEMENT);
    addPath(455, 290, 480, 50, PAVEMENT);

    addRailing(450, 60, 450, 240);
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
    setGuardSight(g1, 60, 180);
    Guard &g2 = addGuard(220, 85, 380, 85, 2, 55, 0);
    setGuardSight(g2, 60, 180);
    Guard &g3 = addGuard(590, 95, 710, 95, 2, 50, 0);
    setGuardSight(g3, 60, 180);
}

static void loadLevel4()
{
    level.name = "Level 4 - The Main Vault";
    level.objective = "Catch the real thief";
    level.briefing = "Every clue points to the chief guard. Get into the vault, find proof and the crown itself, and get out before he finishes his sweep.";
    level.reward = "NAME CLEARED!";
    level.timeLimit = 130;
    level.spawn = makeVec(250, 325);
    level.goal = doorGoal(680, 345);

    addEvidence("Vault Log", 180, 200, "The vault's master key never left. So the chief guard's key was used.");
    addEvidence("Sale Ledger", 520, 250, "Crown sold to a buyer abroad. Payment goes to... the chief guard.");
    addEvidence("Shapla Crown", 730, 215, "Found it! The real crown, hidden in the chief guard's mop bucket.");

    addBoundary(false);

    addBuilding(380, 345, 560, 120, HALL, "MAIN VAULT", 680);
    addBuilding(60, 340, 240, 125, HALL, "SECURITY OFFICE", 180);

    addPath(350, 90, 590, 255, PLAZA);
    addPath(60, 90, 300, 45, PAVEMENT);
    addPath(160, 135, 45, 205, PAVEMENT);

    addSign(200, 24, 230, 62);
    addFlag(160, 240);
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

    Guard &g1 = addGuard(820, 120, 480, 120, 2, 55, 180);
    setGuardSight(g1, 60, 190);
    Guard &g2 = addGuard(850, 160, 850, 280, 2, 50, 90);
    setGuardSight(g2, 60, 170);
    Guard &g3 = addGuard(640, 275, 520, 275, 2.5f, 45, 180);
    setGuardSight(g3, 60, 160);
}

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

const char *levelObjective(int index)
{
    const char *objectives[LEVEL_COUNT] = {
        "Find out how your badge got here",
        "Prove you weren't here at 2:13 am",
        "Find who has a master key",
        "Catch the real thief"};

    if (index < 0 || index >= LEVEL_COUNT)
        return "";
    return objectives[index];
}
