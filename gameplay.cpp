#include "game.h"

static float chaseHold = 0;

static bool isMenuMusicScreen(int s)
{
    return s == MAIN_MENU || s == NAME_ENTRY || s == SCOREBOARD ||
           s == INSTRUCTIONS || s == WIN;
}

static bool isLevelScreen(int s)
{
    return s == PLAYING || s == PAUSED || s == DIALOGUE || s == BRIEFING;
}

static void releaseAllKeys()
{
    for (int i = 0; i < 4; i++)
    {
        letterHeld[i] = false;
        arrowHeld[i] = false;
    }
    shiftHeld = false;
    spaceHeld = false;
}

void setScreen(int s)
{
    if (screen != s)
    {
        setMenuMusic(isMenuMusicScreen(s));
        if (!isLevelScreen(s))
        {
            chaseHold = 0;
            setChaseMusic(false);
        }
    }
    player.vel = makeVec(0, 0);
    player.sprinting = false;
    player.moveBlend = 0;
    screen = s;
    selected = 0;
    screenTime = 0;
    releaseAllKeys();
}

static int buildRoute(Vec2 start, Vec2 end, float radius, Vec2 out[])
{
    Vec2 from = nearestFreeSpot(start, radius);
    Vec2 to = nearestFreeSpot(end, radius);
    int pathLength = findPath(from, to, radius, out + 1, MAX_ROUTE - 1);
    if (pathLength == 0)
        return 0;
    out[0] = from;
    return pathLength + 1;
}

static void setupCrowd()
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
        Vec2 start = makeVec(crowdEnds[i][0], crowdEnds[i][1]);
        Vec2 end = makeVec(crowdEnds[i][2], crowdEnds[i][3]);
        int length = buildRoute(start, end, PLAYER_RADIUS, crowdRoute[crowdCount]);
        if (length > 0)
        {
            crowdLength[crowdCount] = length;
            crowdCount++;
        }
    }
}

static void setupCats()
{
    float catEnds[CAT_COUNT][4] = {
        {150, 300, 350, 310},
        {600, 150, 600, 100},
        {400, 220, 450, 180},
        {750, 300, 800, 280}};

    catCount = 0;
    for (int i = 0; i < CAT_COUNT; i++)
    {
        Vec2 start = makeVec(catEnds[i][0], catEnds[i][1]);
        Vec2 end = makeVec(catEnds[i][2], catEnds[i][3]);
        int length = buildRoute(start, end, 6, catRoute[catCount]);
        if (length > 0)
        {
            catLength[catCount] = length;
            catClock[catCount] = catCount * 2.7f;
            catAffection[catCount] = 0;
            catCount++;
        }
    }
}

static float routeLength(Vec2 route[], int length)
{
    float total = 0;
    for (int i = 1; i < length; i++)
        total += vlength(vsub(route[i], route[i - 1]));
    return total;
}

Vec2 routePose(Vec2 route[], int length, float distance, Vec2 *direction)
{
    float total = routeLength(route, length);
    *direction = makeVec(0, -1);
    if (total < 0.01f)
        return route[0];

    distance = fmodf(distance, total * 2);
    bool goingBack = distance > total;
    if (goingBack)
        distance = total * 2 - distance;

    for (int i = 1; i < length; i++)
    {
        Vec2 segment = vsub(route[i], route[i - 1]);
        float segmentLength = vlength(segment);
        if (segmentLength > 0 && distance <= segmentLength)
        {
            if (goingBack)
                *direction = vscale(segment, -1);
            else
                *direction = segment;
            float part = distance / segmentLength;
            return vadd(route[i - 1], vscale(segment, part));
        }
        distance -= segmentLength;
    }
    return route[length - 1];
}

CatPose catPose(int i)
{
    float clock = catClock[i];
    float phase = fmodf(clock, 12);
    float finishedCycles = floorf(clock / 12);
    float walkTime = finishedCycles * 8 + minf(phase, 8);
    float distance = walkTime * 18;

    CatPose pose;
    pose.position = routePose(catRoute[i], catLength[i], distance, &pose.direction);
    bool petting = catAffection[i] > 1.5f;
    if (petting)
        pose.direction = vsub(player.pos, pose.position);
    pose.moving = phase < 8 && !petting;
    pose.happy = catAffection[i] > 0;
    return pose;
}

