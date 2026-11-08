// Guard.h - security guard: patrol/search/alert state machine, vision cone,
// line-of-sight detection and the detection meter.
#pragma once

#include <string>
#include <vector>

#include "Types.h"

class Map;

struct Waypoint {
    Vec2 p;
    float searchTime = 0.0f;  // seconds spent looking left/right here (0 = walk past)
};

struct GuardConfig {
    std::vector<Waypoint> route;  // visited in order, looping
    float speed = 55.0f;          // walking speed (world units / s)
    float fov = 80.0f;            // full cone angle in degrees
    float range = 240.0f;         // how far the guard can see
    float searchSweep = 70.0f;    // how far left/right he looks while searching
    float startAngle = -90.0f;    // initial facing (degrees, 0 = east, CCW)
    float detectionTimeScale = 1.35f;
    float chaseSpeed = 130.0f;
    float hearingRadius = 170.0f;
    float forgetTime = 7.0f;
};

enum class GuardState { Patrolling, Searching, Alert, Caught };

class Guard {
public:
    static constexpr float RADIUS = 9.0f;

    explicit Guard(const GuardConfig& cfg);
    void reset();

    // playerNoisy: the detective is sprinting (can be heard, even through walls).
    void update(float dt, Vec2 playerPos, const Map& map, bool playerNoisy = false);
    // Distance + angle + line-of-sight test (the same rules the cone shows).
    bool canSee(Vec2 p, const Map& map) const;
    void hearNoise(Vec2 source, float radius);
    void receiveReport(Vec2 lastKnown);

    GuardState state() const { return state_; }
    float detection() const { return detection_; }  // 0..1
    bool hasCaught() const { return state_ == GuardState::Caught; }
    bool isSeeing() const { return seeing_; }
    Vec2 position() const { return pos_; }
    float facingAngle() const { return angle_; }

    void drawVisionCone(const Map& map) const;  // ground layer
    void draw() const;                          // depth-sorted layer
    void drawIndicator(float t) const;          // overlay: meter + ?/!
    void drawSpeechBubble(const std::string& detectiveName) const;  // shown when caught
    void drawRouteDebug() const;                // patrol route (debug view)

private:
    void arriveAtWaypoint();
    void turnTowards(float targetDeg, float degPerSec, float dt);
    bool moveTowards(Vec2 target, float speed, float dt, const Map& map);

    GuardConfig cfg_;
    Vec2 pos_;
    float angle_ = 0.0f;
    GuardState state_ = GuardState::Patrolling;
    size_t target_ = 0;        // index of the waypoint being walked to / searched at
    float stateTimer_ = 0.0f;
    float searchBase_ = 0.0f;  // facing when a search started
    Vec2 lastSeen_;
    float detection_ = 0.0f;
    bool seeing_ = false;
    float walkPhase_ = 0.0f;
    float moveBlend_ = 0.0f;
    float clock_ = 0.0f;
    std::vector<Vec2> path_;
    Vec2 pathTarget_;
    float pathTimer_ = 0;
};
