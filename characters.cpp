// ============================================================
// characters.cpp
// One jointed body model (legs and arms rotate at hip and
// shoulder) used for the detective, the guards, the visitors
// and the six named characters. Also the museum cats.
// ============================================================

#include "game.h"

// ============================================================
// CHARACTER COLOURS
// ============================================================

struct CharacterStyle
{
    Color skin, skinDark, hair, hairHi;
    Color shirt, shirtShade, pants, pantsFar, shoe, sole, outline;
    bool backpack;
    Color pack, packShade, zip;
    bool guardGear; // cap, badge, belt, moustache, radio
    Color cap, capDark, badge, belt;
};

CharacterStyle cs; // the style used by drawCharacter()

void useStyle(int id)
{
    if (id == STYLE_GUARD)
    {
        cs.skin = hexColor(0xb57a52);
        cs.skinDark = hexColor(0x8f5b3a);
        cs.hair = hexColor(0x1b1616);
        cs.hairHi = hexColor(0x3a3030);
        cs.shirt = hexColor(0x3f6fb5);
        cs.shirtShade = hexColor(0x30588f);
        cs.pants = hexColor(0x22304d);
        cs.pantsFar = hexColor(0x19233a);
        cs.shoe = hexColor(0x26262b);
        cs.sole = hexColor(0x0f0f12);
        cs.outline = hexColor(0x1a1d26);
        cs.backpack = false;
        cs.guardGear = true;
        cs.cap = hexColor(0x203759);
        cs.capDark = hexColor(0x111d33);
        cs.badge = hexColor(0xf2c14e);
        cs.belt = hexColor(0x15171c);
        return;
    }
    // detective (and the base for visitors and NPCs)
    cs.skin = hexColor(0xc98b62);
    cs.skinDark = hexColor(0xa36a48);
    cs.hair = hexColor(0x2a2226);
    cs.hairHi = hexColor(0x4d3d40);
    cs.shirt = hexColor(0xe8694a);
    cs.shirtShade = hexColor(0xc24f39);
    cs.pants = hexColor(0x3e5078);
    cs.pantsFar = hexColor(0x2f3d5e);
    cs.shoe = hexColor(0xf5f3ee);
    cs.sole = hexColor(0x3a3a42);
    cs.outline = hexColor(0x2b2530);
    cs.backpack = true;
    cs.pack = hexColor(0x6a4fa3);
    cs.packShade = hexColor(0x513b86);
    cs.zip = hexColor(0xf2c14e);
    cs.guardGear = false;
}

void setShirt(unsigned int rgb)
{
    cs.shirt = hexColor(rgb);
    cs.shirtShade = darker(cs.shirt, 0.2f);
}

void setBackpack(bool on)
{
    cs.backpack = on;
}

// ============================================================
// BODY PARTS
// ============================================================
// Local units: the origin is between the feet, y goes up.

#define HIP_Y 9.5f
#define SHOULDER_Y 20.0f
#define HEAD_Y 28.5f

// A leg rotates at the hip. 'lift' raises the foot.
void drawLeg(float hipX, float deg, float lift, bool far)
{
    glPushMatrix();
    glTranslatef(hipX, HIP_Y + lift, 0);
    glRotatef(deg, 0, 0, 1);
    setColor(cs.outline);
    DrawRoundRect(-3.1f, -9.9f, 6.2f, 10.4f, 2.2f);
    setColor(far ? cs.pantsFar : cs.pants);
    DrawRoundRect(-2.5f, -9.3f, 5.0f, 9.6f, 1.8f);
    setColor(cs.sole);
    DrawCircle(0, -9.6f, 3.5f, 2.2f, 12);
    setColor(far ? darker(cs.shoe, 0.12f) : cs.shoe);
    DrawCircle(0, -9.1f, 3.0f, 1.8f, 12);
    glPopMatrix();
}

