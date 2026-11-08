// ============================================================
// collision.cpp
// Characters are circles, obstacles are rectangles or circles.
//  - circle vs rectangle / circle vs circle
//  - sliding movement that never passes through walls
//  - line of sight (segment vs rectangle) for guard vision
//  - A* path finding on a 20 x 20 grid
// ============================================================

#include "game.h"

// ============================================================
// CIRCLE VS RECTANGLE / CIRCLE
// ============================================================

Vec2 closestPointOnRect(Vec2 p, Rect r)
{
    return makeVec(clampf(p.x, r.x, r.x + r.w), clampf(p.y, r.y, r.y + r.h));
}

bool circleVsRect(Vec2 c, float r, Rect rect)
{
    Vec2 d = vsub(c, closestPointOnRect(c, rect));
    return vdot(d, d) < r * r;
}

bool circleVsCircle(Vec2 a, float ra, Vec2 b, float rb)
{
    Vec2 d = vsub(a, b);
    float rr = ra + rb;
    return vdot(d, d) < rr * rr;
}

// Push the circle out of the rectangle the shortest way.
bool pushOutOfRect(Vec2 &c, float r, Rect rect)
{
    Vec2 cp = closestPointOnRect(c, rect);
    Vec2 d = vsub(c, cp);
    float dist = vlength(d);
    if (dist >= r)
        return false;

    if (dist > 0.0001f)
    {
        // centre outside the box: push along the contact normal
        c = vadd(cp, vscale(d, r / dist));
    }
    else
    {
        // centre inside the box: leave through the nearest side
        float toLeft = c.x - rect.x;
        float toRight = rect.x + rect.w - c.x;
        float toBottom = c.y - rect.y;
        float toTop = rect.y + rect.h - c.y;
        float m = minf(minf(toLeft, toRight), minf(toBottom, toTop));
        if (m == toLeft)
            c.x = rect.x - r;
        else if (m == toRight)
            c.x = rect.x + rect.w + r;
        else if (m == toBottom)
            c.y = rect.y - r;
        else
            c.y = rect.y + rect.h + r;
    }
    return true;
}

bool pushOutOfCircle(Vec2 &c, float r, Vec2 o, float orad)
{
    Vec2 d = vsub(c, o);
    float dist = vlength(d);
    float minDist = r + orad;
    if (dist >= minDist)
        return false;
    Vec2 n = dist > 0.0001f ? vscale(d, 1.0f / dist) : makeVec(0, 1);
    c = vadd(o, vscale(n, minDist));
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
    return makeVec(clampf(c.x, r, WORLD_W - r), clampf(c.y, r, WORLD_H - r));
}

// Move by delta in small steps. After each step push the circle
// out of anything it touches, so it slides along walls.
Vec2 moveCircle(Vec2 from, Vec2 delta, float r)
{
    float maxStep = r * 0.5f;
    int steps = (int)ceilf(vlength(delta) / maxStep);
    if (steps < 1)
        steps = 1;
    Vec2 step = vscale(delta, 1.0f / steps);
    Vec2 c = from;

    for (int s = 0; s < steps; s++)
    {
        c = vadd(c, step);
        // a few passes fix corners where two obstacles meet
        for (int pass = 0; pass < 4; pass++)
        {
            bool moved = false;
            for (int i = 0; i < map.solidCount; i++)
                if (pushOutOfRect(c, r, map.solids[i]))
                    moved = true;
            for (int i = 0; i < map.circleCount; i++)
                if (pushOutOfCircle(c, r, map.circlePos[i], map.circleR[i]))
                    moved = true;
            c = clampToWorld(c, r);
            if (!moved)
                break;
        }
    }
    return c;
}

// If a point is inside an obstacle, find the nearest free point
// by checking rings of 24 directions around it.
Vec2 nearestFreeSpot(Vec2 p, float radius)
{
    if (!overlapsSolid(p, radius))
        return p;
    for (float ring = 5; ring <= 200; ring += 5)
    {
        for (int k = 0; k < 24; k++)
        {
            float a = k * (2 * PI / 24);
            Vec2 q = clampToWorld(makeVec(p.x + cosf(a) * ring, p.y + sinf(a) * ring), radius + 14);
            if (!overlapsSolid(q, radius))
                return q;
        }
    }
    return p;
}

// ============================================================
// LINE INTERSECTION (guard line of sight)
// ============================================================

// Ray o + d*t against segment a..b. Returns the distance t.
bool raySegment(Vec2 o, Vec2 d, Vec2 a, Vec2 b, float &t)
{
    Vec2 e = vsub(b, a);
    float denom = vcross(d, e);
    if (fabsf(denom) < 1e-8f)
        return false; // parallel
    Vec2 ao = vsub(a, o);
    float tt = vcross(ao, e) / denom;
    float u = vcross(ao, d) / denom;
    if (tt < 0 || u < 0 || u > 1)
        return false;
    t = tt;
    return true;
}

float orient(Vec2 a, Vec2 b, Vec2 c)
{
    return vcross(vsub(b, a), vsub(c, a));
}

bool onSegment(Vec2 a, Vec2 b, Vec2 p)
{
    return minf(a.x, b.x) <= p.x && p.x <= maxf(a.x, b.x) &&
           minf(a.y, b.y) <= p.y && p.y <= maxf(a.y, b.y);
}

