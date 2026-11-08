#include "Character.h"

#include <algorithm>
#include <cmath>

#include "Graphics.h"

using namespace gfx;

namespace {

// Body proportions in local units; origin = between the feet, y up.
constexpr float HIP_Y = 9.5f;
constexpr float SHOULDER_Y = 20.0f;
constexpr float HEAD_Y = 28.5f;

// A leg pivots at the hip: rotation (glRotatef) swings it forward/back,
// `lift` raises the foot during the front/back walk cycle.
void drawLeg(const CharacterStyle& S, float hipX, float deg, float lift, bool far) {
    pushTransform(hipX, HIP_Y + lift, deg);
    setColor(S.outline);
    roundedRect(-3.1f, -9.9f, 6.2f, 10.4f, 2.2f);
    setColor(far ? S.pantsFar : S.pants);
    roundedRect(-2.5f, -9.3f, 5.0f, 9.6f, 1.8f);
    setColor(S.sole);
    ellipse(0.0f, -9.6f, 3.5f, 2.2f, 12);
    setColor(far ? darker(S.shoe, 0.12f) : S.shoe);
    ellipse(0.0f, -9.1f, 3.0f, 1.8f, 12);
    popTransform();
}

// An arm pivots at the shoulder.
void drawArm(const CharacterStyle& S, float shoulderX, float deg, bool far) {
    pushTransform(shoulderX, SHOULDER_Y, deg);
    setColor(S.outline);
    roundedRect(-2.8f, -11.4f, 5.6f, 12.2f, 2.6f);
    setColor(far ? darker(S.skin, 0.12f) : S.skin);
    roundedRect(-1.9f, -10.6f, 3.8f, 6.0f, 1.7f);
    circle(0.0f, -10.4f, 2.2f, 10);
    setColor(far ? S.shirtShade : S.shirt);
    roundedRect(-2.3f, -6.0f, 4.6f, 6.6f, 2.0f);  // short sleeve
    popTransform();
}

void drawTorsoFront(const CharacterStyle& S, float w) {
    setColor(S.outline);
    roundedRect(-w * 0.5f - 0.7f, 7.3f, w + 1.4f, 15.0f, 5.0f);
    setColor(S.shirt);
    roundedRect(-w * 0.5f, 8.0f, w, 13.6f, 4.5f);
    setColor(S.shirtShade);
    roundedRect(-w * 0.5f, 8.0f, w, 3.6f, 1.8f);  // hem shading
}

void drawBelt(const CharacterStyle& S, float x0, float w, bool buckle) {
    setColor(S.belt);
    rect(x0, 9.6f, w, 2.4f);
    if (buckle) {
        setColor(S.badge);
        rect(-1.3f, 9.5f, 2.6f, 2.6f);
    }
}

void drawHeadFront(const CharacterStyle& S) {
    setColor(S.outline);
    ellipse(0.0f, HEAD_Y, 8.2f, 7.7f);
    setColor(S.skinDark);
    circle(-7.4f, HEAD_Y - 0.5f, 1.9f, 10);
    circle(7.4f, HEAD_Y - 0.5f, 1.9f, 10);
    setColor(S.skin);
    ellipse(0.0f, HEAD_Y, 7.5f, 7.0f);

    if (S.guardGear) {
        // peaked cap: crown, brim, badge
        setColor(S.cap);
        ellipse(0.0f, HEAD_Y + 4.2f, 8.4f, 4.8f);
        setColor(S.capDark);
        ellipse(0.0f, HEAD_Y + 1.2f, 8.6f, 2.2f);
        setColor(S.badge);
        circle(0.0f, HEAD_Y + 4.6f, 1.7f, 10);
        // stern eyebrows, eyes, moustache
        setColor(S.hair);
        line(-4.0f, HEAD_Y - 0.4f, -1.4f, HEAD_Y - 0.9f, 1.3f);
        line(4.0f, HEAD_Y - 0.4f, 1.4f, HEAD_Y - 0.9f, 1.3f);
        setColor(S.outline);
        ellipse(-2.6f, HEAD_Y - 2.0f, 0.95f, 1.2f, 10);
        ellipse(2.6f, HEAD_Y - 2.0f, 0.95f, 1.2f, 10);
        setColor(S.hair);
        ellipse(-1.7f, HEAD_Y - 4.3f, 2.0f, 0.95f, 10);
        ellipse(1.7f, HEAD_Y - 4.3f, 2.0f, 0.95f, 10);
        return;
    }
    setColor(S.hair);
    ellipse(0.0f, HEAD_Y + 3.2f, 7.9f, 5.0f);
    ellipse(-3.4f, HEAD_Y + 1.4f, 3.8f, 2.2f, 12);
    ellipse(2.6f, HEAD_Y + 1.8f, 4.4f, 2.0f, 12);
    setColor(S.hairHi);
    arc(0.0f, HEAD_Y + 3.0f, 5.5f, 60.0f, 130.0f, 1.5f, 8);
    setColor(S.outline);
    ellipse(-2.6f, HEAD_Y - 1.5f, 1.0f, 1.3f, 10);
    ellipse(2.6f, HEAD_Y - 1.5f, 1.0f, 1.3f, 10);
    setColor(withAlpha(hex(0xff8a80), 0.45f));
    ellipse(-4.4f, HEAD_Y - 3.6f, 1.5f, 0.9f, 10);
    ellipse(4.4f, HEAD_Y - 3.6f, 1.5f, 0.9f, 10);
    setColor(S.skinDark);
    arc(0.0f, HEAD_Y - 3.2f, 1.6f, 200.0f, 340.0f, 1.2f, 6);  // small smile
}

void drawDown(const CharacterStyle& S, float swing, float lift) {
    drawLeg(S, -3.2f, 0.0f, std::max(0.0f, lift), false);
    drawLeg(S, 3.2f, 0.0f, std::max(0.0f, -lift), false);
    drawTorsoFront(S, 14.0f);
    if (S.backpack) {  // straps over the shoulders
        setColor(S.pack);
        thickLine(-4.6f, 21.0f, -4.2f, 12.0f, 2.4f);
        thickLine(4.6f, 21.0f, 4.2f, 12.0f, 2.4f);
        setColor(S.zip);
        rect(-4.9f, 15.5f, 1.4f, 1.4f);
        rect(3.5f, 15.5f, 1.4f, 1.4f);
    }
    if (S.guardGear) {
        drawBelt(S, -7.0f, 14.0f, true);
        setColor(S.badge);  // chest badge (shield shape)
        polygon({{-5.4f, 18.6f}, {-1.8f, 18.6f}, {-1.8f, 16.2f}, {-3.6f, 14.6f}, {-5.4f, 16.2f}});
        setColor(S.shirtShade);  // pocket + epaulettes
        rect(1.8f, 15.0f, 3.6f, 3.0f);
        setColor(S.capDark);
        rect(-7.4f, 19.4f, 3.6f, 1.7f);
        rect(3.8f, 19.4f, 3.6f, 1.7f);
    }
    drawArm(S, -8.0f, -6.0f + swing * 8.0f, false);
    drawArm(S, 8.0f, 6.0f + swing * 8.0f, false);
    setColor(S.skinDark);
    rect(-2.0f, 20.0f, 4.0f, 2.5f);  // neck
    drawHeadFront(S);
}

void drawUp(const CharacterStyle& S, float swing, float lift) {
    drawLeg(S, -3.2f, 0.0f, std::max(0.0f, -lift), false);
    drawLeg(S, 3.2f, 0.0f, std::max(0.0f, lift), false);
    drawArm(S, -8.0f, -6.0f - swing * 8.0f, false);
    drawArm(S, 8.0f, 6.0f - swing * 8.0f, false);
    drawTorsoFront(S, 14.0f);
    if (S.backpack) {
        setColor(S.outline);
        roundedRect(-6.8f, 8.6f, 13.6f, 13.4f, 3.8f);
        setColor(S.pack);
        roundedRect(-6.1f, 9.3f, 12.2f, 12.0f, 3.2f);
        setColor(S.packShade);
        roundedRect(-4.3f, 10.2f, 8.6f, 5.0f, 2.0f);
        setColor(S.zip);
        line(-3.6f, 15.6f, 3.6f, 15.6f, 1.3f);
        circle(3.4f, 15.6f, 0.9f, 8);
        setColor(S.packShade);
        arc(0.0f, 21.4f, 2.4f, 0.0f, 180.0f, 1.6f, 8);  // carry handle
    }
    if (S.guardGear) {
        drawBelt(S, -7.0f, 14.0f, false);
        setColor(hex(0x22262c));  // radio on the belt
        roundedRect(3.4f, 8.2f, 3.2f, 5.2f, 0.8f);
        line(5.6f, 13.4f, 5.6f, 16.4f, 1.0f);
        setColor(S.shirtShade);  // back seam
        line(0.0f, 12.5f, 0.0f, 20.5f, 1.0f);
    }
    // back of the head
    setColor(S.outline);
    ellipse(0.0f, HEAD_Y, 8.2f, 7.7f);
    setColor(S.skinDark);
    circle(-7.4f, HEAD_Y - 0.8f, 1.8f, 10);
    circle(7.4f, HEAD_Y - 0.8f, 1.8f, 10);
    setColor(S.hair);
    ellipse(0.0f, HEAD_Y, 7.5f, 7.0f);
    if (S.guardGear) {
        setColor(S.cap);
        ellipse(0.0f, HEAD_Y + 2.4f, 8.4f, 5.6f);
        setColor(S.capDark);
        rect(-6.0f, HEAD_Y - 1.2f, 12.0f, 1.6f);  // cap band
    } else {
        setColor(S.hairHi);
        arc(0.0f, HEAD_Y + 0.5f, 5.0f, 40.0f, 140.0f, 1.6f, 8);
        arc(0.0f, HEAD_Y - 1.0f, 3.0f, 60.0f, 120.0f, 1.2f, 6);
    }
}

// Drawn facing left; the right-facing pose is the same model mirrored
// with glScalef(-1, 1) by the caller.
void drawSide(const CharacterStyle& S, float swing) {
    const float legDeg = swing * 28.0f, armDeg = swing * 32.0f;
    drawArm(S, 1.2f, armDeg, true);         // far arm (behind the body)
    drawLeg(S, 0.8f, -legDeg, 0.0f, true);  // far leg
    drawLeg(S, -0.8f, legDeg, 0.0f, false); // near leg
    if (S.backpack) {  // on the back (+x side)
        setColor(S.outline);
        roundedRect(2.8f, 8.6f, 7.6f, 13.4f, 3.2f);
        setColor(S.pack);
        roundedRect(3.4f, 9.2f, 6.4f, 12.2f, 2.6f);
        setColor(S.packShade);
        roundedRect(6.4f, 10.0f, 3.2f, 6.0f, 1.4f);
    }
    // torso (narrower in profile)
    setColor(S.outline);
    roundedRect(-6.0f, 7.3f, 10.6f, 15.0f, 4.5f);
    setColor(S.shirt);
    roundedRect(-5.3f, 8.0f, 9.2f, 13.6f, 4.0f);
    setColor(S.shirtShade);
    roundedRect(-5.3f, 8.0f, 9.2f, 3.6f, 1.6f);
    if (S.backpack) {
        setColor(S.pack);
        thickLine(-1.5f, 21.0f, 2.8f, 13.0f, 2.2f);  // strap
    }
    if (S.guardGear) {
        setColor(hex(0x22262c));  // baton on the hip
        thickLine(3.2f, 12.0f, 6.0f, 3.5f, 1.8f);
        drawBelt(S, -5.3f, 9.2f, false);
        setColor(S.badge);
        rect(-4.4f, 15.6f, 2.2f, 2.6f);
    }
    drawArm(S, -0.6f, -armDeg, false);  // near arm
    // head in profile, face towards -x
    setColor(S.outline);
    ellipse(-0.6f, HEAD_Y, 8.0f, 7.6f);
    setColor(S.skin);
    ellipse(-0.6f, HEAD_Y, 7.3f, 6.9f);
    circle(-7.6f, HEAD_Y - 1.4f, 1.3f, 8);  // nose
    setColor(S.hair);
    if (S.guardGear) {
        ellipse(2.4f, HEAD_Y + 0.4f, 5.0f, 4.6f);  // short hair at the back
        setColor(S.cap);
        ellipse(0.6f, HEAD_Y + 3.6f, 7.4f, 4.6f);
        setColor(S.capDark);
        ellipse(-6.2f, HEAD_Y + 1.6f, 4.6f, 1.6f, 12);  // brim points forward
        setColor(S.badge);
        circle(-3.0f, HEAD_Y + 4.0f, 1.4f, 8);
    } else {
        ellipse(2.2f, HEAD_Y + 2.0f, 5.8f, 5.4f);        // back of the head
        ellipse(-2.0f, HEAD_Y + 4.6f, 5.4f, 2.6f, 12);   // top + fringe
        setColor(S.hairHi);
        arc(1.6f, HEAD_Y + 2.6f, 3.8f, 50.0f, 120.0f, 1.4f, 6);
    }
    setColor(S.skin);
    ellipse(0.9f, HEAD_Y - 1.4f, 1.4f, 1.8f, 10);  // ear
    setColor(S.skinDark);
    circle(0.9f, HEAD_Y - 1.4f, 0.6f, 6);
    setColor(S.outline);
    ellipse(-4.4f, HEAD_Y - 0.8f, 0.9f, 1.3f, 10);  // eye
    if (S.guardGear) {
        setColor(S.hair);
        ellipse(-5.9f, HEAD_Y - 3.3f, 2.0f, 0.9f, 10);  // moustache
    } else {
        setColor(S.skinDark);
        line(-6.4f, HEAD_Y - 4.0f, -4.8f, HEAD_Y - 4.2f, 1.1f);  // mouth
    }
}

}  // namespace