// An arm rotates at the shoulder.
void drawArm(float shoulderX, float deg, bool far)
{
    glPushMatrix();
    glTranslatef(shoulderX, SHOULDER_Y, 0);
    glRotatef(deg, 0, 0, 1);
    setColor(cs.outline);
    DrawRoundRect(-2.8f, -11.4f, 5.6f, 12.2f, 2.6f);
    setColor(far ? darker(cs.skin, 0.12f) : cs.skin);
    DrawRoundRect(-1.9f, -10.6f, 3.8f, 6.0f, 1.7f);
    DrawCircle(0, -10.4f, 2.2f, 2.2f, 10);
    setColor(far ? cs.shirtShade : cs.shirt);
    DrawRoundRect(-2.3f, -6.0f, 4.6f, 6.6f, 2.0f); // short sleeve
    glPopMatrix();
}

void drawTorsoFront(float w)
{
    setColor(cs.outline);
    DrawRoundRect(-w * 0.5f - 0.7f, 7.3f, w + 1.4f, 15.0f, 5.0f);
    setColor(cs.shirt);
    DrawRoundRect(-w * 0.5f, 8.0f, w, 13.6f, 4.5f);
    setColor(cs.shirtShade);
    DrawRoundRect(-w * 0.5f, 8.0f, w, 3.6f, 1.8f);
}

void drawBelt(float x0, float w, bool buckle)
{
    setColor(cs.belt);
    DrawRect(x0, 9.6f, w, 2.4f);
    if (buckle)
    {
        setColor(cs.badge);
        DrawRect(-1.3f, 9.5f, 2.6f, 2.6f);
    }
}

void drawHeadFront()
{
    setColor(cs.outline);
    DrawCircle(0, HEAD_Y, 8.2f, 7.7f, 0);
    setColor(cs.skinDark);
    DrawCircle(-7.4f, HEAD_Y - 0.5f, 1.9f, 1.9f, 10); // ears
    DrawCircle(7.4f, HEAD_Y - 0.5f, 1.9f, 1.9f, 10);
    setColor(cs.skin);
    DrawCircle(0, HEAD_Y, 7.5f, 7.0f, 0);

    if (cs.guardGear)
    {
        // cap with badge
        setColor(cs.cap);
        DrawCircle(0, HEAD_Y + 4.2f, 8.4f, 4.8f, 0);
        setColor(cs.capDark);
        DrawCircle(0, HEAD_Y + 1.2f, 8.6f, 2.2f, 0);
        setColor(cs.badge);
        DrawCircle(0, HEAD_Y + 4.6f, 1.7f, 1.7f, 10);
        // stern eyebrows, eyes, moustache
        setColor(cs.hair);
        DrawLine(-4.0f, HEAD_Y - 0.4f, -1.4f, HEAD_Y - 0.9f, 1.3f);
        DrawLine(4.0f, HEAD_Y - 0.4f, 1.4f, HEAD_Y - 0.9f, 1.3f);
        setColor(cs.outline);
        DrawCircle(-2.6f, HEAD_Y - 2.0f, 0.95f, 1.2f, 10);
        DrawCircle(2.6f, HEAD_Y - 2.0f, 0.95f, 1.2f, 10);
        setColor(cs.hair);
        DrawCircle(-1.7f, HEAD_Y - 4.3f, 2.0f, 0.95f, 10);
        DrawCircle(1.7f, HEAD_Y - 4.3f, 2.0f, 0.95f, 10);
        return;
    }
    // hair
    setColor(cs.hair);
    DrawCircle(0, HEAD_Y + 3.2f, 7.9f, 5.0f, 0);
    DrawCircle(-3.4f, HEAD_Y + 1.4f, 3.8f, 2.2f, 12);
    DrawCircle(2.6f, HEAD_Y + 1.8f, 4.4f, 2.0f, 12);
    setColor(cs.hairHi);
    DrawArc(0, HEAD_Y + 3.0f, 5.5f, 60, 130, 1.5f);
    // eyes, cheeks, smile
    setColor(cs.outline);
    DrawCircle(-2.6f, HEAD_Y - 1.5f, 1.0f, 1.3f, 10);
    DrawCircle(2.6f, HEAD_Y - 1.5f, 1.0f, 1.3f, 10);
    setHex(0xff8a80, 0.45f);
    DrawCircle(-4.4f, HEAD_Y - 3.6f, 1.5f, 0.9f, 10);
    DrawCircle(4.4f, HEAD_Y - 3.6f, 1.5f, 0.9f, 10);
    setColor(cs.skinDark);
    DrawArc(0, HEAD_Y - 3.2f, 1.6f, 200, 340, 1.2f);
}

