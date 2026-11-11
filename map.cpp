#include "game.h"

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
#define LAMP_WARM 0xffe39a
#define POND_EDGE 0xb5a78b
#define POND_WATER 0x377b92
#define GARDEN_LAWN 0x6fae55
#define GARDEN_PATH 0xd9ccb0

static Color shadowColor(float alpha)
{
    Color c = {0.10f, 0.16f, 0.08f, alpha};
    return c;
}

static void dropShadow(Rect r, float offset)
{
    setColor(shadowColor(0.10f));
    DrawRoundRect(r.x + offset * 0.4f, r.y - offset * 1.1f, r.w + offset * 0.9f, r.h, 4);
    setColor(shadowColor(0.16f));
    DrawRect(r.x + offset * 0.6f, r.y - offset * 0.6f, r.w, r.h);
}

static void drawGrassStripes()
{
    setHex(GRASS_LIGHT, 0.22f);
    for (float x = 0; x < WORLD_W; x += 80)
        DrawRect(x, 0, 40, WORLD_H);
}

static void drawGrassBlades()
{
    setHex(GRASS_DARK, 0.55f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    for (int i = 0; i < 2600; i++)
    {
        float x = hash01(i, 1) * WORLD_W;
        float y = hash01(i, 2) * WORLD_H;
        float bladeH = 2.5f + hash01(i, 3) * 3;
        glVertex2f(x, y);
        glVertex2f(x - 1.8f, y + bladeH);
        glVertex2f(x, y);
        glVertex2f(x + 1.6f, y + bladeH * 0.85f);
    }
    glEnd();
}

static void drawDaisies()
{
    setHex(WHITE, 0.85f);
    glPointSize(3);
    glBegin(GL_POINTS);
    for (int i = 0; i < 360; i++)
    {
        float x = hash01(i, 7) * WORLD_W;
        float y = hash01(i, 8) * WORLD_H;
        glVertex2f(x, y);
    }
    glEnd();
}

static void drawGrass()
{
    setHex(GRASS);
    DrawRect(0, 0, WORLD_W, WORLD_H);
    drawGrassStripes();
    drawGrassBlades();
    drawDaisies();
}

static void drawPavement(Rect r)
{
    setHex(PAVE_JOINT);
    DrawRectR(rectExpand(r, 2));
    setHex(PAVE);
    DrawRectR(r);

    setHex(PAVE_JOINT, 0.8f);
    glLineWidth(1);
    glBegin(GL_LINES);
    float rowH = 12;
    float tileW = 22;
    float right = r.x + r.w;
    float top = r.y + r.h;

    for (float y = r.y + rowH; y < top; y += rowH)
    {
        glVertex2f(r.x, y);
        glVertex2f(right, y);
    }

    int row = 0;
    for (float y = r.y; y < top; y += rowH)
    {
        float shift = 0;
        if (row % 2 == 1)
            shift = tileW * 0.5f;
        float rowTop = minf(y + rowH, top);
        for (float x = r.x + shift + tileW; x < right; x += tileW)
        {
            glVertex2f(x, y);
            glVertex2f(x, rowTop);
        }
        row++;
    }
    glEnd();
}

static void drawPlaza(Rect r)
{
    setHex(PAVE_JOINT);
    DrawRectR(rectExpand(r, 3));
    float tile = 20;
    float right = r.x + r.w;
    float top = r.y + r.h;

    int col = 0;
    for (float x = r.x; x < right; x += tile)
    {
        int row = 0;
        for (float y = r.y; y < top; y += tile)
        {
            if ((col + row) % 2 == 1)
                setHex(PLAZA_A);
            else
                setHex(PLAZA_B);
            float w = minf(tile, right - x);
            float h = minf(tile, top - y);
            DrawRect(x, y, w, h);
            row++;
        }
        col++;
    }
    setHex(WHITE, 0.35f);
    DrawRectOutline(r.x + 4, r.y + 4, r.w - 8, r.h - 8, 1.5f);
}

static void drawRoadSpeckles(Rect road, int roadIndex)
{
    setHex(ASPHALT_EDGE, 0.7f);
    glPointSize(2);
    glBegin(GL_POINTS);
    int count = (int)(road.w * road.h / 140);
    for (int k = 0; k < count; k++)
    {
        float x = road.x + hash01(k, 31 + roadIndex) * road.w;
        float y = road.y + hash01(k, 47 + roadIndex) * road.h;
        glVertex2f(x, y);
    }
    glEnd();
}

static bool touchesCrossing(Rect dash)
{
    bool touching = false;
    for (int c = 0; c < map.crossingCount; c++)
    {
        Rect zone = rectExpand(map.crossings[c], 6);
        if (rectsOverlap(zone, dash))
            touching = true;
    }
    return touching;
}

static void drawRoadCentreLine(Rect road)
{
    bool horizontal = road.w >= road.h;
    float length = road.h;
    if (horizontal)
        length = road.w;

    Vec2 centre = rectCenter(road);
    for (float s = 8; s + 18 < length; s += 34)
    {
        Rect dash;
        if (horizontal)
            dash = (Rect){road.x + s, centre.y - 1.5f, 18, 3};
        else
            dash = (Rect){centre.x - 1.5f, road.y + s, 3, 18};

        if (!touchesCrossing(dash))
        {
            setHex(ROAD_PAINT);
            DrawRectR(dash);
        }
    }
}

static void drawCrossing(Rect cr)
{
    bool stripesAlongX = true;
    for (int i = 0; i < map.pathCount; i++)
    {
        Rect road = map.paths[i].r;
        if (map.paths[i].kind == ROAD && rectContains(road, rectCenter(cr)))
            stripesAlongX = road.w >= road.h;
    }

    setHex(WHITE, 0.9f);
    if (stripesAlongX)
    {
        for (float x = cr.x; x + 5 <= cr.x + cr.w; x += 10)
            DrawRect(x, cr.y + 2, 6, cr.h - 4);
    }
    else
    {
        for (float y = cr.y; y + 5 <= cr.y + cr.h; y += 10)
            DrawRect(cr.x + 2, y, cr.w - 4, 6);
    }
}

static void drawPaths()
{
    for (int i = 0; i < map.pathCount; i++)
    {
        if (map.paths[i].kind == PAVEMENT)
            drawPavement(map.paths[i].r);
        else if (map.paths[i].kind == PLAZA)
            drawPlaza(map.paths[i].r);
    }

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
        drawRoadSpeckles(map.paths[i].r, i);
        drawRoadCentreLine(map.paths[i].r);
    }

    for (int c = 0; c < map.crossingCount; c++)
        drawCrossing(map.crossings[c]);
}

