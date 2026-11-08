// ============================================================
// map.cpp
// Draws the museum grounds. Three layers:
//  1. ground   : grass, paths, lake, lawn, flower beds, shadows
//  2. objects  : buildings, walls, trees, benches... (these are
//                sorted by y together with the characters in
//                gameplay.cpp, so nearer things cover further ones)
//  3. canopies : tree tops, lamp light, gate arch, clouds
// ============================================================

#include "game.h"

// ============================================================
// COLOURS
// ============================================================

#define GRASS 0x9ccc72
#define GRASS_DARK 0x6fa64f
#define GRASS_LIGHT 0xb9de8e
#define ASPHALT 0x6c717d
#define ASPHALT_EDGE 0x5b606b
#define CURB 0xdcd5c4
#define ROAD_PAINT 0xf6ecbf
#define PAVE 0xeadfc9
#define PAVE_JOINT 0xcdbea2
#define PLAZA_A 0xecdcc1
#define PLAZA_B 0xdac5a2
#define BRICK 0xc4735c
#define BRICK_DARK 0x9b5442
#define BRICK_CAP 0xe6c0a7
#define STONE 0xbbb4a6
#define STONE_LIGHT 0xe4dfd4
#define SOIL 0x7c5b43
#define WOOD 0xb9875a
#define WOOD_DARK 0x83593a
#define IRON 0x434a54
#define GLASS 0x9fd0e6
#define GLASS_DARK 0x5d8fae
#define FRAME 0x4f5c69
#define MUSEUM_BLUE 0x2f5f8c
#define MUSEUM_GOLD 0xe9bb4d
#define WHITE 0xffffff
#define INK 0x2e3440
#define LAMP_WARM 0xffe39a

// shadow colour (dark green, see-through)
Color shadowColor(float a)
{
    Color c = {0.10f, 0.16f, 0.08f, a};
    return c;
}

// Light comes from the top-left, so shadows fall bottom-right
void dropShadow(Rect r, float off)
{
    setColor(shadowColor(0.10f));
    DrawRoundRect(r.x + off * 0.4f, r.y - off * 1.1f, r.w + off * 0.9f, r.h, 4);
    setColor(shadowColor(0.16f));
    DrawRect(r.x + off * 0.6f, r.y - off * 0.6f, r.w, r.h);
}

// ============================================================
// GRASS
// ============================================================

