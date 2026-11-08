#include "Guard.h"

#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "Character.h"
#include "Collision.h"
#include "Graphics.h"
#include "Map.h"
#include "Navigation.h"

using namespace gfx;

namespace {

constexpr int CONE_RAYS = 72;           // rays used to clip the cone against walls
constexpr float DECAY_PER_SEC = 0.24f;
constexpr float GUARD_SCALE = 1.08f;    // guards are a little bigger than detectives


// Smallest signed difference between two angles, in degrees (-180..180].
float angleDiff(float a, float b) {
    float d = std::fmod(a - b + 540.0f, 360.0f) - 180.0f;
    return d;
}

// Yellow-green (calm) -> orange (suspicious) -> red (detection imminent).
Color alertColor(float level) {
    const Color calm = hex(0xeefa7a), sus = hex(0xffa53d), red = hex(0xff4b3e);
    if (level < 0.5f) return mix(calm, sus, level / 0.5f);
    return mix(sus, red, (level - 0.5f) / 0.5f);
}

}  // namespace

Guard::Guard(const GuardConfig& cfg) : cfg_(cfg) { reset(); }

void Guard::reset() {
    pos_ = cfg_.route.empty() ? Vec2{0, 0} : cfg_.route[0].p;
    angle_ = cfg_.startAngle;
    state_ = GuardState::Patrolling;
    target_ = 0;
    stateTimer_ = 0.0f;
    detection_ = 0.0f;
    seeing_ = false;
    walkPhase_ = moveBlend_ = clock_ = 0.0f;
    path_.clear();
    pathTimer_ = 0;
}

bool Guard::canSee(Vec2 p, const Map& map) const {
    Vec2 d = p - pos_;
    float dist = d.length();
    if (dist > cfg_.range) return false;                                              // 1. distance
    if (dist > 1e-3f) {
        float toTarget = radToDeg(std::atan2(d.y, d.x));
        if (std::fabs(angleDiff(toTarget, angle_)) > cfg_.fov * 0.5f) return false;  // 2. angle
    }
    return collision::lineOfSight(pos_, p, map);                                     // 3. obstacles
}

void Guard::turnTowards(float targetDeg, float degPerSec, float dt) {
    float diff = angleDiff(targetDeg, angle_);
    float step = degPerSec * dt;
    angle_ += std::clamp(diff, -step, step);  // smooth rotation, never overshoots
}

void Guard::hearNoise(Vec2 source, float radius) {
    if (state_ == GuardState::Caught || seeing_ || (source - pos_).length() > radius) return;
    lastSeen_ = source;
    if (state_ != GuardState::Alert) stateTimer_ = 0.0f;
    state_ = GuardState::Alert;
}

void Guard::receiveReport(Vec2 lastKnown) {
    if (seeing_ || state_ == GuardState::Caught) return;
    lastSeen_ = lastKnown;
    if (state_ != GuardState::Alert) stateTimer_ = 0.8f;
    state_ = GuardState::Alert;
}

bool Guard::moveTowards(Vec2 target, float speed, float dt, const Map& map) {
    pathTimer_ -= dt;
    if (pathTimer_ <= 0 || (target - pathTarget_).length() > 45) {
        path_ = navigation::findPath(pos_, target, RADIUS, map);
        pathTarget_ = target;
        pathTimer_ = 0.8f;
    }
    while (!path_.empty() && (path_.front() - pos_).length() < 0.5f) path_.erase(path_.begin());
    if (path_.empty()) return false;
    Vec2 to = path_.front() - pos_;
    turnTowards(radToDeg(std::atan2(to.y, to.x)), 300, dt);
    Vec2 next = collision::moveCircle(pos_, to.normalized() * std::min(to.length(), speed * dt), RADIUS, map);
    bool moved = (next - pos_).length() > 0.001f;
    pos_ = next;
    return moved;
}

void Guard::arriveAtWaypoint() {
    if (cfg_.route[target_].searchTime > 0.0f) {
        state_ = GuardState::Searching;
        stateTimer_ = 0.0f;
        searchBase_ = angle_;
    } else {
        target_ = (target_ + 1) % cfg_.route.size();
    }
}

// Search keyframes over normalised time u: look left, look right, back to
// centre, then pause. Values are multiples of searchSweep.
static float searchProfile(float u) {
    static const float key[][2] = {{0.0f, 0.0f}, {0.25f, 1.0f}, {0.6f, -1.0f}, {0.8f, 0.0f}, {1.0f, 0.0f}};
    for (int i = 0; i < 4; ++i) {
        if (u <= key[i + 1][0]) {
            float k = (u - key[i][0]) / (key[i + 1][0] - key[i][0]);
            k = k * k * (3.0f - 2.0f * k);  // smoothstep easing
            return key[i][1] + (key[i + 1][1] - key[i][1]) * k;
        }
    }
    return 0.0f;
}

