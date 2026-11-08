// ============================================================
// ui.cpp
// Menus, HUD and result screens. Coordinates here are view
// units: (0,0) bottom-left, (1000,600) top-right.
// ============================================================

#include "game.h"

#define INK 0x1d2430
#define PAPER 0xfffaf0
#define BLUE 0x2f5f8c
#define GOLD 0xf2c14e
#define RED 0xd63c3c
#define GREEN 0x3f9b5a
#define MUTED 0x5b6470

// ============================================================
// BUTTONS (remembered every frame so the mouse can click them)
// ============================================================

struct ButtonHit
{
    Rect r;
    int index;
};

ButtonHit hits[8];
int hitCount = 0;

void beginButtons()
{
    hitCount = 0;
}

int buttonAt(float x, float y)
{
    for (int i = 0; i < hitCount; i++)
        if (rectContains(hits[i].r, makeVec(x, y)))
            return hits[i].index;
    return -1;
}

// One button. The selected one is gold and gently pulses (scale).
void drawButton(const char *label, bool sel, float cx, float y, float w, int index)
{
    float h = 36;
    if (hitCount < 8)
    {
        hits[hitCount].r = (Rect){cx - w * 0.5f, y, w, h};
        hits[hitCount].index = index;
        hitCount++;
    }
    float s = sel ? 1 + 0.03f * sinf(uiTime * 6) : 1;
    glPushMatrix();
    glTranslatef(cx, y + h * 0.5f, 0);
    glScalef(s, s, 1);
    setHex(INK, 0.25f);
    DrawRoundRect(-w * 0.5f + 3, -h * 0.5f - 3, w, h, 10);
    setHex(sel ? GOLD : 0xe9e2d2);
    DrawRoundRect(-w * 0.5f, -h * 0.5f, w, h, 10);
    setHex(sel ? 0x8a6a10 : 0xb9b0a0);
    DrawRoundRectOutline(-w * 0.5f, -h * 0.5f, w, h, 10, 2);
    if (sel) // little arrow
    {
        setHex(INK);
        DrawTriangle(-w * 0.5f + 14, -6, -w * 0.5f + 14, 6, -w * 0.5f + 24, 0);
    }
    glPopMatrix();
    setHex(INK);
    DrawTextCentered(cx, y + 12, label, FONT_MEDIUM);
}

// all menu items in a column
void drawButtonColumn(float cx, float topY, float w)
{
    for (int i = 0; i < menuItemCount(); i++)
        drawButton(menuItem(i), i == selected, cx, topY - i * 48, w, i);
}

// all menu items in a row
void drawButtonRow(float firstX, float y, float w, float gap)
{
    for (int i = 0; i < menuItemCount(); i++)
        drawButton(menuItem(i), i == selected, firstX + i * gap, y, w, i);
}

// ============================================================
// SMALL HELPERS
// ============================================================

void dim(float a)
{
    setHex(0x101520, a);
    DrawRect(0, 0, VIEW_W, VIEW_H);
}

void panel(float x, float y, float w, float h, unsigned int header, float headerH)
{
    setHex(INK, 0.3f);
    DrawRoundRect(x + 6, y - 6, w, h, 16);
    setHex(PAPER);
    DrawRoundRect(x, y, w, h, 16);
    if (headerH > 0)
    {
        setHex(header);
        DrawRoundRect(x, y + h - headerH, w, headerH, 16);
        DrawRect(x, y + h - headerH, w, 16); // square lower corners
    }
}

void hint(const char *s)
{
    setHex(0xffffff, 0.85f);
    DrawTextCentered(VIEW_W * 0.5f, 14, s, FONT_SMALL);
}

// Draws a character scaled up for a portrait
void portrait(int style, int facing, float x, float y, float scale, float phase, float blend)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    useStyle(style);
    drawCharacter(facing, phase, blend);
    glPopMatrix();
}

