#include "game.h"

#define CONE_RAYS 72
#define DECAY_PER_SEC 0.24f
#define GUARD_SCALE 1.08f

static float angleDiff(float a, float b)
{
    return fmodf(a - b + 540.0f, 360.0f) - 180.0f;
}

static float directionAngle(Vec2 v)
{
    return radToDeg(atan2f(v.y, v.x));
}

static Color alertColor(float level)
{
    Color calm = hexColor(0xeefa7a);
    Color suspicious = hexColor(0xffa53d);
    Color red = hexColor(0xff4b3e);
    if (level < 0.5f)
        return mixColor(calm, suspicious, level / 0.5f);
    return mixColor(suspicious, red, (level - 0.5f) / 0.5f);
}

void resetGuard(Guard &g)
{
    g.pos = g.route[0].p;
    g.angle = g.startAngle;
    g.state = PATROLLING;
    g.target = 0;
    g.stateTimer = 0;
    g.detection = 0;
    g.seeing = false;
    g.walkPhase = 0;
    g.moveBlend = 0;
    g.clock = 0;
    g.pathCount = 0;
    g.pathIndex = 0;
    g.pathTimer = 0;
}

static bool guardCanSee(Guard &g, Vec2 p)
{
    Vec2 toTarget = vsub(p, g.pos);
    float dist = vlength(toTarget);
    if (dist > g.range)
        return false;

    if (dist > 0.001f)
    {
        float targetAngle = directionAngle(toTarget);
        float offCenter = fabsf(angleDiff(targetAngle, g.angle));
        if (offCenter > g.fov * 0.5f)
            return false;
    }

    return lineOfSight(g.pos, p);
}

static void turnTowards(Guard &g, float targetDeg, float degPerSec, float dt)
{
    float diff = angleDiff(targetDeg, g.angle);
    float maxTurn = degPerSec * dt;
    g.angle += clampf(diff, -maxTurn, maxTurn);
}

void guardHearNoise(Guard &g, Vec2 source, float radius)
{
    if (g.state == GUARD_CAUGHT || g.seeing)
        return;
    if (vlength(vsub(source, g.pos)) > radius)
        return;

    g.lastSeen = source;
    if (g.state != ALERT)
        g.stateTimer = 0;
    g.state = ALERT;
}

void guardReceiveReport(Guard &g, Vec2 lastKnown)
{
    if (g.seeing || g.state == GUARD_CAUGHT)
        return;

    g.lastSeen = lastKnown;
    if (g.state != ALERT)
        g.stateTimer = 0.8f;
    g.state = ALERT;
}

static void refreshPath(Guard &g, Vec2 target, float dt)
{
    g.pathTimer -= dt;
    bool timeUp = g.pathTimer <= 0;
    bool targetMoved = vlength(vsub(target, g.pathTarget)) > 45;
    if (!timeUp && !targetMoved)
        return;

    g.pathCount = findPath(g.pos, target, GUARD_RADIUS, g.path, GUARD_PATH_MAX);
    g.pathIndex = 0;
    g.pathTarget = target;
    g.pathTimer = 0.8f;
}

static void skipReachedPathPoints(Guard &g)
{
    while (g.pathIndex < g.pathCount && vlength(vsub(g.path[g.pathIndex], g.pos)) < 0.5f)
        g.pathIndex++;
}

static bool guardMoveTowards(Guard &g, Vec2 target, float speed, float dt)
{
    refreshPath(g, target, dt);
    skipReachedPathPoints(g);
    if (g.pathIndex >= g.pathCount)
        return false;

    Vec2 toNext = vsub(g.path[g.pathIndex], g.pos);
    turnTowards(g, directionAngle(toNext), 300, dt);

    float distance = vlength(toNext);
    Vec2 direction = vnormal(toNext);
    float stepLength = minf(distance, speed * dt);
    Vec2 step = vscale(direction, stepLength);
    Vec2 next = moveCircle(g.pos, step, GUARD_RADIUS);
    float movedDistance = vlength(vsub(next, g.pos));
    g.pos = next;
    return movedDistance > 0.001f;
}