// ============================================================
// FACING DOWN (towards the viewer)
// ============================================================

void drawDown(float swing, float lift)
{
    drawLeg(-3.2f, 0, maxf(0, lift), false);
    drawLeg(3.2f, 0, maxf(0, -lift), false);
    drawTorsoFront(14);
    if (cs.backpack)
    {
        // straps over the shoulders
        setColor(cs.pack);
        DrawThickLine(-4.6f, 21.0f, -4.2f, 12.0f, 2.4f);
        DrawThickLine(4.6f, 21.0f, 4.2f, 12.0f, 2.4f);
        setColor(cs.zip);
        DrawRect(-4.9f, 15.5f, 1.4f, 1.4f);
        DrawRect(3.5f, 15.5f, 1.4f, 1.4f);
    }
    if (cs.guardGear)
    {
        drawBelt(-7, 14, true);
        // shield badge
        setColor(cs.badge);
        glBegin(GL_POLYGON);
        glVertex2f(-5.4f, 18.6f);
        glVertex2f(-1.8f, 18.6f);
        glVertex2f(-1.8f, 16.2f);
        glVertex2f(-3.6f, 14.6f);
        glVertex2f(-5.4f, 16.2f);
        glEnd();
        // pocket and shoulder marks
        setColor(cs.shirtShade);
        DrawRect(1.8f, 15.0f, 3.6f, 3.0f);
        setColor(cs.capDark);
        DrawRect(-7.4f, 19.4f, 3.6f, 1.7f);
        DrawRect(3.8f, 19.4f, 3.6f, 1.7f);
    }
    drawArm(-8, -6 + swing * 8, false);
    drawArm(8, 6 + swing * 8, false);
    setColor(cs.skinDark);
    DrawRect(-2, 20, 4, 2.5f); // neck
    drawHeadFront();
}

// ============================================================
// FACING UP (back to the viewer)
// ============================================================

void drawUp(float swing, float lift)
{
    drawLeg(-3.2f, 0, maxf(0, -lift), false);
    drawLeg(3.2f, 0, maxf(0, lift), false);
    drawArm(-8, -6 - swing * 8, false);
    drawArm(8, 6 - swing * 8, false);
    drawTorsoFront(14);
    if (cs.backpack)
    {
        setColor(cs.outline);
        DrawRoundRect(-6.8f, 8.6f, 13.6f, 13.4f, 3.8f);
        setColor(cs.pack);
        DrawRoundRect(-6.1f, 9.3f, 12.2f, 12.0f, 3.2f);
        setColor(cs.packShade);
        DrawRoundRect(-4.3f, 10.2f, 8.6f, 5.0f, 2.0f);
        setColor(cs.zip);
        DrawLine(-3.6f, 15.6f, 3.6f, 15.6f, 1.3f);
        DrawCircle(3.4f, 15.6f, 0.9f, 0.9f, 8);
        setColor(cs.packShade);
        DrawArc(0, 21.4f, 2.4f, 0, 180, 1.6f); // handle
    }
    if (cs.guardGear)
    {
        drawBelt(-7, 14, false);
        setHex(0x22262c); // radio on the belt
        DrawRoundRect(3.4f, 8.2f, 3.2f, 5.2f, 0.8f);
        DrawLine(5.6f, 13.4f, 5.6f, 16.4f, 1);
        setColor(cs.shirtShade);
        DrawLine(0, 12.5f, 0, 20.5f, 1);
    }
    // back of the head
    setColor(cs.outline);
    DrawCircle(0, HEAD_Y, 8.2f, 7.7f, 0);
    setColor(cs.skinDark);
    DrawCircle(-7.4f, HEAD_Y - 0.8f, 1.8f, 1.8f, 10);
    DrawCircle(7.4f, HEAD_Y - 0.8f, 1.8f, 1.8f, 10);
    setColor(cs.hair);
    DrawCircle(0, HEAD_Y, 7.5f, 7.0f, 0);
    if (cs.guardGear)
    {
        setColor(cs.cap);
        DrawCircle(0, HEAD_Y + 2.4f, 8.4f, 5.6f, 0);
        setColor(cs.capDark);
        DrawRect(-6, HEAD_Y - 1.2f, 12, 1.6f);
    }
    else
    {
        setColor(cs.hairHi);
        DrawArc(0, HEAD_Y + 0.5f, 5.0f, 40, 140, 1.6f);
        DrawArc(0, HEAD_Y - 1.0f, 3.0f, 60, 120, 1.2f);
    }
}