void restartLevel()
{
    loadLevel(levelIndex);
    resetPlayer(level.spawn);
    catcher = -1;
    levelTime = 0;
    awaitingDeparture = true;
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
    chaseHold = 0;
    setChaseMusic(false);
    sprintf(message, "%s. J: view your case file. F: collect evidence.", level.objective);
    messageTime = 5;
    setupCrowd();
    setupCats();
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
    if (levelIndex == 0)
    {
        totalScore = 0;
        totalTime = 0;
    }
    retries = 0;
    restartLevel();
    setScreen(BRIEFING);
}

void retryLevel()
{
    retries++;
    totalTime += levelTime;
    restartLevel();
}

void goToMenu()
{
    levelIndex = 0;
    restartLevel();
    setScreen(MAIN_MENU);
}

float remainingTime()
{
    return maxf(0, level.timeLimit - levelTime);
}

int collectedCount()
{
    int count = 0;
    for (int i = 0; i < level.passCount; i++)
        if (collected[i])
            count++;
    return count;
}

static bool allEvidenceCollected()
{
    return collectedCount() == level.passCount;
}

static int stealthBonus()
{
    if (!spotted)
        return 500;
    if (!alarmed)
        return 250;
    return 0;
}

static void saveFinalScore()
{
    if (!rankedRun)
    {
        strcpy(scoreStatus, "Practice run - not entered on the scoreboard.");
        return;
    }
    if (recordScore(playerName, totalScore, totalTime))
        strcpy(scoreStatus, "Run saved. The best 10 runs appear on the scoreboard.");
    else
        strcpy(scoreStatus, "Could not save to disk. Scores are kept for this session.");
}

static void finishLevel()
{
    playSound(SOUND_COMPLETE);
    result.time = levelTime;
    result.timeBonus = (int)(600 * remainingTime() / level.timeLimit);
    result.stealthBonus = stealthBonus();
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
        saveFinalScore();
        setScreen(WIN);
    }
    else
        setScreen(LEVEL_COMPLETE);
}

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

static void chooseMainMenuItem(int item)
{
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
}

static void chooseNameEntryItem(int item)
{
    if (item == 1)
    {
        setScreen(MAIN_MENU);
        return;
    }
    char clean[20];
    cleanName(nameDraft, clean);
    if (clean[0] == '\0')
        return;
    strcpy(playerName, clean);
    rankedRun = true;
    scoreStatus[0] = '\0';
    startLevel(0);
}

static void choosePauseItem(int item)
{
    if (item == 0)
        setScreen(PLAYING);
    else if (item == 1)
        retryLevel();
    else
        goToMenu();
}