static void drawDuck(Rect lake, float t)
{
    float duckX = lake.x + lake.w * 0.6f + sinf(t * 0.4f) * 30;
    float duckY = lake.y + 70;
    float headY = lake.y + 76;
    glColor3ub(255, 241, 206);
    DrawCircle(duckX, duckY, 9, 5, 0);
    circle(duckX + 7, headY, 4);
    glColor3ub(242, 172, 72);
    DrawTriangle(duckX + 10, headY, duckX + 16, lake.y + 74, duckX + 10, lake.y + 73);
}

static void drawLake(Rect lake, float t)
{
    setHex(POND_EDGE);
    DrawRoundRect(lake.x - 8, lake.y - 8, lake.w + 16, lake.h + 16, 22);
    setHex(POND_WATER);
    DrawRoundRect(lake.x, lake.y, lake.w, lake.h, 18);

    setHex(0x63b5c5, 0.65f);
    for (int i = 0; i < 12; i++)
    {
        float drift = fmodf(i * 37.0f + t * 7, lake.w - 45);
        float x = lake.x + 20 + drift;
        float y = lake.y + 18 + (i % 5) * 25;
        float rippleLength = 16 + sinf(t + i) * 5;
        DrawLine(x, y, x + rippleLength, y, 1.5f);
    }

    glColor3ub(131, 166, 76);
    for (int i = 0; i < 4; i++)
    {
        float padX = lake.x + 24 + i * 17;
        float padY = lake.y + lake.h - 23 - (i % 2) * 13;
        DrawCircle(padX, padY, 8, 4, 0);
    }

    drawDuck(lake, t);
}

Vec2 gardenSpot(Rect f, int k)
{
    Vec2 centre = rectCenter(f);
    if (k >= 4)
        return centre;

    float dx = -115;
    if (k % 2 == 1)
        dx = 115;
    float dy = -52;
    if (k < 2)
        dy = 52;
    return makeVec(centre.x + dx, centre.y + dy);
}

static void drawGardenLawn(Rect f)
{
    glColor3ub(63, 122, 61);
    DrawRoundRect(f.x - 4, f.y - 4, f.w + 8, f.h + 8, 10);
    setHex(GARDEN_LAWN);
    DrawRoundRect(f.x, f.y, f.w, f.h, 8);

    setHex(0x7dba60, 0.6f);
    for (int i = 0; i < 9; i += 2)
    {
        float x = f.x + 6 + i * (f.w - 12) / 9;
        float stripeW = (f.w - 12) / 9;
        DrawRect(x, f.y + 6, stripeW, f.h - 12);
    }
}

static void drawGardenPaths(Rect f, Vec2 c)
{
    setHex(GARDEN_PATH);
    DrawCircle(c.x, c.y, 112, 64, 0);
    setHex(GARDEN_LAWN);
    DrawCircle(c.x, c.y, 92, 46, 0);
    setHex(GARDEN_PATH);
    DrawRect(f.x + 6, c.y - 9, f.w - 12, 18);
    DrawRect(c.x - 9, f.y + 6, 18, f.h - 12);
}

static void drawGardenGravel(Vec2 c)
{
    setHex(0xb9ab8d, 0.8f);
    glPointSize(2);
    glBegin(GL_POINTS);
    for (int i = 0; i < 260; i++)
    {
        float angle = hash01(i, 61) * 2 * PI;
        float spread = 0.82f + hash01(i, 62) * 0.18f;
        float x = c.x + cosf(angle) * 112 * spread;
        float y = c.y + sinf(angle) * 64 * spread;
        glVertex2f(x, y);
    }
    glEnd();
}

static void drawFountain(Vec2 c, float t)
{
    setHex(POND_EDGE);
    DrawCircle(c.x, c.y, 30, 22, 0);
    setHex(POND_WATER);
    DrawCircle(c.x, c.y, 24, 17, 0);

    float wave = fmodf(t * 0.8f, 1);
    setHex(0x9fd8e6, 0.7f * (1 - wave));
    DrawCircleOutline(c.x, c.y, 7 + wave * 15, 5 + wave * 11, 1.2f);

    setHex(STONE_LIGHT);
    DrawCircle(c.x, c.y + 2, 6, 5, 0);
    setHex(0xcfeef7, 0.8f);
    float dropY = c.y + 6 + sinf(t * 6);
    DrawCircle(c.x, dropY, 2.5f, 4, 0);
}

static void drawField(Rect f, float t)
{
    Vec2 centre = rectCenter(f);
    drawGardenLawn(f);
    drawGardenPaths(f, centre);
    drawGardenGravel(centre);
    drawFountain(centre, t);
}

static void drawStatueBase(Vec2 p)
{
    setColor(shadowColor(0.3f));
    DrawRect(p.x - 8, p.y - 10, 22, 12);
    glColor3ub(155, 147, 132);
    DrawRect(p.x - 11, p.y - 6, 22, 12);
    setHex(STONE_LIGHT);
    DrawRect(p.x - 11, p.y + 6, 22, 4);
    glColor3ub(201, 195, 182);
    DrawRect(p.x - 9, p.y - 4, 18, 8);
}

static void drawBust(Vec2 p, Color marble, Color shade)
{
    setColor(shade);
    DrawRoundRect(p.x - 9, p.y + 10, 18, 10, 4);
    setColor(marble);
    DrawRoundRect(p.x - 8, p.y + 11, 16, 9, 4);
    circle(p.x, p.y + 27, 6);
    setColor(shade);
    DrawCircle(p.x + 2, p.y + 26, 3, 4, 0);
}