// ============================================================
// FACING LEFT (right = the same drawing mirrored with glScalef)
// ============================================================

void drawSide(float swing)
{
    float legDeg = swing * 28, armDeg = swing * 32;
    drawArm(1.2f, armDeg, true);        // far arm
    drawLeg(0.8f, -legDeg, 0, true);    // far leg
    drawLeg(-0.8f, legDeg, 0, false);   // near leg
    if (cs.backpack)
    {
        setColor(cs.outline);
        DrawRoundRect(2.8f, 8.6f, 7.6f, 13.4f, 3.2f);
        setColor(cs.pack);
        DrawRoundRect(3.4f, 9.2f, 6.4f, 12.2f, 2.6f);
        setColor(cs.packShade);
        DrawRoundRect(6.4f, 10.0f, 3.2f, 6.0f, 1.4f);
    }
    // body in profile
    setColor(cs.outline);
    DrawRoundRect(-6.0f, 7.3f, 10.6f, 15.0f, 4.5f);
    setColor(cs.shirt);
    DrawRoundRect(-5.3f, 8.0f, 9.2f, 13.6f, 4.0f);
    setColor(cs.shirtShade);
    DrawRoundRect(-5.3f, 8.0f, 9.2f, 3.6f, 1.6f);
    if (cs.backpack)
    {
        setColor(cs.pack);
        DrawThickLine(-1.5f, 21.0f, 2.8f, 13.0f, 2.2f);
    }
    if (cs.guardGear)
    {
        setHex(0x22262c); // baton
        DrawThickLine(3.2f, 12.0f, 6.0f, 3.5f, 1.8f);
        drawBelt(-5.3f, 9.2f, false);
        setColor(cs.badge);
        DrawRect(-4.4f, 15.6f, 2.2f, 2.6f);
    }
    drawArm(-0.6f, -armDeg, false); // near arm

    // head, face towards -x
    setColor(cs.outline);
    DrawCircle(-0.6f, HEAD_Y, 8.0f, 7.6f, 0);
    setColor(cs.skin);
    DrawCircle(-0.6f, HEAD_Y, 7.3f, 6.9f, 0);
    DrawCircle(-7.6f, HEAD_Y - 1.4f, 1.3f, 1.3f, 8); // nose
    setColor(cs.hair);
    if (cs.guardGear)
    {
        DrawCircle(2.4f, HEAD_Y + 0.4f, 5.0f, 4.6f, 0);
        setColor(cs.cap);
        DrawCircle(0.6f, HEAD_Y + 3.6f, 7.4f, 4.6f, 0);
        setColor(cs.capDark);
        DrawCircle(-6.2f, HEAD_Y + 1.6f, 4.6f, 1.6f, 12); // brim
        setColor(cs.badge);
        DrawCircle(-3.0f, HEAD_Y + 4.0f, 1.4f, 1.4f, 8);
    }
    else
    {
        DrawCircle(2.2f, HEAD_Y + 2.0f, 5.8f, 5.4f, 0);
        DrawCircle(-2.0f, HEAD_Y + 4.6f, 5.4f, 2.6f, 12);
        setColor(cs.hairHi);
        DrawArc(1.6f, HEAD_Y + 2.6f, 3.8f, 50, 120, 1.4f);
    }
    setColor(cs.skin);
    DrawCircle(0.9f, HEAD_Y - 1.4f, 1.4f, 1.8f, 10); // ear
    setColor(cs.skinDark);
    DrawCircle(0.9f, HEAD_Y - 1.4f, 0.6f, 0.6f, 6);
    setColor(cs.outline);
    DrawCircle(-4.4f, HEAD_Y - 0.8f, 0.9f, 1.3f, 10); // eye
    if (cs.guardGear)
    {
        setColor(cs.hair);
        DrawCircle(-5.9f, HEAD_Y - 3.3f, 2.0f, 0.9f, 10); // moustache
    }
    else
    {
        setColor(cs.skinDark);
        DrawLine(-6.4f, HEAD_Y - 4.0f, -4.8f, HEAD_Y - 4.2f, 1.1f); // mouth
    }
}

