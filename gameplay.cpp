// ============================================================
// gameplay.cpp
// Game flow (menus -> briefing -> playing -> results),
// the update of one frame, player actions (collect, talk,
// pet, throw a distraction) and drawing the whole world.
// ============================================================

#include "game.h"

// ============================================================
// SCREEN CHANGES
// ============================================================

void setScreen(int s)
{
    // the "caught" voice plays once when we enter that screen
    if (screen != s)
    {
        if (screen == CAUGHT)
            stopSound();
        if (s == CAUGHT)
            playSound(SOUND_CAUGHT);
    }
    player.vel = makeVec(0, 0);
    player.sprinting = false;
    player.moveBlend = 0;
    screen = s;
    selected = 0;
    screenTime = 0;
    for (int i = 0; i < 4; i++)
    {
        letterHeld[i] = false;
        arrowHeld[i] = false;
    }
    shiftHeld = false;
    spaceHeld = false;
}

// ============================================================
// WALKERS AND CATS
// ============================================================

// Builds a route: start point + A* path to the end point.
// Returns the number of points, or 0 if there is no way through.
int buildRoute(Vec2 a, Vec2 b, float radius, Vec2 out[])
{
    Vec2 from = nearestFreeSpot(a, radius);
    Vec2 to = nearestFreeSpot(b, radius);
    int n = findPath(from, to, radius, out + 1, MAX_ROUTE - 1);
    if (n == 0)
        return 0;
    out[0] = from;
    return n + 1;
}

void setupCrowdAndCats()
{
    float crowdEnds[CROWD_COUNT][4] = {
        {100, 200, 900, 200},
        {200, 150, 800, 150},
        {150, 300, 850, 300},
        {300, 100, 700, 100},
        {400, 350, 600, 350},
        {500, 240, 500, 100}};
    crowdCount = 0;
    for (int i = 0; i < CROWD_COUNT; i++)
    {
        int n = buildRoute(makeVec(crowdEnds[i][0], crowdEnds[i][1]),
                           makeVec(crowdEnds[i][2], crowdEnds[i][3]),
                           PLAYER_RADIUS, crowdRoute[crowdCount]);
        if (n > 0)
            crowdLength[crowdCount++] = n;
    }

    float catEnds[CAT_COUNT][4] = {
        {150, 300, 350, 310},
        {600, 150, 600, 100},
        {400, 220, 450, 180},
        {750, 300, 800, 280}};
    catCount = 0;
    for (int i = 0; i < CAT_COUNT; i++)
    {
        int n = buildRoute(makeVec(catEnds[i][0], catEnds[i][1]),
                           makeVec(catEnds[i][2], catEnds[i][3]),
                           6, catRoute[catCount]);
        if (n > 0)
        {
            catLength[catCount] = n;
            catClock[catCount] = catCount * 2.7f;
            catAffection[catCount] = 0;
            catCount++;
        }
    }
}

// Position along a route that goes there and back again (ping-pong).
// direction gets the walking direction.
Vec2 routePose(Vec2 route[], int length, float distance, Vec2 *direction)
{
    float total = 0;
    for (int i = 1; i < length; i++)
        total += vlength(vsub(route[i], route[i - 1]));
    *direction = makeVec(0, -1);
    if (total < 0.01f)
        return route[0];

    distance = fmodf(distance, total * 2);
    bool reverse = distance > total;
    if (reverse)
        distance = total * 2 - distance;

    for (int i = 1; i < length; i++)
    {
        Vec2 d = vsub(route[i], route[i - 1]);
        float seg = vlength(d);
        if (seg > 0 && distance <= seg)
        {
            *direction = reverse ? vscale(d, -1) : d;
            return vadd(route[i - 1], vscale(d, distance / seg));
        }
        distance -= seg;
    }
    return route[length - 1];
}

// Cats walk for 8 seconds, then sit for 4. A happy cat stops
// and looks at the player while being petted.
CatPose catPose(int i)
{
    float clock = catClock[i];
    float phase = fmodf(clock, 12);
    float distance = (floorf(clock / 12) * 8 + minf(phase, 8)) * 18;
    CatPose pose;
    pose.position = routePose(catRoute[i], catLength[i], distance, &pose.direction);
    bool petting = catAffection[i] > 1.5f;
    if (petting)
        pose.direction = vsub(player.pos, pose.position);
    pose.moving = phase < 8 && !petting;
    pose.happy = catAffection[i] > 0;
    return pose;
}