void drawGrass()
{
    setHex(GRASS);
    DrawRect(0, 0, WORLD_W, WORLD_H);

    // mowed stripes
    setHex(GRASS_LIGHT, 0.22f);
    for (float x = 0; x < WORLD_W; x += 80)
        DrawRect(x, 0, 40, WORLD_H);

    // grass tufts: small "V" shapes made from GL_LINES
    setHex(GRASS_DARK, 0.55f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    for (int i = 0; i < 2600; i++)
    {
        float x = hash01(i, 1) * WORLD_W;
        float y = hash01(i, 2) * WORLD_H;
        float s = 2.5f + hash01(i, 3) * 3;
        glVertex2f(x, y);
        glVertex2f(x - 1.8f, y + s);
        glVertex2f(x, y);
        glVertex2f(x + 1.6f, y + s * 0.85f);
    }
    glEnd();

    // tiny white flowers: GL_POINTS
    setHex(WHITE, 0.85f);
    glPointSize(3);
    glBegin(GL_POINTS);
    for (int i = 0; i < 360; i++)
        glVertex2f(hash01(i, 7) * WORLD_W, hash01(i, 8) * WORLD_H);
    glEnd();
}

// ============================================================
// PATHS: pavement, plaza, road with zebra crossings
// ============================================================

void drawPavement(Rect r)
{
    setHex(PAVE_JOINT);
    DrawRectR(rectExpand(r, 2));
    setHex(PAVE);
    DrawRectR(r);

    // brick-pattern joints
    setHex(PAVE_JOINT, 0.8f);
    glLineWidth(1);
    glBegin(GL_LINES);
    float rowH = 12, tileW = 22;
    for (float y = r.y + rowH; y < r.y + r.h; y += rowH)
    {
        glVertex2f(r.x, y);
        glVertex2f(r.x + r.w, y);
    }
    int row = 0;
    for (float y = r.y; y < r.y + r.h; y += rowH, row++)
    {
        float off = (row % 2) ? tileW * 0.5f : 0;
        float y2 = minf(y + rowH, r.y + r.h);
        for (float x = r.x + off + tileW; x < r.x + r.w; x += tileW)
        {
            glVertex2f(x, y);
            glVertex2f(x, y2);
        }
    }
    glEnd();
}

void drawPlaza(Rect r)
{
    setHex(PAVE_JOINT);
    DrawRectR(rectExpand(r, 3));
    float t = 20;
    int ix = 0;
    for (float x = r.x; x < r.x + r.w; x += t, ix++)
    {
        int iy = 0;
        for (float y = r.y; y < r.y + r.h; y += t, iy++)
        {
            setHex(((ix + iy) % 2) ? PLAZA_A : PLAZA_B); // checker tiles
            DrawRect(x, y, minf(t, r.x + r.w - x), minf(t, r.y + r.h - y));
        }
    }
    setHex(WHITE, 0.35f);
    DrawRectOutline(r.x + 4, r.y + 4, r.w - 8, r.h - 8, 1.5f);
}

void drawPaths()
{
    for (int i = 0; i < map.pathCount; i++)
    {
        if (map.paths[i].kind == PAVEMENT)
            drawPavement(map.paths[i].r);
        else if (map.paths[i].kind == PLAZA)
            drawPlaza(map.paths[i].r);
    }

    // roads: curbs first, then asphalt
    setHex(CURB);
    for (int i = 0; i < map.pathCount; i++)
        if (map.paths[i].kind == ROAD)
            DrawRectR(rectExpand(map.paths[i].r, 4));
    setHex(ASPHALT);
    for (int i = 0; i < map.pathCount; i++)
        if (map.paths[i].kind == ROAD)
            DrawRectR(map.paths[i].r);

    for (int i = 0; i < map.pathCount; i++)
    {
        if (map.paths[i].kind != ROAD)
            continue;
        Rect p = map.paths[i].r;

        // asphalt grain
        setHex(ASPHALT_EDGE, 0.7f);
        glPointSize(2);
        glBegin(GL_POINTS);
        int n = (int)(p.w * p.h / 140);
        for (int k = 0; k < n; k++)
            glVertex2f(p.x + hash01(k, 31 + i) * p.w, p.y + hash01(k, 47 + i) * p.h);
        glEnd();

        // centre dashes, skipped over a zebra crossing
        bool horizontal = p.w >= p.h;
        float len = horizontal ? p.w : p.h;
        for (float s = 8; s + 18 < len; s += 34)
        {
            Rect d;
            if (horizontal)
                d = (Rect){p.x + s, rectCenter(p).y - 1.5f, 18, 3};
            else
                d = (Rect){rectCenter(p).x - 1.5f, p.y + s, 3, 18};
            bool blocked = false;
            for (int c = 0; c < map.crossingCount; c++)
                if (rectsOverlap(rectExpand(map.crossings[c], 6), d))
                    blocked = true;
            if (!blocked)
            {
                setHex(ROAD_PAINT);
                DrawRectR(d);
            }
        }
    }

    // zebra crossings (bars along the road underneath)
    for (int c = 0; c < map.crossingCount; c++)
    {
        Rect cr = map.crossings[c];
        bool alongX = true;
        for (int i = 0; i < map.pathCount; i++)
            if (map.paths[i].kind == ROAD && rectContains(map.paths[i].r, rectCenter(cr)))
                alongX = map.paths[i].r.w >= map.paths[i].r.h;
        setHex(WHITE, 0.9f);
        if (alongX)
            for (float x = cr.x; x + 5 <= cr.x + cr.w; x += 10)
                DrawRect(x, cr.y + 2, 6, cr.h - 4);
        else
            for (float y = cr.y; y + 5 <= cr.y + cr.h; y += 10)
                DrawRect(cr.x + 2, y, cr.w - 4, 6);
    }
}

// ============================================================
// LAKE (with ripples and a duck) AND SPORTS LAWN
// ============================================================

void drawLake(Rect lake, float t)
{
    setHex(0xb5a78b);
    DrawRoundRect(lake.x - 8, lake.y - 8, lake.w + 16, lake.h + 16, 22);
    setHex(0x377b92);
    DrawRoundRect(lake.x, lake.y, lake.w, lake.h, 18);

    // moving ripples
    setHex(0x63b5c5, 0.65f);
    for (int i = 0; i < 12; i++)
    {
        float x = lake.x + 20 + fmodf(i * 37.0f + t * 7, lake.w - 45);
        float y = lake.y + 18 + (i % 5) * 25;
        DrawLine(x, y, x + 16 + sinf(t + i) * 5, y, 1.5f);
    }
    // lily pads
    setHex(0x83a64c);
    for (int i = 0; i < 4; i++)
        DrawCircle(lake.x + 24 + i * 17, lake.y + lake.h - 23 - (i % 2) * 13, 8, 4, 0);

    // duck swimming left and right
    float duckX = lake.x + lake.w * 0.6f + sinf(t * 0.4f) * 30;
    setHex(0xfff1ce);
    DrawCircle(duckX, lake.y + 70, 9, 5, 0);
    circle(duckX + 7, lake.y + 76, 4);
    setHex(0xf2ac48);
    DrawTriangle(duckX + 10, lake.y + 76, duckX + 16, lake.y + 74, duckX + 10, lake.y + 73);
}

void drawField(Rect f)
{
    setHex(0x477e45);
    DrawRectR(f);
    setHex(0x528d4b);
    for (int i = 0; i < 7; i += 2)
        DrawRect(f.x + i * f.w / 7, f.y, f.w / 7, f.h);

    setHex(0xe2ecd1);
    Vec2 c = rectCenter(f);
    DrawRectOutline(f.x + 8, f.y + 8, f.w - 16, f.h - 16, 2);
    DrawLine(c.x, f.y + 8, c.x, f.y + f.h - 8, 2);
    DrawCircleOutline(c.x, c.y, 32, 32, 2);
    DrawRectOutline(f.x + 8, f.y + 50, 47, f.h - 100, 2);
    DrawRectOutline(f.x + f.w - 55, f.y + 50, 47, f.h - 100, 2);
    float goals[2] = {f.x - 5, f.x + f.w - 8};
    for (int g = 0; g < 2; g++)
    {
        DrawRectOutline(goals[g], c.y - 24, 13, 48, 2);
        for (int i = 1; i < 6; i++)
            DrawLine(goals[g], c.y - 24 + i * 8, goals[g] + 13, c.y - 24 + i * 8, 1);
    }
}

// ============================================================
// FLOWER BED
// ============================================================

void drawFlowerBed(Rect r, float t, int seed)
{
    setColor(shadowColor(0.18f));
    DrawRoundRect(r.x + 2, r.y - 4, r.w + 2, r.h, 5);
    setHex(0xb88f70);
    DrawRoundRect(r.x - 3, r.y - 3, r.w + 6, r.h + 6, 6);
    setHex(SOIL);
    DrawRoundRect(r.x, r.y, r.w, r.h, 4);

    unsigned int petals[5] = {0xf38fac, 0xf6d55c, 0xffffff, 0xb48be2, 0xf27b57};
    int cols = maxf(1, r.w / 10);
    int rows = maxf(1, r.h / 10);
    for (int i = 0; i < cols; i++)
    {
        for (int j = 0; j < rows; j++)
        {
            int id = seed * 977 + i * 31 + j;
            float px = r.x + (i + 0.5f) * r.w / cols + (hash01(id, 1) - 0.5f) * 3;
            float py = r.y + (j + 0.5f) * r.h / rows + (hash01(id, 2) - 0.5f) * 3;
            setHex(0x4f8e40);
            DrawCircle(px, py, 4.2f, 4.2f, 10);
            setHex(0x6eae55);
            DrawCircle(px - 1, py + 1, 2.6f, 2.6f, 8);
            // flowers gently grow and shrink
            float s = 1 + 0.10f * sinf(t * 2.2f + id);
            setHex(petals[(int)(hash01(id, 3) * 5) % 5]);
            DrawCircle(px + 0.8f, py + 1.2f, 2.3f * s, 2.3f * s, 8);
            setHex(0xffd84a);
            DrawCircle(px + 0.8f, py + 1.2f, 0.8f, 0.8f, 6);
        }
    }
}

// ============================================================
// GROUND LAYER
// ============================================================

void drawGround(float t)
{
    drawGrass();
    drawPaths();
    for (int i = 0; i < map.lakeCount; i++)
        drawLake(map.lakes[i], t);
    for (int i = 0; i < map.fieldCount; i++)
        drawField(map.fields[i]);
    for (int i = 0; i < map.flowerBedCount; i++)
        drawFlowerBed(map.flowerBeds[i], t, i);

    // flat shadows of tall things, under every character
    for (int i = 0; i < map.treeCount; i++)
    {
        Tree tr = map.trees[i];
        DrawSoftCircle(tr.p.x + tr.radius * 0.45f, tr.p.y + tr.radius * 0.05f,
                       tr.radius * 1.1f, tr.radius * 0.6f, shadowColor(0.32f), shadowColor(0));
    }
    for (int i = 0; i < map.buildingCount; i++)
        dropShadow(map.buildings[i].r, map.buildings[i].kind == BOOTH ? 8 : 16);
    for (int i = 0; i < map.signCount; i++)
        dropShadow(map.signs[i], 8);
    setColor(shadowColor(0.25f));
    for (int i = 0; i < map.lampCount; i++)
        DrawThickLine(map.lamps[i].x, map.lamps[i].y, map.lamps[i].x + 20, map.lamps[i].y - 12, 3);
    for (int i = 0; i < map.flagCount; i++)
        DrawThickLine(map.flags[i].x, map.flags[i].y, map.flags[i].x + 46, map.flags[i].y - 26, 2.5f);
}

// ============================================================
// SMALL OBJECTS
// ============================================================

void drawBush(int i)
{
    Bush b = map.bushes[i];
    int seed = i + 3;
    float r = b.radius;
    DrawSoftCircle(b.p.x + r * 0.35f, b.p.y - r * 0.35f, r * 1.1f, r * 0.7f, shadowColor(0.28f), shadowColor(0));

    // five round lobes
    Vec2 lobes[5];
    for (int k = 0; k < 5; k++)
    {
        float a = degToRad(k * 72 + hash01(seed, k) * 30);
        lobes[k] = makeVec(b.p.x + cosf(a) * r * 0.45f, b.p.y + sinf(a) * r * 0.40f);
    }
    setHex(0x3c6b35); // outline
    for (int k = 0; k < 5; k++)
        circle(lobes[k].x, lobes[k].y, r * 0.6f + 1.5f);
    setHex(0x5a9a49); // fill
    for (int k = 0; k < 5; k++)
        circle(lobes[k].x, lobes[k].y, r * 0.6f);
    circle(b.p.x, b.p.y, r * 0.6f);
    setHex(0x7dba60); // highlight
    circle(b.p.x - r * 0.18f, b.p.y + r * 0.18f, r * 0.42f);
    setHex(0xb4e08c, 0.8f);
    circle(b.p.x - r * 0.3f, b.p.y + r * 0.32f, r * 0.15f);
}

void drawBench(int i)
{
    Bench b = map.benches[i];
    glPushMatrix();
    glTranslatef(b.p.x, b.p.y, 0);
    if (b.vertical)
        glRotatef(90, 0, 0, 1);
    setColor(shadowColor(0.25f));
    DrawRoundRect(-17, -10, 38, 13, 3);
    setHex(IRON);
    DrawRect(-16, -7, 3, 15);
    DrawRect(13, -7, 3, 15);
    for (int k = 0; k < 3; k++)
    {
        if (k % 2)
            setHex(WOOD);
        else
            setColor(lighter(hexColor(WOOD), 0.12f));
        DrawRoundRect(-18, -6 + k * 4, 36, 3.3f, 1.2f);
    }
    setHex(WOOD_DARK);
    DrawRoundRect(-18, 6.5f, 36, 3, 1.2f); // backrest
    glPopMatrix();
}

void drawLampBase(int i)
{
    Vec2 p = map.lamps[i];
    setHex(STONE);
    circle(p.x, p.y, 5);
    setHex(IRON);
    circle(p.x, p.y, 3.2f);
    DrawRect(p.x - 1.5f, p.y, 3, 30);
    setHex(WHITE, 0.35f);
    DrawRect(p.x - 1, p.y + 2, 0.8f, 26);
}

void drawRailing(int i)
{
    Railing rl = map.railings[i];
    Vec2 d = vsub(rl.b, rl.a);
    int posts = (int)(vlength(d) / 12) + 1;
    if (posts < 2)
        posts = 2;
    setColor(shadowColor(0.2f));
    DrawThickLine(rl.a.x + 3, rl.a.y - 3, rl.b.x + 3, rl.b.y - 3, 2);
    setHex(IRON);
    for (int k = 0; k < posts; k++)
    {
        Vec2 p = vadd(rl.a, vscale(d, float(k) / (posts - 1)));
        DrawRect(p.x - 1, p.y, 2, 9);
    }
    DrawThickLine(rl.a.x, rl.a.y + 8, rl.b.x, rl.b.y + 8, 2);
    setHex(0x5d6773);
    DrawThickLine(rl.a.x, rl.a.y + 4, rl.b.x, rl.b.y + 4, 1.2f);
}

void drawTableBase(int i)
{
    Vec2 p = map.tables[i].p;
    DrawSoftCircle(p.x + 6, p.y - 6, 22, 14, shadowColor(0.3f), shadowColor(0));
    setHex(WOOD_DARK); // four stools
    for (int k = 0; k < 4; k++)
    {
        float a = degToRad(45 + k * 90);
        DrawRoundRect(p.x + cosf(a) * 15 - 4, p.y + sinf(a) * 12 - 4, 8, 8, 2);
    }
    setHex(WOOD);
    circle(p.x, p.y, 9);
}

void drawFlagPole(int i)
{
    Vec2 p = map.flags[i];
    setHex(STONE);
    circle(p.x, p.y, 7);
    setHex(STONE_LIGHT);
    circle(p.x, p.y, 4.5f);
    setHex(0xc9cdd2);
    DrawRect(p.x - 1.3f, p.y, 2.6f, 82);
    setHex(MUSEUM_GOLD);
    circle(p.x, p.y + 83, 2.4f);
}

void drawTreeTrunk(int i)
{
    Tree tr = map.trees[i];
    float r = tr.radius;
    setHex(0x6b4a32);
    DrawQuad(tr.p.x - 5, tr.p.y - 2, tr.p.x + 5, tr.p.y - 2,
             tr.p.x + 3, tr.p.y + r * 0.6f, tr.p.x - 3, tr.p.y + r * 0.6f);
    setHex(0x8a6444);
    DrawRect(tr.p.x - 2.5f, tr.p.y, 1.5f, r * 0.5f);
}

// ============================================================
// WALL
// ============================================================

void drawWall(int i)
{
    Rect w = map.walls[i];
    setColor(shadowColor(0.25f));
    DrawRect(w.x + 4, w.y - 4, w.w, w.h);
    setHex(BRICK);
    DrawRectR(w);

    glLineWidth(1);
    if (w.w >= w.h)
    {
        // horizontal wall: brick front face, capstone on top
        float faceTop = w.y + w.h * 0.55f;
        setHex(BRICK_DARK, 0.7f);
        glBegin(GL_LINES);
        int row = 0;
        for (float y = w.y; y < faceTop; y += 3.8f, row++)
        {
            glVertex2f(w.x, y);
            glVertex2f(w.x + w.w, y);
            float off = (row % 2) ? 7 : 0;
            for (float x = w.x + off; x < w.x + w.w; x += 14)
            {
                glVertex2f(x, y);
                glVertex2f(x, minf(y + 3.8f, faceTop));
            }
        }
        glEnd();
        setHex(BRICK_CAP);
        DrawRect(w.x, faceTop, w.w, w.y + w.h - faceTop);
        setHex(WHITE, 0.4f);
        DrawLine(w.x, w.y + w.h - 1, w.x + w.w, w.y + w.h - 1, 1);
    }
    else
    {
        // vertical wall seen from above: capstone with joints
        setHex(BRICK_CAP);
        DrawRect(w.x + 2, w.y, w.w - 4, w.h);
        setHex(BRICK_DARK, 0.6f);
        glBegin(GL_LINES);
        for (float y = w.y; y < w.y + w.h; y += 20)
        {
            glVertex2f(w.x + 2, y);
            glVertex2f(w.x + w.w - 2, y);
        }
        glEnd();
    }
}

// ============================================================
// GATE (level 1)
// ============================================================

void drawPillar(float cx, float cy, float size)
{
    float h = size * 0.5f;
    setColor(shadowColor(0.3f));
    DrawRect(cx - h + 5, cy - h - 5, size, size);
    setHex(0xa69e90);
    DrawRect(cx - h, cy - h, size, size);
    setHex(STONE_LIGHT);
    DrawRect(cx - h + 3, cy - h + 3, size - 6, size - 6);
    setHex(0xa69e90, 0.7f);
    DrawLine(cx - h + 3, cy - h + 3, cx + h - 3, cy + h - 3, 1);
    DrawLine(cx - h + 3, cy + h - 3, cx + h - 3, cy - h + 3, 1);
    setHex(MUSEUM_GOLD);
    circle(cx, cy, size * 0.18f);
}

// A gate panel turns around its hinge (glRotatef)
void drawGatePanel(float hx, float hy, float deg, float len)
{
    glPushMatrix();
    glTranslatef(hx, hy, 0);
    glRotatef(deg, 0, 0, 1);
    setColor(shadowColor(0.25f));
    DrawRect(2, -5, len, 4);
    setHex(IRON);
    DrawRect(0, -1.5f, len, 3);
    setHex(0x6c7684);
    for (float x = 4; x < len; x += 6)
        DrawRect(x - 0.8f, -3, 1.6f, 6);
    setHex(MUSEUM_GOLD);
    circle(len, 0, 2);
    glPopMatrix();
}

void drawGate()
{
    setHex(0x9aa0a8, 0.8f);
    DrawLine(map.gateX0, 7, map.gateX1, 7, 2); // gate track
    drawGatePanel(map.gateX0, 7, 78, 46);
    drawGatePanel(map.gateX1, 7, 102, 46);
    drawPillar(map.gateX0 - 8, 9, 20);
    drawPillar(map.gateX1 + 8, 9, 20);
}

// ============================================================
// STONE SIGN (level 4)
// ============================================================

void drawSign(int i)
{
    Rect r = map.signs[i];
    setHex(0x8f8879);
    DrawRoundRect(r.x - 4, r.y - 4, r.w + 8, r.h + 8, 6);
    setHex(STONE_LIGHT);
    DrawRoundRect(r.x, r.y, r.w, r.h, 5);
    setHex(MUSEUM_BLUE);
    DrawRect(r.x, r.y, r.w, 6);

    // emblem: blue circle with an open book
    float ex = r.x + 22, ey = r.y + r.h * 0.55f;
    setHex(MUSEUM_BLUE);
    circle(ex, ey, 15);
    setHex(MUSEUM_GOLD);
    DrawCircleOutline(ex, ey, 13, 13, 1.5f);
    setHex(WHITE);
    DrawQuad(ex - 9, ey - 3, ex - 1, ey - 5, ex - 1, ey + 5, ex - 9, ey + 7);
    DrawQuad(ex + 1, ey - 5, ex + 9, ey - 3, ex + 9, ey + 7, ex + 1, ey + 5);

    setHex(MUSEUM_BLUE);
    DrawText(r.x + 44, r.y + r.h - 18, "CITY MUSEUM", FONT_NORMAL);
    DrawText(r.x + 44, r.y + r.h - 31, "OF HISTORY", FONT_SMALL);
    setHex(0x7b7468);
    DrawText(r.x + 44, r.y + 12, "Main Gate  -  Visitors Welcome", FONT_SMALL);
}

// ============================================================
// BUILDING
// ============================================================

void drawBuilding(int i, float t)
{
    Building b = map.buildings[i];
    Rect r = b.r;
    bool booth = (b.kind == BOOTH);

    // wall, wallDark, roof, roofDark, parapet, accent colours by kind
    unsigned int styles[4][6] = {
        {0xf3e9d6, 0xd6c6a8, 0xd8d4cb, 0xc3beb2, 0xaea595, MUSEUM_BLUE}, // HALL
        {0xedcfb0, 0xcda680, 0xcfb49c, 0xb79a80, 0xa27f66, 0x8c4b3b},    // OFFICE
        {0xf7e2a9, 0xd9be7f, 0xe89c70, 0xcf8158, 0xb66a4b, 0xd8524a},    // GALLERY
        {0xe4ecf3, 0xbcc8d5, 0x4472a8, 0x365e8f, 0x2a4d78, 0x4472a8}};   // BOOTH
    Color wall = hexColor(styles[b.kind][0]);
    Color wallDark = hexColor(styles[b.kind][1]);
    Color roof = hexColor(styles[b.kind][2]);
    Color roofDark = hexColor(styles[b.kind][3]);
    Color parapet = hexColor(styles[b.kind][4]);
    Color accent = hexColor(styles[b.kind][5]);

    float faceH = booth ? r.h * 0.42f : minf(r.h * 0.36f, 58);
    float roofY = r.y + faceH;
    float doorW = booth ? 12 : (b.kind == HALL ? 40 : 30);
    float doorH = faceH * (booth ? 0.8f : 0.72f);

    // entrance steps
    if (!booth)
    {
        for (int k = 2; k >= 0; k--)
        {
            float w = doorW + 16 + k * 8;
            setHex(k % 2 ? STONE : STONE_LIGHT);
            DrawRect(b.doorX - w * 0.5f, r.y - 4 * (k + 1), w, 4 * (k + 1));
        }
    }

    // ---- front wall ----
    DrawGradientRect(r.x, r.y, r.w, faceH, wallDark, wall);
    setColor(darker(wallDark, 0.15f));
    DrawRect(r.x, r.y, r.w, 4);

    if (b.kind == GALLERY)
    {
        // striped awning with round edge
        float aw = r.w - 10, ay = r.y + faceH * 0.45f;
        int stripes = (int)(aw / 12);
        float sw = aw / stripes;
        for (int k = 0; k < stripes; k++)
        {
            if (k % 2)
                setHex(WHITE);
            else
                setColor(accent);
            float x = r.x + 5 + k * sw;
            DrawRect(x, ay, sw, faceH * 0.5f);
            DrawSector(x + sw * 0.5f, ay, sw * 0.5f, 180, 360);
        }
        setHex(WOOD);
        DrawRect(r.x + 14, r.y + 4, r.w - 28, faceH * 0.32f); // counter
        setHex(WOOD_DARK);
        DrawRect(r.x + 14, r.y + 4 + faceH * 0.32f - 3, r.w - 28, 3);
    }
    else
    {
        // windows
        int bays = (int)(r.w / (booth ? 30 : 44));
        if (bays < 1)
            bays = 1;
        float bayW = r.w / bays;
        for (int k = 0; k < bays; k++)
        {
            float cx = r.x + bayW * (k + 0.5f);
            if (fabsf(cx - b.doorX) < doorW * 0.5f + bayW * 0.35f)
                continue; // leave room for the door
            float ww = bayW * 0.56f, wh = faceH * 0.46f, wy = r.y + faceH * 0.30f;
            setHex(FRAME);
            DrawRect(cx - ww * 0.5f - 2, wy - 2, ww + 4, wh + 4);
            DrawGradientRect(cx - ww * 0.5f, wy, ww, wh, hexColor(GLASS_DARK), hexColor(GLASS));
            setHex(WHITE, 0.35f); // reflection
            DrawQuad(cx - ww * 0.5f, wy + wh * 0.2f, cx - ww * 0.1f, wy + wh,
                     cx + ww * 0.1f, wy + wh, cx - ww * 0.5f + ww * 0.15f, wy);
            setHex(FRAME);
            DrawLine(cx, wy, cx, wy + wh, 1.2f);
            DrawLine(cx - ww * 0.5f, wy + wh * 0.5f, cx + ww * 0.5f, wy + wh * 0.5f, 1.2f);
            setColor(lighter(wall, 0.4f));
            DrawRect(cx - ww * 0.5f - 3, wy - 4, ww + 6, 2.5f); // sill
            if (!booth && k > 0)
            {
                setColor(withAlpha(wallDark, 0.7f)); // column between windows
                DrawRect(r.x + bayW * k - 2, r.y + 4, 4, faceH - 4);
            }
        }
        // glass door
        setColor(accent);
        DrawRect(b.doorX - doorW * 0.5f - 3, r.y, doorW + 6, doorH + 3);
        DrawGradientRect(b.doorX - doorW * 0.5f, r.y, doorW, doorH, hexColor(GLASS_DARK), lighter(hexColor(GLASS), 0.2f));
        setHex(FRAME);
        DrawLine(b.doorX, r.y, b.doorX, r.y + doorH, 1.5f);
        setHex(MUSEUM_GOLD);
        DrawCircle(b.doorX - 3, r.y + doorH * 0.45f, 1.3f, 1.3f, 8);
        DrawCircle(b.doorX + 3, r.y + doorH * 0.45f, 1.3f, 1.3f, 8);
        if (!booth)
        {
            setColor(shadowColor(0.3f)); // canopy over the door
            DrawRect(b.doorX - doorW * 0.5f - 8, r.y + doorH - 2, doorW + 18, 5);
            setColor(accent);
            DrawRect(b.doorX - doorW * 0.5f - 10, r.y + doorH + 3, doorW + 20, 5);
        }
    }

    // ---- roof ----
    float roofH = r.y + r.h - roofY;
    float top = r.y + r.h;
    setColor(roof);
    DrawRect(r.x, roofY, r.w, roofH);
    float pw = booth ? 3 : 5;
    setColor(parapet);
    DrawRect(r.x, roofY, r.w, pw);
    DrawRect(r.x, top - pw, r.w, pw);
    DrawRect(r.x, roofY, pw, roofH);
    DrawRect(r.x + r.w - pw, roofY, pw, roofH);
    setHex(WHITE, 0.25f);
    DrawRectOutline(r.x + pw, roofY + pw, r.w - 2 * pw, roofH - 2 * pw, 1);
    setColor(darker(parapet, 0.3f));
    DrawRect(r.x, roofY - 3, r.w, 3);

    if (b.kind == HALL || b.kind == OFFICE)
    {
        // roof slab joints
        setColor(withAlpha(roofDark, 0.6f));
        for (float x = r.x + 40; x < r.x + r.w - pw; x += 40)
            DrawLine(x, roofY + pw, x, top - pw, 1);

        if (b.kind == HALL)
        {
            // two skylight panels
            for (int k = 0; k < 2; k++)
            {
                float px = r.x + 24 + k * 92, py = roofY + roofH * 0.42f;
                setColor(shadowColor(0.35f));
                DrawRect(px + 3, py - 3, 76, 38);
                setHex(0x2c4f7c);
                DrawRect(px, py, 76, 38);
                setHex(0x9cc4ef, 0.7f);
                for (float x = px + 12.7f; x < px + 76; x += 12.7f)
                    DrawLine(x, py, x, py + 38, 1);
                DrawLine(px, py + 19, px + 76, py + 19, 1);
                setHex(WHITE, 0.25f);
                DrawTriangle(px, py + 38, px + 30, py + 38, px, py + 14);
            }
        }
        else
        {
            // glass roof window
            float sx = r.x + r.w * 0.18f, sy = roofY + roofH * 0.35f;
            setColor(roofDark);
            DrawRect(sx - 3, sy - 3, 96, 46);
            DrawGradientRect(sx, sy, 90, 40, hexColor(GLASS_DARK), hexColor(GLASS));
            setHex(FRAME);
            for (float x = sx + 15; x < sx + 90; x += 15)
                DrawLine(x, sy, x, sy + 40, 1);
            DrawLine(sx, sy + 20, sx + 90, sy + 20, 1);
        }

        // water tank seen from above
        float tx = r.x + r.w - 42, ty = top - 30;
        DrawSoftCircle(tx + 6, ty - 5, 20, 16, shadowColor(0.4f), shadowColor(0));
        setHex(0x3f4a57);
        circle(tx, ty, 15);
        setHex(0x5a6b7d);
        circle(tx, ty, 12.5f);
        setHex(0x6f8296);
        circle(tx - 2, ty + 2, 6);

        // AC units with spinning fans (glRotatef animation)
        for (int k = 0; k < 3; k++)
        {
            float ax = r.x + r.w - 100 - k * 34, ay = top - 30;
            if (ax < r.x + 20)
                break;
            setColor(shadowColor(0.35f));
            DrawRect(ax - 10, ay - 15, 24, 24);
            setHex(0xeef0f2);
            DrawRoundRect(ax - 12, ay - 12, 24, 24, 3);
            setHex(0x9aa3ad);
            circle(ax, ay, 9);
            glPushMatrix();
            glTranslatef(ax, ay, 0);
            glRotatef(fmodf(t * 360 * (1 + k * 0.3f), 360), 0, 0, 1);
            setHex(0x5c6670);
            for (int bl = 0; bl < 3; bl++)
                DrawSector(0, 0, 8, bl * 120, bl * 120 + 40);
            glPopMatrix();
        }

        // stair room on the roof
        Rect sr = {r.x + r.w - 150, roofY + 14, 40, 30};
        if (sr.x > r.x + 200)
        {
            dropShadow(sr, 6);
            setColor(wallDark);
            DrawRectR(sr);
            setColor(roof);
            DrawRect(sr.x + 2, sr.y + 6, sr.w - 4, sr.h - 8);
        }
    }

    // name board
    int font = booth ? FONT_SMALL : FONT_NORMAL;
    float tw = TextWidth(b.name, font) + (booth ? 6 : 18);
    float bh = booth ? 10 : 16;
    float by = booth ? top - bh - 3 : roofY + 8;
    float cx = r.x + r.w * 0.5f;
    // keep the board inside the window (text outside it is not drawn)
    cx = clampf(cx, tw * 0.5f + 2, WORLD_W - tw * 0.5f - 2);
    setColor(shadowColor(0.35f));
    DrawRoundRect(cx - tw * 0.5f + 2, by - 2, tw, bh, 3);
    if (booth)
        setHex(WHITE);
    else
        setColor(accent);
    DrawRoundRect(cx - tw * 0.5f, by, tw, bh, 3);
    if (booth)
        setColor(accent);
    else
        setHex(WHITE);
    DrawTextCentered(cx, by + (booth ? 2 : 4.5f), b.name, font);
}

// ============================================================
// CANOPY LAYER
// ============================================================

void drawTreeCanopy(Tree tr, float t, int seed)
{
    float r = tr.radius;
    unsigned int colours[3][4] = {
        {0x355f31, 0x4f8a43, 0x68a853, 0x8fc56c},  // rain tree
        {0x3a6131, 0x5b8f45, 0x76aa55, 0x9acb6e},  // flame tree
        {0x2c5230, 0x3f7a47, 0x559459, 0x78b571}}; // mango tree
    unsigned int *c = colours[tr.kind];

    // gentle sway: rotate and scale around the canopy centre
    float sway = sinf(t * 1.2f + seed * 1.7f) * 1.8f;
    float breathe = 1 + 0.015f * sinf(t * 1.6f + seed);
    glPushMatrix();
    glTranslatef(tr.p.x, tr.p.y + r * 0.6f, 0);
    glRotatef(sway, 0, 0, 1);
    glScalef(breathe, breathe, 1);

    // seven leaf lobes around the centre
    Vec2 lobe[7];
    float lr[7];
    for (int k = 0; k < 7; k++)
    {
        float a = degToRad(k * 360.0f / 7 + hash01(seed, k) * 25);
        float d = r * (0.42f + hash01(seed, k + 10) * 0.12f);
        lobe[k] = makeVec(cosf(a) * d, sinf(a) * d * 0.92f);
        lr[k] = r * (0.48f + hash01(seed, k + 20) * 0.12f);
    }
    setHex(c[0]);
    circle(0, 0, r * 0.62f + 2);
    for (int k = 0; k < 7; k++)
        circle(lobe[k].x, lobe[k].y, lr[k] + 2);
    setHex(c[1]);
    circle(0, 0, r * 0.62f);
    for (int k = 0; k < 7; k++)
        circle(lobe[k].x, lobe[k].y, lr[k]);
    setHex(c[2]);
    for (int k = 0; k < 7; k++)
        circle(lobe[k].x * 0.75f - r * 0.08f, lobe[k].y * 0.75f + r * 0.08f, lr[k] * 0.72f);
    setHex(c[3]);
    circle(-r * 0.22f, r * 0.25f, r * 0.3f);
    circle(r * 0.1f, r * 0.38f, r * 0.18f);
    setColor(withAlpha(lighter(hexColor(c[3]), 0.3f), 0.8f));
    circle(-r * 0.3f, r * 0.34f, r * 0.1f);

    if (tr.kind == FLAME_TREE)
    {
        // red and orange blossoms
        unsigned int fl[3] = {0xe8463a, 0xf27a35, 0xf9a43f};
        for (int k = 0; k < 22; k++)
        {
            float a = hash01(seed, 40 + k) * 2 * PI;
            float d = sqrtf(hash01(seed, 80 + k)) * r * 0.85f;
            float s = 2 + hash01(seed, 120 + k) * 1.4f;
            setHex(fl[k % 3]);
            DrawCircle(cosf(a) * d, sinf(a) * d, s, s, 8);
        }
    }
    else if (tr.kind == MANGO_TREE)
    {
        setHex(0xc9d65a); // mangoes
        for (int k = 0; k < 6; k++)
        {
            float a = hash01(seed, 50 + k) * 2 * PI;
            float d = sqrtf(hash01(seed, 90 + k)) * r * 0.7f;
            DrawCircle(cosf(a) * d, sinf(a) * d, 2, 2.8f, 8);
        }
    }
    glPopMatrix();
}

void drawUmbrella(Table u, float t)
{
    unsigned int tops[3] = {0xe8574e, 0x3f8fb5, 0xf2b33d};
    Color c = hexColor(tops[u.colorIndex % 3]);
    float sway = sinf(t * 1.5f + u.p.x) * 3;

    glPushMatrix();
    glTranslatef(u.p.x, u.p.y + 10, 0);
    glRotatef(22.5f + sway, 0, 0, 1);
    DrawSoftCircle(6, -6, 24, 22, shadowColor(0.25f), shadowColor(0));
    for (int k = 0; k < 8; k++)
    {
        if (k % 2)
            setColor(c);
        else
            setHex(WHITE);
        float a1 = degToRad(k * 45.0f), a2 = degToRad((k + 1) * 45.0f);
        DrawTriangle(0, 0, cosf(a1) * 21, sinf(a1) * 21, cosf(a2) * 21, sinf(a2) * 21);
    }
    setColor(darker(c, 0.3f));
    glLineWidth(1.2f);
    glBegin(GL_LINE_LOOP);
    for (int k = 0; k < 8; k++)
        glVertex2f(cosf(degToRad(k * 45.0f)) * 21, sinf(degToRad(k * 45.0f)) * 21);
    glEnd();
    setHex(WOOD_DARK);
    circle(0, 0, 2.5f);
    glPopMatrix();
}

// Waving flag: GL_QUAD_STRIP moved by a sine wave
void drawFlagCloth(Vec2 p, float t)
{
    float w = 36, h = 22, top = p.y + 80;
    int n = 14;
    Color green = hexColor(0x006a4e);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= n; i++)
    {
        float u = float(i) / n;
        float wave = sinf(u * 6 - t * 5) * 2.2f * u;
        float shade = 0.88f + 0.12f * cosf(u * 6 - t * 5);
        glColor4f(green.r * shade, green.g * shade, green.b * shade, 1);
        glVertex2f(p.x + 1 + u * w, top - h + wave);
        glVertex2f(p.x + 1 + u * w, top + wave);
    }
    glEnd();
    float u = 0.45f;
    setHex(0xf42a41);
    circle(p.x + 1 + u * w, top - h * 0.5f + sinf(u * 6 - t * 5) * 2.2f * u, h * 0.3f);
}