void Guard::update(float dt, Vec2 playerPos, const Map& map, bool playerNoisy) {
    if (state_ == GuardState::Caught || cfg_.route.empty()) return;
    clock_ += dt;

    // ---- detection: distance, angle, line of sight and duration ----
    seeing_ = canSee(playerPos, map);
    if (seeing_) {
        lastSeen_ = playerPos;
        float dist = (playerPos - pos_).length();
        float secondsToCatch = 0.7f + 1.3f * (dist / cfg_.range);  // a short chance to break sight
        detection_ += dt / (secondsToCatch * cfg_.detectionTimeScale);
    } else {
        detection_ -= dt * DECAY_PER_SEC;
    }
    detection_ = std::clamp(detection_, 0.0f, 1.0f);
    // Restore the readable stealth rule: filling the visible meter means capture.
    if (seeing_ && detection_ >= 1.0f) {
        state_ = GuardState::Caught;
        turnTowards(radToDeg(std::atan2(playerPos.y - pos_.y, playerPos.x - pos_.x)), 1e6f, 1.0f);
        moveBlend_ = 0.0f;
        return;
    }
    if (seeing_ && detection_ > 0.12f && state_ != GuardState::Alert) {
        state_ = GuardState::Alert;  // brief sighting -> suspicious
        stateTimer_ = 0.0f;
    }
    // Hearing: running footsteps nearby make the guard turn towards them.
    if (!seeing_ && playerNoisy && (playerPos - pos_).length() < cfg_.hearingRadius) {
        hearNoise(playerPos, cfg_.hearingRadius);
    }

    // ---- behaviour ----
    bool walking = false;
    switch (state_) {
        case GuardState::Patrolling: {
            Vec2 to = cfg_.route[target_].p - pos_;
            float dist = to.length();
            if (dist < 1.5f) {
                pos_ = cfg_.route[target_].p;
                arriveAtWaypoint();
                break;
            }
            walking = moveTowards(cfg_.route[target_].p, cfg_.speed, dt, map);
            break;
        }
        case GuardState::Searching: {
            float dur = cfg_.route[target_].searchTime;
            stateTimer_ += dt;
            angle_ = searchBase_ + cfg_.searchSweep * searchProfile(std::min(1.0f, stateTimer_ / dur));
            if (stateTimer_ >= dur) {
                target_ = (target_ + 1) % cfg_.route.size();
                state_ = GuardState::Patrolling;
            }
            break;
        }
        case GuardState::Alert: {
            // Stare at the spot, then walk over to investigate and scan around.
            Vec2 to = lastSeen_ - pos_;
            float dist = to.length();
            if (seeing_) {
                stateTimer_ = 0.0f;
                turnTowards(radToDeg(std::atan2(to.y, to.x)), 220.0f, dt);
                if (dist > 18.0f) {
                    walking = moveTowards(lastSeen_, cfg_.chaseSpeed, dt, map);
                }
                break;
            }
            stateTimer_ += dt;
            if (stateTimer_ < 0.8f) {
                turnTowards(radToDeg(std::atan2(to.y, to.x)), 220.0f, dt);
            } else if (dist > 14.0f && stateTimer_ < 6.0f) {
                walking = moveTowards(lastSeen_, cfg_.speed * 1.4f, dt, map);
                if (!walking) stateTimer_ = std::max(stateTimer_, 4.5f);  // blocked: just look around
            } else {
                angle_ += 110.0f * dt;  // scan the area
            }
            if (stateTimer_ > cfg_.forgetTime && detection_ < 0.3f) state_ = GuardState::Patrolling;
            break;
        }
        case GuardState::Caught: break;
    }

    moveBlend_ += ((walking ? 1.0f : 0.0f) - moveBlend_) * std::min(1.0f, dt * 10.0f);
    walkPhase_ += dt * 9.0f * moveBlend_;
}

void Guard::drawVisionCone(const Map& map) const {
    // Cast rays across the field of view; each stops at the first sight
    // blocker, so the cone is clipped exactly where vision is blocked.
    std::vector<Vec2> rim;
    rim.reserve(CONE_RAYS + 1);
    for (int i = 0; i <= CONE_RAYS; ++i) {
        float a = degToRad(angle_ - cfg_.fov * 0.5f + cfg_.fov * i / CONE_RAYS);
        Vec2 dir{std::cos(a), std::sin(a)};
        float len = collision::raycast(pos_, dir, cfg_.range, map);
        rim.push_back(pos_ + dir * len);
    }

    Color c = alertColor(detection_);
    float strength = seeing_ ? 0.66f : 0.55f;
    // Translucent fan: opaque-ish at the guard, fading towards the rim.
    glBegin(GL_TRIANGLE_FAN);
    setColor(withAlpha(c, strength));
    glVertex2f(pos_.x, pos_.y);
    for (size_t i = 0; i < rim.size(); ++i) {
        float distT = (rim[i] - pos_).length() / cfg_.range;
        setColor(withAlpha(c, strength * (1.0f - 0.45f * distT)));
        glVertex2f(rim[i].x, rim[i].y);
    }
    glEnd();

    // Bright outline along the clipped rim and the two side edges.
    setColor(withAlpha(darker(c, 0.25f), 0.9f));
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(pos_.x, pos_.y);
    for (const Vec2& p : rim) glVertex2f(p.x, p.y);
    glVertex2f(pos_.x, pos_.y);
    glEnd();

    // Centre "gaze" line.
    float a = degToRad(angle_);
    Vec2 dir{std::cos(a), std::sin(a)};
    float len = collision::raycast(pos_, dir, cfg_.range, map);
    setColor(withAlpha(c, 0.35f));
    line(pos_.x, pos_.y, pos_.x + dir.x * len, pos_.y + dir.y * len, 1.0f);
}