static void drawBrokenColumn(Vec2 p, Color marble, Color shade)
{
    setColor(shade);
    DrawRect(p.x - 6, p.y + 10, 12, 22);
    setColor(marble);
    DrawRect(p.x - 5, p.y + 10, 3, 22);
    DrawRect(p.x, p.y + 10, 3, 20);
    DrawQuad(p.x - 6, p.y + 32, p.x + 6, p.y + 29, p.x + 6, p.y + 31, p.x - 6, p.y + 35);
}

static void drawClayPot(Vec2 p)
{
    setColor(hexColor(0xc9774a));
    DrawCircle(p.x, p.y + 19, 8, 9, 0);
    DrawRect(p.x - 3, p.y + 26, 6, 6);
    DrawRect(p.x - 5, p.y + 31, 10, 2);
    glColor3ub(46, 38, 32);
    DrawRect(p.x - 7, p.y + 18, 14, 2);
    DrawRect(p.x - 6, p.y + 22, 12, 1.5f);
}

static void drawSpinningRing(Vec2 p, float t)
{
    glPushMatrix();
    glTranslatef(p.x, p.y + 22, 0);
    glRotatef(t * 20, 0, 0, 1);
    glColor3ub(217, 164, 65);
    DrawCircleOutline(0, 0, 9, 9, 3.5f);
    glColor3ub(242, 207, 115);
    DrawArc(0, 0, 9, 20, 110, 1.5f);
    glPopMatrix();
}

void drawStatue(Vec2 p, int kind, float t)
{
    drawStatueBase(p);

    Color marble = hexColor(0xeeeae2);
    Color shade = hexColor(0xc8c2b6);
    if (kind == 0)
        drawBust(p, marble, shade);
    else if (kind == 1)
        drawBrokenColumn(p, marble, shade);
    else if (kind == 2)
        drawClayPot(p);
    else
        drawSpinningRing(p, t);
}

static void drawFlower(float x, float y, int id, float t)
{
    unsigned int petalColours[5] = {0xf38fac, 0xf6d55c, 0xffffff, 0xb48be2, 0xf27b57};

    glColor3ub(79, 142, 64);
    DrawCircle(x, y, 4.2f, 4.2f, 10);
    glColor3ub(110, 174, 85);
    DrawCircle(x - 1, y + 1, 2.6f, 2.6f, 8);

    float pulse = 1 + 0.10f * sinf(t * 2.2f + id);
    float petalR = 2.3f * pulse;
    int colourIndex = (int)(hash01(id, 3) * 5) % 5;
    setHex(petalColours[colourIndex]);
    DrawCircle(x + 0.8f, y + 1.2f, petalR, petalR, 8);
    glColor3ub(255, 216, 74);
    DrawCircle(x + 0.8f, y + 1.2f, 0.8f, 0.8f, 6);
}

static void drawFlowerBed(Rect r, float t, int seed)
{
    setColor(shadowColor(0.18f));
    DrawRoundRect(r.x + 2, r.y - 4, r.w + 2, r.h, 5);
    glColor3ub(184, 143, 112);
    DrawRoundRect(r.x - 3, r.y - 3, r.w + 6, r.h + 6, 6);
    setHex(SOIL);
    DrawRoundRect(r.x, r.y, r.w, r.h, 4);

    int cols = maxf(1, r.w / 10);
    int rows = maxf(1, r.h / 10);
    for (int i = 0; i < cols; i++)
    {
        for (int j = 0; j < rows; j++)
        {
            int id = seed * 977 + i * 31 + j;
            float jitterX = (hash01(id, 1) - 0.5f) * 3;
            float jitterY = (hash01(id, 2) - 0.5f) * 3;
            float x = r.x + (i + 0.5f) * r.w / cols + jitterX;
            float y = r.y + (j + 0.5f) * r.h / rows + jitterY;
            drawFlower(x, y, id, t);
        }
    }
}

static void drawGroundShadows()
{
    for (int i = 0; i < map.treeCount; i++)
    {
        Tree tree = map.trees[i];
        float r = tree.radius;
        DrawSoftCircle(tree.p.x + r * 0.45f, tree.p.y + r * 0.05f,
                       r * 1.1f, r * 0.6f, shadowColor(0.32f), shadowColor(0));
    }

    for (int i = 0; i < map.buildingCount; i++)
    {
        float offset = 16;
        if (map.buildings[i].kind == BOOTH)
            offset = 8;
        dropShadow(map.buildings[i].r, offset);
    }

    for (int i = 0; i < map.signCount; i++)
        dropShadow(map.signs[i], 8);

    setColor(shadowColor(0.25f));
    for (int i = 0; i < map.lampCount; i++)
    {
        Vec2 lamp = map.lamps[i];
        DrawThickLine(lamp.x, lamp.y, lamp.x + 20, lamp.y - 12, 3);
    }
    for (int i = 0; i < map.flagCount; i++)
    {
        Vec2 flag = map.flags[i];
        DrawThickLine(flag.x, flag.y, flag.x + 46, flag.y - 26, 2.5f);
    }
}

void drawGround(float t)
{
    drawGrass();
    drawPaths();
    for (int i = 0; i < map.lakeCount; i++)
        drawLake(map.lakes[i], t);
    for (int i = 0; i < map.fieldCount; i++)
        drawField(map.fields[i], t);
    for (int i = 0; i < map.flowerBedCount; i++)
        drawFlowerBed(map.flowerBeds[i], t, i);
    drawGroundShadows();
}

