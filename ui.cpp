#include "game.h"

#define INK 0x1d2430
#define PAPER 0xfffaf0
#define BLUE 0x2f5f8c
#define GOLD 0xf2c14e
#define RED 0xd63c3c
#define GREEN 0x3f9b5a
#define MUTED 0x5b6470

#define MAX_BUTTONS 8
#define BUTTON_HEIGHT 36

struct ButtonHit
{
    Rect r;
    int index;
};

static ButtonHit hits[MAX_BUTTONS];
static int hitCount = 0;

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

static void rememberButton(float left, float bottom, float width, int index)
{
    if (hitCount >= MAX_BUTTONS)
        return;
    hits[hitCount].r.x = left;
    hits[hitCount].r.y = bottom;
    hits[hitCount].r.w = width;
    hits[hitCount].r.h = BUTTON_HEIGHT;
    hits[hitCount].index = index;
    hitCount++;
}

static void drawButtonShape(bool sel, float w, float h)
{
    setHex(INK, 0.25f);
    DrawRoundRect(-w * 0.5f + 3, -h * 0.5f - 3, w, h, 10);

    if (sel)
        setHex(GOLD);
    else
        glColor3ub(233, 226, 210);
    DrawRoundRect(-w * 0.5f, -h * 0.5f, w, h, 10);

    if (sel)
        glColor3ub(138, 106, 16);
    else
        glColor3ub(185, 176, 160);
    DrawRoundRectOutline(-w * 0.5f, -h * 0.5f, w, h, 10, 2);

    if (sel)
    {
        setHex(INK);
        DrawTriangle(-w * 0.5f + 14, -6, -w * 0.5f + 14, 6, -w * 0.5f + 24, 0);
    }
}

static void drawButton(const char *label, bool sel, float cx, float y, float w, int index)
{
    float h = BUTTON_HEIGHT;
    rememberButton(cx - w * 0.5f, y, w, index);

    float scale = 1;
    if (sel)
        scale = 1 + 0.03f * sinf(uiTime * 6);

    glPushMatrix();
    glTranslatef(cx, y + h * 0.5f, 0);
    glScalef(scale, scale, 1);
    drawButtonShape(sel, w, h);
    glPopMatrix();

    setHex(INK);
    DrawTextCentered(cx, y + 12, label, FONT_MEDIUM);
}

static void drawButtonColumn(float cx, float topY, float w)
{
    for (int i = 0; i < menuItemCount(); i++)
        drawButton(menuItem(i), i == selected, cx, topY - i * 48, w, i);
}

static void drawButtonRow(float firstX, float y, float w, float gap)
{
    for (int i = 0; i < menuItemCount(); i++)
        drawButton(menuItem(i), i == selected, firstX + i * gap, y, w, i);
}

static void dim(float alpha)
{
    setHex(0x101520, alpha);
    DrawRect(0, 0, VIEW_W, VIEW_H);
}

static void panel(float x, float y, float w, float h, unsigned int header, float headerH)
{
    setHex(INK, 0.3f);
    DrawRoundRect(x + 6, y - 6, w, h, 16);
    setHex(PAPER);
    DrawRoundRect(x, y, w, h, 16);
    if (headerH > 0)
    {
        setHex(header);
        DrawRoundRect(x, y + h - headerH, w, headerH, 16);
        DrawRect(x, y + h - headerH, w, 16);
    }
}

static void hint(const char *text)
{
    setHex(0xffffff, 0.85f);
    DrawTextCentered(VIEW_W * 0.5f, 14, text, FONT_SMALL);
}

static int readWord(const char *text, int pos, char *word)
{
    int length = 0;
    while (text[pos] && text[pos] != ' ' && length < 99)
    {
        word[length] = text[pos];
        length++;
        pos++;
    }
    word[length] = '\0';
    while (text[pos] == ' ')
        pos++;
    return pos;
}

static void drawWrapped(float x, float y, const char *text, int font, float maxW, float lineH)
{
    char line[400] = "";
    char word[100];
    int pos = 0;
    while (text[pos])
    {
        pos = readWord(text, pos, word);

        char longer[400];
        strcpy(longer, line);
        if (line[0])
            strcat(longer, " ");
        strcat(longer, word);

        if (line[0] && TextWidth(longer, font) > maxW)
        {
            DrawText(x, y, line, font);
            y -= lineH;
            strcpy(line, word);
        }
        else
        {
            strcpy(line, longer);
        }
    }
    if (line[0])
        DrawText(x, y, line, font);
}