// ============================================================
// LEVEL START / RESTART
// ============================================================

void restartLevel()
{
    loadLevel(levelIndex);
    resetPlayer(level.spawn);
    catcher = -1;
    levelTime = 0;
    awaitingDeparture = true; // nothing moves until the player does
    spotted = false;
    alarmed = false;
    for (int i = 0; i < 3; i++)
        collected[i] = false;
    pickupTime = 0;
    petTime = 0;
    alertCueCooldown = 0;
    petCat = -1;
    distractions = 2;
    distractionCooldown = 0;
    noiseTime = 0;
    radioCooldown = 0;
    sprintf(message, "%s. J: view your case file. F: collect evidence.", level.objective);
    messageTime = 5;
    setupCrowdAndCats();
    setupNpcs();
    setScreen(PLAYING);
}

void startLevel(int index)
{
    if (index < 0)
        index = 0;
    if (index > LEVEL_COUNT - 1)
        index = LEVEL_COUNT - 1;
    levelIndex = index;
    if (levelIndex == 0) // a new run
    {
        totalScore = 0;
        totalTime = 0;
    }
    retries = 0;
    restartLevel();
    setScreen(BRIEFING);
}

// another attempt at the same level (costs 100 points)
void retryLevel()
{
    retries++;
    totalTime += levelTime;
    restartLevel();
}

void goToMenu()
{
    levelIndex = 0;
    restartLevel(); // the menu shows level 1 in the background
    setScreen(MAIN_MENU);
}

float remainingTime()
{
    return maxf(0, level.timeLimit - levelTime);
}

int collectedCount()
{
    int n = 0;
    for (int i = 0; i < level.passCount; i++)
        if (collected[i])
            n++;
    return n;
}

// Score = 1000 for clearing + time bonus + stealth bonus - retries
void finishLevel()
{
    playSound(SOUND_COMPLETE);
    result.time = levelTime;
    result.timeBonus = (int)(600 * remainingTime() / level.timeLimit);
    if (!spotted)
        result.stealthBonus = 500;
    else if (!alarmed)
        result.stealthBonus = 250;
    else
        result.stealthBonus = 0;
    result.retryPenalty = retries * 100;
    result.levelScore = 1000 + result.timeBonus + result.stealthBonus - result.retryPenalty;
    if (result.levelScore < 0)
        result.levelScore = 0;
    totalScore += result.levelScore;
    totalTime += levelTime;
    result.totalScore = totalScore;
    result.totalTime = totalTime;

    bool lastLevel = (levelIndex + 1 == LEVEL_COUNT);
    if (lastLevel)
    {
        if (rankedRun)
        {
            if (recordScore(playerName, totalScore, totalTime))
                strcpy(scoreStatus, "Run saved. The best 10 runs appear on the scoreboard.");
            else
                strcpy(scoreStatus, "Could not save to disk. Scores are kept for this session.");
        }
        else
            strcpy(scoreStatus, "Practice run - not entered on the scoreboard.");
    }
    setScreen(lastLevel ? WIN : LEVEL_COMPLETE);
}

// ============================================================
// MENU ITEMS
// ============================================================

int menuItemCount()
{
    switch (screen)
    {
    case MAIN_MENU:
        return 4;
    case NAME_ENTRY:
    case CAUGHT:
    case TIMED_OUT:
    case LEVEL_COMPLETE:
    case WIN:
        return 2;
    case PAUSED:
        return 3;
    case SCOREBOARD:
    case INSTRUCTIONS:
    case BRIEFING:
    case DIALOGUE:
        return 1;
    }
    return 0;
}

const char *menuItem(int i)
{
    const char *mainItems[] = {"Start Game", "Scoreboard", "Instructions", "Exit"};
    const char *nameItems[] = {"Let's Go", "Back"};
    const char *pauseItems[] = {"Resume", "Restart Level", "Main Menu"};
    const char *caughtItems[] = {"Restart Level", "Main Menu"};
    const char *completeItems[] = {"Continue", "Main Menu"};
    const char *winItems[] = {"Scoreboard", "Main Menu"};
    switch (screen)
    {
    case MAIN_MENU:
        return mainItems[i];
    case NAME_ENTRY:
        return nameItems[i];
    case PAUSED:
        return pauseItems[i];
    case CAUGHT:
    case TIMED_OUT:
        return caughtItems[i];
    case LEVEL_COMPLETE:
        return completeItems[i];
    case WIN:
        return winItems[i];
    case BRIEFING:
    case DIALOGUE:
        return "Back to the case";
    case SCOREBOARD:
    case INSTRUCTIONS:
        return "Back";
    }
    return "";
}