// Draws the current style with the feet at (0, 0)
void drawCharacter(int facing, float walkPhase, float moveBlend)
{
    float swing = sinf(walkPhase) * moveBlend;
    switch (facing)
    {
    case DIR_DOWN:
        drawDown(swing, swing * 2.2f);
        break;
    case DIR_UP:
        drawUp(swing, swing * 2.2f);
        break;
    case DIR_LEFT:
        drawSide(swing);
        break;
    case DIR_RIGHT:
        glPushMatrix();
        glScalef(-1, 1, 1); // mirror the left pose
        drawSide(swing);
        glPopMatrix();
        break;
    }
}

void drawCharacterShadow(Vec2 feet, float scale)
{
    Color inner = {0.08f, 0.12f, 0.06f, 0.38f};
    Color outer = {0.08f, 0.12f, 0.06f, 0.0f};
    DrawSoftCircle(feet.x + 1.5f * scale, feet.y + 0.5f, 10 * scale, 4.2f * scale, inner, outer);
}

// Angle in degrees (0 = right, counter-clockwise) -> one of 4 poses
int directionFromAngle(float deg)
{
    float a = fmodf(deg, 360);
    if (a < 0)
        a += 360;
    if (a < 45 || a >= 315)
        return DIR_RIGHT;
    if (a < 135)
        return DIR_UP;
    if (a < 225)
        return DIR_LEFT;
    return DIR_DOWN;
}

// ============================================================
// NAMED CHARACTERS (NPCs) AND THEIR LINES
// ============================================================

void setNpc(int i, float x, float y, const char *name, const char *role, int kind)
{
    npcs[i].position = makeVec(x, y);
    npcs[i].name = name;
    npcs[i].role = role;
    npcs[i].kind = kind;
    npcs[i].lineCount = 0;
    npcs[i].visits = 0;
    npcs[i].lastTask[0] = '\0';
}

void addLine(int i, const char *text)
{
    npcs[i].lines[npcs[i].lineCount++] = text;
}