static void portrait(int style, int facing, float x, float y, float scale)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    useStyle(style);
    drawCharacter(facing, 0, 0);
    glPopMatrix();
}

static void drawTicketHeader(unsigned int maroon)
{
    setHex(maroon);
    DrawRoundRect(-70, 20, 104, 24, 8);
    DrawRect(-62, 20, 96, 24);
    setHex(GOLD);
    DrawRect(-70, 18, 104, 2);
    glColor3ub(255, 246, 224);
    DrawStrokeText(-62, 26, 12, "MUSEUM", 2);
}

static void drawTicketMuseumIcon(unsigned int maroon)
{
    setHex(maroon);
    DrawTriangle(-64, 0, -36, 0, -50, 11);
    for (int i = 0; i < 4; i++)
        DrawRect(-62 + i * 7.0f, -18, 3, 16);
    DrawRect(-65, -22, 30, 4);
}

static void drawTicketBarcode()
{
    setHex(INK);
    for (int i = 0; i < 16; i++)
    {
        float barWidth = 1.5f;
        if (i % 3 == 0)
            barWidth = 3;
        DrawRect(-64 + i * 5.5f, -38, barWidth, 10);
    }
}

static void drawTicketTearLine()
{
    glColor3ub(185, 173, 146);
    for (int i = 0; i < 9; i++)
        DrawRect(33, -40 + i * 9.5f, 2, 5);
    setHex(INK, 0.55f);
    circle(34, 44, 5);
    circle(34, -44, 5);
}

static void museumTicket(float cx, float cy, float deg, float scale)
{
    unsigned int maroon = 0x8c2f39;
    glPushMatrix();
    glTranslatef(cx, cy, 0);
    glRotatef(deg, 0, 0, 1);
    glScalef(scale, scale, 1);

    setHex(INK, 0.3f);
    DrawRoundRect(-64, -50, 140, 88, 8);
    glColor3ub(247, 236, 207);
    DrawRoundRect(-70, -44, 140, 88, 8);

    drawTicketHeader(maroon);
    drawTicketMuseumIcon(maroon);

    glColor3ub(201, 191, 166);
    DrawRect(-28, -3, 54, 3);
    DrawRect(-28, -10, 54, 3);
    DrawRect(-28, -17, 40, 3);

    drawTicketBarcode();
    drawTicketTearLine();

    float bob = sinf(uiTime * 3) * 4;
    setHex(RED);
    DrawStrokeText(40, -26 + bob, 52, "?", 5);
    glPopMatrix();
}

static void legendSwatch(float x, float y, unsigned int c, const char *label)
{
    setColor(hexColor(c, 0.7f));
    DrawSector(x, y, 18, 60, 120);
    setColor(darker(hexColor(c), 0.3f));
    DrawArc(x, y, 18, 60, 120, 1.5f);
    setHex(INK);
    DrawText(x + 22, y + 4, label, FONT_NORMAL);
}

static void drawGameTitle()
{
    const char *title = "Museum Detective Case";
    float titleHeight = 64;
    float titleWidth = StrokeTextWidth(title, titleHeight);
    float pulse = 1 + 0.015f * sinf(uiTime * 2);

    glPushMatrix();
    glTranslatef(VIEW_W * 0.5f, 455, 0);
    glScalef(pulse, pulse, 1);
    setHex(INK, 0.6f);
    DrawStrokeText(-titleWidth * 0.5f + 4, -4, titleHeight, title, 7);
    setHex(GOLD);
    DrawStrokeText(-titleWidth * 0.5f, 0, titleHeight, title, 7);
    glPopMatrix();
}

static void drawMenuSubtitle()
{
    const char *subtitle = "Framed for a museum theft. Sneak in, find the evidence, clear your name!";
    float bandWidth = TextWidth(subtitle, FONT_MEDIUM) + 40;
    setHex(INK, 0.7f);
    DrawRoundRect(VIEW_W * 0.5f - bandWidth * 0.5f, 407, bandWidth, 28, 14);
    glColor3ub(255, 255, 255);
    DrawTextCentered(VIEW_W * 0.5f, 415, subtitle, FONT_MEDIUM);
}