// What happens when a menu button is chosen
void activateItem(int item)
{
    char clean[20];
    switch (screen)
    {
    case MAIN_MENU:
        if (item == 0)
        {
            strcpy(nameDraft, playerName);
            setScreen(NAME_ENTRY);
        }
        else if (item == 1)
            setScreen(SCOREBOARD);
        else if (item == 2)
            setScreen(INSTRUCTIONS);
        else
            quitGame = true;
        break;
    case NAME_ENTRY:
        if (item == 1)
            setScreen(MAIN_MENU);
        else
        {
            cleanName(nameDraft, clean);
            if (clean[0] != '\0')
            {
                strcpy(playerName, clean);
                rankedRun = true;
                scoreStatus[0] = '\0';
                startLevel(0);
            }
        }
        break;
    case SCOREBOARD:
        goToMenu();
        break;
    case INSTRUCTIONS:
        setScreen(MAIN_MENU);
        break;
    case BRIEFING:
        setScreen(PLAYING);
        break;
    case DIALOGUE:
        advanceDialogue();
        break;
    case PAUSED:
        if (item == 0)
            setScreen(PLAYING);
        else if (item == 1)
            retryLevel();
        else
            goToMenu();
        break;
    case CAUGHT:
    case TIMED_OUT:
        if (item == 0)
            retryLevel();
        else
            goToMenu();
        break;
    case LEVEL_COMPLETE:
        if (item == 0)
            startLevel(levelIndex + 1);
        else
            goToMenu();
        break;
    case WIN:
        if (item == 0)
            setScreen(SCOREBOARD);
        else
            goToMenu();
        break;
    }
}

// ============================================================
// UPDATE (called 120 times per second)
// ============================================================

void moveAxis(float *ax, float *ay)
{
    bool up = letterHeld[0] || arrowHeld[0];
    bool down = letterHeld[1] || arrowHeld[1];
    bool left = letterHeld[2] || arrowHeld[2];
    bool right = letterHeld[3] || arrowHeld[3];
    *ax = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
    *ay = (up ? 1.0f : 0.0f) - (down ? 1.0f : 0.0f);
}

void updatePlaying(float dt)
{
    worldTime += dt;
    float ax, ay;
    moveAxis(&ax, &ay);
    bool moving = (ax != 0 || ay != 0);

    // patrols and the clock wait until the player starts moving
    if (awaitingDeparture)
    {
        if (!moving)
            return;
        awaitingDeparture = false;
    }

    levelTime += dt;
    distractionCooldown = maxf(0, distractionCooldown - dt);
    noiseTime = maxf(0, noiseTime - dt);
    radioCooldown -= dt;
    messageTime = maxf(0, messageTime - dt);
    pickupTime = maxf(0, pickupTime - dt);
    petTime = maxf(0, petTime - dt);
    alertCueCooldown = maxf(0, alertCueCooldown - dt);
    if (moving)
        petTime = 0; // moving cancels petting

    updatePlayer(dt, ax, ay, shiftHeld || spaceHeld);

    // cats walk unless they are being petted
    for (int i = 0; i < catCount; i++)
    {
        if (catAffection[i] <= 1.5f)
            catClock[i] += dt;
        catAffection[i] = maxf(0, catAffection[i] - dt);
    }

    // guards
    for (int i = 0; i < guardCount; i++)
    {
        bool wasSeeing = guards[i].seeing;
        updateGuard(guards[i], dt, player.pos, player.sprinting);
        if (!wasSeeing && guards[i].seeing && alertCueCooldown <= 0)
        {
            playSound(SOUND_ALERT);
            alertCueCooldown = 4;
        }
        if (guards[i].state == GUARD_CAUGHT && catcher < 0)
            catcher = i;
        if (guards[i].state == ALERT && guards[i].seeing)
            spotted = true;
        if (guards[i].detection >= 0.6f)
            alarmed = true;
    }

    // a guard who clearly sees the player radios the others
    if (radioCooldown <= 0)
    {
        for (int s = 0; s < guardCount; s++)
        {
            if (!guards[s].seeing || guards[s].detection < 0.4f)
                continue;
            for (int r = 0; r < guardCount; r++)
                if (vlength(vsub(guards[s].pos, guards[r].pos)) < 480)
                    guardReceiveReport(guards[r], player.pos);
            radioCooldown = 2.5f;
            strcpy(message, "Security radio: detective spotted! Break sight and change direction.");
            messageTime = 2.5f;
            break;
        }
    }

    if (catcher >= 0)
    {
        setScreen(CAUGHT);
        return;
    }
    if (remainingTime() <= 0)
    {
        setScreen(TIMED_OUT);
        return;
    }
    if (rectContains(rectExpand(level.goal, PLAYER_RADIUS), player.pos))
    {
        if (collectedCount() == level.passCount)
            finishLevel();
        else
        {
            strcpy(message, "Case incomplete. Collect the remaining evidence. J: checklist.");
            messageTime = 1;
        }
    }
}