// Museum entry ticket made of shapes: header band, museum icon,
// "ADMIT ONE", barcode and a tear-off stub with a "?".
void museumTicket(float cx, float cy, float deg, float scale)
{
    unsigned int maroon = 0x8c2f39;
    glPushMatrix();
    glTranslatef(cx, cy, 0);
    glRotatef(deg, 0, 0, 1);
    glScalef(scale, scale, 1);

    setHex(INK, 0.3f);
    DrawRoundRect(-64, -50, 140, 88, 8);
    setHex(0xf7eccf);
    DrawRoundRect(-70, -44, 140, 88, 8);

    // header band
    setHex(maroon);
    DrawRoundRect(-70, 20, 104, 24, 8);
    DrawRect(-62, 20, 96, 24);
    setHex(GOLD);
    DrawRect(-70, 18, 104, 2);
    setHex(0xfff6e0);
    DrawStrokeText(-62, 26, 12, "MUSEUM", 2);

    // museum icon: roof, four columns, base
    setHex(maroon);
    DrawTriangle(-64, 0, -36, 0, -50, 11);
    for (int i = 0; i < 4; i++)
        DrawRect(-62 + i * 7.0f, -18, 3, 16);
    DrawRect(-65, -22, 30, 4);

    setHex(INK);
    DrawStrokeText(-28, 0, 10, "ADMIT ONE", 1.6f);
    setHex(0xc9bfa6);
    DrawRect(-28, -10, 54, 3);
    DrawRect(-28, -17, 40, 3);

    // barcode
    setHex(INK);
    for (int i = 0; i < 16; i++)
        DrawRect(-64 + i * 5.5f, -38, (i % 3 == 0) ? 3 : 1.5f, 10);

    // tear-off stub: dashed line and punched holes
    setHex(0xb9ad92);
    for (int i = 0; i < 9; i++)
        DrawRect(33, -40 + i * 9.5f, 2, 5);
    setHex(INK, 0.55f);
    circle(34, 44, 5);
    circle(34, -44, 5);

    // bobbing red question mark
    float bob = sinf(uiTime * 3) * 4;
    setHex(RED);
    DrawStrokeText(40, -26 + bob, 52, "?", 5);
    glPopMatrix();
}

void legendSwatch(float x, float y, unsigned int c, const char *label)
{
    setColor(hexColor(c, 0.7f));
    DrawSector(x, y, 18, 60, 120);
    setColor(darker(hexColor(c), 0.3f));
    DrawArc(x, y, 18, 60, 120, 1.5f);
    setHex(INK);
    DrawText(x + 22, y + 4, label, FONT_NORMAL);
}

// ============================================================
// MAIN MENU
// ============================================================

void drawMainMenu()
{
    dim(0.45f);

    // title in stroke font with a shadow, slowly pulsing
    const char *title = "Museum Detective Case";
    float th = 64, tw = StrokeTextWidth(title, th);
    float pulse = 1 + 0.015f * sinf(uiTime * 2);
    glPushMatrix();
    glTranslatef(VIEW_W * 0.5f, 455, 0);
    glScalef(pulse, pulse, 1);
    setHex(INK, 0.6f);
    DrawStrokeText(-tw * 0.5f + 4, -4, th, title, 7);
    setHex(GOLD);
    DrawStrokeText(-tw * 0.5f, 0, th, title, 7);
    glPopMatrix();

    setHex(0xffffff);
    DrawTextCentered(VIEW_W * 0.5f, 415, "A museum stealth game - you are a detective. Collect evidence. Don't get caught!", FONT_MEDIUM);

    museumTicket(220, 250, -12 + sinf(uiTime * 1.3f) * 4, 1.25f);

    // big guard on the right, looking left and down
    portrait(STYLE_GUARD, sinf(uiTime * 0.8f) > 0 ? DIR_LEFT : DIR_DOWN, 790, 160, 4.2f, 0, 0);

    drawButtonColumn(VIEW_W * 0.5f, 330, 260);
    hint("Up/Down or W/S to choose  -  Enter to select  -  Esc to quit");
}

// ============================================================
// NAME ENTRY
// ============================================================

void drawNameEntry()
{
    dim(0.6f);
    panel(200, 115, 600, 380, BLUE, 55);
    setHex(0xffffff);
    DrawTextCentered(500, 459, "WHO'S SNEAKING IN?", FONT_LARGE);
    setHex(INK);
    DrawTextCentered(500, 405, "Enter your player name", FONT_MEDIUM);

    setHex(0xe5e9ea);
    DrawRoundRect(300, 339, 400, 46, 8);
    setHex(BLUE);
    DrawRoundRectOutline(300, 339, 400, 46, 8, 2);
    char shown[24];
    sprintf(shown, "%s%s", nameDraft, fmodf(uiTime, 1) < 0.5f ? "_" : ""); // blinking cursor
    DrawTextCentered(500, 355, shown, FONT_LARGE);

    setHex(MUTED);
    DrawTextCentered(500, 317, "1-18 letters / numbers / spaces / - / _   |   Backspace to edit", FONT_SMALL);
    DrawTextCentered(500, 285, "Find the evidence. Outsmart security. Crack the case.", FONT_MEDIUM);
    DrawTextCentered(500, 260, "Complete all four levels to join the local top 10.", FONT_NORMAL);
    drawButtonColumn(500, 195, 260);
    hint("Type your name - Enter to start - Arrow keys choose buttons - Esc to go back");
}