// Cross product orientation test
bool segmentsIntersect(Vec2 p1, Vec2 p2, Vec2 q1, Vec2 q2)
{
    float d1 = orient(q1, q2, p1), d2 = orient(q1, q2, p2);
    float d3 = orient(p1, p2, q1), d4 = orient(p1, p2, q2);
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;
    // touching counts as blocked
    if (d1 == 0 && onSegment(q1, q2, p1))
        return true;
    if (d2 == 0 && onSegment(q1, q2, p2))
        return true;
    if (d3 == 0 && onSegment(p1, p2, q1))
        return true;
    if (d4 == 0 && onSegment(p1, p2, q2))
        return true;
    return false;
}

void rectCorners(Rect r, Vec2 c[4])
{
    c[0] = makeVec(r.x, r.y);
    c[1] = makeVec(r.x + r.w, r.y);
    c[2] = makeVec(r.x + r.w, r.y + r.h);
    c[3] = makeVec(r.x, r.y + r.h);
}

bool segmentVsRect(Vec2 a, Vec2 b, Rect r)
{
    if (rectContains(r, a) || rectContains(r, b))
        return true;
    Vec2 c[4];
    rectCorners(r, c);
    for (int i = 0; i < 4; i++)
        if (segmentsIntersect(a, b, c[i], c[(i + 1) % 4]))
            return true;
    return false;
}

// How far can a guard see along dir before a wall stops the view?
float raycast(Vec2 origin, Vec2 dir, float maxDist)
{
    float best = maxDist;
    Vec2 c[4];
    for (int i = 0; i < map.blockerCount; i++)
    {
        Rect r = map.blockers[i];
        if (vlength(vsub(closestPointOnRect(origin, r), origin)) > best)
            continue;
        rectCorners(r, c);
        for (int k = 0; k < 4; k++)
        {
            float t;
            if (raySegment(origin, dir, c[k], c[(k + 1) % 4], t) && t < best)
                best = t;
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

// ============================================================
// A* PATH FINDING
// ============================================================
// The museum is split into a 50 x 24 grid of 20 x 20 cells.
// Guards, visitors and cats use it to walk around buildings.

#define GRID_W 50
#define GRID_H 24
#define CELL 20.0f

// Is a straight walk from 'from' to 'to' free? (checked every 5 units)
bool clearPath(Vec2 from, Vec2 to, float radius)
{
    int steps = (int)(vlength(vsub(to, from)) / 5) + 1;
    for (int i = 0; i <= steps; i++)
    {
        Vec2 p = vadd(from, vscale(vsub(to, from), float(i) / steps));
        if (overlapsSolid(p, radius))
            return false;
    }
    return true;
}

Vec2 cellCenter(int id)
{
    return makeVec((id % GRID_W + 0.5f) * CELL, (id / GRID_W + 0.5f) * CELL);
}

// Nearest cell (within 3 cells) that can be reached in a straight line
int nearestCell(Vec2 p, float radius)
{
    int best = -1;
    float bestDist = 110;
    int cx = (int)(p.x / CELL), cy = (int)(p.y / CELL);
    for (int y = cy - 3; y <= cy + 3; y++)
    {
        for (int x = cx - 3; x <= cx + 3; x++)
        {
            if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H)
                continue;
            int id = y * GRID_W + x;
            float d = vlength(vsub(cellCenter(id), p));
            if (d < bestDist && clearPath(p, cellCenter(id), radius))
            {
                bestDist = d;
                best = id;
            }
        }
    }
    return best;
}

// Writes the path (ending at 'to') into out[]. Returns the number
// of points, or 0 if there is no way through.
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

    static float cost[GRID_W * GRID_H];
    static float score[GRID_W * GRID_H]; // cost + distance to goal
    static int parent[GRID_W * GRID_H];
    static bool isOpen[GRID_W * GRID_H];
    static bool closed[GRID_W * GRID_H];

    for (int i = 0; i < GRID_W * GRID_H; i++)
    {
        cost[i] = 1e9f;
        parent[i] = -1;
        isOpen[i] = false;
        closed[i] = false;
    }
    cost[start] = 0;
    score[start] = 0;
    isOpen[start] = true;

    while (true)
    {
        // pick the open cell with the lowest score
        int current = -1;
        for (int i = 0; i < GRID_W * GRID_H; i++)
            if (isOpen[i] && (current < 0 || score[i] < score[current]))
                current = i;
        if (current < 0)
            return 0; // nothing left to try

        if (current == goal)
        {
            // walk back through the parents, then reverse
            Vec2 back[GRID_W * GRID_H];
            int n = 0;
            for (int id = goal; id >= 0; id = parent[id])
                back[n++] = cellCenter(id);
            int count = 0;
            for (int i = n - 1; i >= 0 && count < maxOut - 1; i--)
                out[count++] = back[i];
            out[count++] = to;
            return count;
        }

        isOpen[current] = false;
        closed[current] = true;
        int x = current % GRID_W, y = current / GRID_W;

        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                if (dx == 0 && dy == 0)
                    continue;
                if (x + dx < 0 || x + dx >= GRID_W || y + dy < 0 || y + dy >= GRID_H)
                    continue;
                int next = (y + dy) * GRID_W + x + dx;
                // diagonal steps cost a little more than 20 * sqrt(2), so straight moves win ties
                float step = (dx != 0 && dy != 0) ? 35.35534f : CELL;
                float candidate = cost[current] + step;
                if (closed[next] || candidate >= cost[next])
                    continue;
                if (!clearPath(cellCenter(current), cellCenter(next), radius))
                    continue;
                parent[next] = current;
                cost[next] = candidate;
                score[next] = candidate + vlength(vsub(cellCenter(next), cellCenter(goal)));
                isOpen[next] = true;
            }
        }
    }
}