void updateGame(float dt)
{
    uiTime += dt;
    screenTime += dt;
    switch (screen)
    {
    case MAIN_MENU:
    case NAME_ENTRY:
    case SCOREBOARD:
    case INSTRUCTIONS:
        // living background: guards keep patrolling (player far away)
        worldTime += dt;
        for (int i = 0; i < guardCount; i++)
            updateGuard(guards[i], dt, makeVec(-9999, -9999), false);
        break;
    case PLAYING:
        updatePlaying(dt);
        break;
    case DIALOGUE:
        dialogueTime += dt;
        break;
    default:
        break; // paused and result screens freeze the world
    }
}

// ============================================================
// PLAYER ACTIONS
// ============================================================

Vec2 facingVector()
{
    if (player.facing == DIR_UP)
        return makeVec(0, 1);
    if (player.facing == DIR_DOWN)
        return makeVec(0, -1);
    if (player.facing == DIR_LEFT)
        return makeVec(-1, 0);
    return makeVec(1, 0);
}

// evidence stand within reach (-1 if none)
int nearbyPass()
{
    for (int i = 0; i < level.passCount; i++)
        if (!collected[i] && vlength(vsub(level.checkpoints[i], player.pos)) < 28 &&
            lineOfSight(player.pos, level.checkpoints[i]))
            return i;
    return -1;
}

// NPC (0..5) or cat (6..) in front of the player, -1 if none
int nearbyCharacter()
{
    Vec2 facing = facingVector();
    int found = -1;
    float nearest = 65;
    for (int i = 0; i < NPC_COUNT + catCount; i++)
    {
        Vec2 p = (i < NPC_COUNT) ? npcs[i].position : catPose(i - NPC_COUNT).position;
        Vec2 d = vsub(p, player.pos);
        float dist = vlength(d);
        bool closeEnough = (i < NPC_COUNT) || dist < 38; // cats: must be closer
        bool inFront = dist < 15 || vdot(vnormal(d), facing) > 0.35f;
        if (dist < nearest && closeEnough && inFront && lineOfSight(player.pos, p))
        {
            nearest = dist;
            found = i;
        }
    }
    return found;
}

// mouse position (window pixels) -> view units (0..1000, 0..600)
Vec2 mouseView()
{
    return makeVec((mouseX - vpX) / pixelsPerUnit, (winH - mouseY - vpY) / pixelsPerUnit);
}

bool mouseAimValid()
{
    if (!mouseKnown)
        return false;
    Vec2 v = mouseView();
    return v.x >= 0 && v.x < VIEW_W && v.y >= PLAY_BOTTOM && v.y < PLAY_TOP;
}

// Where the distraction would land: towards the mouse (up to 190),
// or straight ahead (178) when playing with the keyboard only.
Vec2 distractionTarget()
{
    Vec2 dir = facingVector();
    float reach = 178;
    if (mouseAimValid())
    {
        Vec2 v = mouseView();
        Vec2 d = vsub(makeVec(v.x, v.y - PLAY_BOTTOM), player.pos);
        if (vlength(d) < 0.001f)
            return player.pos;
        dir = vnormal(d);
        reach = minf(190, vlength(d));
    }
    // stop in front of walls
    float dist = maxf(0, minf(reach, raycast(player.pos, dir, reach + 12) - 12));
    Vec2 target = clampToWorld(vadd(player.pos, vscale(dir, dist)), 12);
    while (dist > 0 && overlapsSolid(target, GUARD_RADIUS))
    {
        dist = maxf(0, dist - 10);
        target = clampToWorld(vadd(player.pos, vscale(dir, dist)), 12);
    }
    return target;
}