void drawMainMenu()
{
    dim(0.45f);
    drawGameTitle();
    drawMenuSubtitle();

    museumTicket(220, 250, -12 + sinf(uiTime * 1.3f) * 4, 1.25f);

    int guardFacing = DIR_DOWN;
    if (sinf(uiTime * 0.8f) > 0)
        guardFacing = DIR_LEFT;
    portrait(STYLE_GUARD, guardFacing, 790, 160, 4.2f);

    drawButtonColumn(VIEW_W * 0.5f, 330, 260);
    hint("Up/Down or W/S to choose  -  Enter to select  -  M music on/off  -  Esc to quit");
}

static void drawNameBox()
{
    glColor3ub(229, 233, 234);
    DrawRoundRect(300, 339, 400, 46, 8);
    setHex(BLUE);
    DrawRoundRectOutline(300, 339, 400, 46, 8, 2);

    const char *cursor = "";
    if (fmodf(uiTime, 1) < 0.5f)
        cursor = "_";
    char shown[24];
    sprintf(shown, "%s%s", nameDraft, cursor);
    DrawTextCentered(500, 355, shown, FONT_LARGE);
}

void drawNameEntry()
{
    dim(0.6f);
    panel(200, 115, 600, 380, BLUE, 55);
    glColor3ub(255, 255, 255);
    DrawTextCentered(500, 459, "WHO'S SNEAKING IN?", FONT_LARGE);
    setHex(INK);
    DrawTextCentered(500, 405, "Enter your player name", FONT_MEDIUM);

    drawNameBox();

    setHex(MUTED);
    DrawTextCentered(500, 317, "1-18 letters / numbers / spaces / - / _   |   Backspace to edit", FONT_SMALL);
    DrawTextCentered(500, 285, "Find the evidence. Outsmart security. Crack the case.", FONT_MEDIUM);
    DrawTextCentered(500, 260, "Complete all four levels to join the local top 10.", FONT_NORMAL);
    drawButtonColumn(500, 195, 260);
    hint("Type your name - Enter to start - Arrow keys choose buttons - Esc to go back");
}

static unsigned int scoreRowColor(int rank)
{
    if (rank == 0)
        return 0xf8e7a9;
    if (rank % 2 == 1)
        return 0xeee9df;
    return PAPER;
}

static void drawScoreRow(int i)
{
    char text[40];
    float y = 411 - i * 27;
    setHex(scoreRowColor(i));
    DrawRoundRect(185, y - 7, 630, 25, 4);

    setHex(INK);
    sprintf(text, "%d", i + 1);
    DrawText(210, y, text, FONT_NORMAL);
    DrawText(275, y, scores[i].name, FONT_MEDIUM);
    sprintf(text, "%d", scores[i].score);
    DrawText(605, y, text, FONT_NORMAL);
    formatTime(scores[i].seconds, text);
    DrawText(725, y, text, FONT_NORMAL);
}

static void drawScoreHeadings()
{
    setHex(BLUE);
    DrawText(200, 443, "RANK", FONT_SMALL);
    DrawText(275, 443, "PLAYER", FONT_SMALL);
    DrawText(605, 443, "SCORE", FONT_SMALL);
    DrawText(725, 443, "TIME", FONT_SMALL);
}

void drawScoreboard()
{
    dim(0.6f);
    panel(160, 45, 680, 510, BLUE, 55);
    glColor3ub(255, 255, 255);
    DrawTextCentered(500, 518, "DETECTIVE TOP 10", FONT_LARGE);
    setHex(MUTED);
    DrawTextCentered(500, 477, "Completed runs  |  Highest score first, fastest time breaks ties", FONT_NORMAL);
    drawScoreHeadings();

    if (scoreCount == 0)
    {
        setHex(INK);
        DrawTextCentered(500, 330, "No completed runs yet. The first spot could be yours!", FONT_MEDIUM);
    }
    for (int i = 0; i < scoreCount && i < 10; i++)
        drawScoreRow(i);

    const char *status = "Saved on this computer. Every run needs a player name.";
    if (scoreStatus[0])
        status = scoreStatus;
    setHex(MUTED);
    DrawTextCentered(500, 125, status, FONT_SMALL);
    drawButtonColumn(500, 65, 220);
}

