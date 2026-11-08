#include "Player.h"

#include <algorithm>
#include <cmath>

#include "Character.h"
#include "Collision.h"
#include "Graphics.h"
#include "Map.h"

using namespace gfx;

void Player::reset(Vec2 spawn) {
    pos_ = spawn;
    vel_ = {0.0f, 0.0f};
    facing_ = Direction::Up;
    walkPhase_ = 0.0f;
    moveBlend_ = 0.0f;
    stamina_ = 1.0f;
    sprinting_ = exhausted_ = false;
}

void Player::update(float dt, const InputState& input, const Map& map) {
    Vec2 axis = input.axis();
    // Normalising the input vector keeps diagonal speed equal to straight speed.
    Vec2 dir = axis.normalized();

    if (axis.x != 0.0f || axis.y != 0.0f) {
        bool horizontalOk = (axis.x < 0 && facing_ == Direction::Left) || (axis.x > 0 && facing_ == Direction::Right);
        bool verticalOk = (axis.y < 0 && facing_ == Direction::Down) || (axis.y > 0 && facing_ == Direction::Up);
        if (axis.y == 0.0f || (axis.x != 0.0f && !horizontalOk && !verticalOk))
            facing_ = axis.x < 0 ? Direction::Left : Direction::Right;
        else if (axis.x == 0.0f)
            facing_ = axis.y < 0 ? Direction::Down : Direction::Up;
        // diagonal with a matching facing: keep it (avoids flicker)
    }

    // Sprint (Shift) while stamina lasts; running out forces a short rest.
    bool wantsMove = axis.x != 0.0f || axis.y != 0.0f;
    if (exhausted_ && stamina_ > 0.3f) exhausted_ = false;
    sprinting_ = input.sprint && wantsMove && !exhausted_ && stamina_ > 0.0f;
    if (sprinting_) {
        stamina_ -= STAMINA_DRAIN * dt;
        if (stamina_ <= 0.0f) {
            stamina_ = 0.0f;
            exhausted_ = true;
        }
    } else {
        stamina_ = std::min(1.0f, stamina_ + STAMINA_REGEN * dt);
    }

    // Smooth, time-based acceleration towards the target velocity.
    Vec2 target = dir * (SPEED * (sprinting_ ? SPRINT_MULT : 1.0f));
    float k = std::min(1.0f, dt * 32.0f);
    vel_ += (target - vel_) * k;
    // Release stops at once; animation may settle, but the feet never drift.
    if (!wantsMove) vel_ = {0.0f, 0.0f};

    Vec2 newPos = collision::moveCircle(pos_, vel_ * dt, RADIUS, map);
    float actualSpeed = (newPos - pos_).length() / dt;
    pos_ = newPos;

    // Animation state follows the speed actually achieved (walking into a
    // wall does not animate the legs).
    float targetBlend = actualSpeed > 8.0f ? std::min(1.0f, actualSpeed / SPEED) : 0.0f;
    moveBlend_ += (targetBlend - moveBlend_) * std::min(1.0f, dt * 12.0f);
    walkPhase_ += dt * 12.0f * std::max(moveBlend_, actualSpeed / SPEED);  // faster stride when running
    if (walkPhase_ > 2.0f * PI_F) walkPhase_ -= 2.0f * PI_F;
}

void Player::draw(float crouch) const {
    const CharacterStyle& style = studentStyle();
    const float bob = std::fabs(std::sin(walkPhase_)) * 1.3f * moveBlend_;
    drawCharacterShadow(pos_, 1.0f);  // stays on the ground (no bob)
    pushTransform(pos_.x, pos_.y + bob, 0, 1 + 0.08f * crouch, 1 - 0.35f * crouch);
    drawCharacter(style, facing_, walkPhase_, moveBlend_);
    popTransform();
}

void Player::drawCollisionDebug() const {
    setColor({0.1f, 0.9f, 0.3f, 0.25f});
    circle(pos_.x, pos_.y, RADIUS);
    setColor({0.1f, 0.7f, 0.2f, 1.0f});
    circleOutline(pos_.x, pos_.y, RADIUS, 1.5f);
}
