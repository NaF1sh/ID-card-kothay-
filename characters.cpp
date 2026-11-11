#include "game.h"
#include <cstdlib>
#include <ctime>

struct CharacterStyle
{
    Color skin, skinDark, hair, hairHi;
    Color shirt, shirtShade, pants, pantsFar, shoe, sole, outline;
    bool backpack;
    Color pack, packShade, zip;
    bool guardGear;
    Color cap, capDark, badge, belt;
};

static CharacterStyle cs;

#define HIP_Y 9.5f
#define SHOULDER_Y 20.0f
#define HEAD_Y 28.5f

#define NPC_OPU_MAMA 2
#define NPC_DR_JAMIL 4
#define NPCS_WITH_LATER_LINES 8
#define LATER_LINES_PER_NPC 4

static void useGuardStyle()
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
}

static void useDetectiveStyle()
{
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

void useStyle(int id)
{
    if (id == STYLE_GUARD)
        useGuardStyle();
    else
        useDetectiveStyle();
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

static void drawLeg(float hipX, float deg, float lift, bool farSide)
{
    glPushMatrix();
    glTranslatef(hipX, HIP_Y + lift, 0);
    glRotatef(deg, 0, 0, 1);

    setColor(cs.outline);
    DrawRoundRect(-3.1f, -9.9f, 6.2f, 10.4f, 2.2f);
    if (farSide)
        setColor(cs.pantsFar);
    else
        setColor(cs.pants);
    DrawRoundRect(-2.5f, -9.3f, 5.0f, 9.6f, 1.8f);

    setColor(cs.sole);
    DrawCircle(0, -9.6f, 3.5f, 2.2f, 12);
    if (farSide)
        setColor(darker(cs.shoe, 0.12f));
    else
        setColor(cs.shoe);
    DrawCircle(0, -9.1f, 3.0f, 1.8f, 12);
    glPopMatrix();
}

static void drawArm(float shoulderX, float deg, bool farSide)
{
    glPushMatrix();
    glTranslatef(shoulderX, SHOULDER_Y, 0);
    glRotatef(deg, 0, 0, 1);

    setColor(cs.outline);
    DrawRoundRect(-2.8f, -11.4f, 5.6f, 12.2f, 2.6f);
    if (farSide)
        setColor(darker(cs.skin, 0.12f));
    else
        setColor(cs.skin);
    DrawRoundRect(-1.9f, -10.6f, 3.8f, 6.0f, 1.7f);
    DrawCircle(0, -10.4f, 2.2f, 2.2f, 10);

    if (farSide)
        setColor(cs.shirtShade);
    else
        setColor(cs.shirt);
    DrawRoundRect(-2.3f, -6.0f, 4.6f, 6.6f, 2.0f);
    glPopMatrix();
}

static void drawTorsoFront(float width)
{
    float left = -width * 0.5f;
    setColor(cs.outline);
    DrawRoundRect(left - 0.7f, 7.3f, width + 1.4f, 15.0f, 5.0f);
    setColor(cs.shirt);
    DrawRoundRect(left, 8.0f, width, 13.6f, 4.5f);
    setColor(cs.shirtShade);
    DrawRoundRect(left, 8.0f, width, 3.6f, 1.8f);
}

static void drawBelt(float left, float width, bool buckle)
{
    setColor(cs.belt);
    DrawRect(left, 9.6f, width, 2.4f);
    if (buckle)
    {
        setColor(cs.badge);
        DrawRect(-1.3f, 9.5f, 2.6f, 2.6f);
    }
}

static void drawGuardFace()
{
    setColor(cs.cap);
    DrawCircle(0, HEAD_Y + 4.2f, 8.4f, 4.8f, 0);
    setColor(cs.capDark);
    DrawCircle(0, HEAD_Y + 1.2f, 8.6f, 2.2f, 0);
    setColor(cs.badge);
    DrawCircle(0, HEAD_Y + 4.6f, 1.7f, 1.7f, 10);

    setColor(cs.hair);
    DrawLine(-4.0f, HEAD_Y - 0.4f, -1.4f, HEAD_Y - 0.9f, 1.3f);
    DrawLine(4.0f, HEAD_Y - 0.4f, 1.4f, HEAD_Y - 0.9f, 1.3f);
    setColor(cs.outline);
    DrawCircle(-2.6f, HEAD_Y - 2.0f, 0.95f, 1.2f, 10);
    DrawCircle(2.6f, HEAD_Y - 2.0f, 0.95f, 1.2f, 10);
    setColor(cs.hair);
    DrawCircle(-1.7f, HEAD_Y - 4.3f, 2.0f, 0.95f, 10);
    DrawCircle(1.7f, HEAD_Y - 4.3f, 2.0f, 0.95f, 10);
}

static void drawDetectiveFace()
{
    setColor(cs.hair);
    DrawCircle(0, HEAD_Y + 3.2f, 7.9f, 5.0f, 0);
    DrawCircle(-3.4f, HEAD_Y + 1.4f, 3.8f, 2.2f, 12);
    DrawCircle(2.6f, HEAD_Y + 1.8f, 4.4f, 2.0f, 12);
    setColor(cs.hairHi);
    DrawArc(0, HEAD_Y + 3.0f, 5.5f, 60, 130, 1.5f);

    setColor(cs.outline);
    DrawCircle(-2.6f, HEAD_Y - 1.5f, 1.0f, 1.3f, 10);
    DrawCircle(2.6f, HEAD_Y - 1.5f, 1.0f, 1.3f, 10);
    setHex(0xff8a80, 0.45f);
    DrawCircle(-4.4f, HEAD_Y - 3.6f, 1.5f, 0.9f, 10);
    DrawCircle(4.4f, HEAD_Y - 3.6f, 1.5f, 0.9f, 10);
    setColor(cs.skinDark);
    DrawArc(0, HEAD_Y - 3.2f, 1.6f, 200, 340, 1.2f);
}

static void drawHeadFront()
{
    setColor(cs.outline);
    DrawCircle(0, HEAD_Y, 8.2f, 7.7f, 0);
    setColor(cs.skinDark);
    DrawCircle(-7.4f, HEAD_Y - 0.5f, 1.9f, 1.9f, 10);
    DrawCircle(7.4f, HEAD_Y - 0.5f, 1.9f, 1.9f, 10);
    setColor(cs.skin);
    DrawCircle(0, HEAD_Y, 7.5f, 7.0f, 0);

    if (cs.guardGear)
        drawGuardFace();
    else
        drawDetectiveFace();
}

static void drawBackpackStrapsFront()
{
    setColor(cs.pack);
    DrawThickLine(-4.6f, 21.0f, -4.2f, 12.0f, 2.4f);
    DrawThickLine(4.6f, 21.0f, 4.2f, 12.0f, 2.4f);
    setColor(cs.zip);
    DrawRect(-4.9f, 15.5f, 1.4f, 1.4f);
    DrawRect(3.5f, 15.5f, 1.4f, 1.4f);
}

static void drawGuardGearFront()
{
    drawBelt(-7, 14, true);

    setColor(cs.badge);
    glBegin(GL_POLYGON);
    glVertex2f(-5.4f, 18.6f);
    glVertex2f(-1.8f, 18.6f);
    glVertex2f(-1.8f, 16.2f);
    glVertex2f(-3.6f, 14.6f);
    glVertex2f(-5.4f, 16.2f);
    glEnd();

    setColor(cs.shirtShade);
    DrawRect(1.8f, 15.0f, 3.6f, 3.0f);
    setColor(cs.capDark);
    DrawRect(-7.4f, 19.4f, 3.6f, 1.7f);
    DrawRect(3.8f, 19.4f, 3.6f, 1.7f);
}

static void drawDown(float swing, float lift)
{
    drawLeg(-3.2f, 0, maxf(0, lift), false);
    drawLeg(3.2f, 0, maxf(0, -lift), false);
    drawTorsoFront(14);
    if (cs.backpack)
        drawBackpackStrapsFront();
    if (cs.guardGear)
        drawGuardGearFront();
    drawArm(-8, -6 + swing * 8, false);
    drawArm(8, 6 + swing * 8, false);
    setColor(cs.skinDark);
    DrawRect(-2, 20, 4, 2.5f);
    drawHeadFront();
}

static void drawBackpackFromBehind()
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
    DrawArc(0, 21.4f, 2.4f, 0, 180, 1.6f);
}

static void drawGuardGearBack()
{
    drawBelt(-7, 14, false);
    glColor3ub(34, 38, 44);
    DrawRoundRect(3.4f, 8.2f, 3.2f, 5.2f, 0.8f);
    DrawLine(5.6f, 13.4f, 5.6f, 16.4f, 1);
    setColor(cs.shirtShade);
    DrawLine(0, 12.5f, 0, 20.5f, 1);
}

static void drawHeadBack()
{
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

static void drawUp(float swing, float lift)
{
    drawLeg(-3.2f, 0, maxf(0, -lift), false);
    drawLeg(3.2f, 0, maxf(0, lift), false);
    drawArm(-8, -6 - swing * 8, false);
    drawArm(8, 6 - swing * 8, false);
    drawTorsoFront(14);
    if (cs.backpack)
        drawBackpackFromBehind();
    if (cs.guardGear)
        drawGuardGearBack();
    drawHeadBack();
}

static void drawBackpackSide()
{
    setColor(cs.outline);
    DrawRoundRect(2.8f, 8.6f, 7.6f, 13.4f, 3.2f);
    setColor(cs.pack);
    DrawRoundRect(3.4f, 9.2f, 6.4f, 12.2f, 2.6f);
    setColor(cs.packShade);
    DrawRoundRect(6.4f, 10.0f, 3.2f, 6.0f, 1.4f);
}

static void drawTorsoSide()
{
    setColor(cs.outline);
    DrawRoundRect(-6.0f, 7.3f, 10.6f, 15.0f, 4.5f);
    setColor(cs.shirt);
    DrawRoundRect(-5.3f, 8.0f, 9.2f, 13.6f, 4.0f);
    setColor(cs.shirtShade);
    DrawRoundRect(-5.3f, 8.0f, 9.2f, 3.6f, 1.6f);
}

static void drawGuardGearSide()
{
    glColor3ub(34, 38, 44);
    DrawThickLine(3.2f, 12.0f, 6.0f, 3.5f, 1.8f);
    drawBelt(-5.3f, 9.2f, false);
    setColor(cs.badge);
    DrawRect(-4.4f, 15.6f, 2.2f, 2.6f);
}

static void drawSideHair()
{
    setColor(cs.hair);
    if (cs.guardGear)
    {
        DrawCircle(2.4f, HEAD_Y + 0.4f, 5.0f, 4.6f, 0);
        setColor(cs.cap);
        DrawCircle(0.6f, HEAD_Y + 3.6f, 7.4f, 4.6f, 0);
        setColor(cs.capDark);
        DrawCircle(-6.2f, HEAD_Y + 1.6f, 4.6f, 1.6f, 12);
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
}

static void drawHeadSide()
{
    setColor(cs.outline);
    DrawCircle(-0.6f, HEAD_Y, 8.0f, 7.6f, 0);
    setColor(cs.skin);
    DrawCircle(-0.6f, HEAD_Y, 7.3f, 6.9f, 0);
    DrawCircle(-7.6f, HEAD_Y - 1.4f, 1.3f, 1.3f, 8);
    drawSideHair();

    setColor(cs.skin);
    DrawCircle(0.9f, HEAD_Y - 1.4f, 1.4f, 1.8f, 10);
    setColor(cs.skinDark);
    DrawCircle(0.9f, HEAD_Y - 1.4f, 0.6f, 0.6f, 6);
    setColor(cs.outline);
    DrawCircle(-4.4f, HEAD_Y - 0.8f, 0.9f, 1.3f, 10);

    if (cs.guardGear)
    {
        setColor(cs.hair);
        DrawCircle(-5.9f, HEAD_Y - 3.3f, 2.0f, 0.9f, 10);
    }
    else
    {
        setColor(cs.skinDark);
        DrawLine(-6.4f, HEAD_Y - 4.0f, -4.8f, HEAD_Y - 4.2f, 1.1f);
    }
}

static void drawSide(float swing)
{
    float legDeg = swing * 28;
    float armDeg = swing * 32;

    drawArm(1.2f, armDeg, true);
    drawLeg(0.8f, -legDeg, 0, true);
    drawLeg(-0.8f, legDeg, 0, false);
    if (cs.backpack)
        drawBackpackSide();
    drawTorsoSide();
    if (cs.backpack)
    {
        setColor(cs.pack);
        DrawThickLine(-1.5f, 21.0f, 2.8f, 13.0f, 2.2f);
    }
    if (cs.guardGear)
        drawGuardGearSide();
    drawArm(-0.6f, -armDeg, false);
    drawHeadSide();
}

void drawCharacter(int facing, float walkPhase, float moveBlend)
{
    float swing = sinf(walkPhase) * moveBlend;
    float lift = swing * 2.2f;

    if (facing == DIR_DOWN)
        drawDown(swing, lift);
    else if (facing == DIR_UP)
        drawUp(swing, lift);
    else if (facing == DIR_LEFT)
        drawSide(swing);
    else if (facing == DIR_RIGHT)
    {
        glPushMatrix();
        glScalef(-1, 1, 1);
        drawSide(swing);
        glPopMatrix();
    }
}

void drawCharacterShadow(Vec2 feet, float scale)
{
    Color inner = {0.08f, 0.12f, 0.06f, 0.38f};
    Color outer = {0.08f, 0.12f, 0.06f, 0.0f};
    float x = feet.x + 1.5f * scale;
    float y = feet.y + 0.5f;
    DrawSoftCircle(x, y, 10 * scale, 4.2f * scale, inner, outer);
}

int directionFromAngle(float deg)
{
    float angle = fmodf(deg, 360);
    if (angle < 0)
        angle += 360;

    if (angle < 45 || angle >= 315)
        return DIR_RIGHT;
    if (angle < 135)
        return DIR_UP;
    if (angle < 225)
        return DIR_LEFT;
    return DIR_DOWN;
}

static float npcSpots[LEVEL_COUNT][NPC_COUNT][2] = {
    {{240, 55}, {70, 190}, {440, 320}, {760, 150}, {850, 300}, {650, 360}, {370, 60}, {930, 220}},
    {{250, 250}, {600, 305}, {790, 90}, {250, 95}, {120, 300}, {955, 200}, {560, 95}, {380, 240}},
    {{620, 262}, {50, 170}, {800, 305}, {880, 150}, {520, 305}, {170, 180}, {930, 300}, {900, 60}},
    {{100, 300}, {60, 60}, {340, 320}, {480, 50}, {900, 50}, {250, 170}, {700, 50}, {40, 140}}};

static bool npcPresent[NPC_COUNT];

static void seedRandomOnce()
{
    static bool seeded = false;
    if (!seeded)
    {
        srand((unsigned)time(NULL));
        seeded = true;
    }
}

static void chooseWhoIsHere(int level)
{
    seedRandomOnce();

    if (level == 0)
    {
        for (int i = 0; i < NPC_COUNT; i++)
            npcPresent[i] = true;
        return;
    }

    for (int i = 0; i < NPC_COUNT; i++)
        npcPresent[i] = false;

    int count = 0;
    if (level == 1)
    {
        npcPresent[NPC_DR_JAMIL] = true;
        count = 1;
    }
    if (level == 2)
    {
        npcPresent[NPC_OPU_MAMA] = true;
        count = 1;
    }

    int want = 4 + rand() % 2;
    while (count < want)
    {
        int i = rand() % NPC_COUNT;
        if (!npcPresent[i])
        {
            npcPresent[i] = true;
            count++;
        }
    }
}

static const char *laterLines[LEVEL_COUNT - 1][NPCS_WITH_LATER_LINES][LATER_LINES_PER_NPC] = {
    {
        {"Abar dekha! Mui bhabchilam tumi ekhon jail e. Moja korlam, moja.",
         "2:13 e tumi bashay ghumaitasila, mui jani. Kintu proof chara kew manbe?",
         "Distraction ta guard er pichone maro, hamne na. Hei ghuira jaibe, tumi beraiya jaiba.",
         "Mui bahire wait kortachi. Kaj hesh hoile call dio. ...Na thak, tomar phone o to hera laiya gese."},
        {"Ei wing e ami ekbar ek chor dhorsilam. Na, duijon. Asole tinjon. Bishash hoy na? Say wallahi bro, ami dhorsi.",
         "Alarm panel ta dekhso? Police e thakte amra age oita chek kortam. Ami-i shobaire shikhaisi.",
         "Door record e time likha thake. Computer mitha bole na, manush bole. Tumi 2:13 e ekhane silana? Say wallahi bro.",
         "Guard jokhon thame, tumi o thamo. Bish bochor er experience theke bolsi."},
        {"Oi miya, dokan niye bhitore chole aisi. Bahire guard ra cha khay, taka dey na.",
         "Kiyo, apni ekhono dhora khan nai? Bhalo bhalo.",
         "Rat duitay alarm bondho hoilo, ami tokhon cha er pani gorom kortesilam. Kiyo, keu shune nai?",
         "Oi miya, cha ta thanda hoye gese. Guard er moto, dekhe kintu nore na."},
        {"HAHAHA ei statue ta dekhte amar mamar moto... o, eta ancient? HAHAHA",
         "Ei wing e reel banale views beshi. Purano jinish ekhon trending.",
         "Guard amar dike takay, ami phone er dike takai. Fair deal. HAHAHA",
         "Alarm bajle oi sound ta reel e dibo. HAHA na thak, chakri chole jabe."},
        {"Restoration lab ta amar. Ekhane purano jinish thik kori. Ar tehari'r plate.",
         "Alarm panel e master key er chinho ase. Amar kase to nai, ami to bolsi, tehari ase.",
         "2:13 e ami tehari'r shopno dekhtesilam. Shopne kono alarm bajeni.",
         "Ei pot ta tin hajar bochor purano. Amar tehari tin ghonta. Duitai thanda."},
        {"Level 2 bro, difficulty barse. Kore felo kore felo.",
         "Guard er route ekta pattern e chole. Duibar dekh, tarpor ja. Hoye jabe hoye jabe.",
         "Ei purano jinish gular pashe wifi ase. Ancient wifi. Hoye jabe hoye jabe.",
         "Dhora khaile restart. Respawn e kono shorom nai bro."},
        {"Hi guys! Ei wing er tour ashi taka. Tumi shunteso, tai tomar o ashi.",
         "Ei pot er shathe selfie bish taka. Amar shathe na, pot er shathe.",
         "Guard ra ghure ghure pahara dey, ami ghure ghure reel banai. Same kaj, kom salary.",
         "Door record niye bolte pari, kintu oita premium content."},
        {"Ami akash pathabo... ei pot ta koto? Amar room e rakhbo.",
         "Abbu bolse museum tai kine dibe. Shudhu parking nai, oita problem.",
         "Guard ra amake kisu bole na. Gari dekhe bhabe ami VIP.",
         "Tomar moner akashe... ei hall e echo ta joss. Shunle guard ra ghumay jabe hoyto."}},
    {
        {"Gallery te eto chobi, ar amra khujtachi ekta coat er receipt. Jibon re jibon.",
         "Arpita naki kisu dekhche. Hei maiyare reels theika uthaite parle hoy.",
         "Chabi kar kase thake, oi list ta paile ar kono confusion thakbe na.",
         "Statue gular pichone luka. Pathor to ar tomare dhorbe na, hono."},
        {"Painting er chokh tomake follow kore, guard na. Tension nio na.",
         "Ami ekbar ek ghonta statue er moto dariye silam. Undercover. Keu bujhe nai.",
         "Chor ra shob shomoy kisu na kisu fele jay. Eta amar rule number ek. Rule number dui: say wallahi bro.",
         "Stamina shesh? Hato. Ami ekhono dourai, shudhu hatu ta mane na."},
        {"Oi miya, ekhane o ami! Shob jaygay cha lage.",
         "Boro guard coat ta laundry te disilo. Receipt ta dokan e pore silo. Kiyo, ami ki korbo?",
         "Kiyo, chabi kar kase ase oi list ta ekbar dekhen. Mone hoy kaje lagbe.",
         "Oi miya, statue gula cha khay na. Tai ora amar fav na."},
        {"Ami-i to dekhsilam! Coat ar boot. Keu bishash kore na, karon ami tokhon reel dekhtesilam. HAHAHA",
         "Ei painting ta eto serious keno? Ektu hashle ki hoy? HAHA",
         "Statue gula rat duitay o same pose e silo. Ami chek korsi. HAHAHA",
         "Battery 3%. Eta amar jiboner shobcheye boro tension. HAHAHA"},
        {"Gallery te ekta painting e biriyani ase. Tehari na. Disappointed.",
         "Shilpi ra khali pete chobi ake, tai chobi gula eto dukkhi.",
         "Key list e amar nam nai, karon ami chabi haray feli. Tehari'r plate kintu haray na.",
         "Guard ra ghure, ami boshe thaki. Tehari hojom hote dao."},
        {"Side quest koris na bro, evidence age. Kore felo kore felo.",
         "Ei painting ta amar profile pic banabo. Daari soho.",
         "Tinta evidence? Easy. Hoye jabe hoye jabe.",
         "Statue er pichone camp kor, guard pass korle run. Classic."},
        {"Gallery tour, ek painting dosh taka. Tumi tinta dekhso. Tirish.",
         "Arpita'r reel e comment korsi, o amake follow back dey na.",
         "Statue er pichone lukale guard dekhbe na. Tip ta free, ajke offer cholse.",
         "Coat er receipt? Ami shob receipt rakhi. Taka koi jay bujhar jonno."},
        {"Ei painting ta ami akashi. Na, kinsi. Ekoi kotha.",
         "Kal rater sketch ta gold frame e bandhaisi. Coat-wala lok ta clear dekha jay.",
         "Ami akash pathabo tomar moner akashe... gallery te gaile shobai tali dey. Mone mone.",
         "Amar coat Italy theke ashe. Receipt Abbu rakhe."}},
    {
        {"Vault porjonto chole aiso? Mui to bhabchilam gate ei dhora khaiba.",
         "Ekhon khali proof ta lagbe. Ar crown ta. Bas, eituku.",
         "Boro guard er mop er bucket aij eto bhari kene, kew jigay na?",
         "Dhora khaile mui kaimu tomare chini na. Moja korlam. Mostly."},
        {"Vault! Ei jaygay ami ekbar... na thak, oita classified. Kauke bolba na, say wallahi bro.",
         "Je chabi diye alarm bondho kore, tar kase vault er chabi o thake. Simple.",
         "Tinjon guard, kintu tinjon ek shathe ek dike takay na. Opekkha koro.",
         "Boro guard bole se kisu jane na. Say wallahi bro? Hah. Mop er bucket ta dekho."},
        {"Oi miya, vault er shamne dokan disi. Business bhalo.",
         "Kiyo, boro guard aj mop niye ghure. Kono din to ghor muche nai.",
         "Ekta singara khan, last level. Khali pete chor dhora jay na.",
         "Oi miya, crown ta pele ekbar dekhaiyen. Shara jibon cha bechi, crown dekhi nai."},
        {"Vault er reel? Eta to viral hobe! HAHAHA",
         "Boro guard ke reel e tag korbo, 'mop er bhitore ki?' HAHAHAHA",
         "Amar hashi shune guard ashle tumi ulta dike jao. Free distraction. HAHA",
         "Shesh hoile credit dio. Witness: Arpita. HAHAHA"},
        {"Vault e tehari rakha jay na. Ami jiggesh korsi.",
         "Ledger e crown er dam likha. Oi taka diye koto plate tehari hoy hishab korsi. Onek.",
         "Boro guard ke tehari khaite bolsilam. Bole busy. Tehari na kheye manush busy hoy kemne?",
         "Crown ta pele tehari party. Ami arrange korbo, tumi taka diba."},
        {"Final boss bro. Boro guard. Kore felo kore felo.",
         "Scoreboard e ami top e. Tui ekhono duite. Hoye jabe hoye jabe.",
         "Timer dekhis. Shomoy kom mone hoy, but ase. Kore felo kore felo.",
         "Jitle treat dis. Na dile o hoye jabe hoye jabe."},
        {"Vault tour nai, kintu reel ase. Subscribe koro.",
         "Crown er reel e million view. Million view e koto taka? Hishab kortesi.",
         "Boro guard ke mop niye vault e dhukte dekhsi. Ei info ponchash taka. Tumi friend, chollish.",
         "Chor dhora porle ami live e jabo. Live e gift pathaio."},
        {"Ei vault ta amar closet er cheye choto.",
         "Crown ta pawa gele ami ekta copy banabo. Gold e. Normal.",
         "Ami akash pathabo... guard ra amar gaan e nache na keno?",
         "Tumi jitle amar gari te lift dibo. Gaan shunte shunte."}}};

static int currentLevel()
{
    if (levelIndex >= 0 && levelIndex < LEVEL_COUNT)
        return levelIndex;
    return 0;
}

static void setNpc(int i, int level, const char *name, const char *role, int kind)
{
    Vec2 spot = makeVec(npcSpots[level][i][0], npcSpots[level][i][1]);
    npcs[i].here = npcPresent[i];
    npcs[i].position = nearestFreeSpot(spot, PLAYER_RADIUS + 2);
    npcs[i].name = name;
    npcs[i].role = role;
    npcs[i].kind = kind;
    npcs[i].lineCount = 0;
    npcs[i].visits = 0;
    npcs[i].lastTask[0] = '\0';
}

static void addLine(int i, const char *text)
{
    npcs[i].lines[npcs[i].lineCount] = text;
    npcs[i].lineCount++;
}

static void setupAsif(int level)
{
    setNpc(0, level, "Asif", "THE CONSULTANT", CONSULTANT);
    addLine(0, "Kiare, tumi ehane ki koro? Bahire to tomar chobi diya poster lagaiche!");
    addLine(0, "Mui Barishailla manush, mor kotha hono. Dhan, nodi, khal, ei tine Barishal. Ar ei case e mui tomar loge.");
    addLine(0, "Hono, oi stand gula dekhtaso? Kase giya F chaplei evidence ta laiya nibe.");
    addLine(0, "Ei room er hogol record lagbe. Hogol paile gold door ta diya ber hoiya jao.");
    addLine(0, "Mui jani crown tumi nao nai. Tumi to nijer chabi-i khuijja pao na.");
    addLine(0, "Kew dekhya fellei meter ta bhortase. Bhorar age dour diya kono deyal er pichone jao.");
    addLine(0, "Deyal er pichone giya ekta mor ghuro. Guard ra ar tomare khuijja paibe na. Mui koilam, hono.");
}

static void setupCyrus(int level)
{
    setNpc(1, level, "Cyrus", "FORMER AGENT", AGENT);
    addLine(1, "Bhai, police e bish bochor chakri korsi. Ekhono kothao dhuklei age exit khuji.");
    addLine(1, "Tomake fashaise? Say wallahi bro. ...Thik ase, tahole aj palano practice hobe.");
    addLine(1, "Space chepe dour diba, kintu shob shomoy na. Hatle stamina abar bhore jay.");
    addLine(1, "Keu jiggesh korle bolba amar shathe training korteso. Thik ase?");
    addLine(1, "Tumi crown nao nai? Say wallahi bro. Ami chokh dekhei bujhi, police er chokh.");
    addLine(1, "Mouse ta ulta dike rakho, left click dao. Tung kore awaj hobe, guard oidike jabe.");
    addLine(1, "Guard jokhon awaj er dike jay, tokhon fanka diye ber hoye jao. Amader shomoyer trick.");
}

static void setupOpuMama(int level)
{
    setNpc(2, level, "Opu Mama", "MUSEUM CAFE OWNER", SELLER);
    addLine(2, "Oi miya, cha diboni ekta? Chehara dekhe mone hoy rate ghum hoy nai.");
    addLine(2, "Kiyo? Kauke koiben na kintu. Boro guard kal notun card diye cha er bill dise.");
    addLine(2, "Oi miya, prottek bar duita kore distraction pan. Evidence er pashe guard thakle oikhane maren.");
    addLine(2, "Bosen bosen, kotha bolar shomoy ghori thame thake. Kiyo, eto tension koren kene?");
    addLine(2, "Ekta singara apnar, ekta Tonmoy er. Oi miya, biral ta amar dokan khali kore dilo.");
    addLine(2, "Security aisilo, jiggesh kore crown haate kauke dekhsi ni. Ami koilam, kiyo? Ami cha bechi.");
}

static void setupArpita(int level)
{
    setNpc(3, level, "Arpita", "REELS ADDICT", POET);
    addLine(3, "HAHAHAHA ei reel ta dekho... o sorry, tumi ke? HAHAHA");
    addLine(3, "Ar ekta reel, shudhu ekta. ...Eita koto number holo? HAHAHAHA");
    addLine(3, "Kal rate reels dekhtesilam, hothat dekhi ekjon. Gaye detective er coat, paye guard er boot. HAHAHA ki fashion!");
    addLine(3, "Guard er chokher shamne je cone ta, oitar bhitore dhukio na. Reel e dekhsi, sotti. HAHA");
    addLine(3, "Biral er kase giye F chapo, ador korte parba. Tonmoy ke niye reel banale viral. HAHAHA");
    addLine(3, "Amar hashi shune keu ashle amar dosh na. HAHAHAHAHA");
}

static void setupDrJamil(int level)
{
    setNpc(4, level, "Dr. Jamil", "SLEEPY CURATOR", CURATOR);
    addLine(4, "Ha? Tehari ashche? ...O, tumi. Ami bhablam tehari.");
    addLine(4, "Churir rate ami ghumaisilam. Kintu boro guard oi rate tehari khete ashe nai. Eta kemon kotha?");
    addLine(4, "Aste dhire poro, kotha bolar shomoy guard ra dariye thake. Tehari kintu thake na, thanda hoye jay.");
    addLine(4, "Doctor bolse tehari kom khaite. Ami doctor bodle felsi.");
    addLine(4, "Master key duita. Amar kase ekta-o nai. Amar kase ase ekta tehari'r plate.");
    addLine(4, "Case ta solve koro, Puran Dhakar tehari ami khawabo. Half plate. Full chaile nijer taka.");
    addLine(4, "Keu jiggesh korle bolba ami chokh bondho kore chinta kortesilam. Tehari niye.");
}

static void setupMahin(int level)
{
    setNpc(5, level, "Mahin", "NIGHT-SHIFT GAMER", GAMER);
    addLine(5, "Bro, tutorial skip kore direct boss fight e dhuke gesos? Hoye jabe hoye jabe.");
    addLine(5, "Guard er mathay holud question mark mane shudhu sondeho. Ekhono time ase. Kore felo kore felo.");
    addLine(5, "Meter full hoite dish na. Chokher bahire gia ekta mor ghur. Hoye jabe hoye jabe.");
    addLine(5, "Boro guard ei mashe notun phone, notun gari kinse. Sus lage na?");
    addLine(5, "Charta level? Kore felo kore felo. Scoreboard e nam uthbe.");
    addLine(5, "Ekhane wifi pailam, ek pa sorlam, gayeb. Thak, hoye jabe hoye jabe.");
}

static void setupLisa(int level)
{
    setNpc(6, level, "Lisa", "MUSEUM GUIDE", GUIDE);
    addLine(6, "Hi guys, welcome back to my channel! Aj ami museum theke... ei, tumi frame e dhuksho keno?");
    addLine(6, "Tour er ticket ponchash taka. Wanted manush hole eksho. Na, discount nai.");
    addLine(6, "Oi rate night tour disilam. Boro guard crown er room niye onek proshno korsilo. Reel ta save kora ase.");
    addLine(6, "Guard route er sheshe giye ektu dariye charpash dekhe. Ulta dike hata dhorle jao. Ei tip free, porer ta paid.");
    addLine(6, "Opu Mama'r dokan e cha? Na, ami basha theke pani niye ashi. Taka bachai.");
    addLine(6, "Reel ta te like diyo, comment o koro. Ota to free.");
}

static void setupDipra(int level)
{
    setNpc(7, level, "Dipra", "ART STUDENT", STUDENT);
    addLine(7, "Ami akash pathabo tomar moner akashe... o hi! Gaan ta shunso?");
    addLine(7, "Kal rate je je eidik diye gese shobar sketch korsi. Ekta te nokol coat ar boro boro boot. Sketch ta gold frame e bandhabo.");
    addLine(7, "Deyal ar building er pichone thakle guard dekhte pay na. Ami to shob jaygay gari niye jai, lukanor dorkar pore na.");
    addLine(7, "Tonmoy ke model banaisi. Ghontay ekta singara chay. Ami ore pura dokan kine dite chaisilam.");
    addLine(7, "Ei crown ta koto? Abbu ke bolle kine dibe. Ami akash pathabo tomar moner akashe...");
    addLine(7, "Ami akash pathabo... tomar moner akashe... hmm hmm. Porer line mone nai.");
}

static void useLaterLines(int level)
{
    for (int i = 0; i < NPCS_WITH_LATER_LINES; i++)
    {
        npcs[i].lineCount = 0;
        for (int j = 0; j < LATER_LINES_PER_NPC; j++)
            addLine(i, laterLines[level - 1][i][j]);
    }
}

void setupNpcs()
{
    int level = currentLevel();
    chooseWhoIsHere(level);

    setupAsif(level);
    setupCyrus(level);
    setupOpuMama(level);
    setupArpita(level);
    setupDrJamil(level);
    setupMahin(level);
    setupLisa(level);
    setupDipra(level);

    if (level > 0)
        useLaterLines(level);
}

static unsigned int npcShirtColors[NPC_COUNT] = {0xe8d8b5, 0xc34442, 0xf2b95c, 0x9e77b5, 0xd7e0e1, 0x465373, 0x2f8f83, 0xe58fae};
static unsigned int npcHairColors[NPC_COUNT] = {0x282632, 0x37251c, 0x65514a, 0x332c40, 0xb9bcc0, 0x253040, 0x2a1d1a, 0x3a2a24};
static unsigned int npcSkinColors[NPC_COUNT] = {0xc98b62, 0xa96e49, 0xb78057, 0xcd9776, 0xd3a180, 0xbb8059, 0xc78d68, 0xb98060};

static void drawLongHair(unsigned int color)
{
    setHex(color);
    DrawCircle(-7.5f, 22, 3.4f, 9, 0);
    DrawCircle(7.5f, 22, 3.4f, 9, 0);
    DrawRect(-10.5f, 14, 4, 8);
    DrawRect(6.5f, 14, 4, 8);
    setColor(lighter(hexColor(color), 0.15f));
    DrawLine(-8, 15, -8, 28, 1);
    DrawLine(8, 15, 8, 28, 1);
}

static void drawConsultantExtras(float t)
{
    glColor3ub(36, 42, 56);
    DrawCircleOutline(-4, 29, 3.3f, 3.3f, 1.5f);
    DrawCircleOutline(4, 29, 3.3f, 3.3f, 1.5f);
    DrawLine(-1, 29, 1, 29, 1);

    float bob = sinf(t * 2) * 0.6f;
    glColor3ub(49, 93, 145);
    DrawRect(-10, 12 + bob, 20, 13);
    glColor3ub(255, 245, 216);
    DrawRect(-8, 14 + bob, 16, 10);
    glColor3ub(142, 139, 124);
    DrawLine(0, 14 + bob, 0, 24 + bob, 1);
    for (int i = 0; i < 3; i++)
    {
        DrawLine(-6, 17 + i * 2 + bob, -2, 17 + i * 2 + bob, 1);
        DrawLine(2, 17 + i * 2 + bob, 6, 17 + i * 2 + bob, 1);
    }
}

static void drawAgentExtras(float t)
{
    glColor3ub(29, 36, 48);
    DrawRoundRect(-7, 27, 6, 4, 1.5f);
    DrawRoundRect(1, 27, 6, 4, 1.5f);
    DrawLine(-1, 29.5f, 1, 29.5f, 1);

    float tilt = sinf(t * 1.5f) * 1.5f;
    glColor3ub(107, 74, 50);
    DrawThickLine(9, 6, 14, 12 + tilt, 2.5f);
    glColor3ub(201, 162, 74);
    DrawCircleOutline(16, 15 + tilt, 5, 5, 2);
    setHex(0xbfe6f5, 0.6f);
    circle(16, 15 + tilt, 4);
    setHex(0xffffff, 0.8f);
    circle(14.5f, 16.5f + tilt, 1.2f);
}

static void drawSellerExtras(float t)
{
    glColor3ub(250, 241, 223);
    DrawRect(-6, 9, 12, 12);
    DrawRect(8, 16, 6, 7);
    glColor3ub(163, 118, 75);
    DrawCircle(11, 23, 3, 1.4f, 0);
    glColor3ub(94, 64, 52);
    DrawCircle(-1.7f, 24, 2, 1, 0);
    DrawCircle(1.7f, 24, 2, 1, 0);

    float steam = sinf(t * 2);
    setHex(0xffffff, 0.65f);
    DrawArc(11 + steam, 28, 3, 70, 220, 1);
    DrawArc(12 - steam, 33, 2, -90, 70, 1);
}

static void drawTehariPlate(float t)
{
    glColor3ub(255, 255, 255);
    DrawCircle(14, 13, 7, 3, 0);
    glColor3ub(231, 184, 74);
    DrawCircle(14, 14, 5, 2.4f, 0);
    glColor3ub(122, 74, 44);
    circle(12, 14.5f, 0.9f);
    circle(15.5f, 13.5f, 0.9f);
    circle(14, 15.5f, 0.8f);
    glColor3ub(63, 138, 58);
    circle(16.5f, 15, 0.7f);
    setHex(0xffffff, 0.6f);
    DrawArc(13 + sinf(t * 2), 20, 2, 70, 220, 1);
}

static void drawCuratorExtras(float t)
{
    glColor3ub(98, 104, 120);
    DrawCircleOutline(-3.5f, 27, 2.8f, 2.8f, 1);
    DrawCircleOutline(3.5f, 27, 2.8f, 2.8f, 1);
    DrawLine(-1, 27, 1, 27, 1);
    glColor3ub(141, 82, 97);
    DrawTriangle(-2, 20, 2, 20, 0, 11);

    drawTehariPlate(t);

    glColor3ub(29, 36, 48);
    DrawText(12, 34 + sinf(t * 2) * 2, "z Z", FONT_SMALL);
}

static void drawGuideExtras(float t)
{
    drawLongHair(0x2a1d1a);
    glColor3ub(255, 255, 255);
    DrawRect(-6, 17, 5, 3);

    float wave = sinf(t * 2) * 1.0f;
    glColor3ub(58, 63, 75);
    DrawThickLine(9, 9, 15, 33 + wave, 1.4f);
    glColor3ub(29, 36, 48);
    DrawRoundRect(11.5f, 32 + wave, 7, 10, 1.5f);
    glColor3ub(159, 216, 240);
    DrawRect(12.5f, 33.5f + wave, 5, 7);

    bool recordLightOn = fmodf(t, 1.2f) < 0.6f;
    if (recordLightOn)
    {
        glColor3ub(229, 57, 53);
        circle(17, 40 + wave, 0.9f);
    }
}

static void drawMusicNotes(float t)
{
    for (int i = 0; i < 2; i++)
    {
        float rise = fmodf(t * 0.6f + i * 0.5f, 1.0f);
        setHex(0x6b4aa0, 1 - rise);
        float noteX = 9 + i * 4 + sinf(t * 3 + i) * 1.5f;
        float noteY = 33 + rise * 8;
        circle(noteX, noteY, 1.4f);
        DrawLine(noteX + 1.3f, noteY, noteX + 1.3f, noteY + 5, 1);
        DrawLine(noteX + 1.3f, noteY + 5, noteX + 3.3f, noteY + 4, 1);
    }
}

static void drawStudentExtras(float t)
{
    drawLongHair(0x3a2a24);
    glColor3ub(140, 47, 57);
    DrawCircle(1, 35, 8, 3, 0);
    circle(1, 38, 1.5f);

    glColor3ub(74, 106, 138);
    DrawRect(-15, 10, 11, 14);
    glColor3ub(253, 246, 230);
    DrawRect(-14, 11, 9, 12);
    glColor3ub(138, 138, 138);
    DrawLine(-12, 15, -7, 19, 1);
    DrawLine(-12, 19, -8, 14, 1);

    float pencilMove = sinf(t * 4);
    glColor3ub(232, 191, 119);
    DrawThickLine(6, 12 + pencilMove, 10, 20 + pencilMove, 2);
    glColor3ub(233, 192, 74);
    DrawArc(0, 22, 4, 200, 340, 1.2f);
    DrawRect(7.5f, 9, 3.5f, 2.2f);

    drawMusicNotes(t);
}

static void drawGamerExtras()
{
    glColor3ub(37, 48, 64);
    DrawCircle(0, 22, 5.6f, 2.6f, 0);
    DrawRect(-6.2f, 22, 1.6f, 4.5f);
    DrawRect(4.6f, 22, 1.6f, 4.5f);
    DrawCircle(-1.5f, 24.3f, 1.9f, 0.7f, 10);
    DrawCircle(1.5f, 24.3f, 1.9f, 0.7f, 10);

    glColor3ub(56, 183, 195);
    DrawArc(0, 30, 9, 0, 180, 3);
    DrawRect(-10, 26, 4, 8);
    DrawRect(6, 26, 4, 8);

    glColor3ub(37, 48, 64);
    DrawRoundRect(-7, 11, 14, 7, 2);
    glColor3ub(131, 247, 239);
    DrawRect(-3, 13, 6, 3);
    glColor3ub(231, 188, 111);
    circle(-5, 14, 1);
    circle(5, 14, 1);
}

static void drawReelsFanExtras(float t)
{
    drawLongHair(0x332c40);
    glColor3ub(29, 36, 48);
    DrawRoundRect(7, 10, 7, 12, 1.5f);

    float flick = fmodf(t * 0.7f, 1.0f);
    if (flick < 0.33f)
        glColor3ub(242, 138, 166);
    else if (flick < 0.66f)
        glColor3ub(127, 211, 240);
    else
        glColor3ub(242, 209, 107);
    DrawRect(8, 12, 5, 8);
    glColor3ub(255, 255, 255);
    DrawTriangle(9.8f, 14.5f, 9.8f, 17.5f, 12, 16);

    bool laughing = fmodf(t, 4.5f) < 1.6f;
    if (laughing)
    {
        glColor3ub(192, 57, 43);
        DrawText(-12, 40 + sinf(t * 12), "HAHA", FONT_SMALL);
    }
}

static void useNpcStyle(int kind)
{
    useStyle(STYLE_DETECTIVE);
    setShirt(npcShirtColors[kind]);
    cs.hair = hexColor(npcHairColors[kind]);
    cs.hairHi = lighter(cs.hair, 0.15f);
    cs.skin = hexColor(npcSkinColors[kind]);
    cs.skinDark = darker(cs.skin, 0.22f);
    cs.backpack = (kind == CONSULTANT || kind == GAMER);
}

void drawNpc(int i, float x, float y, float t)
{
    int kind = npcs[i].kind;
    useNpcStyle(kind);

    drawCharacterShadow(makeVec(x, y), 1.05f);
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(1.15f, 1.15f, 1);
    drawCharacter(DIR_DOWN, 0, 0);

    if (kind == CONSULTANT)
        drawConsultantExtras(t);
    else if (kind == AGENT)
        drawAgentExtras(t);
    else if (kind == SELLER)
        drawSellerExtras(t);
    else if (kind == CURATOR)
        drawCuratorExtras(t);
    else if (kind == GUIDE)
        drawGuideExtras(t);
    else if (kind == STUDENT)
        drawStudentExtras(t);
    else if (kind == GAMER)
        drawGamerExtras();
    else
        drawReelsFanExtras(t);
    glPopMatrix();
}

static unsigned int catCoatColors[4] = {0xb98c59, 0x62656b, 0xd9cbb4, 0x977965};

struct CatLook
{
    Color fur, shade, cream;
    bool side, back;
    float stride;
    float headX, headY;
};

static CatLook cat;

static void drawCatTail(CatPose pose, float t, int index)
{
    float swaySpeed = 1.8f;
    if (pose.happy)
        swaySpeed = 4;
    float sway = sinf(t * swaySpeed + index) * 2;

    float tailX = -6;
    if (cat.side)
        tailX = -10;

    setColor(cat.shade);
    DrawArc(tailX, 10, 6, 90 + sway * 4, 280, 2.5f);
    setColor(cat.fur);
    DrawArc(tailX, 10, 5.5f, 95 + sway * 4, 270, 1.5f);
}

static void drawCatLegs(CatPose pose)
{
    for (int i = 0; i < 4; i++)
    {
        float x;
        if (cat.side)
            x = -8 + i * 5.0f;
        else if (i % 2)
            x = 3.5f;
        else
            x = -3.5f;

        float y = 0;
        if (!cat.side && i < 2)
            y = 5;

        float lift = 0;
        if (pose.moving)
        {
            float legDirection = 1;
            if (i % 2)
                legDirection = -1;
            lift = maxf(0, cat.stride * legDirection) * 2;
        }

        if (i < 2)
            setColor(cat.shade);
        else
            setColor(cat.fur);
        DrawRoundRect(x - 1.3f, y + lift, 2.6f, 7, 1);
        setColor(cat.cream);
        DrawCircle(x + 0.5f, y + lift + 1, 1.8f, 1.1f, 10);
    }
}

static void drawCatBody(int index)
{
    setColor(cat.shade);
    if (cat.side)
        DrawCircle(0, 8, 11, 5.5f, 0);
    else
        DrawCircle(0, 8, 6, 8, 0);

    setColor(cat.fur);
    if (cat.side)
        DrawCircle(0, 8.7f, 10.5f, 5, 0);
    else
        DrawCircle(0, 8.7f, 5.5f, 7.5f, 0);

    bool hasStripes = (index != 1);
    if (hasStripes)
    {
        setColor(cat.shade);
        for (int i = -1; i <= 1; i++)
            DrawLine(i * 4 - 2, 12, i * 4 - 3, 9, 1.3f);
    }
}

static void drawCatHead(float t, int index)
{
    float hx = cat.headX;
    float hy = cat.headY;

    float ear = 0;
    if (fmodf(t + index, 7) < 0.18f)
        ear = 1;

    setColor(cat.shade);
    DrawTriangle(hx - 5.5f, hy + 2, hx - 5 + ear, hy + 8, hx - 0.5f, hy + 4);
    DrawTriangle(hx + 0.5f, hy + 4, hx + 5, hy + 8, hx + 5.5f, hy + 2);
    DrawCircle(hx, hy, 6, 5.5f, 0);
    setColor(cat.fur);
    DrawCircle(hx, hy + 0.4f, 5.6f, 5, 0);
    glColor3ub(197, 147, 133);
    DrawTriangle(hx - 4.5f, hy + 4, hx - 4 + ear, hy + 6.5f, hx - 2, hy + 4);
    DrawTriangle(hx + 2, hy + 4, hx + 4, hy + 6.5f, hx + 4.5f, hy + 4);
}

static void drawCatFace(CatPose pose, float t, int index)
{
    float hx = cat.headX;
    float hy = cat.headY;
    float blink = fmodf(t + index * 1.3f, 5.2f);
    bool eyesClosed = pose.happy || blink < 0.15f;

    setColor(cat.cream);
    DrawCircle(hx, hy - 2, 3.8f, 2.2f, 0);
    for (int sign = -1; sign <= 1; sign += 2)
    {
        glColor3ub(57, 57, 50);
        if (eyesClosed)
            DrawLine(hx + sign * 2 - 1, hy + 1, hx + sign * 2 + 1, hy + 1.3f, 1);
        else
            DrawCircle(hx + sign * 2.4f, hy + 1, 0.7f, 1.2f, 10);
        glColor3ub(120, 107, 92);
        DrawLine(hx + sign * 3, hy - 1.8f, hx + sign * 7, hy - 1, 0.8f);
    }
    glColor3ub(152, 108, 101);
    DrawTriangle(hx - 1, hy - 1, hx + 1, hy - 1, hx, hy - 2.2f);
}

static void drawTonmoyCollar()
{
    float hx = cat.headX;
    float hy = cat.headY;
    glColor3ub(200, 50, 60);
    DrawRoundRect(hx - 4.5f, hy - 6.2f, 9, 2.2f, 1);
    glColor3ub(242, 193, 78);
    circle(hx, hy - 7, 1.2f);
}

static void drawCatHeart(float t)
{
    float y = 28 + sinf(t * 3);
    setHex(0xc58a7f, 0.85f);
    circle(-1.2f, y, 1.8f);
    circle(1.2f, y, 1.8f);
    DrawTriangle(-3, y, 3, y, 0, y - 3.5f);
}

static void drawTonmoyNameTag(CatPose pose)
{
    float offsetX = 0;
    if (cat.side)
    {
        if (pose.direction.x < 0)
            offsetX = -8.1f;
        else
            offsetX = 8.1f;
    }
    float tagX = pose.position.x + offsetX;
    float tagY = pose.position.y + 0.9f * (cat.headY - 6);

    float textHeight = 4.4f;
    float tagWidth = StrokeTextWidth("Tonmoy", textHeight) + 3;
    glColor3ub(200, 50, 60);
    DrawRoundRect(tagX - tagWidth / 2, tagY - textHeight - 4, tagWidth, textHeight + 3, 1.5f);
    glColor3ub(255, 246, 224);
    DrawStrokeText(tagX - tagWidth / 2 + 1.5f, tagY - textHeight - 2.5f, textHeight, "Tonmoy", 1.3f);
}

void drawCat(CatPose pose, float t, int index)
{
    cat.fur = hexColor(catCoatColors[index % 4]);
    cat.shade = darker(cat.fur, 0.22f);
    cat.cream = hexColor(0xece0ca);
    cat.side = fabsf(pose.direction.x) > fabsf(pose.direction.y) * 0.7f;
    cat.back = !cat.side && pose.direction.y > 0 && !pose.happy;
    cat.stride = 0;
    if (pose.moving)
        cat.stride = sinf(t * 10 + index);

    cat.headX = 0;
    if (cat.side)
        cat.headX = 9;
    float headBaseY = 14;
    if (cat.back)
        headBaseY = 17;
    cat.headY = headBaseY + fabsf(cat.stride) * 0.5f;

    bool flipped = cat.side && pose.direction.x < 0;
    float scaleX = 0.9f;
    if (flipped)
        scaleX = -0.9f;

    drawCharacterShadow(pose.position, 0.58f);
    glPushMatrix();
    glTranslatef(pose.position.x, pose.position.y, 0);
    glScalef(scaleX, 0.9f, 1);

    drawCatTail(pose, t, index);
    drawCatLegs(pose);
    drawCatBody(index);
    drawCatHead(t, index);
    if (!cat.back)
        drawCatFace(pose, t, index);
    if (index == 0 && !cat.back)
        drawTonmoyCollar();
    if (pose.happy)
        drawCatHeart(t);
    glPopMatrix();

    if (index == 0 && !cat.back)
        drawTonmoyNameTag(pose);
}
