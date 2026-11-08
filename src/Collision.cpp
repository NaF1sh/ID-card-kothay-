#include "Collision.h"

#include <algorithm>
#include <cmath>

#include "Map.h"

namespace collision {

Vec2 closestPointOnRect(Vec2 p, const Rect& r) {
    return {std::clamp(p.x, r.left(), r.right()), std::clamp(p.y, r.bottom(), r.top())};
}

bool circleVsRect(Vec2 c, float r, const Rect& rect) {
    Vec2 d = c - closestPointOnRect(c, rect);
    return dot(d, d) < r * r;
}

bool circleVsCircle(Vec2 a, float ra, Vec2 b, float rb) {
    Vec2 d = a - b;
    float rr = ra + rb;
    return dot(d, d) < rr * rr;
}

bool pushOutOfRect(Vec2& c, float r, const Rect& rect) {
    Vec2 cp = closestPointOnRect(c, rect);
    Vec2 d = c - cp;
    float dist = d.length();
    if (dist >= r) return false;
    if (dist > 1e-4f) {
        // Centre is outside the box: push along the contact normal.
        c = cp + d * (r / dist);
    } else {
        // Centre is inside the box: leave through the nearest side.
        float toLeft = c.x - rect.left(), toRight = rect.right() - c.x;
        float toBottom = c.y - rect.bottom(), toTop = rect.top() - c.y;
        float m = std::min({toLeft, toRight, toBottom, toTop});
        if (m == toLeft) c.x = rect.left() - r;
        else if (m == toRight) c.x = rect.right() + r;
        else if (m == toBottom) c.y = rect.bottom() - r;
        else c.y = rect.top() + r;
    }
    return true;
}

bool pushOutOfCircle(Vec2& c, float r, const Circle& o) {
    Vec2 d = c - o.c;
    float dist = d.length();
    float minDist = r + o.r;
    if (dist >= minDist) return false;
    Vec2 n = dist > 1e-4f ? d / dist : Vec2{0.0f, 1.0f};
    c = o.c + n * minDist;
    return true;
}

bool overlapsSolid(Vec2 c, float r, const Map& map) {
    for (const Rect& rect : map.solidRects())
        if (circleVsRect(c, r, rect)) return true;
    for (const Circle& o : map.solidCircles())
        if (circleVsCircle(c, r, o.c, o.r)) return true;
    return false;
}

Vec2 clampToWorld(Vec2 c, float r) {
    return {std::clamp(c.x, r, WORLD_W - r), std::clamp(c.y, r, WORLD_H - r)};
}

Vec2 moveCircle(Vec2 from, Vec2 delta, float r, const Map& map) {
    const float maxStep = r * 0.5f;
    int steps = std::max(1, static_cast<int>(std::ceil(delta.length() / maxStep)));
    Vec2 step = delta / static_cast<float>(steps);
    Vec2 c = from;
    for (int s = 0; s < steps; ++s) {
        c += step;
        // A few relaxation passes resolve corners where two obstacles meet.
        for (int iter = 0; iter < 4; ++iter) {
            bool moved = false;
            for (const Rect& rect : map.solidRects()) moved |= pushOutOfRect(c, r, rect);
            for (const Circle& o : map.solidCircles()) moved |= pushOutOfCircle(c, r, o);
            c = clampToWorld(c, r);
            if (!moved) break;
        }
    }
    return c;
}

namespace {
// The four edges of a rectangle as segments.
void rectEdges(const Rect& r, Vec2 out[4][2]) {
    Vec2 bl{r.left(), r.bottom()}, br{r.right(), r.bottom()}, tr{r.right(), r.top()}, tl{r.left(), r.top()};
    out[0][0] = bl; out[0][1] = br;
    out[1][0] = br; out[1][1] = tr;
    out[2][0] = tr; out[2][1] = tl;
    out[3][0] = tl; out[3][1] = bl;
}
}  // namespace

bool raySegment(Vec2 o, Vec2 d, Vec2 a, Vec2 b, float& t) {
    // Solve o + d*t = a + e*u with e = b - a, using 2D cross products.
    Vec2 e = b - a;
    float denom = cross(d, e);
    if (std::fabs(denom) < 1e-8f) return false;  // parallel
    Vec2 ao = a - o;
    float tt = cross(ao, e) / denom;
    float u = cross(ao, d) / denom;
    if (tt < 0.0f || u < 0.0f || u > 1.0f) return false;
    t = tt;
    return true;
}

bool segmentsIntersect(Vec2 p1, Vec2 p2, Vec2 q1, Vec2 q2) {
    auto orient = [](Vec2 a, Vec2 b, Vec2 c) { return cross(b - a, c - a); };
    float d1 = orient(q1, q2, p1), d2 = orient(q1, q2, p2);
    float d3 = orient(p1, p2, q1), d4 = orient(p1, p2, q2);
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) return true;
    // collinear / touching cases count as intersecting (conservative for sight)
    auto onSeg = [](Vec2 a, Vec2 b, Vec2 p) {
        return std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x) && std::min(a.y, b.y) <= p.y &&
               p.y <= std::max(a.y, b.y);
    };
    if (d1 == 0 && onSeg(q1, q2, p1)) return true;
    if (d2 == 0 && onSeg(q1, q2, p2)) return true;
    if (d3 == 0 && onSeg(p1, p2, q1)) return true;
    if (d4 == 0 && onSeg(p1, p2, q2)) return true;
    return false;
}

bool segmentVsRect(Vec2 a, Vec2 b, const Rect& r) {
    if (r.contains(a) || r.contains(b)) return true;
    Vec2 edges[4][2];
    rectEdges(r, edges);
    for (auto& e : edges)
        if (segmentsIntersect(a, b, e[0], e[1])) return true;
    return false;
}

float raycast(Vec2 origin, Vec2 dir, float maxDist, const Map& map) {
    float best = maxDist;
    Vec2 edges[4][2];
    for (const Rect& r : map.sightBlockers()) {
        // cheap reject: blocker farther than the current best hit
        Vec2 cp = closestPointOnRect(origin, r);
        if ((cp - origin).length() > best) continue;
        rectEdges(r, edges);
        for (auto& e : edges) {
            float t;
            if (raySegment(origin, dir, e[0], e[1], t) && t < best) best = t;
        }
    }
    return best;
}

bool lineOfSight(Vec2 a, Vec2 b, const Map& map) {
    for (const Rect& r : map.sightBlockers())
        if (segmentVsRect(a, b, r)) return false;
    return true;
}

}  // namespace collision