// ============================================================
// SCOREBOARD
// ============================================================

void drawScoreboard()
{
    char buf[40];
    dim(0.6f);
    panel(160, 45, 680, 510, BLUE, 55);
    setHex(0xffffff);
    DrawTextCentered(500, 518, "DETECTIVE TOP 10", FONT_LARGE);
    setHex(MUTED);
    DrawTextCentered(500, 477, "Completed runs  |  Highest score first, fastest time breaks ties", FONT_NORMAL);
    setHex(BLUE);
    DrawText(200, 443, "RANK", FONT_SMALL);
    DrawText(275, 443, "PLAYER", FONT_SMALL);
    DrawText(605, 443, "SCORE", FONT_SMALL);
    DrawText(725, 443, "TIME", FONT_SMALL);

    if (scoreCount == 0)
    {
        setHex(INK);
        DrawTextCentered(500, 330, "No completed runs yet. The first spot could be yours!", FONT_MEDIUM);
    }
    for (int i = 0; i < scoreCount && i < 10; i++)
    {
        float y = 411 - i * 27;
        setHex(i == 0 ? 0xf8e7a9 : (i % 2 ? 0xeee9df : PAPER));
        DrawRoundRect(185, y - 7, 630, 25, 4);
        setHex(INK);
        sprintf(buf, "%d", i + 1);
        DrawText(210, y, buf, FONT_NORMAL);
        DrawText(275, y, scores[i].name, FONT_MEDIUM);
        sprintf(buf, "%d", scores[i].score);
        DrawText(605, y, buf, FONT_NORMAL);
        formatTime(scores[i].seconds, buf);
        DrawText(725, y, buf, FONT_NORMAL);
    }
    setHex(MUTED);
    DrawTextCentered(500, 125, scoreStatus[0] ? scoreStatus : "Saved on this computer. Every run needs a player name.", FONT_SMALL);
    drawButtonColumn(500, 65, 220);
}

// ============================================================
// INSTRUCTIONS
// ============================================================

void drawInstructions()
{
    float x = 90, y = 70, w = 820, h = 470;
    dim(0.55f);
    panel(x, y, w, h, BLUE, 50);
    setHex(0xffffff);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 33, "HOW TO PLAY", FONT_LARGE);

    float cy = y + h - 80;
    setHex(BLUE);
    DrawText(x + 30, cy, "OBJECTIVE", FONT_MEDIUM);
    setHex(INK);
    DrawText(x + 30, cy - 22, "Build your detective case file: entrance, artifacts wing, gallery, then the vault.", FONT_NORMAL);
    DrawText(x + 30, cy - 40, "Press F at evidence stands. J opens your case file. Gold marks the exit point.", FONT_NORMAL);

    cy -= 80;
    setHex(BLUE);
    DrawText(x + 30, cy, "CONTROLS", FONT_MEDIUM);
    const char *keys[6] = {"W A S D / Arrows", "Shift / Space", "Mouse / Click", "F", "P / R", "Esc"};
    const char *actions[6] = {"Move", "Sprint - fast but noisy", "Aim / throw clink (2)",
                              "Collect / talk / pet", "Pause / retry (-100)", "Main menu"};
    for (int i = 0; i < 6; i++)
    {
        float rx = x + 30 + (i % 2) * 380, ry = cy - 24 - (i / 2) * 20;
        setHex(BLUE);
        DrawText(rx, ry, keys[i], FONT_NORMAL);
        setHex(INK);
        DrawText(rx + 140, ry, actions[i], FONT_NORMAL);
    }

    cy -= 110;
    setHex(BLUE);
    DrawText(x + 30, cy, "GUARDS", FONT_MEDIUM);
    setHex(INK);
    DrawText(x + 30, cy - 22, "Guards see inside their vision cone. Walls, buildings and monuments block sight.", FONT_NORMAL);
    DrawText(x + 30, cy - 40, "A full detection meter means capture. Guards outrun walking; sprint and use cover.", FONT_NORMAL);
    DrawText(x + 30, cy - 58, "Aim with the mouse and left-click to lure guards away. Break sight and change route.", FONT_NORMAL);
    legendSwatch(x + 60, cy - 100, 0xeefa7a, "Calm");
    legendSwatch(x + 250, cy - 100, 0xffa53d, "Suspicious ( ? )");
    legendSwatch(x + 500, cy - 100, 0xff4b3e, "Pursuit ( ! )");

    drawButtonColumn(VIEW_W * 0.5f, y + 18, 200);
}