void setupNpcs()
{
    setNpc(0, 285, 325, "Rafi", "THE CONSULTANT", CONSULTANT);
    addLine(0, "Wait... where's your ticket? Please tell me this is a practice run.");
    addLine(0, "Okay. Breathe. See those evidence stands? Walk up and press F to collect each evidence.");
    addLine(0, "Bring both records to the entrance desk. That gets your case file started.");
    addLine(0, "And tomorrow? Buy a ticket. I cannot punch a hole in your face.");
    addLine(0, "I saved you a spot on the tour. Front row. You're welcome.");
    addLine(0, "You look worried. A brief sighting is your warning. Sprint for cover before the meter fills!");
    addLine(0, "Get behind a wall and turn another corner. They'll lose track of you.");

    setNpc(1, 410, 140, "Bappi", "FORMER AGENT", AGENT);
    addLine(1, "Bhai! Quick warm-up before the case? ...Why are you looking at security?");
    addLine(1, "No ticket? Right. Today we practice the most important skill: getting away.");
    addLine(1, "Save your sprint for a tight spot. Walking lets your stamina come back.");
    addLine(1, "If anyone asks, you were training with me. My case record: still twelve percent.");
    addLine(1, "We need a lead investigator. You already have the running-away part down.");
    addLine(1, "Point the mouse away from your route, then left-click. The clink draws nearby guards.");
    addLine(1, "Take the gap while they investigate. That's what I call an assist!");

    setNpc(2, 180, 325, "Mintu Mama", "GALLERY OWNER", SELLER);
    addLine(2, "Cha lagbe? You look like someone who has forgotten something important.");
    addLine(2, "Your ticket? Again? Sit. The tea is hot and the gossip is hotter.");
    addLine(2, "You get two distractions each attempt. Save one for a guard near an evidence stand.");
    addLine(2, "Tea first, panic later. The clock waits while we talk.");
    addLine(2, "One singara for you. One for the cat. Don't mix up the orders.");
    addLine(2, "Security asked if I'd seen a detective without a ticket.");
    addLine(2, "I told them I see detectives without sleep, money, and hope. Be more specific.");

    setNpc(3, 360, 110, "Nila", "LAKESIDE POET", POET);
    addLine(3, "Shh. The duck is thinking. Or looking for a singara. Hard to tell.");
    addLine(3, "I come here when the galleries get too loud. You can stay a moment.");
    addLine(3, "Careful by the water: guards can see across the lake. A solid wall is better cover.");
    addLine(3, "There. Practical advice. Please don't tell anyone; I have a poetic reputation.");
    addLine(3, "I wrote a poem about deadlines. I'll finish it tomorrow.");
    addLine(3, "Get close to a cat and press F for a quick scratch behind the ears.");
    addLine(3, "A slow blink means trust. At least, that's what I tell myself.");

    setNpc(4, 675, 325, "Dr. Nap", "SLEEPY CURATOR", CURATOR);
    addLine(4, "Hm? Yes, yes. Excellent question. What was the question?");
    addLine(4, "Oh, your ticket. I left mine in yesterday's coat. We shall call this field research.");
    addLine(4, "Take your time reading. The alarm and the guards wait during a conversation.");
    addLine(4, "Now, if anyone asks, I was thinking with my eyes closed.");
    addLine(4, "Surprise inspection! Where is your museum ticket?");
    addLine(4, "That expression earns partial credit.");
    addLine(4, "Go on. Collect your evidence and close the case. I haven't written the report yet.");

    setNpc(5, 190, 270, "Pixel", "NIGHT-SHIFT GAMER", GAMER);
    addLine(5, "Hey! You look like you just walked into a boss fight without reading the controls.");
    addLine(5, "Good news: a yellow question mark is a warning. You still have time to move.");
    addLine(5, "Do not let the detection meter fill. Break sight and take another turn.");
    addLine(5, "Bad news: museum Wi-Fi is still the actual final boss.");
    addLine(5, "I found a Wi-Fi signal. It disappeared when I moved one step.");
    addLine(5, "Finish all four levels and your name goes on the scoreboard.");
    addLine(5, "First, just close the case. Speedruns can wait until you know the museum.");
}