void throwDistraction()
{
    if (screen != PLAYING || distractions <= 0 || distractionCooldown > 0)
        return;
    if (mouseKnown && !mouseAimValid())
        return;
    awaitingDeparture = false;
    playSound(SOUND_DISTRACTION);
    distractions--;
    noisePos = distractionTarget();
    noiseTime = 2.5f;
    distractionCooldown = 1;
    for (int i = 0; i < guardCount; i++)
        guardHearNoise(guards[i], noisePos, 310);
    strcpy(message, "Clink! Nearby guards investigate the sound. Slip past while they turn.");
    messageTime = 2.5f;
}

void talk()
{
    int who = nearbyCharacter();
    if (who < 0)
        return;
    awaitingDeparture = false;

    // ---- cat: pet it ----
    if (who >= NPC_COUNT)
    {
        const char *catNames[4] = {"Biscuit", "Shadow", "Mishti", "Professor Meow"};
        int cat = who - NPC_COUNT;
        if (catAffection[cat] > 1.5f)
            return;
        catAffection[cat] = 3;
        petCat = cat;
        petTime = 0.85f;
        player.vel = makeVec(0, 0);
        sprintf(message, "%s leans into your hand. Prrrr...", catNames[cat % 4]);
        messageTime = 3;
        playSound(SOUND_PET);
        return;
    }

    // ---- NPC: talk ----
    Npc &npc = npcs[who];
    dialogueCharacter = who;
    dialogueTime = 0;
    speakerName = npc.name;
    speakerRole = npc.role;
    dialogueText[0] = '\0';

    // this level's contact person tells you what to collect
    int contacts[3] = {CONSULTANT, CURATOR, SELLER};
    if (levelIndex < 3 && npc.kind == contacts[levelIndex])
    {
        char task[300];
        if (collectedCount() == level.passCount)
            strcpy(task, "You have everything. Take the evidence to the gold exit to close this case.");
        else
        {
            strcpy(task, "I can help with your case file. Find ");
            bool first = true;
            for (int i = 0; i < level.passCount; i++)
            {
                if (collected[i])
                    continue;
                if (!first)
                    strcat(task, " and ");
                strcat(task, level.passNames[i]);
                first = false;
            }
            strcat(task, ". Press F at each evidence stand.");
        }
        if (strcmp(task, npc.lastTask) != 0) // only when the task changed
        {
            strcpy(dialogueText, task);
            strcpy(npc.lastTask, task);
        }
    }

    // otherwise the next line of their conversation
    if (dialogueText[0] == '\0')
    {
        if (npc.lineCount == 0)
            return;
        strcpy(dialogueText, npc.lines[npc.visits % npc.lineCount]);
        npc.visits++;
    }
    setScreen(DIALOGUE);
}

// F key: collect evidence if close, otherwise talk / pet
void interact()
{
    int pass = nearbyPass();
    if (pass >= 0)
    {
        awaitingDeparture = false;
        collected[pass] = true;
        pickupPos = level.checkpoints[pass];
        pickupTime = 1;
        playSound(SOUND_PICKUP);
        if (collectedCount() == level.passCount)
            sprintf(message, "%s collected. Case file ready! Go to the gold exit.", level.passNames[pass]);
        else
            sprintf(message, "%s collected. Check J for the remaining evidence.", level.passNames[pass]);
        messageTime = 4;
        return;
    }
    talk();
}

// text appears 42 letters per second; first press shows it all
void advanceDialogue()
{
    int len = strlen(dialogueText);
    if (dialogueTime * 42 < len)
        dialogueTime = len / 42.0f + 1;
    else
        setScreen(PLAYING);
}

// ============================================================
// DRAWING THE WORLD
// ============================================================
// Everything that stands up (buildings, trees, people...) is put
// in a list with the y of its feet. The list is sorted from top
// (far) to bottom (near), then drawn in that order, so nearer
// things cover the ones behind them.

enum DepthType
{
    D_BUSH,
    D_TABLE,
    D_BENCH,
    D_RAILING,
    D_LAMP,
    D_FLAG,
    D_SIGN,
    D_TREE,
    D_BUILDING,
    D_WALL,
    D_GATE,
    D_BALL,
    D_PLAYER_FOOTBALL,
    D_CROWD,
    D_NPC,
    D_CAT,
    D_PLAYER,
    D_GUARD
};

struct DepthItem
{
    float y;
    int type;
    int index;
};

#define MAX_DEPTH 200
DepthItem depthItems[MAX_DEPTH];
int depthCount;

