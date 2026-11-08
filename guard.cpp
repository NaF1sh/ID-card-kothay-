// ============================================================
// guard.cpp
// Museum security guard.
//  States: PATROLLING -> SEARCHING (look left/right at a stop)
//          ALERT (saw or heard something) -> GUARD_CAUGHT
//  Detection = distance check + angle check + line of sight
// ============================================================

#include "game.h"

#define CONE_RAYS 72      // rays used to cut the cone at walls
#define DECAY_PER_SEC 0.24f
#define GUARD_SCALE 1.08f // guards are a bit bigger than the detective

// Smallest difference between two angles (-180..180 degrees)
float angleDiff(float a, float b)
{
    return fmodf(a - b + 540.0f, 360.0f) - 180.0f;
}

// Yellow-green (calm) -> orange (suspicious) -> red (almost caught)
Color alertColor(float level)
{
    Color calm = hexColor(0xeefa7a);
    Color sus = hexColor(0xffa53d);
    Color red = hexColor(0xff4b3e);
    if (level < 0.5f)
        return mixColor(calm, sus, level / 0.5f);
    return mixColor(sus, red, (level - 0.5f) / 0.5f);
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

// ============================================================
// DETECTION
// ============================================================

bool guardCanSee(Guard &g, Vec2 p)
{
    Vec2 d = vsub(p, g.pos);
    float dist = vlength(d);

    // 1. distance
    if (dist > g.range)
        return false;

    // 2. angle (inside the cone?)
    if (dist > 0.001f)
    {
        float toTarget = radToDeg(atan2f(d.y, d.x));
        if (fabsf(angleDiff(toTarget, g.angle)) > g.fov * 0.5f)
            return false;
    }

    // 3. obstacles
    return lineOfSight(g.pos, p);
}

void turnTowards(Guard &g, float targetDeg, float degPerSec, float dt)
{
    float diff = angleDiff(targetDeg, g.angle);
    float step = degPerSec * dt;
    g.angle += clampf(diff, -step, step); // smooth turn, never overshoots
}

void guardHearNoise(Guard &g, Vec2 source, float radius)
{
    if (g.state == GUARD_CAUGHT || g.seeing || vlength(vsub(source, g.pos)) > radius)
        return;
    g.lastSeen = source;
    if (g.state != ALERT)
        g.stateTimer = 0;
    g.state = ALERT;
}

// Another guard saw the detective and called on the radio
void guardReceiveReport(Guard &g, Vec2 lastKnown)
{
    if (g.seeing || g.state == GUARD_CAUGHT)
        return;
    g.lastSeen = lastKnown;
    if (g.state != ALERT)
        g.stateTimer = 0.8f;
    g.state = ALERT;
}

// ============================================================
// MOVEMENT (follows an A* path, replanned every 0.8 s)
// ============================================================

bool guardMoveTowards(Guard &g, Vec2 target, float speed, float dt)
{
    g.pathTimer -= dt;
    if (g.pathTimer <= 0 || vlength(vsub(target, g.pathTarget)) > 45)
    {
        g.pathCount = findPath(g.pos, target, GUARD_RADIUS, g.path, MAX_PATH);
        g.pathIndex = 0;
        g.pathTarget = target;
        g.pathTimer = 0.8f;
    }
    // skip points we are already standing on
    while (g.pathIndex < g.pathCount && vlength(vsub(g.path[g.pathIndex], g.pos)) < 0.5f)
        g.pathIndex++;
    if (g.pathIndex >= g.pathCount)
        return false;

    Vec2 to = vsub(g.path[g.pathIndex], g.pos);
    turnTowards(g, radToDeg(atan2f(to.y, to.x)), 300, dt);
    Vec2 step = vscale(vnormal(to), minf(vlength(to), speed * dt));
    Vec2 next = moveCircle(g.pos, step, GUARD_RADIUS);
    bool moved = vlength(vsub(next, g.pos)) > 0.001f;
    g.pos = next;
    return moved;
}

void arriveAtWaypoint(Guard &g)
{
    if (g.route[g.target].searchTime > 0)
    {
        g.state = SEARCHING;
        g.stateTimer = 0;
        g.searchBase = g.angle;
    }
    else
    {
        g.target = (g.target + 1) % g.routeCount;
    }
}

// Look left, look right, back to centre, pause.
// Key frames with smoothstep easing between them.
float searchProfile(float u)
{
    float key[5][2] = {{0, 0}, {0.25f, 1}, {0.6f, -1}, {0.8f, 0}, {1, 0}};
    for (int i = 0; i < 4; i++)
    {
        if (u <= key[i + 1][0])
        {
            float k = (u - key[i][0]) / (key[i + 1][0] - key[i][0]);
            k = k * k * (3 - 2 * k);
            return key[i][1] + (key[i + 1][1] - key[i][1]) * k;
        }
    }
    return 0;
}

// ============================================================
// UPDATE
// ============================================================

void updateGuard(Guard &g, float dt, Vec2 playerPos, bool playerNoisy)
{
    if (g.state == GUARD_CAUGHT || g.routeCount == 0)
        return;
    g.clock += dt;

    // ---- detection meter ----
    g.seeing = guardCanSee(g, playerPos);
    if (g.seeing)
    {
        g.lastSeen = playerPos;
        float dist = vlength(vsub(playerPos, g.pos));
        float secondsToCatch = 0.7f + 1.3f * (dist / g.range); // closer = faster
        g.detection += dt / (secondsToCatch * g.detectionTimeScale);
    }
    else
    {
        g.detection -= dt * DECAY_PER_SEC;
    }
    g.detection = clampf(g.detection, 0, 1);

    // full meter while seeing = caught
    if (g.seeing && g.detection >= 1)
    {
        g.state = GUARD_CAUGHT;
        g.angle = radToDeg(atan2f(playerPos.y - g.pos.y, playerPos.x - g.pos.x));
        g.moveBlend = 0;
        return;
    }
    if (g.seeing && g.detection > 0.12f && g.state != ALERT)
    {
        g.state = ALERT;
        g.stateTimer = 0;
    }
    // hearing: running footsteps nearby
    if (!g.seeing && playerNoisy && vlength(vsub(playerPos, g.pos)) < g.hearingRadius)
        guardHearNoise(g, playerPos, g.hearingRadius);

    // ---- behaviour ----
    bool walking = false;
    switch (g.state)
    {
    case PATROLLING:
    {
        Vec2 wp = g.route[g.target].p;
        if (vlength(vsub(wp, g.pos)) < 1.5f)
        {
            g.pos = wp;
            arriveAtWaypoint(g);
            break;
        }
        walking = guardMoveTowards(g, wp, g.speed, dt);
        break;
    }
    case SEARCHING:
    {
        float dur = g.route[g.target].searchTime;
        g.stateTimer += dt;
        g.angle = g.searchBase + g.searchSweep * searchProfile(minf(1, g.stateTimer / dur));
        if (g.stateTimer >= dur)
        {
            g.target = (g.target + 1) % g.routeCount;
            g.state = PATROLLING;
        }
        break;
    }
    case ALERT:
    {
        // stare at the spot, walk over to check, then scan around
        Vec2 to = vsub(g.lastSeen, g.pos);
        float dist = vlength(to);
        if (g.seeing)
        {
            g.stateTimer = 0;
            turnTowards(g, radToDeg(atan2f(to.y, to.x)), 220, dt);
            if (dist > 18)
                walking = guardMoveTowards(g, g.lastSeen, g.chaseSpeed, dt);
            break;
        }
        g.stateTimer += dt;
        if (g.stateTimer < 0.8f)
        {
            turnTowards(g, radToDeg(atan2f(to.y, to.x)), 220, dt);
        }
        else if (dist > 14 && g.stateTimer < 6)
        {
            walking = guardMoveTowards(g, g.lastSeen, g.speed * 1.4f, dt);
            if (!walking)
                g.stateTimer = maxf(g.stateTimer, 4.5f); // blocked: just look around
        }
        else
        {
            g.angle += 110 * dt; // scan the area
        }
        if (g.stateTimer > g.forgetTime && g.detection < 0.3f)
            g.state = PATROLLING;
        break;
    }
    default:
        break;
    }

    g.moveBlend += ((walking ? 1.0f : 0.0f) - g.moveBlend) * minf(1, dt * 10);
    g.walkPhase += dt * 9 * g.moveBlend;
}

// ============================================================
// VISION CONE (GL_TRIANGLE_FAN cut by walls)
// ============================================================

void drawVisionCone(Guard &g)
{
    Vec2 rim[CONE_RAYS + 1];
    for (int i = 0; i <= CONE_RAYS; i++)
    {
        float a = degToRad(g.angle - g.fov * 0.5f + g.fov * i / CONE_RAYS);
        Vec2 dir = makeVec(cosf(a), sinf(a));
        float len = raycast(g.pos, dir, g.range);
        rim[i] = vadd(g.pos, vscale(dir, len));
    }

    Color c = alertColor(g.detection);
    float strength = g.seeing ? 0.66f : 0.55f;

    // transparent fan, fading towards the rim
    glBegin(GL_TRIANGLE_FAN);
    setColor(withAlpha(c, strength));
    glVertex2f(g.pos.x, g.pos.y);
    for (int i = 0; i <= CONE_RAYS; i++)
    {
        float t = vlength(vsub(rim[i], g.pos)) / g.range;
        setColor(withAlpha(c, strength * (1 - 0.45f * t)));
        glVertex2f(rim[i].x, rim[i].y);
    }
    glEnd();

    // outline
    setColor(withAlpha(darker(c, 0.25f), 0.9f));
    glLineWidth(2);
    glBegin(GL_LINE_STRIP);
    glVertex2f(g.pos.x, g.pos.y);
    for (int i = 0; i <= CONE_RAYS; i++)
        glVertex2f(rim[i].x, rim[i].y);
    glVertex2f(g.pos.x, g.pos.y);
    glEnd();

    // centre "gaze" line
    float a = degToRad(g.angle);
    Vec2 dir = makeVec(cosf(a), sinf(a));
    float len = raycast(g.pos, dir, g.range);
    setColor(withAlpha(c, 0.35f));
    DrawLine(g.pos.x, g.pos.y, g.pos.x + dir.x * len, g.pos.y + dir.y * len, 1);
}

// ============================================================
// GUARD BODY, METER, SPEECH BUBBLE
// ============================================================

void drawGuard(Guard &g)
{
    drawCharacterShadow(g.pos, GUARD_SCALE);
    // walk bob while moving, slow breathing while standing
    float bob = fabsf(sinf(g.walkPhase)) * 1.2f * g.moveBlend;
    float breathe = 1 + 0.012f * sinf(g.clock * 2.4f) * (1 - g.moveBlend);

    glPushMatrix();
    glTranslatef(g.pos.x, g.pos.y + bob, 0);
    glScalef(GUARD_SCALE, GUARD_SCALE * breathe, 1);
    useStyle(STYLE_GUARD);
    drawCharacter(directionFromAngle(g.angle), g.walkPhase, g.moveBlend);
    glPopMatrix();
}

// Detection meter above the head, plus "?" or "!"
void drawGuardIndicator(Guard &g, float t)
{
    if (g.detection <= 0.01f && !g.seeing)
        return;
    float x = g.pos.x, y = g.pos.y + 44;
    float w = 30, h = 5;

    setHex(0x1d2430, 0.75f);
    DrawRoundRect(x - w * 0.5f - 1.5f, y - 1.5f, w + 3, h + 3, 3);
    setColor(alertColor(g.detection));
    if (g.detection > 0.02f)
        DrawRoundRect(x - w * 0.5f, y, w * g.detection, h, 2);

    if (g.state != ALERT && g.state != GUARD_CAUGHT && g.detection < 0.3f)
        return;
    bool alarm = g.detection >= 0.6f;
    float pop = 1 + 0.12f * sinf(t * (alarm ? 14 : 6));

    glPushMatrix();
    glTranslatef(x, y + 17, 0);
    glScalef(pop, pop, 1);
    setHex(0x1d2430, 0.3f);
    circle(1.5f, -1.5f, 9);
    setHex(alarm ? 0xff4b3e : 0xffd23f);
    circle(0, 0, 9);
    setHex(0xffffff);
    DrawCircleOutline(0, 0, 9, 9, 1.5f);
    glPopMatrix();

    setHex(alarm ? 0xffffff : 0x1d2430);
    DrawTextCentered(x, y + 12, alarm ? "!" : "?", FONT_MEDIUM);
}

void drawSpeechBubble(Guard &g)
{
    char line1[80];
    sprintf(line1, "Ei apni ki detective %s?!", playerName);
    const char *line2 = "Museum theke ber hote chay, kintu permission chara na!";

    float w = maxf(TextWidth(line1, FONT_MEDIUM), TextWidth(line2, FONT_MEDIUM)) + 28;
    float h = 58;
    float bx = clampf(g.pos.x - w * 0.5f, 10, WORLD_W - w - 10);
    float by = minf(g.pos.y + 62, WORLD_H - h - 10);
    float tailX = clampf(g.pos.x, bx + 16, bx + w - 16);

    setHex(0x1d2430, 0.3f);
    DrawRoundRect(bx + 4, by - 4, w, h, 12);
    setHex(0xffffff);
    DrawRoundRect(bx, by, w, h, 12);
    DrawTriangle(tailX - 8, by + 1, tailX + 8, by + 1, g.pos.x, maxf(by - 18, g.pos.y + 40));
    setHex(0xd63c3c);
    DrawRoundRectOutline(bx, by, w, h, 12, 2.5f);
    DrawTextCentered(bx + w * 0.5f, by + h - 24, line1, FONT_MEDIUM);
    setHex(0x1d2430);
    DrawTextCentered(bx + w * 0.5f, by + 12, line2, FONT_MEDIUM);
}

// Debug view (G key): the patrol route
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
        setHex(g.route[i].searchTime > 0 ? 0xffa53d : 0x2f5f8c);
        circle(g.route[i].p.x, g.route[i].p.y, i == g.target ? 6 : 4);
    }
}