static void drawSectionTitle(float x, float y, const char *title)
{
    setHex(BLUE);
    DrawText(x, y, title, FONT_MEDIUM);
}

static void drawObjectiveHelp(float x, float cy)
{
    drawSectionTitle(x + 30, cy, "OBJECTIVE");
    setHex(INK);
    DrawText(x + 30, cy - 22, "You were framed for stealing the Shapla Crown. Clear your name: entrance, artifacts, gallery, vault.", FONT_NORMAL);
    DrawText(x + 30, cy - 40, "Press F at evidence stands. J opens your case file. Gold marks the exit point.", FONT_NORMAL);
}

static void drawControlsHelp(float x, float cy)
{
    const char *keys[6] = {"W A S D / Arrows", "Shift / Space", "Mouse / Click",
                           "F", "P / R", "M / Esc"};
    const char *actions[6] = {"Move", "Sprint - fast but noisy", "Aim / throw clink (2)",
                              "Collect / talk / pet", "Pause / retry (-100)", "Music on/off / Main menu"};

    drawSectionTitle(x + 30, cy, "CONTROLS");
    for (int i = 0; i < 6; i++)
    {
        int column = i % 2;
        int row = i / 2;
        float rowX = x + 30 + column * 380;
        float rowY = cy - 24 - row * 20;
        setHex(BLUE);
        DrawText(rowX, rowY, keys[i], FONT_NORMAL);
        setHex(INK);
        DrawText(rowX + 140, rowY, actions[i], FONT_NORMAL);
    }
}

static void drawGuardsHelp(float x, float cy)
{
    drawSectionTitle(x + 30, cy, "GUARDS");
    setHex(INK);
    DrawText(x + 30, cy - 22, "Guards see inside their vision cone. Walls, buildings and monuments block sight.", FONT_NORMAL);
    DrawText(x + 30, cy - 40, "A full detection meter means capture. Guards outrun walking; sprint and use cover.", FONT_NORMAL);
    DrawText(x + 30, cy - 58, "Aim with the mouse and left-click to lure guards away. Break sight and change route.", FONT_NORMAL);
    legendSwatch(x + 60, cy - 100, 0xeefa7a, "Calm");
    legendSwatch(x + 250, cy - 100, 0xffa53d, "Suspicious ( ? )");
    legendSwatch(x + 500, cy - 100, 0xff4b3e, "Pursuit ( ! )");
}

void drawInstructions()
{
    float x = 90;
    float y = 70;
    float w = 820;
    float h = 470;
    dim(0.55f);
    panel(x, y, w, h, BLUE, 50);
    glColor3ub(255, 255, 255);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 33, "HOW TO PLAY", FONT_LARGE);

    float cy = y + h - 80;
    drawObjectiveHelp(x, cy);
    cy -= 80;
    drawControlsHelp(x, cy);
    cy -= 110;
    drawGuardsHelp(x, cy);

    drawButtonColumn(VIEW_W * 0.5f, y + 18, 200);
}

static void drawMeter(float x, const char *label, float value, unsigned int c)
{
    glColor3ub(230, 237, 240);
    DrawText(x, 578, label, FONT_SMALL);
    glColor3ub(52, 67, 81);
    DrawRoundRect(x, 555, 135, 9, 4);
    setHex(c);
    if (value > 0.01f)
        DrawRoundRect(x, 555, 135 * value, 9, 4);
}

static void drawHudBars()
{
    setHex(INK);
    DrawRect(0, PLAY_TOP, VIEW_W, VIEW_H - PLAY_TOP);
    DrawRect(0, 0, VIEW_W, PLAY_BOTTOM);
    glColor3ub(60, 83, 102);
    DrawRect(0, PLAY_TOP, VIEW_W, 2);
    DrawRect(0, PLAY_BOTTOM - 2, VIEW_W, 2);
}

static void drawHudLevelAndTime()
{
    char text[200];
    glColor3ub(255, 250, 240);
    DrawText(18, 575, level.name, FONT_MEDIUM);
    glColor3ub(184, 203, 213);
    DrawText(18, 551, level.objective, FONT_NORMAL);

    float timeLeft = remainingTime();
    char clock[16];
    formatTime(ceilf(timeLeft), clock);
    if (timeLeft <= 20)
        glColor3ub(255, 137, 118);
    else
        setHex(GOLD);
    sprintf(text, "TIME  %s", clock);
    DrawText(410, 575, text, FONT_MEDIUM);

    glColor3ub(184, 203, 213);
    sprintf(text, "Stages cleared: %d / 4", levelIndex);
    DrawText(410, 551, text, FONT_NORMAL);
}

