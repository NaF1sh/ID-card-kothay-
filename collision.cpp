#include "game.h"

#define GRID_W 50
#define GRID_H 24
#define CELL_COUNT (GRID_W * GRID_H)
#define CELL 20.0f
#define DIAGONAL_STEP 35.35534f

static Vec2 closestPointOnRect(Vec2 p, Rect r)
{
    float x = clampf(p.x, r.x, r.x + r.w);
    float y = clampf(p.y, r.y, r.y + r.h);
    return makeVec(x, y);
}

bool circleVsRect(Vec2 c, float r, Rect rect)
{
    Vec2 closest = closestPointOnRect(c, rect);
    Vec2 offset = vsub(c, closest);
    return vdot(offset, offset) < r * r;
}

static bool circleVsCircle(Vec2 a, float ra, Vec2 b, float rb)
{
    Vec2 offset = vsub(a, b);
    float touchDistance = ra + rb;
    return vdot(offset, offset) < touchDistance * touchDistance;
}

static void pushToNearestEdge(Vec2 &c, float r, Rect rect)
{
    float toLeft = c.x - rect.x;
    float toRight = rect.x + rect.w - c.x;
    float toBottom = c.y - rect.y;
    float toTop = rect.y + rect.h - c.y;
    float nearest = minf(minf(toLeft, toRight), minf(toBottom, toTop));

    if (nearest == toLeft)
        c.x = rect.x - r;
    else if (nearest == toRight)
        c.x = rect.x + rect.w + r;
    else if (nearest == toBottom)
        c.y = rect.y - r;
    else
        c.y = rect.y + rect.h + r;
}

static bool pushOutOfRect(Vec2 &c, float r, Rect rect)
{
    Vec2 closest = closestPointOnRect(c, rect);
    Vec2 offset = vsub(c, closest);
    float dist = vlength(offset);
    if (dist >= r)
        return false;

    if (dist > 0.0001f)
    {
        Vec2 pushed = vscale(offset, r / dist);
        c = vadd(closest, pushed);
    }
    else
    {
        pushToNearestEdge(c, r, rect);
    }
    return true;
}

static bool pushOutOfCircle(Vec2 &c, float r, Vec2 center, float centerRadius)
{
    Vec2 offset = vsub(c, center);
    float dist = vlength(offset);
    float minDist = r + centerRadius;
    if (dist >= minDist)
        return false;

    Vec2 direction;
    if (dist > 0.0001f)
        direction = vscale(offset, 1.0f / dist);
    else
        direction = makeVec(0, 1);
    c = vadd(center, vscale(direction, minDist));
    return true;
}

bool overlapsSolid(Vec2 c, float r)
{
    for (int i = 0; i < map.solidCount; i++)
        if (circleVsRect(c, r, map.solids[i]))
            return true;
    for (int i = 0; i < map.circleCount; i++)
        if (circleVsCircle(c, r, map.circlePos[i], map.circleR[i]))
            return true;
    return false;
}

Vec2 clampToWorld(Vec2 c, float r)
{
    float x = clampf(c.x, r, WORLD_W - r);
    float y = clampf(c.y, r, WORLD_H - r);
    return makeVec(x, y);
}

static bool pushOutOfAllSolids(Vec2 &c, float r)
{
    bool moved = false;
    for (int i = 0; i < map.solidCount; i++)
        if (pushOutOfRect(c, r, map.solids[i]))
            moved = true;
    for (int i = 0; i < map.circleCount; i++)
        if (pushOutOfCircle(c, r, map.circlePos[i], map.circleR[i]))
            moved = true;
    return moved;
}

Vec2 moveCircle(Vec2 from, Vec2 delta, float r)
{
    float maxStep = r * 0.5f;
    float distance = vlength(delta);
    int steps = (int)ceilf(distance / maxStep);
    if (steps < 1)
        steps = 1;
    Vec2 step = vscale(delta, 1.0f / steps);
    Vec2 c = from;

    for (int s = 0; s < steps; s++)
    {
        c = vadd(c, step);
        for (int pass = 0; pass < 4; pass++)
        {
            bool moved = pushOutOfAllSolids(c, r);
            c = clampToWorld(c, r);
            if (!moved)
                break;
        }
    }
    return c;
}