void drawBush(int i)
{
    Bush b = map.bushes[i];
    int seed = i + 3;
    float r = b.radius;
    DrawSoftCircle(b.p.x + r * 0.35f, b.p.y - r * 0.35f, r * 1.1f, r * 0.7f, shadowColor(0.28f), shadowColor(0));

    Vec2 lobes[5];
    for (int k = 0; k < 5; k++)
    {
        float angle = degToRad(k * 72 + hash01(seed, k) * 30);
        float x = b.p.x + cosf(angle) * r * 0.45f;
        float y = b.p.y + sinf(angle) * r * 0.40f;
        lobes[k] = makeVec(x, y);
    }

    float lobeR = r * 0.6f;
    glColor3ub(60, 107, 53);
    for (int k = 0; k < 5; k++)
        circle(lobes[k].x, lobes[k].y, lobeR + 1.5f);
    glColor3ub(90, 154, 73);
    for (int k = 0; k < 5; k++)
        circle(lobes[k].x, lobes[k].y, lobeR);
    circle(b.p.x, b.p.y, lobeR);

    glColor3ub(125, 186, 96);
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

    for (int plank = 0; plank < 3; plank++)
    {
        if (plank % 2 == 1)
            setHex(WOOD);
        else
            setColor(lighter(hexColor(WOOD), 0.12f));
        DrawRoundRect(-18, -6 + plank * 4, 36, 3.3f, 1.2f);
    }
    setHex(WOOD_DARK);
    DrawRoundRect(-18, 6.5f, 36, 3, 1.2f);
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
    Vec2 span = vsub(rl.b, rl.a);
    int posts = (int)(vlength(span) / 12) + 1;
    if (posts < 2)
        posts = 2;

    setColor(shadowColor(0.2f));
    DrawThickLine(rl.a.x + 3, rl.a.y - 3, rl.b.x + 3, rl.b.y - 3, 2);

    setHex(IRON);
    for (int k = 0; k < posts; k++)
    {
        float fraction = float(k) / (posts - 1);
        Vec2 post = vadd(rl.a, vscale(span, fraction));
        DrawRect(post.x - 1, post.y, 2, 9);
    }
    DrawThickLine(rl.a.x, rl.a.y + 8, rl.b.x, rl.b.y + 8, 2);
    glColor3ub(93, 103, 115);
    DrawThickLine(rl.a.x, rl.a.y + 4, rl.b.x, rl.b.y + 4, 1.2f);
}