static void drawHudMeters()
{
    float detection = 0;
    bool anyAlert = false;
    for (int i = 0; i < guardCount; i++)
    {
        detection = maxf(detection, guards[i].detection);
        if (guards[i].state == ALERT)
            anyAlert = true;
    }

    const char *label = "SUSPICION";
    if (anyAlert)
        label = "SPOTTED - FIND COVER";
    unsigned int meterColor = GOLD;
    if (detection > 0.6f)
        meterColor = 0xff8976;
    drawMeter(655, label, detection, meterColor);
    drawMeter(830, "STAMINA", player.stamina, 0x63c7dd);
}

static void makeInteractPrompt(char *prompt)
{
    strcpy(prompt, "F: Interact");

    int who = nearbyCharacter();
    if (who >= 0 && who < NPC_COUNT)
        sprintf(prompt, "F: Talk to %s", npcs[who].name);
    else if (who >= NPC_COUNT)
        strcpy(prompt, "F: Pet the cat");

    int pass = nearbyPass();
    if (pass >= 0)
        sprintf(prompt, "F: Take %s", level.passNames[pass]);
}

static void drawHudBottomRow()
{
    char text[200];
    glColor3ub(131, 247, 239);
    sprintf(text, "Evidence %d/%d", collectedCount(), level.passCount);
    DrawText(18, 35, text, FONT_MEDIUM);
    setHex(GOLD);
    sprintf(text, "Distractions %d", distractions);
    DrawText(155, 35, text, FONT_MEDIUM);

    makeInteractPrompt(text);
    glColor3ub(255, 255, 255);
    DrawText(340, 35, text, FONT_NORMAL);
    glColor3ub(184, 203, 213);
    DrawText(635, 35, "WASD move   Space sprint   J case file   P pause   M music", FONT_NORMAL);
}

static void drawHudMessage()
{
    const char *text = "Keep the detection meter from filling. Sprint to escape; hide to lose pursuit. J: case file.";
    if (messageTime > 0)
        text = message;
    if (awaitingDeparture)
        text = "READY WHEN YOU ARE - move or interact to begin. Patrols and the clock are paused.";
    DrawText(18, 13, text, FONT_SMALL);
}

void drawHud()
{
    drawHudBars();
    drawHudLevelAndTime();
    drawHudMeters();
    drawHudBottomRow();
    drawHudMessage();
}

static void drawStageStamp(int i, float x)
{
    const char *stamps[4] = {"ENTRANCE", "ARTIFACTS", "GALLERY", "VAULT"};
    bool solved = i < levelIndex;
    bool current = i == levelIndex;

    if (solved)
        glColor3ub(212, 227, 203);
    else if (current)
        glColor3ub(243, 219, 160);
    else
        glColor3ub(230, 225, 214);
    DrawRoundRect(x, 432, 148, 42, 5);

    const char *status = "NEXT";
    if (solved)
    {
        setHex(GREEN);
        status = "SOLVED";
    }
    else
    {
        setHex(INK);
        if (current)
            status = "IN PROGRESS";
    }
    DrawTextCentered(x + 74, 450, stamps[i], FONT_NORMAL);
    DrawTextCentered(x + 74, 438, status, FONT_SMALL);
}

static void drawChecklistItem(int i)
{
    float y = 304 - i * 31;
    if (collected[i])
        setHex(GREEN);
    else
        setHex(MUTED);
    DrawRectOutline(183, y - 2, 12, 12, 1.5f);
    if (collected[i])
    {
        DrawLine(185, y + 3, 188, y, 1.5f);
        DrawLine(188, y, 194, y + 8, 1.5f);
    }
    DrawText(211, y, level.passNames[i], FONT_MEDIUM);
    if (collected[i])
    {
        setHex(MUTED);
        DrawText(345, y + 3, level.passClues[i], FONT_SMALL);
    }
}