Vec2 nearestFreeSpot(Vec2 p, float radius)
{
    if (!overlapsSolid(p, radius))
        return p;

    for (float ring = 5; ring <= 200; ring += 5)
    {
        for (int k = 0; k < 24; k++)
        {
            float angle = k * (2 * PI / 24);
            float x = p.x + cosf(angle) * ring;
            float y = p.y + sinf(angle) * ring;
            Vec2 spot = clampToWorld(makeVec(x, y), radius + 14);
            if (!overlapsSolid(spot, radius))
                return spot;
        }
    }
    return p;
}

static bool raySegment(Vec2 origin, Vec2 dir, Vec2 a, Vec2 b, float &hitDistance)
{
    Vec2 edge = vsub(b, a);
    float denom = vcross(dir, edge);
    if (fabsf(denom) < 1e-8f)
        return false;

    Vec2 originToA = vsub(a, origin);
    float alongRay = vcross(originToA, edge) / denom;
    float alongEdge = vcross(originToA, dir) / denom;
    if (alongRay < 0 || alongEdge < 0 || alongEdge > 1)
        return false;

    hitDistance = alongRay;
    return true;
}

static float orient(Vec2 a, Vec2 b, Vec2 c)
{
    return vcross(vsub(b, a), vsub(c, a));
}

static bool onSegment(Vec2 a, Vec2 b, Vec2 p)
{
    bool insideX = minf(a.x, b.x) <= p.x && p.x <= maxf(a.x, b.x);
    bool insideY = minf(a.y, b.y) <= p.y && p.y <= maxf(a.y, b.y);
    return insideX && insideY;
}

static bool oppositeSides(float side1, float side2)
{
    return (side1 > 0 && side2 < 0) || (side1 < 0 && side2 > 0);
}

static bool segmentsIntersect(Vec2 p1, Vec2 p2, Vec2 q1, Vec2 q2)
{
    float sideP1 = orient(q1, q2, p1);
    float sideP2 = orient(q1, q2, p2);
    float sideQ1 = orient(p1, p2, q1);
    float sideQ2 = orient(p1, p2, q2);

    if (oppositeSides(sideP1, sideP2) && oppositeSides(sideQ1, sideQ2))
        return true;
    if (sideP1 == 0 && onSegment(q1, q2, p1))
        return true;
    if (sideP2 == 0 && onSegment(q1, q2, p2))
        return true;
    if (sideQ1 == 0 && onSegment(p1, p2, q1))
        return true;
    if (sideQ2 == 0 && onSegment(p1, p2, q2))
        return true;
    return false;
}

static void rectCorners(Rect r, Vec2 corners[4])
{
    corners[0] = makeVec(r.x, r.y);
    corners[1] = makeVec(r.x + r.w, r.y);
    corners[2] = makeVec(r.x + r.w, r.y + r.h);
    corners[3] = makeVec(r.x, r.y + r.h);
}

static bool segmentVsRect(Vec2 a, Vec2 b, Rect r)
{
    if (rectContains(r, a) || rectContains(r, b))
        return true;

    Vec2 corners[4];
    rectCorners(r, corners);
    for (int i = 0; i < 4; i++)
        if (segmentsIntersect(a, b, corners[i], corners[(i + 1) % 4]))
            return true;
    return false;
}

float raycast(Vec2 origin, Vec2 dir, float maxDist)
{
    float best = maxDist;
    Vec2 corners[4];
    for (int i = 0; i < map.blockerCount; i++)
    {
        Rect r = map.blockers[i];
        Vec2 closest = closestPointOnRect(origin, r);
        float distanceToRect = vlength(vsub(closest, origin));
        if (distanceToRect > best)
            continue;

        rectCorners(r, corners);
        for (int k = 0; k < 4; k++)
        {
            float hit;
            if (raySegment(origin, dir, corners[k], corners[(k + 1) % 4], hit) && hit < best)
                best = hit;
        }
    }
    return best;
}

bool lineOfSight(Vec2 a, Vec2 b)
{
    for (int i = 0; i < map.blockerCount; i++)
        if (segmentVsRect(a, b, map.blockers[i]))
            return false;
    return true;
}