static void goToNextWaypoint(Guard &g)
{
    g.target = (g.target + 1) % g.routeCount;
}

static void arriveAtWaypoint(Guard &g)
{
    if (g.route[g.target].searchTime > 0)
    {
        g.state = SEARCHING;
        g.stateTimer = 0;
        g.searchBase = g.angle;
    }
    else
    {
        goToNextWaypoint(g);
    }
}

static float searchProfile(float progress)
{
    float keyTime[5] = {0, 0.25f, 0.6f, 0.8f, 1};
    float keyTurn[5] = {0, 1, -1, 0, 0};

    for (int i = 0; i < 4; i++)
    {
        if (progress <= keyTime[i + 1])
        {
            float t = (progress - keyTime[i]) / (keyTime[i + 1] - keyTime[i]);
            float smooth = t * t * (3 - 2 * t);
            return keyTurn[i] + (keyTurn[i + 1] - keyTurn[i]) * smooth;
        }
    }
    return 0;
}

static void updateDetection(Guard &g, float dt, Vec2 playerPos)
{
    g.seeing = guardCanSee(g, playerPos);
    if (g.seeing)
    {
        g.lastSeen = playerPos;
        float dist = vlength(vsub(playerPos, g.pos));
        float secondsToCatch = 0.7f + 1.3f * (dist / g.range);
        g.detection += dt / (secondsToCatch * g.detectionTimeScale);
    }
    else
    {
        g.detection -= dt * DECAY_PER_SEC;
    }
    g.detection = clampf(g.detection, 0, 1);
}

static void catchPlayer(Guard &g, Vec2 playerPos)
{
    g.state = GUARD_CAUGHT;
    g.angle = directionAngle(vsub(playerPos, g.pos));
    g.moveBlend = 0;
}

static bool patrol(Guard &g, float dt)
{
    Vec2 waypoint = g.route[g.target].p;
    if (vlength(vsub(waypoint, g.pos)) < 1.5f)
    {
        g.pos = waypoint;
        arriveAtWaypoint(g);
        return false;
    }
    return guardMoveTowards(g, waypoint, g.speed, dt);
}

static void searchAround(Guard &g, float dt)
{
    float duration = g.route[g.target].searchTime;
    g.stateTimer += dt;
    float progress = minf(1, g.stateTimer / duration);
    g.angle = g.searchBase + g.searchSweep * searchProfile(progress);

    if (g.stateTimer >= duration)
    {
        goToNextWaypoint(g);
        g.state = PATROLLING;
    }
}

static bool chaseVisiblePlayer(Guard &g, float dt)
{
    Vec2 toPlayer = vsub(g.lastSeen, g.pos);
    g.stateTimer = 0;
    turnTowards(g, directionAngle(toPlayer), 220, dt);
    if (vlength(toPlayer) > 18)
        return guardMoveTowards(g, g.lastSeen, g.chaseSpeed, dt);
    return false;
}

static bool checkLastSeenSpot(Guard &g, float dt)
{
    Vec2 toSpot = vsub(g.lastSeen, g.pos);
    float dist = vlength(toSpot);
    bool walking = false;

    g.stateTimer += dt;
    if (g.stateTimer < 0.8f)
    {
        turnTowards(g, directionAngle(toSpot), 220, dt);
    }
    else if (dist > 14 && g.stateTimer < 6)
    {
        walking = guardMoveTowards(g, g.lastSeen, g.speed * 1.4f, dt);
        if (!walking)
            g.stateTimer = maxf(g.stateTimer, 4.5f);
    }
    else
    {
        g.angle += 110 * dt;
    }

    if (g.stateTimer > g.forgetTime && g.detection < 0.3f)
        g.state = PATROLLING;
    return walking;
}

static void updateWalkAnimation(Guard &g, float dt, bool walking)
{
    float targetBlend = 0.0f;
    if (walking)
        targetBlend = 1.0f;
    g.moveBlend += (targetBlend - g.moveBlend) * minf(1, dt * 10);
    g.walkPhase += dt * 9 * g.moveBlend;
}