void drawLampHead(Vec2 p, float t)
{
    float glow = 0.22f + 0.05f * sinf(t * 3 + p.x);
    DrawSoftCircle(p.x, p.y + 31, 26, 22, hexColor(LAMP_WARM, glow), hexColor(LAMP_WARM, 0));
    setHex(IRON);
    DrawCircle(p.x, p.y + 31, 7.5f, 4.5f, 0);
    setHex(LAMP_WARM);
    circle(p.x, p.y + 31, 2.8f);
}

void drawGateArch(float t)
{
    float x0 = map.gateX0 - 6, x1 = map.gateX1 + 6, y = 16, h = 13;
    setColor(shadowColor(0.25f));
    DrawRect(x0 + 8, y - 12, x1 - x0, h);
    setColor(darker(hexColor(MUSEUM_BLUE), 0.25f));
    DrawRect(x0, y - 2, x1 - x0, h + 4);
    setHex(MUSEUM_BLUE);
    DrawRect(x0, y, x1 - x0, h);
    setHex(MUSEUM_GOLD);
    DrawRect(x0, y + h - 2, x1 - x0, 2);
    setHex(WHITE);
    DrawTextCentered((x0 + x1) * 0.5f, y + 3, "CITY MUSEUM OF HISTORY", FONT_SMALL);

    // pillar lamps
    float lampX[2] = {map.gateX0 - 8, map.gateX1 + 8};
    for (int i = 0; i < 2; i++)
    {
        float glow = 0.25f + 0.05f * sinf(t * 2.5f + lampX[i]);
        DrawSoftCircle(lampX[i], 9, 20, 18, hexColor(LAMP_WARM, glow), hexColor(LAMP_WARM, 0));
        setHex(LAMP_WARM);
        circle(lampX[i], 9, 3);
    }
}