// Student palette: warm colours so they read clearly against the
// blue-uniformed guards.
const CharacterStyle& studentStyle() {
    static const CharacterStyle style = [] {
    CharacterStyle s;
    s.skin = hex(0xc98b62);
    s.skinDark = hex(0xa36a48);
    s.hair = hex(0x2a2226);
    s.hairHi = hex(0x4d3d40);
    s.shirt = hex(0xe8694a);
    s.shirtShade = hex(0xc24f39);
    s.pants = hex(0x3e5078);
    s.pantsFar = hex(0x2f3d5e);
    s.shoe = hex(0xf5f3ee);
    s.sole = hex(0x3a3a42);
    s.outline = hex(0x2b2530);
    s.backpack = true;
    s.pack = hex(0x6a4fa3);
    s.packShade = hex(0x513b86);
    s.zip = hex(0xf2c14e);
    return s;
    }();
    return style;
}

// Security guard: blue uniform, cap, badge.
const CharacterStyle& guardStyle() {
    static const CharacterStyle style = [] {
    CharacterStyle s;
    s.skin = hex(0xb57a52);
    s.skinDark = hex(0x8f5b3a);
    s.hair = hex(0x1b1616);
    s.hairHi = hex(0x3a3030);
    s.shirt = hex(0x3f6fb5);
    s.shirtShade = hex(0x30588f);
    s.pants = hex(0x22304d);
    s.pantsFar = hex(0x19233a);
    s.shoe = hex(0x26262b);
    s.sole = hex(0x0f0f12);
    s.outline = hex(0x1a1d26);
    s.guardGear = true;
    s.cap = hex(0x203759);
    s.capDark = hex(0x111d33);
    s.badge = hex(0xf2c14e);
    s.belt = hex(0x15171c);
    return s;
    }();
    return style;
}

void drawCharacter(const CharacterStyle& s, Direction facing, float walkPhase, float moveBlend) {
    const float swing = std::sin(walkPhase) * moveBlend;
    switch (facing) {
        case Direction::Down: drawDown(s, swing, swing * 2.2f); break;
        case Direction::Up: drawUp(s, swing, swing * 2.2f); break;
        case Direction::Left: drawSide(s, swing); break;
        case Direction::Right:
            pushTransform(0.0f, 0.0f, 0.0f, -1.0f, 1.0f);  // mirror the left pose
            drawSide(s, swing);
            popTransform();
            break;
    }
}

void drawCharacterShadow(Vec2 feet, float scale) {
    softEllipse(feet.x + 1.5f * scale, feet.y + 0.5f, 10.0f * scale, 4.2f * scale, {0.08f, 0.12f, 0.06f, 0.38f},
                {0.08f, 0.12f, 0.06f, 0.0f});
}

Direction directionFromAngle(float deg) {
    float a = std::fmod(deg, 360.0f);
    if (a < 0) a += 360.0f;
    if (a < 45.0f || a >= 315.0f) return Direction::Right;
    if (a < 135.0f) return Direction::Up;
    if (a < 225.0f) return Direction::Left;
    return Direction::Down;
}