// ============================================================
// HUD (top and bottom bands while playing)
// ============================================================

void drawMeter(float x, const char *label, float value, unsigned int c)
{
    setHex(0xe6edf0);
    DrawText(x, 578, label, FONT_SMALL);
    setHex(0x344351);
    DrawRoundRect(x, 555, 135, 9, 4);
    setHex(c);
    if (value > 0.01f)
        DrawRoundRect(x, 555, 135 * value, 9, 4);
}

void drawHud()
{
    char buf[200];
    setHex(INK);
    DrawRect(0, PLAY_TOP, VIEW_W, VIEW_H - PLAY_TOP);
    DrawRect(0, 0, VIEW_W, PLAY_BOTTOM);
    setHex(0x3c5366);
    DrawRect(0, PLAY_TOP, VIEW_W, 2);
    DrawRect(0, PLAY_BOTTOM - 2, VIEW_W, 2);

    // ---- top band ----
    setHex(0xfffaf0);
    DrawText(18, 575, level.name, FONT_MEDIUM);
    setHex(0xb8cbd5);
    DrawText(18, 551, level.objective, FONT_NORMAL);

    float left = remainingTime();
    char t[16];
    formatTime(ceilf(left), t);
    setHex(left <= 20 ? 0xff8976 : GOLD);
    sprintf(buf, "TIME  %s", t);
    DrawText(410, 575, buf, FONT_MEDIUM);
    setHex(0xb8cbd5);
    sprintf(buf, "Ticket stamps: %d / 4", levelIndex);
    DrawText(410, 551, buf, FONT_NORMAL);

    float detection = 0;
    bool anyAlert = false;
    for (int i = 0; i < guardCount; i++)
    {
        detection = maxf(detection, guards[i].detection);
        if (guards[i].state == ALERT)
            anyAlert = true;
    }
    drawMeter(655, anyAlert ? "SPOTTED - FIND COVER" : "SUSPICION", detection, detection > 0.6f ? 0xff8976 : GOLD);
    drawMeter(830, "STAMINA", player.stamina, 0x63c7dd);

    // ---- bottom band ----
    setHex(0x83f7ef);
    sprintf(buf, "Evidence %d/%d", collectedCount(), level.passCount);
    DrawText(18, 35, buf, FONT_MEDIUM);
    setHex(GOLD);
    sprintf(buf, "Click: Clink %d", distractions);
    DrawText(155, 35, buf, FONT_MEDIUM);

    // what F does right now
    strcpy(buf, "F: Interact");
    int who = nearbyCharacter();
    if (who >= 0)
    {
        if (who < NPC_COUNT)
            sprintf(buf, "F: Talk to %s", npcs[who].name);
        else
            strcpy(buf, "F: Pet the cat");
    }
    int pass = nearbyPass();
    if (pass >= 0)
        sprintf(buf, "F: Take %s", level.passNames[pass]);
    setHex(0xffffff);
    DrawText(340, 35, buf, FONT_NORMAL);
    setHex(0xb8cbd5);
    DrawText(635, 35, "WASD move   Space sprint   J tasks   P pause", FONT_NORMAL);

    const char *msg = messageTime > 0 ? message : "Keep the detection meter from filling. Sprint to escape; hide to lose pursuit. J: case file.";
    if (awaitingDeparture)
        msg = "READY WHEN YOU ARE - move or interact to begin. Patrols and the clock are paused.";
    DrawText(18, 13, msg, FONT_SMALL);
}

// ============================================================
// BRIEFING (case file, J key)
// ============================================================