void addDepth(float y, int type, int index)
{
    if (depthCount < MAX_DEPTH)
    {
        depthItems[depthCount].y = y;
        depthItems[depthCount].type = type;
        depthItems[depthCount].index = index;
        depthCount++;
    }
}

// insertion sort: bigger y (further away) first, keeps equal items in order
void sortDepth()
{
    for (int i = 1; i < depthCount; i++)
    {
        DepthItem key = depthItems[i];
        int j = i - 1;
        while (j >= 0 && depthItems[j].y < key.y)
        {
            depthItems[j + 1] = depthItems[j];
            j--;
        }
        depthItems[j + 1] = key;
    }
}

// football and footballers on the level 3 lawn
Vec2 ballPos()
{
    Vec2 c = rectCenter(map.fields[0]);
    return makeVec(c.x + sinf(worldTime * 0.65f) * 115, c.y + cosf(worldTime * 0.9f) * 45);
}

Vec2 footballerPos(int i)
{
    Vec2 c = rectCenter(map.fields[0]);
    return makeVec(c.x + sinf(worldTime * 0.65f + i * 0.5f) * 100,
                   c.y + cosf(worldTime * 0.9f + i * 1.7f) * 55);
}

Vec2 crowdPos(int i, Vec2 *dir)
{
    return routePose(crowdRoute[i], crowdLength[i], worldTime * (36 + (i % 3) * 7) + i * 137, dir);
}

void drawVisitor(Vec2 p, Vec2 dir, unsigned int shirt, bool backpack, float scale, float phase, float blend)
{
    drawCharacterShadow(p, 0.9f);
    glPushMatrix();
    glTranslatef(p.x, p.y, 0);
    glScalef(scale, scale, 1);
    useStyle(STYLE_DETECTIVE); // visitors use the same body, other shirts
    setShirt(shirt);
    setBackpack(backpack);
    drawCharacter(directionFromAngle(radToDeg(atan2f(dir.y, dir.x))), phase, blend);
    glPopMatrix();
}

void drawDepthItem(DepthItem it)
{
    int i = it.index;
    unsigned int shirts[5] = {0xc57462, 0x8b9b68, 0x8b79aa, 0xd3b772, 0x67a5a1};
    Vec2 p, dir;
    switch (it.type)
    {
    case D_BUSH:
        drawBush(i);
        break;
    case D_TABLE:
        drawTableBase(i);
        break;
    case D_BENCH:
        drawBench(i);
        break;
    case D_RAILING:
        drawRailing(i);
        break;
    case D_LAMP:
        drawLampBase(i);
        break;
    case D_FLAG:
        drawFlagPole(i);
        break;
    case D_SIGN:
        drawSign(i);
        break;
    case D_TREE:
        drawTreeTrunk(i);
        break;
    case D_BUILDING:
        drawBuilding(i, worldTime);
        break;
    case D_WALL:
        drawWall(i);
        break;
    case D_GATE:
        drawGate();
        break;
    case D_BALL:
        p = ballPos();
        drawCharacterShadow(p, 0.35f);
        setHex(0xffffff);
        circle(p.x, p.y + 4, 5);
        setHex(0x293544);
        circle(p.x, p.y + 4, 2);
        circle(p.x - 3, p.y + 6, 1);
        circle(p.x + 3, p.y + 2, 1);
        break;
    case D_PLAYER_FOOTBALL:
        p = footballerPos(i);
        drawVisitor(p, vsub(ballPos(), p), (i % 2) ? 0xc74f49 : 0xf0d573, false, 0.95f, worldTime * 11 + i, 0.9f);
        break;
    case D_CROWD:
        p = crowdPos(i, &dir);
        drawVisitor(p, dir, shirts[i % 5], i % 2 == 0, 0.92f, worldTime * 7 + i, 0.7f);
        break;
    case D_NPC:
        drawNpc(i, npcs[i].position.x, npcs[i].position.y, worldTime);
        break;
    case D_CAT:
        drawCat(catPose(i), worldTime, i);
        break;
    case D_PLAYER:
        drawPlayer(petTime > 0 ? sinf(PI * (1 - petTime / 0.85f)) : 0);
        if (petTime > 0 && petCat >= 0)
        {
            // arm reaching down to the cat
            Vec2 cat = catPose(petCat).position;
            Vec2 hand = makeVec(cat.x, cat.y + 13 + sinf(worldTime * 18) * 1.5f);
            setHex(0xc98b62);
            DrawThickLine(player.pos.x + 6, player.pos.y + 13, hand.x, hand.y, 3);
            circle(hand.x, hand.y, 2.5f);
        }
        break;
    case D_GUARD:
        drawGuard(guards[i]);
        break;
    }
}