// Slow cloud shadows drifting across the grounds
void drawClouds(float t)
{
    Color inner = {0.15f, 0.20f, 0.30f, 0.06f};
    Color outer = {0.15f, 0.20f, 0.30f, 0.0f};
    for (int c = 0; c < 7; c++)
    {
        float x = fmodf(c * 610.0f + t * (7 + (c % 3) * 2), WORLD_W + 400) - 200;
        float y = 120 + c * 160;
        for (int k = 0; k < 4; k++)
            DrawSoftCircle(x + (k - 1.5f) * 45, y + sinf(k * 1.9f + c) * 18, 70, 42, inner, outer);
    }
}

void drawCanopies(float t)
{
    for (int i = 0; i < map.treeCount; i++)
        drawTreeCanopy(map.trees[i], t, i + 1);
    for (int i = 0; i < map.tableCount; i++)
        drawUmbrella(map.tables[i], t);
    for (int i = 0; i < map.flagCount; i++)
        drawFlagCloth(map.flags[i], t);
    for (int i = 0; i < map.lampCount; i++)
        drawLampHead(map.lamps[i], t);
    if (map.hasGate)
        drawGateArch(t);
    drawClouds(t);
}

// ============================================================
// DEBUG VIEW (G key): red = blocks movement, blue = blocks sight
// ============================================================