void activateItem(int item)
{
    switch (screen)
    {
    case MAIN_MENU:
        chooseMainMenuItem(item);
        break;
    case NAME_ENTRY:
        chooseNameEntryItem(item);
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
        choosePauseItem(item);
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

static bool directionHeld(int key)
{
    return letterHeld[key] || arrowHeld[key];
}

static void readMoveKeys(float *moveX, float *moveY)
{
    *moveX = 0;
    *moveY = 0;
    if (directionHeld(3))
        *moveX += 1;
    if (directionHeld(2))
        *moveX -= 1;
    if (directionHeld(0))
        *moveY += 1;
    if (directionHeld(1))
        *moveY -= 1;
}

static void countDownTimers(float dt, bool moving)
{
    levelTime += dt;
    distractionCooldown = maxf(0, distractionCooldown - dt);
    noiseTime = maxf(0, noiseTime - dt);
    radioCooldown -= dt;
    messageTime = maxf(0, messageTime - dt);
    pickupTime = maxf(0, pickupTime - dt);
    petTime = maxf(0, petTime - dt);
    alertCueCooldown = maxf(0, alertCueCooldown - dt);
    if (moving)
        petTime = 0;
}

static void updateCats(float dt)
{
    for (int i = 0; i < catCount; i++)
    {
        bool beingPetted = catAffection[i] > 1.5f;
        if (!beingPetted)
            catClock[i] += dt;
        catAffection[i] = maxf(0, catAffection[i] - dt);
    }
}

static void updateGuards(float dt)
{
    for (int i = 0; i < guardCount; i++)
    {
        bool wasSeeing = guards[i].seeing;
        updateGuard(guards[i], dt, player.pos, player.sprinting);
        bool startedSeeing = !wasSeeing && guards[i].seeing;
        if (startedSeeing && alertCueCooldown <= 0)
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
        if (guards[i].state == ALERT)
            chaseHold = 3;
    }
    chaseHold = maxf(0, chaseHold - dt);
    setChaseMusic(chaseHold > 0);
}

static void radioOtherGuards(int reporter)
{
    for (int i = 0; i < guardCount; i++)
    {
        float distance = vlength(vsub(guards[reporter].pos, guards[i].pos));
        if (distance < 480)
            guardReceiveReport(guards[i], player.pos);
    }
    radioCooldown = 2.5f;
    strcpy(message, "Security radio: detective spotted! Break sight and change direction.");
    messageTime = 2.5f;
}

static void checkSecurityRadio()
{
    if (radioCooldown > 0)
        return;
    for (int i = 0; i < guardCount; i++)
    {
        if (guards[i].seeing && guards[i].detection >= 0.4f)
        {
            radioOtherGuards(i);
            return;
        }
    }
}

static void checkExit()
{
    Rect exitArea = rectExpand(level.goal, PLAYER_RADIUS);
    if (!rectContains(exitArea, player.pos))
        return;
    if (allEvidenceCollected())
        finishLevel();
    else
    {
        strcpy(message, "Case incomplete. Collect the remaining evidence. J: checklist.");
        messageTime = 1;
    }
}

static void updatePlaying(float dt)
{
    worldTime += dt;
    float moveX, moveY;
    readMoveKeys(&moveX, &moveY);
    bool moving = (moveX != 0 || moveY != 0);

    if (awaitingDeparture)
    {
        if (!moving)
            return;
        awaitingDeparture = false;
    }

    countDownTimers(dt, moving);
    updatePlayer(dt, moveX, moveY, shiftHeld || spaceHeld);
    updateCats(dt);
    updateGuards(dt);
    checkSecurityRadio();

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
    checkExit();
}

static void updateMenuBackground(float dt)
{
    worldTime += dt;
    Vec2 nobody = makeVec(-9999, -9999);
    for (int i = 0; i < guardCount; i++)
        updateGuard(guards[i], dt, nobody, false);
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
        updateMenuBackground(dt);
        break;
    case PLAYING:
        updatePlaying(dt);
        break;
    case DIALOGUE:
        dialogueTime += dt;
        break;
    default:
        break;
    }
}

static Vec2 facingVector()
{
    if (player.facing == DIR_UP)
        return makeVec(0, 1);
    if (player.facing == DIR_DOWN)
        return makeVec(0, -1);
    if (player.facing == DIR_LEFT)
        return makeVec(-1, 0);
    return makeVec(1, 0);
}

int nearbyPass()
{
    for (int i = 0; i < level.passCount; i++)
    {
        if (collected[i])
            continue;
        Vec2 stand = level.checkpoints[i];
        float distance = vlength(vsub(stand, player.pos));
        if (distance < 28 && lineOfSight(player.pos, stand))
            return i;
    }
    return -1;
}

int nearbyCharacter()
{
    Vec2 facing = facingVector();
    int found = -1;
    float nearest = 65;
    for (int i = 0; i < NPC_COUNT + catCount; i++)
    {
        bool isNpc = i < NPC_COUNT;
        if (isNpc && !npcs[i].here)
            continue;

        Vec2 position;
        if (isNpc)
            position = npcs[i].position;
        else
            position = catPose(i - NPC_COUNT).position;

        Vec2 offset = vsub(position, player.pos);
        float distance = vlength(offset);
        bool closeEnough = isNpc || distance < 38;
        bool inFront = distance < 15 || vdot(vnormal(offset), facing) > 0.35f;
        if (distance < nearest && closeEnough && inFront && lineOfSight(player.pos, position))
        {
            nearest = distance;
            found = i;
        }
    }
    return found;
}

static Vec2 mouseView()
{
    float viewX = (mouseX - vpX) / pixelsPerUnit;
    float viewY = (winH - mouseY - vpY) / pixelsPerUnit;
    return makeVec(viewX, viewY);
}

bool mouseAimValid()
{
    if (!mouseKnown)
        return false;
    Vec2 view = mouseView();
    return view.x >= 0 && view.x < VIEW_W && view.y >= PLAY_BOTTOM && view.y < PLAY_TOP;
}

static Vec2 pointAhead(Vec2 direction, float distance)
{
    Vec2 point = vadd(player.pos, vscale(direction, distance));
    return clampToWorld(point, 12);
}

Vec2 distractionTarget()
{
    Vec2 direction = facingVector();
    float reach = 178;
    if (mouseAimValid())
    {
        Vec2 view = mouseView();
        Vec2 mouseWorld = makeVec(view.x, view.y - PLAY_BOTTOM);
        Vec2 toMouse = vsub(mouseWorld, player.pos);
        if (vlength(toMouse) < 0.001f)
            return player.pos;
        direction = vnormal(toMouse);
        reach = minf(190, vlength(toMouse));
    }

    float wallDistance = raycast(player.pos, direction, reach + 12) - 12;
    float distance = maxf(0, minf(reach, wallDistance));
    Vec2 target = pointAhead(direction, distance);
    while (distance > 0 && overlapsSolid(target, GUARD_RADIUS))
    {
        distance = maxf(0, distance - 10);
        target = pointAhead(direction, distance);
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

static void petTheCat(int cat)
{
    const char *catNames[4] = {"Tonmoy", "Shadow", "Mishti", "Professor Meow"};
    if (catAffection[cat] > 1.5f)
        return;
    catAffection[cat] = 3;
    petCat = cat;
    petTime = 0.85f;
    player.vel = makeVec(0, 0);
    sprintf(message, "%s leans into your hand. Prrrr...", catNames[cat % 4]);
    messageTime = 3;
    playSound(SOUND_PET);
}

static void writeRemainingEvidence(char task[])
{
    if (allEvidenceCollected())
    {
        strcpy(task, "Shob evidence pawa gese. Ebar gold exit e jao.");
        return;
    }
    strcpy(task, "Ekhono baki: ");
    bool first = true;
    for (int i = 0; i < level.passCount; i++)
    {
        if (collected[i])
            continue;
        if (!first)
            strcat(task, " ar ");
        strcat(task, level.passNames[i]);
        first = false;
    }
    strcat(task, ". Stand er kase giye F chapo.");
}

static bool isLevelContact(Npc &npc)
{
    int contacts[3] = {CONSULTANT, CURATOR, SELLER};
    return levelIndex < 3 && npc.kind == contacts[levelIndex];
}

static void talkToNpc(int who)
{
    Npc &npc = npcs[who];
    dialogueCharacter = who;
    dialogueTime = 0;
    speakerName = npc.name;
    speakerRole = npc.role;
    dialogueText[0] = '\0';

    if (isLevelContact(npc))
    {
        char task[300];
        writeRemainingEvidence(task);
        bool taskChanged = strcmp(task, npc.lastTask) != 0;
        if (taskChanged)
        {
            strcpy(dialogueText, task);
            strcpy(npc.lastTask, task);
        }
    }

    if (dialogueText[0] == '\0')
    {
        if (npc.lineCount == 0)
            return;
        strcpy(dialogueText, npc.lines[npc.visits % npc.lineCount]);
        npc.visits++;
    }
    setScreen(DIALOGUE);
}

static void talk()
{
    int who = nearbyCharacter();
    if (who < 0)
        return;
    awaitingDeparture = false;

    if (who >= NPC_COUNT)
        petTheCat(who - NPC_COUNT);
    else
        talkToNpc(who);
}

static void collectEvidence(int pass)
{
    awaitingDeparture = false;
    collected[pass] = true;
    pickupPos = level.checkpoints[pass];
    pickupTime = 1;
    playSound(SOUND_PICKUP);
    if (allEvidenceCollected())
        sprintf(message, "%s: %s  All evidence found - go to the gold exit!", level.passNames[pass], level.passClues[pass]);
    else
        sprintf(message, "%s: %s", level.passNames[pass], level.passClues[pass]);
    messageTime = 7;
}

void interact()
{
    int pass = nearbyPass();
    if (pass >= 0)
        collectEvidence(pass);
    else
        talk();
}

void advanceDialogue()
{
    int length = strlen(dialogueText);
    bool stillTyping = dialogueTime * 42 < length;
    if (stillTyping)
        dialogueTime = length / 42.0f + 1;
    else
        setScreen(PLAYING);
}

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
    D_STATUE,
    D_ART_VISITOR,
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
static DepthItem depthItems[MAX_DEPTH];
static int depthCount;

static void addDepth(float y, int type, int index)
{
    if (depthCount >= MAX_DEPTH)
        return;
    depthItems[depthCount].y = y;
    depthItems[depthCount].type = type;
    depthItems[depthCount].index = index;
    depthCount++;
}

static void sortDepth()
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

static Vec2 artVisitorPos(int i, Vec2 *dir)
{
    Vec2 center = rectCenter(map.fields[0]);
    bool clockwise = (i % 2 != 0);

    float angle = worldTime * 0.22f;
    if (clockwise)
        angle = -angle;
    angle += i * PI * 0.5f + (i % 2) * PI * 0.25f;

    float turn = 1;
    if (clockwise)
        turn = -1;
    *dir = makeVec(-sinf(angle) * 102 * turn, cosf(angle) * 55 * turn);
    return makeVec(center.x + cosf(angle) * 102, center.y + sinf(angle) * 55);
}

static Vec2 crowdPos(int i, Vec2 *dir)
{
    int speed = 36 + (i % 3) * 7;
    int startOffset = i * 137;
    float distance = worldTime * speed + startOffset;
    return routePose(crowdRoute[i], crowdLength[i], distance, dir);
}

static void drawVisitor(Vec2 p, Vec2 dir, unsigned int shirt, bool backpack, float phase, float blend)
{
    float scale = 0.92f;
    float angle = radToDeg(atan2f(dir.y, dir.x));
    drawCharacterShadow(p, 0.9f);
    glPushMatrix();
    glTranslatef(p.x, p.y, 0);
    glScalef(scale, scale, 1);
    useStyle(STYLE_DETECTIVE);
    setShirt(shirt);
    setBackpack(backpack);
    drawCharacter(directionFromAngle(angle), phase, blend);
    glPopMatrix();
}

static unsigned int visitorShirt(int number)
{
    unsigned int shirts[5] = {0xc57462, 0x8b9b68, 0x8b79aa, 0xd3b772, 0x67a5a1};
    return shirts[number % 5];
}

static void drawArtVisitor(int i)
{
    Vec2 dir;
    Vec2 p = artVisitorPos(i, &dir);
    drawVisitor(p, dir, visitorShirt(i + 2), i == 1, worldTime * 6 + i, 0.6f);
}

static void drawCrowdVisitor(int i)
{
    Vec2 dir;
    Vec2 p = crowdPos(i, &dir);
    drawVisitor(p, dir, visitorShirt(i), i % 2 == 0, worldTime * 7 + i, 0.7f);
}

static void drawPlayerWithPettingHand()
{
    float handWave = 0;
    if (petTime > 0)
        handWave = sinf(PI * (1 - petTime / 0.85f));
    drawPlayer(handWave);

    if (petTime <= 0 || petCat < 0)
        return;
    Vec2 cat = catPose(petCat).position;
    Vec2 hand = makeVec(cat.x, cat.y + 13 + sinf(worldTime * 18) * 1.5f);
    glColor3ub(201, 139, 98);
    DrawThickLine(player.pos.x + 6, player.pos.y + 13, hand.x, hand.y, 3);
    circle(hand.x, hand.y, 2.5f);
}

static void drawDepthItem(DepthItem item)
{
    int i = item.index;
    switch (item.type)
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
    case D_STATUE:
        drawStatue(gardenSpot(map.fields[0], i), i, worldTime);
        break;
    case D_ART_VISITOR:
        drawArtVisitor(i);
        break;
    case D_CROWD:
        drawCrowdVisitor(i);
        break;
    case D_NPC:
        drawNpc(i, npcs[i].position.x, npcs[i].position.y, worldTime);
        break;
    case D_CAT:
        drawCat(catPose(i), worldTime, i);
        break;
    case D_PLAYER:
        drawPlayerWithPettingHand();
        break;
    case D_GUARD:
        drawGuard(guards[i]);
        break;
    }
}

static void drawNameTag(float x, float y, const char *name, unsigned int boxColor, float alpha, float height, float corner)
{
    float width = TextWidth(name, FONT_SMALL) + 14;
    setHex(boxColor, alpha);
    DrawRoundRect(x - width / 2, y, width, height, corner);
}

static void drawEvidenceStand(int i)
{
    Vec2 p = level.checkpoints[i];
    setHex(0x63a9b0, 0.18f);
    DrawCircle(p.x, p.y, 22, 12, 0);
    glColor3ub(183, 222, 212);
    DrawCircleOutline(p.x, p.y, 22, 12, 1.5f);
    glColor3ub(118, 101, 82);
    DrawRoundRect(p.x - 13, p.y + 2, 26, 16, 2);
    glColor3ub(182, 156, 117);
    DrawRect(p.x - 12, p.y + 6, 24, 13);
    glColor3ub(255, 241, 207);
    DrawRect(p.x - 7, p.y + 9, 14, 14);
    glColor3ub(86, 140, 150);
    DrawRect(p.x - 5, p.y + 18, 10, 2);
    glColor3ub(170, 160, 138);
    DrawLine(p.x - 4, p.y + 15, p.x + 4, p.y + 15, 1);
    DrawLine(p.x - 4, p.y + 12, p.x + 3, p.y + 12, 1);

    if (vlength(vsub(p, player.pos)) < 140)
    {
        drawNameTag(p.x, p.y + 31, level.passNames[i], 0x26353d, 0.92f, 19, 4);
        glColor3ub(255, 237, 195);
        DrawTextCentered(p.x, p.y + 37, level.passNames[i], FONT_SMALL);
    }
}

static void drawExitMarker()
{
    Vec2 center = rectCenter(level.goal);
    float pulse = 1 + 0.15f * sinf(worldTime * 3);
    glPushMatrix();
    glTranslatef(center.x, center.y - 10, 0);
    glScalef(pulse, pulse, 1);
    DrawSoftCircle(0, 0, 30, 16, hexColor(0xffe066, 0.45f), hexColor(0xffe066, 0));
    setHex(0xffd23f, 0.9f);
    DrawCircleOutline(0, 0, 22, 11, 2);
    glPopMatrix();

    glColor3ub(255, 229, 160);
    if (allEvidenceCollected())
        DrawTextCentered(center.x, center.y + 27, "EXIT OPEN", FONT_SMALL);
    else
        DrawTextCentered(center.x, center.y + 27, "NEEDS ALL EVIDENCE", FONT_SMALL);
}

static void drawGoalMarker()
{
    for (int i = 0; i < level.passCount; i++)
        if (!collected[i])
            drawEvidenceStand(i);
    drawExitMarker();
}

static void drawNpcNames()
{
    for (int i = 0; i < NPC_COUNT; i++)
    {
        if (!npcs[i].here)
            continue;
        Vec2 p = npcs[i].position;
        if (vlength(vsub(p, player.pos)) >= 170)
            continue;
        drawNameTag(p.x, p.y + 49, npcs[i].name, 0x1d2430, 0.9f, 20, 5);
        glColor3ub(255, 229, 160);
        DrawTextCentered(p.x, p.y + 55, npcs[i].name, FONT_SMALL);
    }
}

static void drawSprintRing()
{
    float progress = fmodf(worldTime * 2.5f, 1);
    setHex(0xffffff, 0.35f * (1 - progress));
    DrawCircleOutline(player.pos.x, player.pos.y, 12 + progress * 30, 6 + progress * 15, 1.5f);
}

static void drawNoiseRing()
{
    setHex(0xffd23f, noiseTime / 2.5f);
    float radius = 12 + (2.5f - noiseTime) * 42;
    DrawCircleOutline(noisePos.x, noisePos.y, radius, radius, 2);
    DrawTextCentered(noisePos.x, noisePos.y + 15, "CLINK!", FONT_SMALL);
}

static void drawAimCrosshair()
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

    const char *label = "E";
    if (distractionCooldown > 0)
        label = "WAIT";
    else if (mouseAimValid())
        label = "CLICK";
    DrawTextCentered(t.x, t.y + 16, label, FONT_SMALL);
}

static void drawWorldOverlays()
{
    drawNpcNames();
    for (int i = 0; i < guardCount; i++)
        drawGuardIndicator(guards[i], worldTime);
    if (player.sprinting)
        drawSprintRing();
    if (noiseTime > 0)
        drawNoiseRing();

    bool canAim = !mouseKnown || mouseAimValid();
    if (distractions > 0 && screen == PLAYING && canAim)
        drawAimCrosshair();
}

static void drawPickupSparkle()
{
    float progress = 1 - pickupTime;
    for (int i = 0; i < 8; i++)
    {
        float angle = i * PI / 4;
        float x = pickupPos.x + cosf(angle) * (10 + progress * 32);
        float y = pickupPos.y + 10 + sinf(angle) * (6 + progress * 20);
        setHex(0xe6c779, pickupTime);
        circle(x, y, 2 * pickupTime);
    }
    setHex(0xfff0bf, pickupTime);
    DrawTextCentered(pickupPos.x, pickupPos.y + 38 + progress * 25, "COLLECTED", FONT_SMALL);
}

static float wallDepth(Rect wall)
{
    bool lying = wall.w >= wall.h;
    if (lying)
        return wall.y;
    return WORLD_H;
}

static void addSceneryDepths()
{
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
        addDepth(wallDepth(map.walls[i]), D_WALL, i);
    if (map.hasGate)
        addDepth(0, D_GATE, 0);
}

static void addGardenDepths()
{
    if (map.fieldCount == 0)
        return;
    for (int i = 0; i < 4; i++)
        addDepth(gardenSpot(map.fields[0], i).y - 6, D_STATUE, i);
    Vec2 unusedDir;
    for (int i = 0; i < 4; i++)
        addDepth(artVisitorPos(i, &unusedDir).y, D_ART_VISITOR, i);
}

static void addPeopleDepths(bool showPlayer)
{
    Vec2 unusedDir;
    for (int i = 0; i < crowdCount; i++)
        addDepth(crowdPos(i, &unusedDir).y, D_CROWD, i);
    for (int i = 0; i < NPC_COUNT; i++)
        if (npcs[i].here)
            addDepth(npcs[i].position.y, D_NPC, i);
    for (int i = 0; i < catCount; i++)
        addDepth(catPose(i).position.y, D_CAT, i);
    if (showPlayer)
        addDepth(player.pos.y, D_PLAYER, 0);
    for (int i = 0; i < guardCount; i++)
        addDepth(guards[i].pos.y, D_GUARD, i);
}

static void drawSortedByDepth(bool showPlayer)
{
    depthCount = 0;
    addSceneryDepths();
    addGardenDepths();
    addPeopleDepths(showPlayer);
    sortDepth();
    for (int i = 0; i < depthCount; i++)
        drawDepthItem(depthItems[i]);
}

void drawWorld(bool showPlayer)
{
    drawGround(worldTime);
    if (showPlayer)
        drawGoalMarker();
    for (int i = 0; i < guardCount; i++)
        drawVisionCone(guards[i]);

    drawSortedByDepth(showPlayer);
    drawCanopies(worldTime);

    if (showDebug)
    {
        drawCollisionDebug();
        for (int i = 0; i < guardCount; i++)
            drawGuardRoute(guards[i]);
    }
    if (showPlayer)
    {
        drawWorldOverlays();
        if (pickupTime > 0)
            drawPickupSparkle();
    }
    if (catcher >= 0)
        drawSpeechBubble(guards[catcher]);
}