// Evidence stands, the exit ring and its label
void drawGoalMarker()
{
    for (int i = 0; i < level.passCount; i++)
    {
        if (collected[i])
            continue;
        Vec2 p = level.checkpoints[i];
        setHex(0x63a9b0, 0.18f);
        DrawCircle(p.x, p.y, 22, 12, 0);
        setHex(0xb7ded4);
        DrawCircleOutline(p.x, p.y, 22, 12, 1.5f);
        // a paper folder on a low stand
        setHex(0x766552);
        DrawRoundRect(p.x - 13, p.y + 2, 26, 16, 2);
        setHex(0xb69c75);
        DrawRect(p.x - 12, p.y + 6, 24, 13);
        setHex(0xfff1cf);
        DrawRect(p.x - 7, p.y + 9, 14, 14);
        setHex(0x568c96);
        DrawRect(p.x - 5, p.y + 18, 10, 2);
        setHex(0xaaa08a);
        DrawLine(p.x - 4, p.y + 15, p.x + 4, p.y + 15, 1);
        DrawLine(p.x - 4, p.y + 12, p.x + 3, p.y + 12, 1);
        if (vlength(vsub(p, player.pos)) < 140)
        {
            float w = TextWidth(level.passNames[i], FONT_SMALL) + 14;
            setHex(0x26353d, 0.92f);
            DrawRoundRect(p.x - w / 2, p.y + 31, w, 19, 4);
            setHex(0xffedc3);
            DrawTextCentered(p.x, p.y + 37, level.passNames[i], FONT_SMALL);
        }
    }

    // pulsing gold ring at the exit (scale animation)
    Vec2 c = rectCenter(level.goal);
    float pulse = 1 + 0.15f * sinf(worldTime * 3);
    glPushMatrix();
    glTranslatef(c.x, c.y - 10, 0);
    glScalef(pulse, pulse, 1);
    DrawSoftCircle(0, 0, 30, 16, hexColor(0xffe066, 0.45f), hexColor(0xffe066, 0));
    setHex(0xffd23f, 0.9f);
    DrawCircleOutline(0, 0, 22, 11, 2);
    glPopMatrix();
    setHex(0xffe5a0);
    DrawTextCentered(c.x, c.y + 27, collectedCount() == level.passCount ? "EXIT OPEN" : "NEEDS ALL EVIDENCE", FONT_SMALL);
}

// Small things drawn over the world while playing
void drawWorldOverlays()
{
    // name tags of nearby NPCs
    for (int i = 0; i < NPC_COUNT; i++)
    {
        Vec2 p = npcs[i].position;
        if (vlength(vsub(p, player.pos)) >= 170)
            continue;
        float w = TextWidth(npcs[i].name, FONT_SMALL) + 14;
        setHex(0x1d2430, 0.9f);
        DrawRoundRect(p.x - w / 2, p.y + 49, w, 20, 5);
        setHex(0xffe5a0);
        DrawTextCentered(p.x, p.y + 55, npcs[i].name, FONT_SMALL);
    }

    for (int i = 0; i < guardCount; i++)
        drawGuardIndicator(guards[i], worldTime);

    // footstep rings while sprinting (sprinting is noisy)
    if (player.sprinting)
    {
        float k = fmodf(worldTime * 2.5f, 1);
        setHex(0xffffff, 0.35f * (1 - k));
        DrawCircleOutline(player.pos.x, player.pos.y, 12 + k * 30, 6 + k * 15, 1.5f);
    }

    // CLINK ring where the distraction landed
    if (noiseTime > 0)
    {
        setHex(0xffd23f, noiseTime / 2.5f);
        float r = 12 + (2.5f - noiseTime) * 42;
        DrawCircleOutline(noisePos.x, noisePos.y, r, r, 2);
        DrawTextCentered(noisePos.x, noisePos.y + 15, "CLINK!", FONT_SMALL);
    }

    // aiming crosshair
    if (distractions > 0 && screen == PLAYING && (!mouseKnown || mouseAimValid()))
    {
        Vec2 t = distractionTarget();
        if (distractionCooldown > 0)
            setHex(0xb5b9b9, 0.6f);
        else
            setHex(0xffe5a0, 0.8f);
        DrawCircleOutline(t.x, t.y, 8, 8, 1);
        DrawLine(t.x - 12, t.y, t.x - 5, t.y, 1);
        DrawLine(t.x + 5, t.y, t.x + 12, t.y, 1);
        DrawLine(t.x, t.y - 12, t.x, t.y - 5, 1);
        DrawLine(t.x, t.y + 5, t.x, t.y + 12, 1);
        const char *label = distractionCooldown > 0 ? "WAIT" : (mouseAimValid() ? "CLICK" : "E");
        DrawTextCentered(t.x, t.y + 16, label, FONT_SMALL);
    }
}

