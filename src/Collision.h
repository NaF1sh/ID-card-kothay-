// Collision.h - geometric collision tests and resolution.
// Characters are circles; obstacles are axis-aligned rectangles or circles.
#pragma once

#include "Types.h"

class Map;

namespace collision {

Vec2 closestPointOnRect(Vec2 p, const Rect& r);
bool circleVsRect(Vec2 c, float r, const Rect& rect);
bool circleVsCircle(Vec2 a, float ra, Vec2 b, float rb);

// Push circle (c, r) out of an obstacle along the shortest direction.
// Returns true if the circle was overlapping.
bool pushOutOfRect(Vec2& c, float r, const Rect& rect);
bool pushOutOfCircle(Vec2& c, float r, const Circle& o);

bool overlapsSolid(Vec2 c, float r, const Map& map);
Vec2 clampToWorld(Vec2 c, float r);

// Moves a circle by `delta`, sliding along obstacles and staying inside the
// world. Large moves are split into sub-steps so nothing can be tunnelled.
Vec2 moveCircle(Vec2 from, Vec2 delta, float r, const Map& map);

// ---- line intersection (used for guard line of sight) ----------------------

// Ray  o + d*t (t >= 0)  vs segment  a..b.  On hit returns true and the ray
// parameter t (distance along d if d is unit length).
bool raySegment(Vec2 o, Vec2 d, Vec2 a, Vec2 b, float& t);

// Do segments p1..p2 and q1..q2 intersect? (cross-product orientation test)
bool segmentsIntersect(Vec2 p1, Vec2 p2, Vec2 q1, Vec2 q2);
bool segmentVsRect(Vec2 a, Vec2 b, const Rect& r);

// Distance along unit direction `dir` until the first sight blocker,
// or maxDist if nothing is hit. Used to clip vision cones.
float raycast(Vec2 origin, Vec2 dir, float maxDist, const Map& map);

// True if no sight blocker (building, wall, monument) crosses a..b.
bool lineOfSight(Vec2 a, Vec2 b, const Map& map);

}  // namespace collision