// Draws NPC i standing at (x, y), with the item that shows their job
void drawNpc(int i, float x, float y, float t)
{
    int kind = npcs[i].kind;
    unsigned int shirts[6] = {0xe8d8b5, 0xc34442, 0xf2b95c, 0x9e77b5, 0xd7e0e1, 0x465373};
    unsigned int hair[6] = {0x282632, 0x37251c, 0x65514a, 0x332c40, 0xb9bcc0, 0x253040};
    unsigned int skin[6] = {0xc98b62, 0xa96e49, 0xb78057, 0xcd9776, 0xd3a180, 0xbb8059};

    useStyle(STYLE_DETECTIVE);
    setShirt(shirts[kind]);
    cs.hair = hexColor(hair[kind]);
    cs.hairHi = lighter(cs.hair, 0.15f);
    cs.skin = hexColor(skin[kind]);
    cs.skinDark = darker(cs.skin, 0.22f);
    cs.backpack = (kind == CONSULTANT || kind == GAMER);

    drawCharacterShadow(makeVec(x, y), 1.05f);
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(1.15f, 1.15f, 1);
    drawCharacter(DIR_DOWN, 0, 0);

    if (kind == CONSULTANT)
    {
        // glasses and an open book
        setHex(0x242a38);
        DrawCircleOutline(-4, 29, 3.3f, 3.3f, 1.5f);
        DrawCircleOutline(4, 29, 3.3f, 3.3f, 1.5f);
        DrawLine(-1, 29, 1, 29, 1);
        float bob = sinf(t * 2) * 0.6f;
        setHex(0x315d91);
        DrawRect(-10, 12 + bob, 20, 13);
        setHex(0xfff5d8);
        DrawRect(-8, 14 + bob, 16, 10);
        setHex(0x8e8b7c);
        DrawLine(0, 14 + bob, 0, 24 + bob, 1);
        for (int k = 0; k < 3; k++)
        {
            DrawLine(-6, 17 + k * 2 + bob, -2, 17 + k * 2 + bob, 1);
            DrawLine(2, 17 + k * 2 + bob, 6, 17 + k * 2 + bob, 1);
        }
    }
    else if (kind == AGENT)
    {
        // jersey number and a football
        setHex(0xffffff);
        DrawTextCentered(0, 16, "10", FONT_SMALL);
        circle(15, 4, 7);
        setHex(0x26303e);
        circle(15, 4, 2.8f);
        for (int k = 0; k < 5; k++)
        {
            float a = k * 2 * PI / 5;
            circle(15 + cosf(a) * 5.5f, 4 + sinf(a) * 5.5f, 1.5f);
        }
    }
    else if (kind == SELLER)
    {
        // apron and a steaming tea cup
        setHex(0xfaf1df);
        DrawRect(-6, 9, 12, 12);
        DrawRect(8, 16, 6, 7);
        setHex(0xa3764b);
        DrawCircle(11, 23, 3, 1.4f, 0);
        setHex(0x5e4034);
        DrawCircle(-1.7f, 24, 2, 1, 0);
        DrawCircle(1.7f, 24, 2, 1, 0);
        setHex(0xffffff, 0.65f);
        DrawArc(11 + sinf(t * 2), 28, 3, 70, 220, 1);
        DrawArc(12 - sinf(t * 2), 33, 2, -90, 70, 1);
    }
    else if (kind == CURATOR)
    {
        // glasses, tie, bag and "Z z"
        setHex(0x626878);
        DrawCircleOutline(-3.5f, 27, 2.8f, 2.8f, 1);
        DrawCircleOutline(3.5f, 27, 2.8f, 2.8f, 1);
        DrawLine(-1, 27, 1, 27, 1);
        setHex(0x8d5261);
        DrawTriangle(-2, 20, 2, 20, 0, 11);
        setHex(0x735c46);
        DrawRoundRect(9, 4, 10, 10, 2);
        DrawArc(14, 14, 3, 0, 180, 1);
        setHex(0x1d2430);
        DrawText(10, 42 + sinf(t * 2) * 2, "Z z", FONT_SMALL);
    }
    else if (kind == GAMER)
    {
        // headphones and a game controller
        setHex(0x38b7c3);
        DrawArc(0, 30, 9, 0, 180, 3);
        DrawRect(-10, 26, 4, 8);
        DrawRect(6, 26, 4, 8);
        setHex(0x253040);
        DrawRoundRect(-7, 11, 14, 7, 2);
        setHex(0x83f7ef);
        DrawRect(-3, 13, 6, 3);
        setHex(0xe7bc6f);
        circle(-5, 14, 1);
        circle(5, 14, 1);
    }
    else
    {
        // poet: long hair, pencil and notebook
        setHex(0x523c65);
        DrawCircle(-7, 23, 3, 6, 0);
        DrawCircle(7, 23, 3, 6, 0);
        setHex(0xe8bf77);
        DrawThickLine(-5, 21, 5, 19, 3);
        DrawRect(3, 10, 3, 10);
        setHex(0x655681);
        DrawRect(7, 12, 9, 13);
        setHex(0xf8e7cd);
        DrawRect(8, 14, 7, 10);
        setHex(0x9b8b7a);
        DrawLine(10, 21, 14, 21, 1);
        DrawLine(10, 18, 14, 18, 1);
    }
    glPopMatrix();
}

// ============================================================
// CATS
// ============================================================