void drawCollisionDebug()
{
    glColor4f(1, 0.2f, 0.2f, 0.18f);
    for (int i = 0; i < map.solidCount; i++)
        DrawRectR(map.solids[i]);
    glColor4f(1, 0.1f, 0.1f, 0.9f);
    for (int i = 0; i < map.solidCount; i++)
        DrawRectOutline(map.solids[i].x, map.solids[i].y, map.solids[i].w, map.solids[i].h, 1.5f);
    for (int i = 0; i < map.circleCount; i++)
        DrawCircleOutline(map.circlePos[i].x, map.circlePos[i].y, map.circleR[i], map.circleR[i], 1.5f);
    glColor4f(0.2f, 0.4f, 1, 0.9f);
    for (int i = 0; i < map.blockerCount; i++)
        DrawRectOutline(map.blockers[i].x + 2, map.blockers[i].y + 2, map.blockers[i].w - 4, map.blockers[i].h - 4, 1);

    // player collision circle
    glColor4f(0.1f, 0.9f, 0.3f, 0.25f);
    circle(player.pos.x, player.pos.y, PLAYER_RADIUS);
    glColor4f(0.1f, 0.7f, 0.2f, 1);
    DrawCircleOutline(player.pos.x, player.pos.y, PLAYER_RADIUS, PLAYER_RADIUS, 1.5f);
}