void drawBriefing()
{
    dim(0.55f);
    panel(145, 65, 710, 475, BLUE, 55);
    setHex(PAPER);
    DrawTextCentered(500, 505, "DETECTIVE CASE FILE", FONT_LARGE);

    for (int i = 0; i < 4; i++)
        drawStageStamp(i, 180 + i * 164);

    setHex(BLUE);
    DrawText(180, 398, level.objective, FONT_MEDIUM);
    setHex(MUTED);
    drawWrapped(180, 372, level.briefing, FONT_NORMAL, 640, 17);

    setHex(INK);
    DrawText(180, 334, "EVIDENCE CHECKLIST", FONT_SMALL);
    for (int i = 0; i < level.passCount; i++)
        drawChecklistItem(i);

    setHex(BLUE);
    DrawText(180, 190, "F: interact     Mouse: aim / left-click: clink     J: case file", FONT_NORMAL);
    setHex(MUTED);
    DrawText(180, 167, "Walk quietly. Sprint to cover if spotted. A full detection meter means capture.", FONT_NORMAL);
    DrawText(180, 147, "The clock waits while you read. Each solved stage opens the next one.", FONT_NORMAL);
    drawButton(menuItem(0), true, 500, 86, 240, 0);
}

void drawPauseMenu()
{
    float w = 340;
    float h = 260;
    float x = VIEW_W * 0.5f - w * 0.5f;
    float y = VIEW_H * 0.5f - h * 0.5f;
    dim(0.55f);
    panel(x, y, w, h, BLUE, 50);
    glColor3ub(255, 255, 255);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 33, "PAUSED", FONT_LARGE);
    drawButtonColumn(VIEW_W * 0.5f, y + h - 100, 240);
}

static void copyTextRange(char *out, int from, int to)
{
    strncpy(out, dialogueText + from, to - from);
    out[to - from] = '\0';
}

static int findLineEnd(int lineStart, int textLength)
{
    int lineEnd = lineStart;
    int wordStart = lineStart;
    while (wordStart <= textLength)
    {
        int wordEnd = wordStart;
        while (wordEnd < textLength && dialogueText[wordEnd] != ' ')
            wordEnd++;

        char candidate[300];
        copyTextRange(candidate, lineStart, wordEnd);
        if (TextWidth(candidate, FONT_MEDIUM) > 754 && lineEnd > lineStart)
            break;

        lineEnd = wordEnd;
        if (wordEnd >= textLength)
            break;
        wordStart = wordEnd + 1;
    }
    return lineEnd;
}

static void drawDialogueText(int revealed)
{
    int textLength = strlen(dialogueText);
    int lineStart = 0;
    float y = 179;
    setHex(INK);
    while (lineStart < textLength)
    {
        int lineEnd = findLineEnd(lineStart, textLength);
        if (revealed > lineStart)
        {
            int stop = lineEnd;
            if (revealed < lineEnd)
                stop = revealed;
            char line[300];
            copyTextRange(line, lineStart, stop);
            DrawText(178, y, line, FONT_MEDIUM);
        }
        y -= 25;
        lineStart = lineEnd + 1;
    }
}

static void drawDialogueSpeaker()
{
    glColor3ub(220, 233, 230);
    DrawRoundRect(44, 92, 112, 126, 10);

    glPushMatrix();
    glTranslatef(100, 113, 0);
    glScalef(2.2f, 2.2f, 1);
    drawNpc(dialogueCharacter, 0, 0, uiTime);
    glPopMatrix();

    setHex(BLUE);
    DrawText(178, 211, speakerName, FONT_MEDIUM);
    setHex(MUTED);
    DrawText(178 + TextWidth(speakerName, FONT_MEDIUM) + 20, 212, speakerRole, FONT_SMALL);
}

void drawDialogue()
{
    panel(24, 70, 952, 172, BLUE, 0);
    setHex(BLUE);
    DrawRoundRectOutline(30, 76, 940, 160, 12, 3);
    drawDialogueSpeaker();

    int revealed = (int)(dialogueTime * 42);
    drawDialogueText(revealed);

    bool ready = revealed >= (int)strlen(dialogueText);
    setHex(MUTED);
    if (ready)
        DrawText(178, 94, "F / Enter / Space / click: finish     Esc: leave", FONT_SMALL);
    else
        DrawText(178, 94, "F / Enter / Space / click: reveal text     Esc: leave", FONT_SMALL);

    if (ready)
    {
        float bob = sinf(uiTime * 5) * 2;
        setHex(BLUE);
        DrawTriangle(936, 105 + bob, 948, 105 + bob, 942, 97 + bob);
    }
}