void drawPickupSparkle()
{
    float k = 1 - pickupTime;
    for (int i = 0; i < 8; i++)
    {
        float a = i * PI / 4;
        setHex(0xe6c779, pickupTime);
        circle(pickupPos.x + cosf(a) * (10 + k * 32),
               pickupPos.y + 10 + sinf(a) * (6 + k * 20), 2 * pickupTime);
    }
    setHex(0xfff0bf, pickupTime);
    DrawTextCentered(pickupPos.x, pickupPos.y + 38 + k * 25, "COLLECTED", FONT_SMALL);
}

// Draws the museum, everyone in it and the in-world markers.
// Coordinates here are world units (0..1000, 0..478).
void drawWorld(bool showPlayer)
{
    drawGround(worldTime);
    if (showPlayer)
        drawGoalMarker();
    for (int i = 0; i < guardCount; i++)
        drawVisionCone(guards[i]);

    // ---- build the depth list ----
    depthCount = 0;
    for (int i = 0; i < map.bushCount; i++)
        addDepth(map.bushes[i].p.y, D_BUSH, i);
    for (int i = 0; i < map.tableCount; i++)
        addDepth(map.tables[i].p.y, D_TABLE, i);
    for (int i = 0; i < map.benchCount; i++)
        addDepth(map.benches[i].p.y - 6, D_BENCH, i);
    for (int i = 0; i < map.railingCount; i++)
        addDepth(minf(map.railings[i].a.y, map.railings[i].b.y), D_RAILING, i);
    for (int i = 0; i < map.lampCount; i++)
        addDepth(map.lamps[i].y, D_LAMP, i);
    for (int i = 0; i < map.flagCount; i++)
        addDepth(map.flags[i].y, D_FLAG, i);
    for (int i = 0; i < map.signCount; i++)
        addDepth(map.signs[i].y, D_SIGN, i);
    for (int i = 0; i < map.treeCount; i++)
        addDepth(map.trees[i].p.y, D_TREE, i);
    for (int i = 0; i < map.buildingCount; i++)
        addDepth(map.buildings[i].r.y, D_BUILDING, i);
    for (int i = 0; i < map.wallCount; i++)
    {
        Rect w = map.walls[i];
        addDepth(w.w >= w.h ? w.y : WORLD_H, D_WALL, i); // side walls always behind
    }
    if (map.hasGate)
        addDepth(0, D_GATE, 0);
    if (map.fieldCount > 0)
    {
        addDepth(ballPos().y, D_BALL, 0);
        for (int i = 0; i < 4; i++)
            addDepth(footballerPos(i).y, D_PLAYER_FOOTBALL, i);
    }
    Vec2 dir;
    for (int i = 0; i < crowdCount; i++)
        addDepth(crowdPos(i, &dir).y, D_CROWD, i);
    for (int i = 0; i < NPC_COUNT; i++)
        addDepth(npcs[i].position.y, D_NPC, i);
    for (int i = 0; i < catCount; i++)
        addDepth(catPose(i).position.y, D_CAT, i);
    if (showPlayer)
        addDepth(player.pos.y, D_PLAYER, 0);
    for (int i = 0; i < guardCount; i++)
        addDepth(guards[i].pos.y, D_GUARD, i);

    sortDepth();
    for (int i = 0; i < depthCount; i++)
        drawDepthItem(depthItems[i]);

    drawCanopies(worldTime); // tree tops, lamp light, clouds

    if (showDebug)
    {
        drawCollisionDebug();
        for (int i = 0; i < guardCount; i++)
            drawGuardRoute(guards[i]);
    }
    if (showPlayer)
        drawWorldOverlays();
    if (showPlayer && pickupTime > 0)
        drawPickupSparkle();
    if (catcher >= 0)
        drawSpeechBubble(guards[catcher]);
}