void Guard::draw() const {
    const CharacterStyle& style = guardStyle();
    drawCharacterShadow(pos_, GUARD_SCALE);
    // Walk cycle while moving, a slight breathing scale while standing.
    float bob = std::fabs(std::sin(walkPhase_)) * 1.2f * moveBlend_;
    float breathe = 1.0f + 0.012f * std::sin(clock_ * 2.4f) * (1.0f - moveBlend_);
    pushTransform(pos_.x, pos_.y + bob, 0.0f, GUARD_SCALE, GUARD_SCALE * breathe);
    drawCharacter(style, directionFromAngle(angle_), walkPhase_, moveBlend_);
    popTransform();
}

void Guard::drawIndicator(float t) const {
    if (detection_ <= 0.01f && !seeing_) return;
    const float x = pos_.x, y = pos_.y + 44.0f;

    // detection meter
    const float w = 30.0f, h = 5.0f;
    setColor(hex(0x1d2430, 0.75f));
    roundedRect(x - w * 0.5f - 1.5f, y - 1.5f, w + 3.0f, h + 3.0f, 3.0f);
    setColor(alertColor(detection_));
    if (detection_ > 0.02f) roundedRect(x - w * 0.5f, y, w * detection_, h, 2.0f);

    // "?" when suspicious, "!" when detection is close (popping scale animation)
    if (state_ != GuardState::Alert && state_ != GuardState::Caught && detection_ < 0.3f) return;
    bool alarm = detection_ >= 0.6f;
    float pop = 1.0f + 0.12f * std::sin(t * (alarm ? 14.0f : 6.0f));
    pushTransform(x, y + 17.0f, 0.0f, pop, pop);
    setColor(hex(0x1d2430, 0.3f));
    circle(1.5f, -1.5f, 9.0f);
    setColor(alarm ? hex(0xff4b3e) : hex(0xffd23f));
    circle(0.0f, 0.0f, 9.0f);
    setColor(hex(0xffffff));
    circleOutline(0.0f, 0.0f, 9.0f, 1.5f);
    popTransform();
    setColor(alarm ? hex(0xffffff) : hex(0x1d2430));
    textCentered(x, y + 12.0f, alarm ? "!" : "?", Font::Medium);
}

void Guard::drawSpeechBubble(const std::string& detectiveName) const {
    std::string line1 = "Ei apni ki detective " + detectiveName + "?!";
    const char* line2 = "Museum theke ber hote chay, kintu permission chara na!";
    float w = std::max(textWidth(line1, Font::Medium), textWidth(line2, Font::Medium)) + 28.0f;
    float h = 58.0f;
    float bx = std::clamp(pos_.x - w * 0.5f, 10.0f, WORLD_W - w - 10.0f);
    float by = std::min(pos_.y + 62.0f, WORLD_H - h - 10.0f);
    float tailX = std::clamp(pos_.x, bx + 16.0f, bx + w - 16.0f);

    setColor(hex(0x1d2430, 0.3f));
    roundedRect(bx + 4, by - 4, w, h, 12);
    setColor(hex(0xffffff));
    roundedRect(bx, by, w, h, 12);
    triangle({tailX - 8, by + 1}, {tailX + 8, by + 1}, {pos_.x, std::max(by - 18.0f, pos_.y + 40.0f)});
    setColor(hex(0xd63c3c));
    roundedRectOutline(bx, by, w, h, 12, 2.5f);
    textCentered(bx + w * 0.5f, by + h - 24.0f, line1, Font::Medium);
    setColor(hex(0x1d2430));
    textCentered(bx + w * 0.5f, by + 12.0f, line2, Font::Medium);
}

void Guard::drawRouteDebug() const {
    if (cfg_.route.size() > 1) {
        setColor(hex(0x2f5f8c, 0.8f));
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        for (const Waypoint& w : cfg_.route) glVertex2f(w.p.x, w.p.y);
        glEnd();
    }
    for (size_t i = 0; i < cfg_.route.size(); ++i) {
        const Waypoint& w = cfg_.route[i];
        setColor(w.searchTime > 0 ? hex(0xffa53d) : hex(0x2f5f8c));
        circle(w.p.x, w.p.y, i == target_ ? 6.0f : 4.0f);
    }
}
