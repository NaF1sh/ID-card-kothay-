#include "Campus.h"
#include "Character.h"
#include "Graphics.h"
#include <cmath>
using namespace gfx;

std::vector<CampusNpc> campusCharacters(int levelIndex) {
    std::vector<CampusNpc> npcs = {
        {{285, 325}, "Rafi", "THE CONSULTANT", CampusRole::Topper, {
            {"Wait... where's your badge? Please tell me this is a practice run.",
             "Okay. Breathe. See those evidence stands? Walk up and press F to collect each evidence.",
             "Bring both records to the entrance desk. That gets your case file started.",
             "And tomorrow? Bring your badge. I cannot laminate your face."},
            {"I saved you a seat. Front row. You're welcome.",
             "You look worried. A brief sighting is your warning. Sprint for cover before the meter fills!",
             "Get behind a wall and turn another corner. They'll lose track of you."}}},
        {{410, 140}, "Bappi", "FORMER AGENT", CampusRole::Captain, {
            {"Bhai! Quick warm-up before the case? ...Why are you looking at security?",
             "No badge? Right. Today we practice the most important skill: getting away.",
             "Save your sprint for a tight spot. Walking lets your stamina come back.",
             "If anyone asks, you were training with me. Attendance: still twelve percent."},
            {"We need a lead investigator. You already have the running-away part down.",
             "Point the mouse away from your route, then left-click. The clink draws nearby guards.",
             "Take the gap while they investigate. That's what I call an assist!"}}},
        {{180, 325}, "Mintu Mama", "GALLERY OWNER", CampusRole::Seller, {
            {"Cha lagbe? You look like someone who has forgotten something important.",
            "Your badge? Again? Sit. The tea is hot and the gossip is hotter.",
            "You get two distractions each attempt. Save one for a guard near an evidence stand.",
            "Tea first, panic later. The clock waits while we talk."},
            {"One singara for you. One for the cat. Don't mix up the orders.",
            "Security asked if I'd seen a detective without a badge.",
            "I told them I see detectives without sleep, money, and hope. Be more specific."}}},
        {{360, 110}, "Nila", "LAKESIDE POET", CampusRole::Poet, {
            {"Shh. The duck is thinking. Or looking for a singara. Hard to tell.",
             "I come here when campus gets too loud. You can stay a moment.",
             "Careful by the water: guards can see across the lake. A solid wall is better cover.",
             "There. Practical advice. Please don't tell anyone; I have a poetic reputation."},
            {"I wrote a poem about deadlines. I'll finish it tomorrow.",
             "Get close to a cat and press F for a quick scratch behind the ears.",
             "A slow blink means trust. At least, that's what I tell myself."}}},
        {{675, 325}, "Dr. Nap", "SLEEPY CURATOR", CampusRole::Professor, {
            {"Hm? Yes, yes. Excellent question. What was the question?",
             "Oh, your badge. I left mine in yesterday's coat. We shall call this field research.",
             "Take your time reading. The alarm and the guards wait during a conversation.",
             "Now, if anyone asks, I was thinking with my eyes closed."},
            {"Surprise inspection! Where is your detective badge?",
             "That expression earns partial credit.",
             "Go on. Collect your evidence and close the case. I haven't written the report yet."}}},
        {{190, 270}, "Pixel", "CLUB ROOM GAMER", CampusRole::Gamer, {
            {"Hey! You look like you just walked into a boss fight without reading the controls.",
             "Good news: a yellow question mark is a warning. You still have time to move.",
             "Do not let the detection meter fill. Break sight and take another turn.",
             "Bad news: museum Wi-Fi is still the actual final boss."},
            {"I found a Wi-Fi signal. It disappeared when I moved one step.",
             "Finish all four levels and your name goes on the scoreboard.",
             "First, just close the case. Speedruns can wait until you know the museum."}}}
    };
    (void)levelIndex;
    return npcs;
}