void drawBriefing()
{
    dim(0.55f);
    panel(145, 65, 710, 475, BLUE, 55);
    setHex(PAPER);
    DrawTextCentered(500, 505, "DETECTIVE CASE FILE", FONT_LARGE);

    const char *stamps[4] = {"ENTRANCE", "ARTIFACTS", "GALLERY", "VAULT"};
    for (int i = 0; i < 4; i++)
    {
        float x = 180 + i * 164;
        setHex(i < levelIndex ? 0xd4e3cb : (i == levelIndex ? 0xf3dba0 : 0xe6e1d6));
        DrawRoundRect(x, 432, 148, 42, 5);
        setHex(i < levelIndex ? GREEN : INK);
        DrawTextCentered(x + 74, 450, stamps[i], FONT_NORMAL);
        DrawTextCentered(x + 74, 438, i < levelIndex ? "SOLVED" : (i == levelIndex ? "IN PROGRESS" : "NEXT"), FONT_SMALL);
    }
    setHex(BLUE);
    DrawText(180, 398, level.objective, FONT_MEDIUM);
    setHex(MUTED);
    DrawText(180, 372, level.briefing, FONT_NORMAL);

    setHex(INK);
    DrawText(180, 334, "EVIDENCE CHECKLIST", FONT_SMALL);
    for (int i = 0; i < level.passCount; i++)
    {
        float y = 304 - i * 31;
        setHex(collected[i] ? GREEN : MUTED);
        DrawRectOutline(183, y - 2, 12, 12, 1.5f);
        if (collected[i]) // tick
        {
            DrawLine(185, y + 3, 188, y, 1.5f);
            DrawLine(188, y, 194, y + 8, 1.5f);
        }
        DrawText(211, y, level.passNames[i], FONT_MEDIUM);
    }
    setHex(BLUE);
    DrawText(180, 190, "F: interact     Mouse: aim / left-click: clink     J: case file", FONT_NORMAL);
    setHex(MUTED);
    DrawText(180, 167, "Walk quietly. Sprint to cover if spotted. A full detection meter means capture.", FONT_NORMAL);
    DrawText(180, 147, "The clock waits while you read. Each solved stage opens the next one.", FONT_NORMAL);
    drawButton(menuItem(0), true, 500, 86, 240, 0);
}

// ============================================================
// PAUSE
// ============================================================

void drawPauseMenu()
{
    float w = 340, h = 260, x = VIEW_W * 0.5f - w * 0.5f, y = VIEW_H * 0.5f - h * 0.5f;
    dim(0.55f);
    panel(x, y, w, h, BLUE, 50);
    setHex(0xffffff);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 33, "PAUSED", FONT_LARGE);
    drawButtonColumn(VIEW_W * 0.5f, y + h - 100, 240);
}

// ============================================================
// DIALOGUE BOX
// ============================================================

void drawDialogue()
{
    panel(24, 70, 952, 172, BLUE, 0);
    setHex(BLUE);
    DrawRoundRectOutline(30, 76, 940, 160, 12, 3);
    setHex(0xdce9e6);
    DrawRoundRect(44, 92, 112, 126, 10);

    // portrait of the speaker
    glPushMatrix();
    glTranslatef(100, 113, 0);
    glScalef(2.2f, 2.2f, 1);
    drawNpc(dialogueCharacter, 0, 0, uiTime);
    glPopMatrix();

    setHex(BLUE);
    DrawText(178, 211, speakerName, FONT_MEDIUM);
    setHex(MUTED);
    DrawText(178 + TextWidth(speakerName, FONT_MEDIUM) + 20, 212, speakerRole, FONT_SMALL);

    // word wrap the full text first, then show only the revealed letters
    int len = strlen(dialogueText);
    int revealed = (int)(dialogueTime * 42);
    int start = 0;
    float y = 179;
    setHex(INK);
    while (start < len)
    {
        int best = start;
        int end = start;
        while (end <= len)
        {
            // find the next space (or the end of the text)
            int space = end;
            while (space < len && dialogueText[space] != ' ')
                space++;
            char part[300];
            strncpy(part, dialogueText + start, space - start);
            part[space - start] = '\0';
            if (TextWidth(part, FONT_MEDIUM) > 754 && best > start)
                break;
            best = space;
            if (space >= len)
                break;
            end = space + 1;
        }
        if (revealed > start)
        {
            int stop = best < revealed ? best : revealed;
            char line[300];
            strncpy(line, dialogueText + start, stop - start);
            line[stop - start] = '\0';
            DrawText(178, y, line, FONT_MEDIUM);
        }
        y -= 25;
        start = best + 1;
    }

    bool ready = revealed >= len;
    setHex(MUTED);
    DrawText(178, 94, ready ? "F / Enter / Space / click: finish     Esc: leave"
                            : "F / Enter / Space / click: reveal text     Esc: leave", FONT_SMALL);
    if (ready)
    {
        float bob = sinf(uiTime * 5) * 2;
        setHex(BLUE);
        DrawTriangle(936, 105 + bob, 948, 105 + bob, 942, 97 + bob);
    }
}

