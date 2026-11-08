// Types.h - shared math / geometry types used by every module.
// Kept dependency-free (no OpenGL) so game logic can be compiled and reasoned
// about independently from rendering.
#pragma once

#include <cmath>
#include <cstdint>

// Logical world (campus) size in world units - 1 screen view per level.
constexpr float VIEW_W = 1000.0f;
constexpr float VIEW_H = 600.0f;
// Dedicated HUD bands: the world viewport never extends underneath them.
constexpr float PLAY_BOTTOM = 58.0f;
constexpr float PLAY_TOP = 536.0f;
constexpr float PLAY_H = PLAY_TOP - PLAY_BOTTOM;
constexpr float WORLD_W = VIEW_W;
constexpr float WORLD_H = PLAY_H;

constexpr float PI_F = 3.14159265358979f;
inline float degToRad(float d) { return d * PI_F / 180.0f; }
inline float radToDeg(float r) { return r * 180.0f / PI_F; }

struct Vec2 {
    float x = 0.0f, y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const {
        float l = length();
        return l > 1e-6f ? Vec2{x / l, y / l} : Vec2{0.0f, 0.0f};
    }
};

inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline float cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

// Axis-aligned rectangle, (x, y) is the bottom-left corner (y grows upward).
struct Rect {
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;

    float left() const { return x; }
    float right() const { return x + w; }
    float bottom() const { return y; }
    float top() const { return y + h; }
    Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
    bool contains(Vec2 p) const { return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h; }
    bool intersects(const Rect& o) const {
        return x < o.x + o.w && o.x < x + w && y < o.y + o.h && o.y < y + h;
    }
    Rect expanded(float m) const { return {x - m, y - m, w + 2 * m, h + 2 * m}; }
};

struct Circle {
    Vec2 c;
    float r = 0.0f;
};

struct Color {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

enum class Direction { Down, Up, Left, Right };

// Which movement directions are currently held. Filled from key-press and
// key-release callbacks, so movement never depends on keyboard auto-repeat.
struct InputState {
    bool up = false, down = false, left = false, right = false;
    bool sprint = false;  // Shift held
    Vec2 axis() const {
        return {(right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f), (up ? 1.0f : 0.0f) - (down ? 1.0f : 0.0f)};
    }
};