static void drawCaughtSpeech(float bx, float by)
{
    float bw = 470;
    float bh = 80;
    glColor3ub(255, 255, 255);
    DrawRoundRect(bx, by, bw, bh, 14);
    DrawTriangle(bx + 2, by + 30, bx + 2, by + 50, bx - 24, by + 34);
    setHex(RED);
    DrawRoundRectOutline(bx, by, bw, bh, 14, 2.5f);

    float shakeSize = 0;
    if (fmodf(screenTime, 2) < 0.3f)
        shakeSize = 2;
    float shake = sinf(screenTime * 30) * shakeSize;
    DrawTextFit(bx + bw * 0.5f + shake, by + 48, "\"" CAUGHT_LINE1 "\"", FONT_LARGE, bw - 30);
    setHex(INK);
    DrawTextFit(bx + bw * 0.5f, by + 18, "\"" CAUGHT_LINE2 "\"", FONT_MEDIUM, bw - 30);
}

void drawGameOver()
{
    float w = 700;
    float h = 230;
    float x = VIEW_W * 0.5f - w * 0.5f;
    float y = 40;
    dim(0.35f);
    panel(x, y, w, h, RED, 46);
    glColor3ub(255, 255, 255);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 30, "CAUGHT BY SECURITY!", FONT_LARGE);

    glColor3ub(220, 231, 243);
    circle(x + 100, y + 95, 70);
    portrait(STYLE_GUARD, DIR_DOWN, x + 100, y + 20, 4);

    drawCaughtSpeech(x + 200, y + 95);

    drawButtonRow(x + 330, y + 26, 200, 230);
}

void drawTimeout()
{
    dim(0.55f);
    panel(240, 150, 520, 300, RED, 54);
    glColor3ub(255, 255, 255);
    DrawTextCentered(500, 413, "TIME'S UP!", FONT_LARGE);
    setHex(INK);
    DrawTextCentered(500, 360, "The trail went cold. Try a faster route.", FONT_MEDIUM);
    setHex(MUTED);
    DrawTextCentered(500, 328, "Use distractions to open a gap in the patrols.", FONT_NORMAL);
    DrawTextCentered(500, 305, "Retry costs 100 points; failed time counts for the scoreboard.", FONT_SMALL);
    drawButtonColumn(500, 230, 260);
}

static float starPointRadius(int point)
{
    if (point % 2 == 1)
        return 7;
    return 17;
}

static void drawStar(float cx, float cy, float size, unsigned int c)
{
    glPushMatrix();
    glTranslatef(cx, cy, 0);
    glScalef(size, size, 1);
    setHex(c);
    for (int j = 0; j < 10; j++)
    {
        float angle1 = degToRad(90 + j * 36);
        float angle2 = degToRad(90 + (j + 1) * 36);
        float radius1 = starPointRadius(j);
        float radius2 = starPointRadius(j + 1);
        DrawTriangle(0, 0, cosf(angle1) * radius1, sinf(angle1) * radius1,
                     cosf(angle2) * radius2, sinf(angle2) * radius2);
    }
    glPopMatrix();
}

static int starsEarned()
{
    if (result.levelScore >= 1700)
        return 3;
    if (result.levelScore >= 1300)
        return 2;
    return 1;
}

static void drawLevelStars(float y)
{
    int stars = starsEarned();
    for (int i = 0; i < 3; i++)
    {
        float grow = clampf((screenTime - i * 0.25f) * 3, 0, 1);
        float size = grow * (1 + 0.3f * sinf(grow * PI));
        unsigned int starColor = 0xd8d2c4;
        if (i < stars)
            starColor = GOLD;
        drawStar(VIEW_W * 0.5f + (i - 1) * 50, y, size, starColor);
    }
}

