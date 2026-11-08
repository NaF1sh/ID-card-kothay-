#include "UI.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Character.h"
#include "Graphics.h"

using namespace gfx;

namespace ui {

namespace {

struct ButtonHit {
    Rect r;
    int index;
};
std::vector<ButtonHit> g_hits;

const Color INK = hex(0x1d2430);
const Color PAPER = hex(0xfffaf0);
const Color BLUE = hex(0x2f5f8c);
const Color GOLD = hex(0xf2c14e);
const Color RED = hex(0xd63c3c);
const Color GREEN = hex(0x3f9b5a);
const Color MUTED = hex(0x5b6470);

void dim(float a) {
    setColor(withAlpha(hex(0x101520), a));
    rect(0, 0, VIEW_W, VIEW_H);
}

void panel(float x, float y, float w, float h, const Color& header, float headerH) {
    setColor(withAlpha(INK, 0.3f));
    roundedRect(x + 6, y - 6, w, h, 16);
    setColor(PAPER);
    roundedRect(x, y, w, h, 16);
    if (headerH > 0) {
        setColor(header);
        roundedRect(x, y + h - headerH, w, headerH, 16);
        rect(x, y + h - headerH, w, 16);  // square off the lower corners
    }
}

// Vertical list of buttons; the selected one is highlighted and pulses
// (scaling animation).
void buttons(const Items& items, int selected, float cx, float topY, float w, float t, int firstIndex = 0) {
    const float h = 36.0f, gap = 12.0f;
    for (size_t i = 0; i < items.size(); ++i) {
        float y = topY - i * (h + gap);
        bool sel = static_cast<int>(i) == selected;
        g_hits.push_back({{cx - w * 0.5f, y, w, h}, firstIndex + static_cast<int>(i)});
        float s = sel ? 1.0f + 0.03f * std::sin(t * 6.0f) : 1.0f;
        pushTransform(cx, y + h * 0.5f, 0.0f, s, s);
        setColor(withAlpha(INK, 0.25f));
        roundedRect(-w * 0.5f + 3, -h * 0.5f - 3, w, h, 10);
        setColor(sel ? GOLD : hex(0xe9e2d2));
        roundedRect(-w * 0.5f, -h * 0.5f, w, h, 10);
        setColor(sel ? hex(0x8a6a10) : hex(0xb9b0a0));
        roundedRectOutline(-w * 0.5f, -h * 0.5f, w, h, 10, 2.0f);
        if (sel) {  // little arrow marker
            setColor(INK);
            triangle({-w * 0.5f + 14, -6}, {-w * 0.5f + 14, 6}, {-w * 0.5f + 24, 0});
        }
        popTransform();
        setColor(INK);
        textCentered(cx, y + 12, items[i], Font::Medium);
    }
}

void hint(const char* s) {
    setColor(withAlpha(hex(0xffffff), 0.85f));
    textCentered(VIEW_W * 0.5f, 14, s, Font::Small);
}

// A detective badge / case file card built from shapes; the "?" is stroke text
// so it rotates and scales with the card's matrix.
void idCard(float cx, float cy, float deg, float scale, float t) {
    pushTransform(cx, cy, deg, scale, scale);
    setColor(withAlpha(INK, 0.3f));
    roundedRect(-70 + 6, -44 - 6, 140, 88, 10);
    setColor(hex(0xffffff));
    roundedRect(-70, -44, 140, 88, 10);
    setColor(BLUE);
    roundedRect(-70, 22, 140, 22, 10);
    rect(-70, 22, 140, 10);
    setColor(GOLD);
    rect(-70, 20, 140, 3);
    // photo with silhouette
    setColor(hex(0xdfe6ee));
    rect(-60, -32, 38, 46);
    setColor(hex(0x9aa7b6));
    circle(-41, -2, 9);
    ellipse(-41, -26, 14, 10);
    // "text" lines
    setColor(hex(0xc7ced8));
    for (int i = 0; i < 4; ++i) rect(-12, 6 - i * 10.0f, 70 - i * 12.0f, 4);
    // barcode
    setColor(INK);
    for (int i = 0; i < 14; ++i) rect(-12 + i * 5.0f, -38, (i % 3 == 0) ? 3.0f : 1.5f, 8);
    // big red question mark
    float bob = std::sin(t * 3.0f) * 4.0f;
    setColor(RED);
    strokeText(28, -30 + bob, 70, "?", 5.0f);
    popTransform();
}

void legendSwatch(float x, float y, const Color& c, const char* label) {
    setColor(withAlpha(c, 0.7f));
    sector(x, y, 18, 60, 120);
    setColor(darker(c, 0.3f));
    arc(x, y, 18, 60, 120, 1.5f);
    setColor(INK);
    text(x + 22, y + 4, label, Font::Normal);
}

}  // namespace

void beginFrame() { g_hits.clear(); }

std::string formatTime(float seconds) {
    int s = static_cast<int>(seconds);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d:%02d", s / 60, s % 60);
    return buf;
}

int buttonAt(Vec2 p) {
    for (const ButtonHit& b : g_hits)
        if (b.r.contains(p)) return b.index;
    return -1;
}

void drawMainMenu(const Items& items, int selected, float t) {
    dim(0.45f);
    // title (stroke font: scalable geometry) with a drop shadow
    const char* title = "Museum Detective Case";
    float th = 64.0f, tw = strokeTextWidth(title, th);
    float pulse = 1.0f + 0.015f * std::sin(t * 2.0f);
    pushTransform(VIEW_W * 0.5f, 455, 0.0f, pulse, pulse);
    setColor(withAlpha(INK, 0.6f));
    strokeText(-tw * 0.5f + 4, -4, th, title, 7.0f);
    setColor(GOLD);
    strokeText(-tw * 0.5f, 0, th, title, 7.0f);
    popTransform();
    setColor(hex(0xffffff));
    textCentered(VIEW_W * 0.5f, 415, "A museum stealth game - you are a detective. Collect evidence. Don't get caught!", Font::Medium);

    idCard(220, 250, -12.0f + std::sin(t * 1.3f) * 4.0f, 1.25f, t);
    // guard portrait on the right, glancing left and right
    pushTransform(790, 160, 0.0f, 4.2f, 4.2f);
    drawCharacter(guardStyle(), std::sin(t * 0.8f) > 0 ? Direction::Left : Direction::Down, 0.0f, 0.0f);
    popTransform();

    buttons(items, selected, VIEW_W * 0.5f, 330, 260, t);
    hint("Up/Down or W/S to choose  -  Enter to select  -  Esc to quit");
}

void drawInstructions(const Items& items, int selected, float t) {
    dim(0.55f);
    const float x = 90, y = 70, w = 820, h = 470;
    panel(x, y, w, h, BLUE, 50);
    setColor(hex(0xffffff));
    textCentered(VIEW_W * 0.5f, y + h - 33, "HOW TO PLAY", Font::Large);

    float cy = y + h - 80;
    setColor(BLUE);
    text(x + 30, cy, "OBJECTIVE", Font::Medium);
    setColor(INK);
    text(x + 30, cy - 22, "Build your detective case file: entrance, artifacts wing, gallery, then the vault.", Font::Normal);
    text(x + 30, cy - 40, "Press F at evidence stands. J opens your case file. Gold marks the exit point.", Font::Normal);

    cy -= 80;
    setColor(BLUE);
    text(x + 30, cy, "CONTROLS", Font::Medium);
    const char* controls[][2] = {{"W A S D / Arrows", "Move"}, {"Shift / Space", "Sprint - fast but noisy"},
                                 {"Mouse / Click", "Aim / throw clink (2)"}, {"F", "Collect / talk / pet"},
                                 {"P / R", "Pause / retry (-100)"}, {"Esc", "Main menu"}};
    for (int i = 0; i < 6; ++i) {
        float rx = x + 30 + (i % 2) * 380, ry = cy - 24 - (i / 2) * 20;
        setColor(BLUE);
        text(rx, ry, controls[i][0], Font::Normal);
        setColor(INK);
        text(rx + 140, ry, controls[i][1], Font::Normal);
    }

    cy -= 110;
    setColor(BLUE);
    text(x + 30, cy, "GUARDS", Font::Medium);
    setColor(INK);
    text(x + 30, cy - 22, "Guards see inside their vision cone. Walls, buildings and monuments block sight.", Font::Normal);
    text(x + 30, cy - 40, "A full detection meter means capture. Guards outrun walking; sprint and use cover.", Font::Normal);
    text(x + 30, cy - 58, "Aim with the mouse and left-click to lure guards away. Break sight and change route.", Font::Normal);
    legendSwatch(x + 60, cy - 100, hex(0xeefa7a), "Calm");
    legendSwatch(x + 250, cy - 100, hex(0xffa53d), "Suspicious ( ? )");
    legendSwatch(x + 500, cy - 100, hex(0xff4b3e), "Pursuit ( ! )");

    buttons(items, selected, VIEW_W * 0.5f, y + 18, 200, t);
}

void drawHud(const HudInfo& h, float t) {
    // These opaque bands are outside the world viewport, never over the campus.
    setColor(INK); rect(0, PLAY_TOP, VIEW_W, VIEW_H - PLAY_TOP); rect(0, 0, VIEW_W, PLAY_BOTTOM);
    setColor(hex(0x3c5366)); rect(0, PLAY_TOP, VIEW_W, 2); rect(0, PLAY_BOTTOM - 2, VIEW_W, 2);
    setColor(hex(0xfffaf0)); text(18, 575, h.levelName, Font::Medium);
    setColor(hex(0xb8cbd5)); text(18, 551, h.objective, Font::Normal);
    setColor(h.timeLeft <= 20 ? hex(0xff8976) : GOLD);
    text(410, 575, "TIME  " + formatTime(std::ceil(h.timeLeft)), Font::Medium);
    setColor(hex(0xb8cbd5)); text(410, 551, "ID stamps: " + std::to_string(h.levelNumber - 1) + " / 4", Font::Normal);
    auto meter = [&](float x, const std::string& label, float value, Color color) {
        setColor(hex(0xe6edf0)); text(x, 578, label, Font::Small);
        setColor(hex(0x344351)); roundedRect(x, 555, 135, 9, 4);
        setColor(color);
        if (value > 0.01f) roundedRect(x, 555, 135 * value, 9, 4);
    };
    meter(655, h.anyAlert ? "SPOTTED - FIND COVER" : "SUSPICION", h.detection,
          h.detection > 0.6f ? hex(0xff8976) : GOLD);
    meter(830, "STAMINA", h.stamina, hex(0x63c7dd));
    setColor(hex(0x83f7ef));
    text(18, 35, "Evidence " + std::to_string(h.passes) + "/" + std::to_string(h.passesNeeded), Font::Medium);
    setColor(GOLD); text(155, 35, "Click: Clink " + std::to_string(h.distractions), Font::Medium);
    setColor(hex(0xffffff));
    text(340, 35, h.talkPrompt.empty() ? "F: Interact" : h.talkPrompt, Font::Normal);
    setColor(hex(0xb8cbd5));
    text(635, 35, "WASD move   Space sprint   J tasks   P pause", Font::Normal);
    text(18, 13, h.message, Font::Small);
    (void)t;
}

void drawBriefing(const Level& level, int stage, const std::vector<bool>& collected, const Items& items, float t) {
    dim(0.55f);
    panel(145, 65, 710, 475, BLUE, 55);
    setColor(PAPER); textCentered(500, 505, "DETECTIVE CASE FILE", Font::Large);
    const char* stamps[] = {"ENTRANCE", "ARTIFACTS", "GALLERY", "VAULT"};
    for (int i = 0; i < 4; ++i) {
        float x = 180 + i * 164;
        setColor(i < stage ? hex(0xd4e3cb) : i == stage ? hex(0xf3dba0) : hex(0xe6e1d6));
        roundedRect(x, 432, 148, 42, 5);
        setColor(i < stage ? GREEN : INK);
        textCentered(x + 74, 450, stamps[i], Font::Normal);
        textCentered(x + 74, 438, i < stage ? "APPROVED" : i == stage ? "IN PROGRESS" : "NEXT", Font::Small);
    }
    setColor(BLUE); text(180, 398, level.objective, Font::Medium);
    setColor(MUTED); text(180, 372, level.briefing, Font::Normal);
    setColor(INK); text(180, 334, "EVIDENCE CHECKLIST", Font::Small);
    for (size_t i = 0; i < level.passNames.size(); ++i) {
        float y = 304 - i * 31;
        setColor(collected[i] ? GREEN : MUTED);
        rectOutline(183, y - 2, 12, 12, 1.5f);
        if (collected[i]) { line(185, y + 3, 188, y); line(188, y, 194, y + 8); }
        text(211, y, level.passNames[i], Font::Medium);
    }
    setColor(BLUE); text(180, 190, "F: interact     Mouse: aim / left-click: clink     J: case file", Font::Normal);
    setColor(MUTED); text(180, 167, "Walk quietly. Sprint to cover if spotted. A full detection meter means capture.", Font::Normal);
    text(180, 147, "The clock waits while you read. Each approval opens the next step.", Font::Normal);
    buttons(items, 0, 500, 86, 240, t);
}

void drawPauseMenu(const Items& items, int selected, float t) {
    dim(0.55f);
    const float w = 340, h = 260, x = VIEW_W * 0.5f - w * 0.5f, y = VIEW_H * 0.5f - h * 0.5f;
    panel(x, y, w, h, BLUE, 50);
    setColor(hex(0xffffff));
    textCentered(VIEW_W * 0.5f, y + h - 33, "PAUSED", Font::Large);
    buttons(items, selected, VIEW_W * 0.5f, y + h - 100, 240, t);
}

void drawGameOver(const Items& items, int selected, float t) {
    dim(0.35f);
    const float w = 700, h = 230, x = VIEW_W * 0.5f - w * 0.5f, y = 40;
    panel(x, y, w, h, RED, 46);
    setColor(hex(0xffffff));
    textCentered(VIEW_W * 0.5f, y + h - 30, "CAUGHT BY SECURITY!", Font::Large);

    // guard portrait (the in-game model scaled up)
    setColor(hex(0xdce7f3));
    circle(x + 100, y + 95, 70);
    pushTransform(x + 100, y + 20, 0.0f, 4.0f, 4.0f);
    drawCharacter(guardStyle(), Direction::Down, 0.0f, 0.0f);
    popTransform();

    // speech bubble with the famous line
    const float bx = x + 200, by = y + 95, bw = 470, bh = 80;
    setColor(hex(0xffffff));
    roundedRect(bx, by, bw, bh, 14);
    triangle({bx + 2, by + 30}, {bx + 2, by + 50}, {bx - 24, by + 34});
    setColor(RED);
    roundedRectOutline(bx, by, bw, bh, 14, 2.5f);
    float shake = std::sin(t * 30.0f) * (std::fmod(t, 2.0f) < 0.3f ? 2.0f : 0.0f);
    textCentered(bx + bw * 0.5f + shake, by + 48, "\"Ei apni ki detective?!\"" , Font::Large);
    setColor(INK);
    textCentered(bx + bw * 0.5f, by + 18, "\"Museum theke ber hote chay, kintu permission chara na!\"", Font::Medium);

    // horizontal buttons
    const float bwid = 200;
    for (size_t i = 0; i < items.size(); ++i) {
        Items one{items[i]};
        buttons(one, static_cast<int>(i) == selected ? 0 : -1, x + 330 + i * (bwid + 30), y + 26, bwid, t,
                static_cast<int>(i));
    }
}

void drawLevelComplete(const std::string& levelName, const LevelResult& r, const Items& items, int selected, float t,
                       const std::string& reward, const std::string& next) {
    dim(0.45f);
    const float w = 520, h = 340, x = VIEW_W * 0.5f - w * 0.5f, y = VIEW_H * 0.5f - h * 0.5f;
    panel(x, y, w, h, GREEN, 50);
    setColor(hex(0xffffff));
    textCentered(VIEW_W * 0.5f, y + h - 33, reward, Font::Medium);
    setColor(INK);
    textCentered(VIEW_W * 0.5f, y + h - 78, levelName, Font::Medium);

    setColor(MUTED); textCentered(500, y + 73, "Next: " + next, Font::Small);

    // stars earned (1-3), popping in one after another (scale animation)
    int stars = r.levelScore >= 1700 ? 3 : (r.levelScore >= 1300 ? 2 : 1);
    for (int i = 0; i < 3; ++i) {
        float k = std::clamp((t - i * 0.25f) * 3.0f, 0.0f, 1.0f);
        float s = k * (1.0f + 0.3f * std::sin(k * PI_F));
        std::vector<Vec2> star;
        for (int j = 0; j < 10; ++j) {
            float a = degToRad(90.0f + j * 36.0f), rad = (j % 2) ? 7.0f : 17.0f;
            star.push_back({std::cos(a) * rad, std::sin(a) * rad});
        }
        pushTransform(VIEW_W * 0.5f + (i - 1) * 50.0f, y + h - 112, 0.0f, s, s);
        setColor(i < stars ? GOLD : hex(0xd8d2c4));
        // star = fan of triangles around the centre (the outline is concave)
        for (int j = 0; j < 10; ++j) triangle({0, 0}, star[j], star[(j + 1) % 10]);
        popTransform();
    }

    // score breakdown
    struct Row {
        const char* label;
        std::string value;
    };
    const Row rows[] = {
        {"Time", formatTime(r.time)},
        {"Level clear", "+1000"},
        {"Time bonus", "+" + std::to_string(r.timeBonus)},
        {"Stealth bonus", "+" + std::to_string(r.stealthBonus)},
        {"Retries", r.retryPenalty ? "-" + std::to_string(r.retryPenalty) : "0"},
    };
    float ry = y + h - 150;
    for (const Row& row : rows) {
        setColor(MUTED);
        text(x + 120, ry, row.label, Font::Normal);
        setColor(INK);
        text(x + w - 120 - textWidth(row.value, Font::Normal), ry, row.value, Font::Normal);
        ry -= 17;
    }
    setColor(BLUE);
    textCentered(VIEW_W * 0.5f, ry - 6,
                 "Level score " + std::to_string(r.levelScore) + "     Total " + std::to_string(r.totalScore),
                 Font::Medium);

    for (size_t i = 0; i < items.size(); ++i) {
        Items one{items[i]};
        buttons(one, static_cast<int>(i) == selected ? 0 : -1, VIEW_W * 0.5f - 120 + i * 240.0f, y + 22, 210, t,
                static_cast<int>(i));
    }
}

void drawVictory(const LevelResult& r, const Items& items, int selected, float t, const std::string& status) {
    dim(0.4f);
    // confetti: procedurally placed rotating quads
    static const Color conf[] = {hex(0xf2c14e), hex(0xe8694a), hex(0x4aa3d9), hex(0x3f9b5a), hex(0xb48be2)};
    for (int i = 0; i < 90; ++i) {
        float sx = hash01(i, 1) * VIEW_W + std::sin(t * 1.5f + i) * 20.0f;
        float speed = 40.0f + hash01(i, 2) * 60.0f;
        float sy = VIEW_H + 20.0f - std::fmod(t * speed + hash01(i, 3) * (VIEW_H + 40.0f), VIEW_H + 40.0f);
        pushTransform(sx, sy, t * (90.0f + hash01(i, 4) * 200.0f), 1.0f, 0.6f + 0.4f * std::sin(t * 4.0f + i));
        setColor(conf[i % 5]);
        rect(-4, -2.5f, 8, 5);
        popTransform();
    }
    const float w = 600, h = 330, x = VIEW_W * 0.5f - w * 0.5f, y = VIEW_H * 0.5f - h * 0.5f;
    panel(x, y, w, h, GREEN, 54);
    setColor(hex(0xffffff));
    textCentered(VIEW_W * 0.5f, y + h - 36, "CONGRATULATIONS!", Font::Large);
    setColor(INK);
    textCentered(VIEW_W * 0.5f, y + h - 86, "Case closed! You recovered the stolen evidence.", Font::Medium);
    setColor(MUTED);
    textCentered(VIEW_W * 0.5f, y + h - 108, "The museum is safe again... for now.", Font::Normal);
    setColor(BLUE);
    textCentered(VIEW_W * 0.5f, y + h - 136,
                 "FINAL SCORE  " + std::to_string(r.totalScore) + "      TOTAL TIME  " + formatTime(r.totalTime),
                 Font::Medium);
    // the detective, jumping for joy (translation + squash/stretch scaling)
    float jump = std::fabs(std::sin(t * 4.0f));
    setColor(MUTED);
    textCentered(VIEW_W * 0.5f, y + h - 160, status, Font::Small);
    pushTransform(VIEW_W * 0.5f, y + 65 + jump * 10.0f, 0.0f, 2.4f * (1.0f - 0.06f * jump), 2.4f * (1.0f + 0.08f * jump));
    drawCharacter(studentStyle(), Direction::Down, t * 10.0f, 0.6f);
    popTransform();
    idCard(x + 90, y + 150, 15.0f + std::sin(t * 2.0f) * 8.0f, 0.55f, t);
    idCard(x + w - 90, y + 150, -15.0f - std::sin(t * 2.0f) * 8.0f, 0.55f, t);
    for (size_t i = 0; i < items.size(); ++i) {
        Items one{items[i]};
        buttons(one, static_cast<int>(i) == selected ? 0 : -1, VIEW_W * 0.5f - 120 + i * 240.0f, y + 26, 210, t,
                static_cast<int>(i));
    }
}

void drawNameEntry(const std::string& name, const Items& items, int selected, float t) {
    dim(0.6f);
    panel(200, 115, 600, 380, BLUE, 55);
    setColor(hex(0xffffff)); textCentered(500, 459, "WHO'S SNEAKING IN?", Font::Large);
    setColor(INK); textCentered(500, 405, "Enter your player name", Font::Medium);
    setColor(hex(0xe5e9ea)); roundedRect(300, 339, 400, 46, 8);
    setColor(BLUE); roundedRectOutline(300, 339, 400, 46, 8, 2);
    textCentered(500, 355, name + (std::fmod(t, 1) < 0.5f ? "_" : ""), Font::Large);
    setColor(MUTED); textCentered(500, 317, "1-18 letters / numbers / spaces / - / _   |   Backspace to edit", Font::Small);
    textCentered(500, 285, "Get a temporary ID. Outsmart security. Make it to class.", Font::Medium);
    textCentered(500, 260, "Complete all four levels to join the local top 10.", Font::Normal);
    buttons(items, selected, 500, 195, 260, t);
    hint("Type your name - Enter to start - Arrow keys choose buttons - Esc to go back");
}

void drawScoreboard(const std::vector<ScoreEntry>& entries, const std::string& status, const Items& items, int selected, float t) {
    dim(0.6f);
    panel(160, 45, 680, 510, BLUE, 55);
    setColor(hex(0xffffff)); textCentered(500, 518, "CAMPUS TOP 10", Font::Large);
    setColor(MUTED); textCentered(500, 477, "Completed runs  |  Highest score first, fastest time breaks ties", Font::Normal);
    setColor(BLUE);
    text(200, 443, "RANK", Font::Small); text(275, 443, "PLAYER", Font::Small);
    text(605, 443, "SCORE", Font::Small); text(725, 443, "TIME", Font::Small);
    if (entries.empty()) {
        setColor(INK); textCentered(500, 330, "No completed runs yet. The first spot could be yours!", Font::Medium);
    }
    for (size_t i = 0; i < entries.size(); ++i) {
        float y = 411 - i * 27;
        setColor(i == 0 ? hex(0xf8e7a9) : (i % 2 ? hex(0xeee9df) : PAPER));
        roundedRect(185, y - 7, 630, 25, 4);
        setColor(INK);
        text(210, y, std::to_string(i + 1), Font::Normal);
        text(275, y, entries[i].name, Font::Medium);
        text(605, y, std::to_string(entries[i].score), Font::Normal);
        text(725, y, formatTime(entries[i].seconds), Font::Normal);
    }
    setColor(MUTED); textCentered(500, 125, status.empty() ? "Saved on this computer. Every run needs a player name." : status, Font::Small);
    buttons(items, selected, 500, 65, 220, t);
}

void drawDialogue(const std::string& name, const std::string& role, const std::string& page,
                  size_t pageIndex, size_t pageCount, size_t revealed, const CampusNpc* npc, int cat, float t) {
    // A compact RPG text box: one speech beat, a portrait, and an explicit next prompt.
    panel(24, 70, 952, 172, BLUE, 0);
    setColor(BLUE); roundedRectOutline(30, 76, 940, 160, 12, 3);
    setColor(hex(0xdce9e6)); roundedRect(44, 92, 112, 126, 10);
    pushTransform(100, 113, 0, 2.2f, 2.2f);
    if (npc) {
        CampusNpc portrait = *npc; portrait.position = {0, 0};
        drawCampusNpc(portrait, t);
    } else drawCampusCat({{0, 5}, {0, -1}, false, true}, t, cat);
    popTransform();
    setColor(BLUE); text(178, 211, name, Font::Medium);
    setColor(MUTED); text(178 + textWidth(name, Font::Medium) + 20, 212, role, Font::Small);
    setColor(INK);
    // Wrap the complete page first so words don't jump as letters appear.
    size_t start = 0;
    float y = 179;
    while (start < page.size()) {
        size_t end = start;
        size_t best = start;
        while (end < page.size()) {
            size_t space = page.find(' ', end);
            size_t candidate = space == std::string::npos ? page.size() : space;
            if (textWidth(page.substr(start, candidate - start), Font::Medium) > 754 && best > start) break;
            best = candidate;
            if (space == std::string::npos) break;
            end = space + 1;
        }
        if (revealed > start) text(178, y, page.substr(start, std::min(best, revealed) - start), Font::Medium);
        y -= 25;
        start = best + 1;
    }
    bool ready = revealed >= page.size();
    setColor(MUTED);
    text(178, 94, "F / Enter / Space / click: " + std::string(!ready ? "reveal text" : pageIndex + 1 < pageCount ? "next" : "finish") +
         "     Esc: leave", Font::Small);
    setColor(BLUE);
    if (pageCount > 1) text(865, 94, std::to_string(pageIndex + 1) + " / " + std::to_string(pageCount), Font::Small);
    if (ready) {
        float bob = std::sin(t * 5) * 2;
        triangle({936, 105 + bob}, {948, 105 + bob}, {942, 97 + bob});
    }
}

void drawTimeout(const Items& items, int selected, float t) {
    dim(0.55f);
    panel(240, 150, 520, 300, RED, 54);
    setColor(hex(0xffffff)); textCentered(500, 413, "THE BELL RANG!", Font::Large);
    setColor(INK); textCentered(500, 360, "Too late for class. Try a faster route.", Font::Medium);
    setColor(MUTED); textCentered(500, 328, "Use distractions to open a gap in the patrols.", Font::Normal);
    textCentered(500, 305, "Retry costs 100 points; failed time counts for the scoreboard.", Font::Small);
    buttons(items, selected, 500, 230, 260, t);
}

}  // namespace ui