void updateGuard(Guard &g, float dt, Vec2 playerPos, bool playerNoisy)
{
    if (g.state == GUARD_CAUGHT || g.routeCount == 0)
        return;
    g.clock += dt;

    updateDetection(g, dt, playerPos);
    if (g.seeing && g.detection >= 1)
    {
        catchPlayer(g, playerPos);
        return;
    }
    if (g.seeing && g.detection > 0.12f && g.state != ALERT)
    {
        g.state = ALERT;
        g.stateTimer = 0;
    }
    if (!g.seeing && playerNoisy && vlength(vsub(playerPos, g.pos)) < g.hearingRadius)
        guardHearNoise(g, playerPos, g.hearingRadius);

    bool walking = false;
    if (g.state == PATROLLING)
        walking = patrol(g, dt);
    else if (g.state == SEARCHING)
        searchAround(g, dt);
    else if (g.state == ALERT && g.seeing)
        walking = chaseVisiblePlayer(g, dt);
    else if (g.state == ALERT)
        walking = checkLastSeenSpot(g, dt);

    updateWalkAnimation(g, dt, walking);
}

static void findConeRim(Guard &g, Vec2 rim[])
{
    float firstAngle = g.angle - g.fov * 0.5f;
    for (int i = 0; i <= CONE_RAYS; i++)
    {
        float rayAngle = degToRad(firstAngle + g.fov * i / CONE_RAYS);
        Vec2 dir = makeVec(cosf(rayAngle), sinf(rayAngle));
        float length = raycast(g.pos, dir, g.range);
        rim[i] = vadd(g.pos, vscale(dir, length));
    }
}

static void drawConeFill(Guard &g, Vec2 rim[], Color c)
{
    float strength = 0.55f;
    if (g.seeing)
        strength = 0.66f;

    glBegin(GL_TRIANGLE_FAN);
    setColor(withAlpha(c, strength));
    glVertex2f(g.pos.x, g.pos.y);
    for (int i = 0; i <= CONE_RAYS; i++)
    {
        float distanceRatio = vlength(vsub(rim[i], g.pos)) / g.range;
        setColor(withAlpha(c, strength * (1 - 0.45f * distanceRatio)));
        glVertex2f(rim[i].x, rim[i].y);
    }
    glEnd();
}

static void drawConeOutline(Guard &g, Vec2 rim[], Color c)
{
    setColor(withAlpha(darker(c, 0.25f), 0.9f));
    glLineWidth(2);
    glBegin(GL_LINE_STRIP);
    glVertex2f(g.pos.x, g.pos.y);
    for (int i = 0; i <= CONE_RAYS; i++)
        glVertex2f(rim[i].x, rim[i].y);
    glVertex2f(g.pos.x, g.pos.y);
    glEnd();
}

static void drawLookLine(Guard &g, Color c)
{
    float lookAngle = degToRad(g.angle);
    Vec2 dir = makeVec(cosf(lookAngle), sinf(lookAngle));
    float length = raycast(g.pos, dir, g.range);
    setColor(withAlpha(c, 0.35f));
    DrawLine(g.pos.x, g.pos.y, g.pos.x + dir.x * length, g.pos.y + dir.y * length, 1);
}

void drawVisionCone(Guard &g)
{
    Vec2 rim[CONE_RAYS + 1];
    findConeRim(g, rim);

    Color c = alertColor(g.detection);
    drawConeFill(g, rim, c);
    drawConeOutline(g, rim, c);
    drawLookLine(g, c);
}

void drawGuard(Guard &g)
{
    drawCharacterShadow(g.pos, GUARD_SCALE);
    float bob = fabsf(sinf(g.walkPhase)) * 1.2f * g.moveBlend;
    float breathe = 1 + 0.012f * sinf(g.clock * 2.4f) * (1 - g.moveBlend);

    glPushMatrix();
    glTranslatef(g.pos.x, g.pos.y + bob, 0);
    glScalef(GUARD_SCALE, GUARD_SCALE * breathe, 1);
    useStyle(STYLE_GUARD);
    drawCharacter(directionFromAngle(g.angle), g.walkPhase, g.moveBlend);
    glPopMatrix();
}