void drawTableBase(int i)
{
    Vec2 p = map.tables[i].p;
    DrawSoftCircle(p.x + 6, p.y - 6, 22, 14, shadowColor(0.3f), shadowColor(0));

    setHex(WOOD_DARK);
    for (int chair = 0; chair < 4; chair++)
    {
        float angle = degToRad(45 + chair * 90);
        float x = p.x + cosf(angle) * 15 - 4;
        float y = p.y + sinf(angle) * 12 - 4;
        DrawRoundRect(x, y, 8, 8, 2);
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
    glColor3ub(201, 205, 210);
    DrawRect(p.x - 1.3f, p.y, 2.6f, 82);
    setHex(MUSEUM_GOLD);
    circle(p.x, p.y + 83, 2.4f);
}

void drawTreeTrunk(int i)
{
    Tree tree = map.trees[i];
    float x = tree.p.x;
    float y = tree.p.y;
    float trunkTop = y + tree.radius * 0.6f;
    glColor3ub(107, 74, 50);
    DrawQuad(x - 5, y - 2, x + 5, y - 2, x + 3, trunkTop, x - 3, trunkTop);
    glColor3ub(138, 100, 68);
    DrawRect(x - 2.5f, y, 1.5f, tree.radius * 0.5f);
}

static void drawBrickRows(Rect w, float faceTop)
{
    setHex(BRICK_DARK, 0.7f);
    glBegin(GL_LINES);
    int row = 0;
    for (float y = w.y; y < faceTop; y += 3.8f)
    {
        glVertex2f(w.x, y);
        glVertex2f(w.x + w.w, y);

        float shift = 0;
        if (row % 2 == 1)
            shift = 7;
        float brickTop = minf(y + 3.8f, faceTop);
        for (float x = w.x + shift; x < w.x + w.w; x += 14)
        {
            glVertex2f(x, y);
            glVertex2f(x, brickTop);
        }
        row++;
    }
    glEnd();
}

static void drawHorizontalWall(Rect w)
{
    float faceTop = w.y + w.h * 0.55f;
    drawBrickRows(w, faceTop);
    setHex(BRICK_CAP);
    DrawRect(w.x, faceTop, w.w, w.y + w.h - faceTop);
    setHex(WHITE, 0.4f);
    float edgeY = w.y + w.h - 1;
    DrawLine(w.x, edgeY, w.x + w.w, edgeY, 1);
}

static void drawVerticalWall(Rect w)
{
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

void drawWall(int i)
{
    Rect w = map.walls[i];
    setColor(shadowColor(0.25f));
    DrawRect(w.x + 4, w.y - 4, w.w, w.h);
    setHex(BRICK);
    DrawRectR(w);

    glLineWidth(1);
    if (w.w >= w.h)
        drawHorizontalWall(w);
    else
        drawVerticalWall(w);
}

static void drawPillar(float cx, float cy, float size)
{
    float half = size * 0.5f;
    float left = cx - half;
    float bottom = cy - half;
    setColor(shadowColor(0.3f));
    DrawRect(left + 5, bottom - 5, size, size);
    glColor3ub(166, 158, 144);
    DrawRect(left, bottom, size, size);
    setHex(STONE_LIGHT);
    DrawRect(left + 3, bottom + 3, size - 6, size - 6);

    setHex(0xa69e90, 0.7f);
    DrawLine(left + 3, bottom + 3, cx + half - 3, cy + half - 3, 1);
    DrawLine(left + 3, cy + half - 3, cx + half - 3, bottom + 3, 1);
    setHex(MUSEUM_GOLD);
    circle(cx, cy, size * 0.18f);
}

static void drawGatePanel(float hingeX, float hingeY, float angle, float length)
{
    glPushMatrix();
    glTranslatef(hingeX, hingeY, 0);
    glRotatef(angle, 0, 0, 1);
    setColor(shadowColor(0.25f));
    DrawRect(2, -5, length, 4);
    setHex(IRON);
    DrawRect(0, -1.5f, length, 3);
    glColor3ub(108, 118, 132);
    for (float x = 4; x < length; x += 6)
        DrawRect(x - 0.8f, -3, 1.6f, 6);
    setHex(MUSEUM_GOLD);
    circle(length, 0, 2);
    glPopMatrix();
}

void drawGate()
{
    setHex(0x9aa0a8, 0.8f);
    DrawLine(map.gateX0, 7, map.gateX1, 7, 2);
    drawGatePanel(map.gateX0, 7, 78, 46);
    drawGatePanel(map.gateX1, 7, 102, 46);
    drawPillar(map.gateX0 - 8, 9, 20);
    drawPillar(map.gateX1 + 8, 9, 20);
}

static void drawOpenBookEmblem(float x, float y)
{
    setHex(MUSEUM_BLUE);
    circle(x, y, 15);
    setHex(MUSEUM_GOLD);
    DrawCircleOutline(x, y, 13, 13, 1.5f);
    setHex(WHITE);
    DrawQuad(x - 9, y - 3, x - 1, y - 5, x - 1, y + 5, x - 9, y + 7);
    DrawQuad(x + 1, y - 5, x + 9, y - 3, x + 9, y + 7, x + 1, y + 5);
}

void drawSign(int i)
{
    Rect r = map.signs[i];
    glColor3ub(143, 136, 121);
    DrawRoundRect(r.x - 4, r.y - 4, r.w + 8, r.h + 8, 6);
    setHex(STONE_LIGHT);
    DrawRoundRect(r.x, r.y, r.w, r.h, 5);
    setHex(MUSEUM_BLUE);
    DrawRect(r.x, r.y, r.w, 6);

    drawOpenBookEmblem(r.x + 22, r.y + r.h * 0.55f);

    float textLeft = r.x + 44;
    float textW = r.w - 52;
    float textCentre = textLeft + textW * 0.5f;
    setHex(MUSEUM_BLUE);
    DrawTextFit(textCentre, r.y + r.h - 20, "CITY MUSEUM", FONT_MEDIUM, textW);
    DrawTextFit(textCentre, r.y + r.h - 34, "OF HISTORY", FONT_NORMAL, textW);
    glColor3ub(123, 116, 104);
    DrawTextFit(textCentre, r.y + 13, "Main Gate  -  Visitors Welcome", FONT_SMALL, textW);
}

struct BuildingLook
{
    Building b;
    Rect r;
    bool booth;
    Color wall, wallDark, roof, roofDark, parapet, accent;
    float faceH, roofY, roofH, top, doorW, doorH, parapetW;
};

static BuildingLook makeBuildingLook(Building b)
{
    unsigned int styles[4][6] = {
        {0xf3e9d6, 0xd6c6a8, 0xd8d4cb, 0xc3beb2, 0xaea595, MUSEUM_BLUE},
        {0xedcfb0, 0xcda680, 0xcfb49c, 0xb79a80, 0xa27f66, 0x8c4b3b},
        {0xf7e2a9, 0xd9be7f, 0xe89c70, 0xcf8158, 0xb66a4b, 0xd8524a},
        {0xe4ecf3, 0xbcc8d5, 0x4472a8, 0x365e8f, 0x2a4d78, 0x4472a8}};

    BuildingLook L;
    L.b = b;
    L.r = b.r;
    L.booth = (b.kind == BOOTH);
    L.wall = hexColor(styles[b.kind][0]);
    L.wallDark = hexColor(styles[b.kind][1]);
    L.roof = hexColor(styles[b.kind][2]);
    L.roofDark = hexColor(styles[b.kind][3]);
    L.parapet = hexColor(styles[b.kind][4]);
    L.accent = hexColor(styles[b.kind][5]);

    if (L.booth)
    {
        L.faceH = L.r.h * 0.42f;
        L.doorW = 12;
        L.doorH = L.faceH * 0.8f;
        L.parapetW = 3;
    }
    else
    {
        L.faceH = minf(L.r.h * 0.36f, 58);
        L.doorW = 30;
        if (b.kind == HALL)
            L.doorW = 40;
        L.doorH = L.faceH * 0.72f;
        L.parapetW = 5;
    }
    L.roofY = L.r.y + L.faceH;
    L.roofH = L.r.y + L.r.h - L.roofY;
    L.top = L.r.y + L.r.h;
    return L;
}

static void drawEntranceSteps(BuildingLook L)
{
    for (int k = 2; k >= 0; k--)
    {
        float stepW = L.doorW + 16 + k * 8;
        float stepH = 4 * (k + 1);
        if (k % 2 == 1)
            setHex(STONE);
        else
            setHex(STONE_LIGHT);
        DrawRect(L.b.doorX - stepW * 0.5f, L.r.y - stepH, stepW, stepH);
    }
}

static void drawGalleryAwning(BuildingLook L)
{
    Rect r = L.r;
    float awningW = r.w - 10;
    float awningY = r.y + L.faceH * 0.45f;
    int stripes = (int)(awningW / 12);
    float stripeW = awningW / stripes;
    for (int k = 0; k < stripes; k++)
    {
        if (k % 2 == 1)
            setHex(WHITE);
        else
            setColor(L.accent);
        float x = r.x + 5 + k * stripeW;
        DrawRect(x, awningY, stripeW, L.faceH * 0.5f);
        DrawSector(x + stripeW * 0.5f, awningY, stripeW * 0.5f, 180, 360);
    }

    float counterH = L.faceH * 0.32f;
    setHex(WOOD);
    DrawRect(r.x + 14, r.y + 4, r.w - 28, counterH);
    setHex(WOOD_DARK);
    DrawRect(r.x + 14, r.y + 4 + counterH - 3, r.w - 28, 3);
}

static void drawWindow(BuildingLook L, float cx, float bayW)
{
    float winW = bayW * 0.56f;
    float winH = L.faceH * 0.46f;
    float winY = L.r.y + L.faceH * 0.30f;
    float left = cx - winW * 0.5f;
    float right = cx + winW * 0.5f;

    setHex(FRAME);
    DrawRect(left - 2, winY - 2, winW + 4, winH + 4);
    DrawGradientRect(left, winY, winW, winH, hexColor(GLASS_DARK), hexColor(GLASS));

    setHex(WHITE, 0.35f);
    DrawQuad(left, winY + winH * 0.2f, cx - winW * 0.1f, winY + winH,
             cx + winW * 0.1f, winY + winH, left + winW * 0.15f, winY);

    setHex(FRAME);
    DrawLine(cx, winY, cx, winY + winH, 1.2f);
    DrawLine(left, winY + winH * 0.5f, right, winY + winH * 0.5f, 1.2f);

    setColor(lighter(L.wall, 0.4f));
    DrawRect(left - 3, winY - 4, winW + 6, 2.5f);
}

static void drawDoor(BuildingLook L)
{
    float doorX = L.b.doorX;
    float left = doorX - L.doorW * 0.5f;
    float bottom = L.r.y;

    setColor(L.accent);
    DrawRect(left - 3, bottom, L.doorW + 6, L.doorH + 3);
    DrawGradientRect(left, bottom, L.doorW, L.doorH, hexColor(GLASS_DARK), lighter(hexColor(GLASS), 0.2f));
    setHex(FRAME);
    DrawLine(doorX, bottom, doorX, bottom + L.doorH, 1.5f);

    float handleY = bottom + L.doorH * 0.45f;
    setHex(MUSEUM_GOLD);
    DrawCircle(doorX - 3, handleY, 1.3f, 1.3f, 8);
    DrawCircle(doorX + 3, handleY, 1.3f, 1.3f, 8);

    if (!L.booth)
    {
        setColor(shadowColor(0.3f));
        DrawRect(left - 8, bottom + L.doorH - 2, L.doorW + 18, 5);
        setColor(L.accent);
        DrawRect(left - 10, bottom + L.doorH + 3, L.doorW + 20, 5);
    }
}

static void drawWindowsAndDoor(BuildingLook L)
{
    Rect r = L.r;
    float bayTarget = 44;
    if (L.booth)
        bayTarget = 30;
    int bays = (int)(r.w / bayTarget);
    if (bays < 1)
        bays = 1;
    float bayW = r.w / bays;

    for (int k = 0; k < bays; k++)
    {
        float cx = r.x + bayW * (k + 0.5f);
        float gapForDoor = L.doorW * 0.5f + bayW * 0.35f;
        if (fabsf(cx - L.b.doorX) < gapForDoor)
            continue;

        drawWindow(L, cx, bayW);

        if (!L.booth && k > 0)
        {
            setColor(withAlpha(L.wallDark, 0.7f));
            DrawRect(r.x + bayW * k - 2, r.y + 4, 4, L.faceH - 4);
        }
    }
    drawDoor(L);
}

static void drawFrontWall(BuildingLook L)
{
    Rect r = L.r;
    DrawGradientRect(r.x, r.y, r.w, L.faceH, L.wallDark, L.wall);
    setColor(darker(L.wallDark, 0.15f));
    DrawRect(r.x, r.y, r.w, 4);

    if (L.b.kind == GALLERY)
        drawGalleryAwning(L);
    else
        drawWindowsAndDoor(L);
}

static void drawRoof(BuildingLook L)
{
    Rect r = L.r;
    float pw = L.parapetW;
    setColor(L.roof);
    DrawRect(r.x, L.roofY, r.w, L.roofH);

    setColor(L.parapet);
    DrawRect(r.x, L.roofY, r.w, pw);
    DrawRect(r.x, L.top - pw, r.w, pw);
    DrawRect(r.x, L.roofY, pw, L.roofH);
    DrawRect(r.x + r.w - pw, L.roofY, pw, L.roofH);

    setHex(WHITE, 0.25f);
    DrawRectOutline(r.x + pw, L.roofY + pw, r.w - 2 * pw, L.roofH - 2 * pw, 1);
    setColor(darker(L.parapet, 0.3f));
    DrawRect(r.x, L.roofY - 3, r.w, 3);
}

static void drawRoofSeams(BuildingLook L)
{
    Rect r = L.r;
    float pw = L.parapetW;
    setColor(withAlpha(L.roofDark, 0.6f));
    for (float x = r.x + 40; x < r.x + r.w - pw; x += 40)
        DrawLine(x, L.roofY + pw, x, L.top - pw, 1);
}

static void drawSolarPanel(float x, float y)
{
    setColor(shadowColor(0.35f));
    DrawRect(x + 3, y - 3, 76, 38);
    glColor3ub(44, 79, 124);
    DrawRect(x, y, 76, 38);

    setHex(0x9cc4ef, 0.7f);
    for (float lineX = x + 12.7f; lineX < x + 76; lineX += 12.7f)
        DrawLine(lineX, y, lineX, y + 38, 1);
    DrawLine(x, y + 19, x + 76, y + 19, 1);

    setHex(WHITE, 0.25f);
    DrawTriangle(x, y + 38, x + 30, y + 38, x, y + 14);
}

static void drawSolarPanels(BuildingLook L)
{
    for (int k = 0; k < 2; k++)
    {
        float x = L.r.x + 24 + k * 92;
        float y = L.roofY + L.roofH * 0.42f;
        drawSolarPanel(x, y);
    }
}

static void drawSkylight(BuildingLook L)
{
    float x = L.r.x + L.r.w * 0.18f;
    float y = L.roofY + L.roofH * 0.35f;
    setColor(L.roofDark);
    DrawRect(x - 3, y - 3, 96, 46);
    DrawGradientRect(x, y, 90, 40, hexColor(GLASS_DARK), hexColor(GLASS));

    setHex(FRAME);
    for (float lineX = x + 15; lineX < x + 90; lineX += 15)
        DrawLine(lineX, y, lineX, y + 40, 1);
    DrawLine(x, y + 20, x + 90, y + 20, 1);
}

static void drawWaterTank(BuildingLook L)
{
    float x = L.r.x + L.r.w - 42;
    float y = L.top - 30;
    DrawSoftCircle(x + 6, y - 5, 20, 16, shadowColor(0.4f), shadowColor(0));
    glColor3ub(63, 74, 87);
    circle(x, y, 15);
    glColor3ub(90, 107, 125);
    circle(x, y, 12.5f);
    glColor3ub(111, 130, 150);
    circle(x - 2, y + 2, 6);
}

static void drawAcUnit(float x, float y, int k, float t)
{
    setColor(shadowColor(0.35f));
    DrawRect(x - 10, y - 15, 24, 24);
    glColor3ub(238, 240, 242);
    DrawRoundRect(x - 12, y - 12, 24, 24, 3);
    glColor3ub(154, 163, 173);
    circle(x, y, 9);

    float fanAngle = fmodf(t * 360 * (1 + k * 0.3f), 360);
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(fanAngle, 0, 0, 1);
    glColor3ub(92, 102, 112);
    for (int blade = 0; blade < 3; blade++)
        DrawSector(0, 0, 8, blade * 120, blade * 120 + 40);
    glPopMatrix();
}

static void drawAcUnits(BuildingLook L, float t)
{
    for (int k = 0; k < 3; k++)
    {
        float x = L.r.x + L.r.w - 100 - k * 34;
        float y = L.top - 30;
        if (x < L.r.x + 20)
            break;
        drawAcUnit(x, y, k, t);
    }
}

static void drawStairRoom(BuildingLook L)
{
    Rect r = L.r;
    Rect room = {r.x + r.w - 100 - 3 * 34 - 46, L.roofY + 14, 40, 30};
    if (room.x <= r.x + 200)
        return;
    dropShadow(room, 6);
    setColor(L.wallDark);
    DrawRectR(room);
    setColor(L.roof);
    DrawRect(room.x + 2, room.y + 6, room.w - 4, room.h - 8);
}

static void drawRoofEquipment(BuildingLook L, float t)
{
    drawRoofSeams(L);
    if (L.b.kind == HALL)
        drawSolarPanels(L);
    else
        drawSkylight(L);
    drawWaterTank(L);
    drawAcUnits(L, t);
    drawStairRoom(L);
}

static void drawNameBoard(BuildingLook L)
{
    int font = FONT_NORMAL;
    float pad = 18;
    float boardH = 16;
    float boardY = L.roofY + 8;
    float textOffset = 4.5f;
    if (L.booth)
    {
        font = FONT_SMALL;
        pad = 6;
        boardH = 11;
        boardY = L.top - boardH - 3;
        textOffset = 2.5f;
    }

    float boardW = minf(TextWidth(L.b.name, font) + pad, L.r.w - 6);
    float cx = L.r.x + L.r.w * 0.5f;
    float left = cx - boardW * 0.5f;

    setColor(shadowColor(0.35f));
    DrawRoundRect(left + 2, boardY - 2, boardW, boardH, 3);

    if (L.booth)
        setHex(WHITE);
    else
        setColor(L.accent);
    DrawRoundRect(left, boardY, boardW, boardH, 3);

    if (L.booth)
        setColor(L.accent);
    else
        setHex(WHITE);
    DrawTextFit(cx, boardY + textOffset, L.b.name, font, boardW - pad + 2);
}

void drawBuilding(int i, float t)
{
    BuildingLook L = makeBuildingLook(map.buildings[i]);

    if (!L.booth)
        drawEntranceSteps(L);
    drawFrontWall(L);
    drawRoof(L);
    if (L.b.kind == HALL || L.b.kind == OFFICE)
        drawRoofEquipment(L, t);
    drawNameBoard(L);
}

static void drawCanopyLobes(float r, int seed, unsigned int *shades)
{
    Vec2 lobe[7];
    float lobeR[7];
    for (int k = 0; k < 7; k++)
    {
        float angle = degToRad(k * 360.0f / 7 + hash01(seed, k) * 25);
        float distance = r * (0.42f + hash01(seed, k + 10) * 0.12f);
        lobe[k] = makeVec(cosf(angle) * distance, sinf(angle) * distance * 0.92f);
        lobeR[k] = r * (0.48f + hash01(seed, k + 20) * 0.12f);
    }

    setHex(shades[0]);
    circle(0, 0, r * 0.62f + 2);
    for (int k = 0; k < 7; k++)
        circle(lobe[k].x, lobe[k].y, lobeR[k] + 2);

    setHex(shades[1]);
    circle(0, 0, r * 0.62f);
    for (int k = 0; k < 7; k++)
        circle(lobe[k].x, lobe[k].y, lobeR[k]);

    setHex(shades[2]);
    for (int k = 0; k < 7; k++)
    {
        float x = lobe[k].x * 0.75f - r * 0.08f;
        float y = lobe[k].y * 0.75f + r * 0.08f;
        circle(x, y, lobeR[k] * 0.72f);
    }
}

static void drawCanopyHighlights(float r, unsigned int lightest)
{
    setHex(lightest);
    circle(-r * 0.22f, r * 0.25f, r * 0.3f);
    circle(r * 0.1f, r * 0.38f, r * 0.18f);
    Color shine = withAlpha(lighter(hexColor(lightest), 0.3f), 0.8f);
    setColor(shine);
    circle(-r * 0.3f, r * 0.34f, r * 0.1f);
}

static void drawFlameBlossoms(float r, int seed)
{
    unsigned int blossomColours[3] = {0xe8463a, 0xf27a35, 0xf9a43f};
    for (int k = 0; k < 22; k++)
    {
        float angle = hash01(seed, 40 + k) * 2 * PI;
        float distance = sqrtf(hash01(seed, 80 + k)) * r * 0.85f;
        float size = 2 + hash01(seed, 120 + k) * 1.4f;
        setHex(blossomColours[k % 3]);
        DrawCircle(cosf(angle) * distance, sinf(angle) * distance, size, size, 8);
    }
}

static void drawMangoes(float r, int seed)
{
    glColor3ub(201, 214, 90);
    for (int k = 0; k < 6; k++)
    {
        float angle = hash01(seed, 50 + k) * 2 * PI;
        float distance = sqrtf(hash01(seed, 90 + k)) * r * 0.7f;
        DrawCircle(cosf(angle) * distance, sinf(angle) * distance, 2, 2.8f, 8);
    }
}

static void drawTreeCanopy(Tree tree, float t, int seed)
{
    float r = tree.radius;
    unsigned int colours[3][4] = {
        {0x355f31, 0x4f8a43, 0x68a853, 0x8fc56c},
        {0x3a6131, 0x5b8f45, 0x76aa55, 0x9acb6e},
        {0x2c5230, 0x3f7a47, 0x559459, 0x78b571}};
    unsigned int *shades = colours[tree.kind];

    float sway = sinf(t * 1.2f + seed * 1.7f) * 1.8f;
    float breathe = 1 + 0.015f * sinf(t * 1.6f + seed);
    glPushMatrix();
    glTranslatef(tree.p.x, tree.p.y + r * 0.6f, 0);
    glRotatef(sway, 0, 0, 1);
    glScalef(breathe, breathe, 1);

    drawCanopyLobes(r, seed, shades);
    drawCanopyHighlights(r, shades[3]);
    if (tree.kind == FLAME_TREE)
        drawFlameBlossoms(r, seed);
    else if (tree.kind == MANGO_TREE)
        drawMangoes(r, seed);
    glPopMatrix();
}

static void drawUmbrella(Table table, float t)
{
    unsigned int topColours[3] = {0xe8574e, 0x3f8fb5, 0xf2b33d};
    Color colour = hexColor(topColours[table.colorIndex % 3]);
    float sway = sinf(t * 1.5f + table.p.x) * 3;

    glPushMatrix();
    glTranslatef(table.p.x, table.p.y + 10, 0);
    glRotatef(22.5f + sway, 0, 0, 1);
    DrawSoftCircle(6, -6, 24, 22, shadowColor(0.25f), shadowColor(0));

    for (int k = 0; k < 8; k++)
    {
        if (k % 2 == 1)
            setColor(colour);
        else
            setHex(WHITE);
        float a1 = degToRad(k * 45.0f);
        float a2 = degToRad((k + 1) * 45.0f);
        DrawTriangle(0, 0, cosf(a1) * 21, sinf(a1) * 21, cosf(a2) * 21, sinf(a2) * 21);
    }

    setColor(darker(colour, 0.3f));
    glLineWidth(1.2f);
    glBegin(GL_LINE_LOOP);
    for (int k = 0; k < 8; k++)
    {
        float angle = degToRad(k * 45.0f);
        glVertex2f(cosf(angle) * 21, sinf(angle) * 21);
    }
    glEnd();

    setHex(WOOD_DARK);
    circle(0, 0, 2.5f);
    glPopMatrix();
}

static void drawFlagCloth(Vec2 p, float t)
{
    float w = 36;
    float h = 22;
    float top = p.y + 80;
    int pieces = 14;
    Color green = hexColor(0x006a4e);

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= pieces; i++)
    {
        float along = float(i) / pieces;
        float wave = sinf(along * 6 - t * 5) * 2.2f * along;
        float shade = 0.88f + 0.12f * cosf(along * 6 - t * 5);
        glColor4f(green.r * shade, green.g * shade, green.b * shade, 1);
        float x = p.x + 1 + along * w;
        glVertex2f(x, top - h + wave);
        glVertex2f(x, top + wave);
    }
    glEnd();

    float sunAlong = 0.45f;
    float sunX = p.x + 1 + sunAlong * w;
    float sunWave = sinf(sunAlong * 6 - t * 5) * 2.2f * sunAlong;
    glColor3ub(244, 42, 65);
    circle(sunX, top - h * 0.5f + sunWave, h * 0.3f);
}

static void drawLampHead(Vec2 p, float t)
{
    float glow = 0.22f + 0.05f * sinf(t * 3 + p.x);
    float headY = p.y + 31;
    DrawSoftCircle(p.x, headY, 26, 22, hexColor(LAMP_WARM, glow), hexColor(LAMP_WARM, 0));
    setHex(IRON);
    DrawCircle(p.x, headY, 7.5f, 4.5f, 0);
    setHex(LAMP_WARM);
    circle(p.x, headY, 2.8f);
}

static void drawGateLamps(float t)
{
    float lampX[2] = {map.gateX0 - 8, map.gateX1 + 8};
    for (int i = 0; i < 2; i++)
    {
        float glow = 0.25f + 0.05f * sinf(t * 2.5f + lampX[i]);
        DrawSoftCircle(lampX[i], 9, 20, 18, hexColor(LAMP_WARM, glow), hexColor(LAMP_WARM, 0));
        setHex(LAMP_WARM);
        circle(lampX[i], 9, 3);
    }
}

static void drawGateArch(float t)
{
    float x0 = map.gateX0 - 18;
    float x1 = map.gateX1 + 18;
    float width = x1 - x0;
    float y = 16;
    float h = 14;

    setColor(shadowColor(0.25f));
    DrawRect(x0 + 8, y - 12, width, h);
    setColor(darker(hexColor(MUSEUM_BLUE), 0.25f));
    DrawRect(x0, y - 2, width, h + 4);
    setHex(MUSEUM_BLUE);
    DrawRect(x0, y, width, h);
    setHex(MUSEUM_GOLD);
    DrawRect(x0, y + h - 2, width, 2);
    setHex(WHITE);
    DrawTextFit((x0 + x1) * 0.5f, y + 3.5f, "CITY MUSEUM OF HISTORY", FONT_SMALL, width - 8);

    drawGateLamps(t);
}

static void drawClouds(float t)
{
    Color inner = {0.15f, 0.20f, 0.30f, 0.06f};
    Color outer = {0.15f, 0.20f, 0.30f, 0.0f};
    for (int c = 0; c < 7; c++)
    {
        float speed = 7 + (c % 3) * 2;
        float x = fmodf(c * 610.0f + t * speed, WORLD_W + 400) - 200;
        float y = 120 + c * 160;
        for (int k = 0; k < 4; k++)
        {
            float puffX = x + (k - 1.5f) * 45;
            float puffY = y + sinf(k * 1.9f + c) * 18;
            DrawSoftCircle(puffX, puffY, 70, 42, inner, outer);
        }
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

void drawCollisionDebug()
{
    glColor4f(1, 0.2f, 0.2f, 0.18f);
    for (int i = 0; i < map.solidCount; i++)
        DrawRectR(map.solids[i]);

    glColor4f(1, 0.1f, 0.1f, 0.9f);
    for (int i = 0; i < map.solidCount; i++)
    {
        Rect s = map.solids[i];
        DrawRectOutline(s.x, s.y, s.w, s.h, 1.5f);
    }
    for (int i = 0; i < map.circleCount; i++)
        DrawCircleOutline(map.circlePos[i].x, map.circlePos[i].y, map.circleR[i], map.circleR[i], 1.5f);

    glColor4f(0.2f, 0.4f, 1, 0.9f);
    for (int i = 0; i < map.blockerCount; i++)
    {
        Rect b = map.blockers[i];
        DrawRectOutline(b.x + 2, b.y + 2, b.w - 4, b.h - 4, 1);
    }

    glColor4f(0.1f, 0.9f, 0.3f, 0.25f);
    circle(player.pos.x, player.pos.y, PLAYER_RADIUS);
    glColor4f(0.1f, 0.7f, 0.2f, 1);
    DrawCircleOutline(player.pos.x, player.pos.y, PLAYER_RADIUS, PLAYER_RADIUS, 1.5f);
}