bool clearPath(Vec2 from, Vec2 to, float radius)
{
    Vec2 line = vsub(to, from);
    int steps = (int)(vlength(line) / 5) + 1;
    for (int i = 0; i <= steps; i++)
    {
        float fraction = float(i) / steps;
        Vec2 p = vadd(from, vscale(line, fraction));
        if (overlapsSolid(p, radius))
            return false;
    }
    return true;
}

static Vec2 cellCenter(int id)
{
    float x = (id % GRID_W + 0.5f) * CELL;
    float y = (id / GRID_W + 0.5f) * CELL;
    return makeVec(x, y);
}

static bool insideGrid(int x, int y)
{
    return x >= 0 && x < GRID_W && y >= 0 && y < GRID_H;
}

static int nearestCell(Vec2 p, float radius)
{
    int best = -1;
    float bestDist = 110;
    int cellX = (int)(p.x / CELL);
    int cellY = (int)(p.y / CELL);

    for (int y = cellY - 3; y <= cellY + 3; y++)
    {
        for (int x = cellX - 3; x <= cellX + 3; x++)
        {
            if (!insideGrid(x, y))
                continue;
            int id = y * GRID_W + x;
            float dist = vlength(vsub(cellCenter(id), p));
            if (dist < bestDist && clearPath(p, cellCenter(id), radius))
            {
                bestDist = dist;
                best = id;
            }
        }
    }
    return best;
}

static float cost[CELL_COUNT];
static float score[CELL_COUNT];
static int parent[CELL_COUNT];
static bool isOpen[CELL_COUNT];
static bool closed[CELL_COUNT];

static void resetSearch(int start)
{
    for (int i = 0; i < CELL_COUNT; i++)
    {
        cost[i] = 1e9f;
        parent[i] = -1;
        isOpen[i] = false;
        closed[i] = false;
    }
    cost[start] = 0;
    score[start] = 0;
    isOpen[start] = true;
}

static int lowestScoreOpenCell()
{
    int best = -1;
    for (int i = 0; i < CELL_COUNT; i++)
        if (isOpen[i] && (best < 0 || score[i] < score[best]))
            best = i;
    return best;
}

static int buildPath(int goal, Vec2 to, Vec2 out[], int maxOut)
{
    Vec2 backwards[CELL_COUNT];
    int n = 0;
    for (int id = goal; id >= 0; id = parent[id])
        backwards[n++] = cellCenter(id);

    int count = 0;
    for (int i = n - 1; i >= 0 && count < maxOut - 1; i--)
        out[count++] = backwards[i];
    out[count++] = to;
    return count;
}

static void tryNeighbour(int current, int next, float stepCost, int goal, float radius)
{
    float candidate = cost[current] + stepCost;
    if (closed[next] || candidate >= cost[next])
        return;
    if (!clearPath(cellCenter(current), cellCenter(next), radius))
        return;

    parent[next] = current;
    cost[next] = candidate;
    float distanceToGoal = vlength(vsub(cellCenter(next), cellCenter(goal)));
    score[next] = candidate + distanceToGoal;
    isOpen[next] = true;
}

static void openNeighbours(int current, int goal, float radius)
{
    int x = current % GRID_W;
    int y = current / GRID_W;

    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            if (dx == 0 && dy == 0)
                continue;
            if (!insideGrid(x + dx, y + dy))
                continue;

            int next = (y + dy) * GRID_W + x + dx;
            float stepCost = CELL;
            if (dx != 0 && dy != 0)
                stepCost = DIAGONAL_STEP;
            tryNeighbour(current, next, stepCost, goal, radius);
        }
    }
}

int findPath(Vec2 from, Vec2 to, float radius, Vec2 out[], int maxOut)
{
    if (clearPath(from, to, radius))
    {
        out[0] = to;
        return 1;
    }

    int start = nearestCell(from, radius);
    int goal = nearestCell(to, radius);
    if (start < 0 || goal < 0)
        return 0;

    resetSearch(start);
    while (true)
    {
        int current = lowestScoreOpenCell();
        if (current < 0)
            return 0;
        if (current == goal)
            return buildPath(goal, to, out, maxOut);

        isOpen[current] = false;
        closed[current] = true;
        openNeighbours(current, goal, radius);
    }
}