void drawCampusNpc(const CampusNpc& npc, float time) {
    CharacterStyle style = studentStyle();
    static const uint32_t shirts[] = {0xe8d8b5, 0xc34442, 0xf2b95c, 0x9e77b5, 0xd7e0e1, 0x465373};
    style.shirt = hex(shirts[int(npc.kind)]);
    style.shirtShade = darker(style.shirt, 0.2f);
    static const uint32_t hair[] = {0x282632, 0x37251c, 0x65514a, 0x332c40, 0xb9bcc0, 0x253040};
    static const uint32_t skin[] = {0xc98b62, 0xa96e49, 0xb78057, 0xcd9776, 0xd3a180, 0xbb8059};
    style.hair = hex(hair[int(npc.kind)]);
    style.hairHi = lighter(style.hair, 0.15f);
    style.skin = hex(skin[int(npc.kind)]);
    style.skinDark = darker(style.skin, 0.22f);
    style.backpack = npc.kind == CampusRole::Topper || npc.kind == CampusRole::Gamer;
    drawCharacterShadow(npc.position, 1.05f);
    pushTransform(npc.position.x, npc.position.y, 0, 1.15f, 1.15f);
    drawCharacter(style, Direction::Down, 0, 0);
    if (npc.kind == CampusRole::Topper) {
        setColor(hex(0x242a38));
        circleOutline(-4, 29, 3.3f, 1.5f); circleOutline(4, 29, 3.3f, 1.5f); line(-1, 29, 1, 29);
        float bob = std::sin(time * 2) * 0.6f;
        setColor(hex(0x315d91)); rect(-10, 12 + bob, 20, 13);
        setColor(hex(0xfff5d8)); rect(-8, 14 + bob, 16, 10);
        setColor(hex(0x8e8b7c)); line(0, 14 + bob, 0, 24 + bob);
        for (int i = 0; i < 3; ++i) { line(-6, 17 + i * 2 + bob, -2, 17 + i * 2 + bob); line(2, 17 + i * 2 + bob, 6, 17 + i * 2 + bob); }
    } else if (npc.kind == CampusRole::Captain) {
        setColor(hex(0xffffff)); textCentered(0, 16, "10", Font::Small);
        circle(15, 4, 7); setColor(hex(0x26303e)); circle(15, 4, 2.8f);
        for (int i = 0; i < 5; ++i) { float a = i * 2 * PI_F / 5; circle(15 + std::cos(a) * 5.5f, 4 + std::sin(a) * 5.5f, 1.5f); }
    } else if (npc.kind == CampusRole::Seller) {
        setColor(hex(0xfaf1df)); rect(-6, 9, 12, 12); rect(8, 16, 6, 7);
        setColor(hex(0xa3764b)); ellipse(11, 23, 3, 1.4f);
        setColor(hex(0x5e4034)); ellipse(-1.7f, 24, 2, 1); ellipse(1.7f, 24, 2, 1);
        setColor(hex(0xffffff, 0.65f));
        arc(11 + std::sin(time * 2), 28, 3, 70, 220, 1);
        arc(12 - std::sin(time * 2), 33, 2, -90, 70, 1);
    } else if (npc.kind == CampusRole::Professor) {
        setColor(hex(0x626878)); circleOutline(-3.5f, 27, 2.8f, 1); circleOutline(3.5f, 27, 2.8f, 1); line(-1, 27, 1, 27);
        setColor(hex(0x8d5261)); triangle({-2, 20}, {2, 20}, {0, 11});
        setColor(hex(0x735c46)); roundedRect(9, 4, 10, 10, 2); arc(14, 14, 3, 0, 180, 1);
        setColor(hex(0x1d2430)); text(10, 42 + std::sin(time * 2) * 2, "Z z", Font::Small);
    } else if (npc.kind == CampusRole::Gamer) {
        setColor(hex(0x38b7c3)); arc(0, 30, 9, 0, 180, 3); rect(-10, 26, 4, 8); rect(6, 26, 4, 8);
        setColor(hex(0x253040)); roundedRect(-7, 11, 14, 7, 2);
        setColor(hex(0x83f7ef)); rect(-3, 13, 6, 3);
        setColor(hex(0xe7bc6f)); circle(-5, 14, 1); circle(5, 14, 1);
    } else {
        setColor(hex(0x523c65)); ellipse(-7, 23, 3, 6); ellipse(7, 23, 3, 6);
        setColor(hex(0xe8bf77)); thickLine(-5, 21, 5, 19, 3); rect(3, 10, 3, 10);
        setColor(hex(0x655681)); rect(7, 12, 9, 13);
        setColor(hex(0xf8e7cd)); rect(8, 14, 7, 10);
        setColor(hex(0x9b8b7a)); line(10, 21, 14, 21); line(10, 18, 14, 18);
    }
    popTransform();
}