void drawCat(CatPose pose, float t, int index)
{
    unsigned int coats[4] = {0xb98c59, 0x62656b, 0xd9cbb4, 0x977965};
    Color fur = hexColor(coats[index % 4]);
    Color shade = darker(fur, 0.22f);
    Color cream = hexColor(0xece0ca);

    bool side = fabsf(pose.direction.x) > fabsf(pose.direction.y) * 0.7f;
    bool back = !side && pose.direction.y > 0 && !pose.happy;
    float stride = pose.moving ? sinf(t * 10 + index) : 0;
    float blink = fmodf(t + index * 1.3f, 5.2f);

    drawCharacterShadow(pose.position, 0.58f);
    glPushMatrix();
    glTranslatef(pose.position.x, pose.position.y, 0);
    glScalef(side && pose.direction.x < 0 ? -0.9f : 0.9f, 0.9f, 1);

    // tail
    float sway = sinf(t * (pose.happy ? 4 : 1.8f) + index) * 2;
    setColor(shade);
    DrawArc(side ? -10 : -6, 10, 6, 90 + sway * 4, 280, 2.5f);
    setColor(fur);
    DrawArc(side ? -10 : -6, 10, 5.5f, 95 + sway * 4, 270, 1.5f);

    // four legs
    for (int i = 0; i < 4; i++)
    {
        float x = side ? -8 + i * 5.0f : (i % 2 ? 3.5f : -3.5f);
        float y = (!side && i < 2) ? 5 : 0;
        float lift = pose.moving ? maxf(0, stride * (i % 2 ? -1 : 1)) * 2 : 0;
        setColor(i < 2 ? shade : fur);
        DrawRoundRect(x - 1.3f, y + lift, 2.6f, 7, 1);
        setColor(cream);
        DrawCircle(x + 0.5f, y + lift + 1, 1.8f, 1.1f, 10);
    }

    // body
    setColor(shade);
    DrawCircle(0, 8, side ? 11 : 6, side ? 5.5f : 8, 0);
    setColor(fur);
    DrawCircle(0, 8.7f, side ? 10.5f : 5.5f, side ? 5 : 7.5f, 0);
    if (index != 1)
    {
        setColor(shade); // stripes
        for (int i = -1; i <= 1; i++)
            DrawLine(i * 4 - 2, 12, i * 4 - 3, 9, 1.3f);
    }

    // head and ears (an ear twitches now and then)
    float hx = side ? 9 : 0;
    float hy = (back ? 17 : 14) + fabsf(stride) * 0.5f;
    float ear = fmodf(t + index, 7) < 0.18f ? 1 : 0;
    setColor(shade);
    DrawTriangle(hx - 5.5f, hy + 2, hx - 5 + ear, hy + 8, hx - 0.5f, hy + 4);
    DrawTriangle(hx + 0.5f, hy + 4, hx + 5, hy + 8, hx + 5.5f, hy + 2);
    DrawCircle(hx, hy, 6, 5.5f, 0);
    setColor(fur);
    DrawCircle(hx, hy + 0.4f, 5.6f, 5, 0);
    setHex(0xc59385);
    DrawTriangle(hx - 4.5f, hy + 4, hx - 4 + ear, hy + 6.5f, hx - 2, hy + 4);
    DrawTriangle(hx + 2, hy + 4, hx + 4, hy + 6.5f, hx + 4.5f, hy + 4);

    if (!back)
    {
        // face: muzzle, eyes (closed when happy or blinking), whiskers, nose
        setColor(cream);
        DrawCircle(hx, hy - 2, 3.8f, 2.2f, 0);
        for (int sign = -1; sign <= 1; sign += 2)
        {
            setHex(0x393932);
            if (pose.happy || blink < 0.15f)
                DrawLine(hx + sign * 2 - 1, hy + 1, hx + sign * 2 + 1, hy + 1.3f, 1);
            else
                DrawCircle(hx + sign * 2.4f, hy + 1, 0.7f, 1.2f, 10);
            setHex(0x786b5c);
            DrawLine(hx + sign * 3, hy - 1.8f, hx + sign * 7, hy - 1, 0.8f);
        }
        setHex(0x986c65);
        DrawTriangle(hx - 1, hy - 1, hx + 1, hy - 1, hx, hy - 2.2f);
    }
    if (pose.happy)
    {
        // little heart
        float y = 28 + sinf(t * 3);
        setHex(0xc58a7f, 0.85f);
        circle(-1.2f, y, 1.8f);
        circle(1.2f, y, 1.8f);
        DrawTriangle(-3, y, 3, y, 0, y - 3.5f);
    }
    glPopMatrix();
}