static void drawDetectionBar(Guard &g, float x, float y)
{
    float w = 30, h = 5;
    setHex(0x1d2430, 0.75f);
    DrawRoundRect(x - w * 0.5f - 1.5f, y - 1.5f, w + 3, h + 3, 3);
    setColor(alertColor(g.detection));
    if (g.detection > 0.02f)
        DrawRoundRect(x - w * 0.5f, y, w * g.detection, h, 2);
}

static void drawAlertBadge(float x, float y, float t, bool alarm)
{
    float pulseSpeed = 6;
    unsigned int badgeColor = 0xffd23f;
    unsigned int signColor = 0x1d2430;
    const char *sign = "?";
    if (alarm)
    {
        pulseSpeed = 14;
        badgeColor = 0xff4b3e;
        signColor = 0xffffff;
        sign = "!";
    }
    float pop = 1 + 0.12f * sinf(t * pulseSpeed);

    glPushMatrix();
    glTranslatef(x, y + 17, 0);
    glScalef(pop, pop, 1);
    setHex(0x1d2430, 0.3f);
    circle(1.5f, -1.5f, 9);
    setHex(badgeColor);
    circle(0, 0, 9);
    glColor3ub(255, 255, 255);
    DrawCircleOutline(0, 0, 9, 9, 1.5f);
    glPopMatrix();

    setHex(signColor);
    DrawTextCentered(x, y + 12, sign, FONT_MEDIUM);
}

void drawGuardIndicator(Guard &g, float t)
{
    if (g.detection <= 0.01f && !g.seeing)
        return;
    float x = g.pos.x;
    float y = g.pos.y + 44;
    drawDetectionBar(g, x, y);

    bool worried = g.state == ALERT || g.state == GUARD_CAUGHT || g.detection >= 0.3f;
    if (!worried)
        return;
    bool alarm = g.detection >= 0.6f;
    drawAlertBadge(x, y, t, alarm);
}

void drawSpeechBubble(Guard &g)
{
    const char *line1 = CAUGHT_LINE1;
    const char *line2 = CAUGHT_LINE2;

    float textWidth = maxf(TextWidth(line1, FONT_MEDIUM), TextWidth(line2, FONT_MEDIUM));
    float w = textWidth + 28;
    float h = 58;
    float boxX = clampf(g.pos.x - w * 0.5f, 10, WORLD_W - w - 10);
    float boxY = minf(g.pos.y + 62, WORLD_H - h - 10);
    float tailX = clampf(g.pos.x, boxX + 16, boxX + w - 16);
    float tailTipY = maxf(boxY - 18, g.pos.y + 40);

    setHex(0x1d2430, 0.3f);
    DrawRoundRect(boxX + 4, boxY - 4, w, h, 12);
    glColor3ub(255, 255, 255);
    DrawRoundRect(boxX, boxY, w, h, 12);
    DrawTriangle(tailX - 8, boxY + 1, tailX + 8, boxY + 1, g.pos.x, tailTipY);
    glColor3ub(214, 60, 60);
    DrawRoundRectOutline(boxX, boxY, w, h, 12, 2.5f);
    DrawTextCentered(boxX + w * 0.5f, boxY + h - 24, line1, FONT_MEDIUM);
    glColor3ub(29, 36, 48);
    DrawTextCentered(boxX + w * 0.5f, boxY + 12, line2, FONT_MEDIUM);
}

void drawGuardRoute(Guard &g)
{
    if (g.routeCount > 1)
    {
        setHex(0x2f5f8c, 0.8f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < g.routeCount; i++)
            glVertex2f(g.route[i].p.x, g.route[i].p.y);
        glEnd();
    }

    for (int i = 0; i < g.routeCount; i++)
    {
        if (g.route[i].searchTime > 0)
            glColor3ub(255, 165, 61);
        else
            glColor3ub(47, 95, 140);

        float dotRadius = 4;
        if (i == g.target)
            dotRadius = 6;
        circle(g.route[i].p.x, g.route[i].p.y, dotRadius);
    }
}