// ============================================================
// CAUGHT / TIME'S UP
// ============================================================

void drawGameOver()
{
    float t = screenTime;
    float w = 700, h = 230, x = VIEW_W * 0.5f - w * 0.5f, y = 40;
    dim(0.35f);
    panel(x, y, w, h, RED, 46);
    setHex(0xffffff);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 30, "CAUGHT BY SECURITY!", FONT_LARGE);

    setHex(0xdce7f3);
    circle(x + 100, y + 95, 70);
    portrait(STYLE_GUARD, DIR_DOWN, x + 100, y + 20, 4, 0, 0);

    // speech bubble
    float bx = x + 200, by = y + 95, bw = 470, bh = 80;
    setHex(0xffffff);
    DrawRoundRect(bx, by, bw, bh, 14);
    DrawTriangle(bx + 2, by + 30, bx + 2, by + 50, bx - 24, by + 34);
    setHex(RED);
    DrawRoundRectOutline(bx, by, bw, bh, 14, 2.5f);
    float shake = sinf(t * 30) * (fmodf(t, 2) < 0.3f ? 2 : 0);
    DrawTextCentered(bx + bw * 0.5f + shake, by + 48, "\"Ei apni ki detective?!\"", FONT_LARGE);
    setHex(INK);
    DrawTextCentered(bx + bw * 0.5f, by + 18, "\"Museum theke ber hote chay, kintu permission chara na!\"", FONT_MEDIUM);

    drawButtonRow(x + 330, y + 26, 200, 230);
}

void drawTimeout()
{
    dim(0.55f);
    panel(240, 150, 520, 300, RED, 54);
    setHex(0xffffff);
    DrawTextCentered(500, 413, "TIME'S UP!", FONT_LARGE);
    setHex(INK);
    DrawTextCentered(500, 360, "The trail went cold. Try a faster route.", FONT_MEDIUM);
    setHex(MUTED);
    DrawTextCentered(500, 328, "Use distractions to open a gap in the patrols.", FONT_NORMAL);
    DrawTextCentered(500, 305, "Retry costs 100 points; failed time counts for the scoreboard.", FONT_SMALL);
    drawButtonColumn(500, 230, 260);
}

// ============================================================
// LEVEL COMPLETE (stars + score breakdown)
// ============================================================

void drawStar(float cx, float cy, float s, unsigned int c)
{
    glPushMatrix();
    glTranslatef(cx, cy, 0);
    glScalef(s, s, 1);
    setHex(c);
    // a star is a fan of 10 triangles (its outline is not convex)
    for (int j = 0; j < 10; j++)
    {
        float a1 = degToRad(90 + j * 36), a2 = degToRad(90 + (j + 1) * 36);
        float r1 = (j % 2) ? 7 : 17, r2 = ((j + 1) % 2) ? 7 : 17;
        DrawTriangle(0, 0, cosf(a1) * r1, sinf(a1) * r1, cosf(a2) * r2, sinf(a2) * r2);
    }
    glPopMatrix();
}

