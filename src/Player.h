// Player.h - the student: movement, collision and the animated character.
#pragma once

#include "Types.h"

class Map;

class Player {
public:
    static constexpr float RADIUS = 8.0f;   // collision circle (feet), not the drawing
    static constexpr float SPEED = 115.0f;  // walking speed, world units per second
    static constexpr float SPRINT_MULT = 1.7f;
    static constexpr float STAMINA_DRAIN = 0.4f;   // per second while sprinting
    static constexpr float STAMINA_REGEN = 0.22f;  // per second otherwise

    void reset(Vec2 spawn);
    void update(float dt, const InputState& input, const Map& map);
    void draw(float crouch = 0) const;
    void stop() { vel_ = {}; sprinting_ = false; }
    void drawCollisionDebug() const;

    Vec2 position() const { return pos_; }
    Direction facing() const { return facing_; }
    bool isMoving() const { return moveBlend_ > 0.05f; }
    // Sprinting is fast but noisy: nearby guards hear the footsteps.
    bool isSprinting() const { return sprinting_; }
    float stamina() const { return stamina_; }  // 0..1

private:
    Vec2 pos_;
    Vec2 vel_;
    Direction facing_ = Direction::Up;
    float walkPhase_ = 0.0f;  // radians, drives the limb swing
    float moveBlend_ = 0.0f;  // 0 = standing pose, 1 = full walk cycle
    float stamina_ = 1.0f;
    bool sprinting_ = false;
    bool exhausted_ = false;  // ran out: must recover before sprinting again
};