static void drawScoreBreakdown(float x, float w, float rowY)
{
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

    for (int i = 0; i < 5; i++)
    {
        setHex(MUTED);
        DrawText(x + 120, rowY, labels[i], FONT_NORMAL);
        setHex(INK);
        DrawText(x + w - 120 - TextWidth(values[i], FONT_NORMAL), rowY, values[i], FONT_NORMAL);
        rowY -= 17;
    }

    char text[120];
    setHex(BLUE);
    sprintf(text, "Level score %d     Total %d", result.levelScore, result.totalScore);
    DrawTextCentered(VIEW_W * 0.5f, rowY - 6, text, FONT_MEDIUM);
}

void drawLevelComplete()
{
    char text[120];
    float w = 520;
    float h = 340;
    float x = VIEW_W * 0.5f - w * 0.5f;
    float y = VIEW_H * 0.5f - h * 0.5f;
    dim(0.45f);
    panel(x, y, w, h, GREEN, 50);
    glColor3ub(255, 255, 255);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 33, level.reward, FONT_MEDIUM);
    setHex(INK);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 78, level.name, FONT_MEDIUM);
    setHex(MUTED);
    sprintf(text, "Next: %s", levelObjective(levelIndex + 1));
    DrawTextCentered(500, y + 73, text, FONT_SMALL);

    drawLevelStars(y + h - 112);
    drawScoreBreakdown(x, w, y + h - 150);

    drawButtonRow(VIEW_W * 0.5f - 120, y + 22, 210, 240);
}

static void drawConfetti(float t)
{
    unsigned int colors[5] = {0xf2c14e, 0xe8694a, 0x4aa3d9, 0x3f9b5a, 0xb48be2};
    for (int i = 0; i < 90; i++)
    {
        float pieceX = hash01(i, 1) * VIEW_W + sinf(t * 1.5f + i) * 20;
        float speed = 40 + hash01(i, 2) * 60;
        float fallen = fmodf(t * speed + hash01(i, 3) * (VIEW_H + 40), VIEW_H + 40);
        float pieceY = VIEW_H + 20 - fallen;
        float spin = t * (90 + hash01(i, 4) * 200);
        float flip = 0.6f + 0.4f * sinf(t * 4 + i);

        glPushMatrix();
        glTranslatef(pieceX, pieceY, 0);
        glRotatef(spin, 0, 0, 1);
        glScalef(1, flip, 1);
        setHex(colors[i % 5]);
        DrawRect(-4, -2.5f, 8, 5);
        glPopMatrix();
    }
}

static void drawJumpingDetective(float t, float y)
{
    float jump = fabsf(sinf(t * 4));
    glPushMatrix();
    glTranslatef(VIEW_W * 0.5f, y + 65 + jump * 10, 0);
    glScalef(2.4f * (1 - 0.06f * jump), 2.4f * (1 + 0.08f * jump), 1);
    useStyle(STYLE_DETECTIVE);
    drawCharacter(DIR_DOWN, t * 10, 0.6f);
    glPopMatrix();
}

static void drawVictoryText(float y, float h)
{
    char text[120];
    char clock[16];
    glColor3ub(255, 255, 255);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 36, "CONGRATULATIONS!", FONT_LARGE);
    setHex(INK);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 86, "Name cleared! The chief guard stole the crown.", FONT_MEDIUM);
    setHex(MUTED);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 108, "He framed you with your own badge. The Shapla Crown is home.", FONT_NORMAL);
    setHex(BLUE);
    formatTime(result.totalTime, clock);
    sprintf(text, "FINAL SCORE  %d      TOTAL TIME  %s", result.totalScore, clock);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 136, text, FONT_MEDIUM);
    setHex(MUTED);
    DrawTextCentered(VIEW_W * 0.5f, y + h - 160, scoreStatus, FONT_SMALL);
}

void drawVictory()
{
    float t = screenTime;
    dim(0.4f);
    drawConfetti(t);

    float w = 600;
    float h = 330;
    float x = VIEW_W * 0.5f - w * 0.5f;
    float y = VIEW_H * 0.5f - h * 0.5f;
    panel(x, y, w, h, GREEN, 54);
    drawVictoryText(y, h);
    drawJumpingDetective(t, y);

    museumTicket(x + 90, y + 150, 15 + sinf(t * 2) * 8, 0.55f);
    museumTicket(x + w - 90, y + 150, -15 - sinf(t * 2) * 8, 0.55f);

    drawButtonRow(VIEW_W * 0.5f - 120, y + 26, 210, 240);
}