void drawLevelComplete()
{
    char buf[120], t[16];
    float w = 520, h = 340, x = VIEW_W * 0.5f - w * 0.5f, y = VIEW_H * 0.5f - h * 0.5f;
    dim(0.45f);
    panel(x, y, w, h, GREEN, 50);
    setHex(0xffffff);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 33, level.reward, FONT_MEDIUM);
    setHex(INK);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 78, level.name, FONT_MEDIUM);
    setHex(MUTED);
    sprintf(buf, "Next: %s", levelObjective(levelIndex + 1));
    DrawTextCentered(500, y + 73, buf, FONT_SMALL);

    // 1-3 stars pop in one after another
    int stars = result.levelScore >= 1700 ? 3 : (result.levelScore >= 1300 ? 2 : 1);
    for (int i = 0; i < 3; i++)
    {
        float k = clampf((screenTime - i * 0.25f) * 3, 0, 1);
        float s = k * (1 + 0.3f * sinf(k * PI));
        drawStar(VIEW_W * 0.5f + (i - 1) * 50, y + h - 112, s, i < stars ? GOLD : 0xd8d2c4);
    }

    // score breakdown
    const char *labels[5] = {"Time", "Level clear", "Time bonus", "Stealth bonus", "Retries"};
    char values[5][20];
    formatTime(result.time, values[0]);
    strcpy(values[1], "+1000");
    sprintf(values[2], "+%d", result.timeBonus);
    sprintf(values[3], "+%d", result.stealthBonus);
    if (result.retryPenalty)
        sprintf(values[4], "-%d", result.retryPenalty);
    else
        strcpy(values[4], "0");
    float ry = y + h - 150;
    for (int i = 0; i < 5; i++)
    {
        setHex(MUTED);
        DrawText(x + 120, ry, labels[i], FONT_NORMAL);
        setHex(INK);
        DrawText(x + w - 120 - TextWidth(values[i], FONT_NORMAL), ry, values[i], FONT_NORMAL);
        ry -= 17;
    }
    setHex(BLUE);
    sprintf(buf, "Level score %d     Total %d", result.levelScore, result.totalScore);
    DrawTextCentered(VIEW_W * 0.5f, ry - 6, buf, FONT_MEDIUM);
    (void)t;

    drawButtonRow(VIEW_W * 0.5f - 120, y + 22, 210, 240);
}

// ============================================================
// VICTORY
// ============================================================

void drawVictory()
{
    float t = screenTime;
    char buf[120], tt[16];
    dim(0.4f);

    // confetti: falling, spinning little rectangles
    unsigned int conf[5] = {0xf2c14e, 0xe8694a, 0x4aa3d9, 0x3f9b5a, 0xb48be2};
    for (int i = 0; i < 90; i++)
    {
        float sx = hash01(i, 1) * VIEW_W + sinf(t * 1.5f + i) * 20;
        float speed = 40 + hash01(i, 2) * 60;
        float sy = VIEW_H + 20 - fmodf(t * speed + hash01(i, 3) * (VIEW_H + 40), VIEW_H + 40);
        glPushMatrix();
        glTranslatef(sx, sy, 0);
        glRotatef(t * (90 + hash01(i, 4) * 200), 0, 0, 1);
        glScalef(1, 0.6f + 0.4f * sinf(t * 4 + i), 1);
        setHex(conf[i % 5]);
        DrawRect(-4, -2.5f, 8, 5);
        glPopMatrix();
    }

    float w = 600, h = 330, x = VIEW_W * 0.5f - w * 0.5f, y = VIEW_H * 0.5f - h * 0.5f;
    panel(x, y, w, h, GREEN, 54);
    setHex(0xffffff);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 36, "CONGRATULATIONS!", FONT_LARGE);
    setHex(INK);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 86, "Case closed! You recovered the stolen evidence.", FONT_MEDIUM);
    setHex(MUTED);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 108, "The museum is safe again... for now.", FONT_NORMAL);
    setHex(BLUE);
    formatTime(result.totalTime, tt);
    sprintf(buf, "FINAL SCORE  %d      TOTAL TIME  %s", result.totalScore, tt);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 136, buf, FONT_MEDIUM);
    setHex(MUTED);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 160, scoreStatus, FONT_SMALL);

    // the detective jumping for joy (squash and stretch)
    float jump = fabsf(sinf(t * 4));
    glPushMatrix();
    glTranslatef(VIEW_W * 0.5f, y + 65 + jump * 10, 0);
    glScalef(2.4f * (1 - 0.06f * jump), 2.4f * (1 + 0.08f * jump), 1);
    useStyle(STYLE_DETECTIVE);
    drawCharacter(DIR_DOWN, t * 10, 0.6f);
    glPopMatrix();

    museumTicket(x + 90, y + 150, 15 + sinf(t * 2) * 8, 0.55f);
    museumTicket(x + w - 90, y + 150, -15 - sinf(t * 2) * 8, 0.55f);

    drawButtonRow(VIEW_W * 0.5f - 120, y + 26, 210, 240);
}