void drawCampusCat(const CampusCatPose& pose, float time, int index) {
    const Color coats[] = {hex(0xb98c59), hex(0x62656b), hex(0xd9cbb4), hex(0x977965)};
    Color fur = coats[index % 4], shade = darker(fur, 0.22f), cream = hex(0xece0ca);
    bool side = std::fabs(pose.direction.x) > std::fabs(pose.direction.y) * 0.7f;
    bool back = !side && pose.direction.y > 0 && !pose.happy;
    float stride = pose.moving ? std::sin(time * 10 + index) : 0;
    float blink = std::fmod(time + index * 1.3f, 5.2f);
    drawCharacterShadow(pose.position, 0.58f);
    pushTransform(pose.position.x, pose.position.y, 0, side && pose.direction.x < 0 ? -0.9f : 0.9f, 0.9f);
    // Low, four-legged silhouette. A resting cat loafs rather than standing upright.
    float sway = std::sin(time * (pose.happy ? 4 : 1.8f) + index) * 2;
    setColor(shade);
    arc(side ? -10 : -6, 10, 6, 90 + sway * 4, 280, 2.5f);
    setColor(fur);
    arc(side ? -10 : -6, 10, 5.5f, 95 + sway * 4, 270, 1.5f);
    for (int i = 0; i < 4; ++i) {
        float x = side ? -8 + i * 5.0f : (i % 2 ? 3.5f : -3.5f);
        float y = !side && i < 2 ? 5 : 0;
        float lift = pose.moving ? std::max(0.0f, stride * (i % 2 ? -1 : 1)) * 2 : 0;
        setColor(i < 2 ? shade : fur);
        roundedRect(x - 1.3f, y + lift, 2.6f, 7, 1);
        setColor(cream); ellipse(x + 0.5f, y + lift + 1, 1.8f, 1.1f, 10);
    }
    setColor(shade); ellipse(0, 8, side ? 11 : 6, side ? 5.5f : 8);
    setColor(fur); ellipse(0, 8.7f, side ? 10.5f : 5.5f, side ? 5 : 7.5f);
    if (index != 1) {
        setColor(shade);
        for (int i = -1; i <= 1; ++i) line(i * 4 - 2, 12, i * 4 - 3, 9, 1.3f);
    }
    float hx = side ? 9 : 0, hy = (back ? 17 : 14) + std::fabs(stride) * 0.5f;
    float ear = std::fmod(time + index, 7) < 0.18f ? 1 : 0;
    setColor(shade);
    triangle({hx - 5.5f, hy + 2}, {hx - 5 + ear, hy + 8}, {hx - 0.5f, hy + 4});
    triangle({hx + 0.5f, hy + 4}, {hx + 5, hy + 8}, {hx + 5.5f, hy + 2});
    ellipse(hx, hy, 6, 5.5f);
    setColor(fur); ellipse(hx, hy + 0.4f, 5.6f, 5);
    setColor(hex(0xc59385));
    triangle({hx - 4.5f, hy + 4}, {hx - 4 + ear, hy + 6.5f}, {hx - 2, hy + 4});
    triangle({hx + 2, hy + 4}, {hx + 4, hy + 6.5f}, {hx + 4.5f, hy + 4});
    if (!back) {
        setColor(cream); ellipse(hx, hy - 2, 3.8f, 2.2f);
        setColor(hex(0x393932));
        for (int sign : {-1, 1}) {
            if (pose.happy || blink < 0.15f) line(hx + sign * 2 - 1, hy + 1, hx + sign * 2 + 1, hy + 1.3f, 1);
            else ellipse(hx + sign * 2.4f, hy + 1, 0.7f, 1.2f, 10);
            setColor(hex(0x786b5c));
            line(hx + sign * 3, hy - 1.8f, hx + sign * 7, hy - 1, 0.8f);
            setColor(hex(0x393932));
        }
        setColor(hex(0x986c65)); triangle({hx - 1, hy - 1}, {hx + 1, hy - 1}, {hx, hy - 2.2f});
    }
    if (pose.happy) {
        float y = 28 + std::sin(time * 3);
        setColor(hex(0xc58a7f, 0.85f)); circle(-1.2f, y, 1.8f); circle(1.2f, y, 1.8f);
        triangle({-3, y}, {3, y}, {0, y - 3.5f});
    }
    popTransform();
}
