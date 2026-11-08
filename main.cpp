// ============================================================
// MUSEUM DETECTIVE CASE
// main.cpp : global variables, init(), display(), keyboard,
//            mouse, reshape and the timer. Program starts here.
//
// Controls: WASD / arrows move, Shift or Space sprint,
//           F collect / talk / pet, mouse click or E throw a
//           distraction, J case file, P pause, R retry,
//           G debug view, Esc menu.
// ============================================================

#include "game.h"

// ============================================================
// GLOBAL VARIABLES
// ============================================================

int screen = MAIN_MENU;
int selected = 0;
float screenTime = 0;
float uiTime = 0;
float worldTime = 0;
bool quitGame = false;

Map map;
Level level;
int levelIndex = 0;
Player player;
Guard guards[MAX_GUARDS];
int guardCount = 0;
Npc npcs[NPC_COUNT];

Vec2 crowdRoute[CROWD_COUNT][MAX_ROUTE];
int crowdLength[CROWD_COUNT];
int crowdCount = 0;
Vec2 catRoute[CAT_COUNT][MAX_ROUTE];
int catLength[CAT_COUNT];
int catCount = 0;
float catClock[CAT_COUNT];
float catAffection[CAT_COUNT];

bool letterHeld[4]; // W S A D
bool arrowHeld[4];  // up down left right
bool shiftHeld = false;
bool spaceHeld = false;

int mouseX = 0, mouseY = 0;
bool mouseKnown = false;

int winW = 1000, winH = 600;
int vpX = 0, vpY = 0, vpW = 1000, vpH = 600;
float pixelsPerUnit = 1;

float levelTime = 0;
float totalTime = 0;
int totalScore = 0;
int retries = 0;
bool awaitingDeparture = true;
bool spotted = false;
bool alarmed = false;
bool collected[3];
float pickupTime = 0;
Vec2 pickupPos;
float petTime = 0;
int petCat = -1;
float alertCueCooldown = 0;
int distractions = 2;
float distractionCooldown = 0;
float noiseTime = 0;
Vec2 noisePos;
float radioCooldown = 0;
char message[300];
float messageTime = 0;
int catcher = -1;
bool showDebug = false;

char playerName[20] = "";
char nameDraft[20] = "";
bool rankedRun = false;
char scoreStatus[120] = "";
LevelResult result;

char dialogueText[300];
const char *speakerName = "";
const char *speakerRole = "";
int dialogueCharacter = 0;
float dialogueTime = 0;

ScoreEntry scores[11];
int scoreCount = 0;

int lastTime = 0;    // ms, for the timer
float timeLeftOver = 0; // fixed-step accumulator

#define FIXED_DT (1.0f / 120.0f) // the game updates 120 times a second

// ============================================================
// INIT
// ============================================================

void init()
{
    glClearColor(0.11f, 0.14f, 0.19f, 1); // dark blue-grey
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-500, 500, -300, 300, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    loadScores();
    levelIndex = 0;
    restartLevel(); // the menu shows level 1 in the background
    setScreen(MAIN_MENU);
}

// ============================================================
// DISPLAY
// ============================================================

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    bool inMenu = (screen == MAIN_MENU || screen == NAME_ENTRY ||
                   screen == SCOREBOARD || screen == INSTRUCTIONS);

    // ---- world: (0,0) of the museum sits just above the bottom HUD ----
    glPushMatrix();
    glTranslatef(-500, -300 + PLAY_BOTTOM, 0);
    drawWorld(!inMenu);
    glPopMatrix();

    // ---- screen: (0,0) bottom-left, (1000,600) top-right ----
    glPushMatrix();
    glTranslatef(-500, -300, 0);
    if (!inMenu && screen != WIN)
        drawHud();

    beginButtons();
    switch (screen)
    {
    case MAIN_MENU:
        drawMainMenu();
        break;
    case NAME_ENTRY:
        drawNameEntry();
        break;
    case SCOREBOARD:
        drawScoreboard();
        break;
    case INSTRUCTIONS:
        drawInstructions();
        break;
    case BRIEFING:
        drawBriefing();
        break;
    case PAUSED:
        drawPauseMenu();
        break;
    case DIALOGUE:
        drawDialogue();
        break;
    case CAUGHT:
        drawGameOver();
        break;
    case TIMED_OUT:
        drawTimeout();
        break;
    case LEVEL_COMPLETE:
        drawLevelComplete();
        break;
    case WIN:
        drawVictory();
        break;
    }
    glPopMatrix();

    glutSwapBuffers();
}

// ============================================================
// RESHAPE: keep the 1000 x 600 picture, add black bars if the
// window has another shape (letterbox)
// ============================================================

void reshape(int w, int h)
{
    if (h == 0)
        h = 1;
    winW = w;
    winH = h;
    pixelsPerUnit = minf(w / VIEW_W, h / VIEW_H);
    vpW = (int)(VIEW_W * pixelsPerUnit);
    vpH = (int)(VIEW_H * pixelsPerUnit);
    vpX = (w - vpW) / 2;
    vpY = (h - vpH) / 2;
    glViewport(vpX, vpY, vpW, vpH);
}

// ============================================================
// KEYBOARD
// ============================================================

int letterToDir(unsigned char key)
{
    if (key == 'w' || key == 'W')
        return 0;
    if (key == 's' || key == 'S')
        return 1;
    if (key == 'a' || key == 'A')
        return 2;
    if (key == 'd' || key == 'D')
        return 3;
    return -1;
}

int arrowToDir(int key)
{
    if (key == GLUT_KEY_UP)
        return 0;
    if (key == GLUT_KEY_DOWN)
        return 1;
    if (key == GLUT_KEY_LEFT)
        return 2;
    if (key == GLUT_KEY_RIGHT)
        return 3;
    return -1;
}

void readShift()
{
    shiftHeld = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
}

void my_keyboard(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    // ---- typing a name ----
    if (screen == NAME_ENTRY)
    {
        int len = strlen(nameDraft);
        if (key == 27)
            setScreen(MAIN_MENU);
        else if (key == 13)
            activateItem(selected);
        else if (key == 8 && len > 0)
            nameDraft[len - 1] = '\0';
        else if (len < 18 && ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') ||
                              (key >= '0' && key <= '9') || key == ' ' || key == '-' || key == '_'))
        {
            nameDraft[len] = key;
            nameDraft[len + 1] = '\0';
        }
        return;
    }

    // ---- reading a dialogue ----
    if (screen == DIALOGUE)
    {
        if (key == 27)
            setScreen(PLAYING);
        else if (key == 'f' || key == 'F' || key == 13 || key == ' ')
            advanceDialogue();
        return;
    }

    if (screen == PLAYING)
        readShift();

    int d = letterToDir(key);
    if (d >= 0)
        letterHeld[d] = true;

    // ---- menu navigation ----
    int n = menuItemCount();
    if (screen != PLAYING && n > 0)
    {
        if (d == 0 || d == 2)
            selected = (selected + n - 1) % n;
        if (d == 1 || d == 3)
            selected = (selected + 1) % n;
        if (key == 13 || key == ' ')
        {
            activateItem(selected);
            return;
        }
    }

    switch (key)
    {
    case 27: // Esc
        if (screen == MAIN_MENU)
            quitGame = true;
        else if (screen == INSTRUCTIONS)
            setScreen(MAIN_MENU);
        else if (screen == BRIEFING)
            setScreen(PLAYING);
        else
            goToMenu();
        break;
    case 'p':
    case 'P':
        if (screen == PLAYING)
            setScreen(PAUSED);
        else if (screen == PAUSED)
            setScreen(PLAYING);
        break;
    case 'r':
    case 'R':
        if (screen == PLAYING || screen == PAUSED || screen == CAUGHT || screen == TIMED_OUT)
            retryLevel();
        break;
    case 'e':
    case 'E':
        throwDistraction();
        break;
    case 'f':
    case 'F':
        if (screen == PLAYING)
            interact();
        break;
    case 'j':
    case 'J':
        if (screen == PLAYING)
            setScreen(BRIEFING);
        else if (screen == BRIEFING)
            setScreen(PLAYING);
        break;
    case 'g':
    case 'G':
        showDebug = !showDebug;
        break;
    case ' ': // Space = sprint
        spaceHeld = true;
        break;
    }
}

void my_keyboard_up(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    if (key == ' ')
        spaceHeld = false;
    int d = letterToDir(key);
    if (d >= 0)
        letterHeld[d] = false;
    readShift();
}

void spe_key(int key, int x, int y)
{
    (void)x;
    (void)y;
    if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R)
    {
        shiftHeld = true;
        return;
    }
    int d = arrowToDir(key);
    if (d < 0)
        return;
    arrowHeld[d] = true;
    int n = menuItemCount();
    if (screen != PLAYING && n > 0)
    {
        if (d == 0 || d == 2)
            selected = (selected + n - 1) % n;
        else
            selected = (selected + 1) % n;
    }
}

void spe_key_up(int key, int x, int y)
{
    (void)x;
    (void)y;
    if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R)
    {
        shiftHeld = false;
        return;
    }
    int d = arrowToDir(key);
    if (d >= 0)
        arrowHeld[d] = false;
}

// ============================================================
// MOUSE
// ============================================================

// window pixels -> view units (0..1000 x 0..600, y up)
float viewX(int px)
{
    return (px - vpX) / pixelsPerUnit;
}

float viewY(int py)
{
    return (winH - py - vpY) / pixelsPerUnit;
}

void my_mouse_move(int x, int y)
{
    mouseX = x;
    mouseY = y;
    mouseKnown = true;
    if (screen == PLAYING)
        return;
    int i = buttonAt(viewX(x), viewY(y)); // hover selects a button
    if (i >= 0)
        selected = i;
}

void my_mouse(int button, int state, int x, int y)
{
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        return;
    mouseX = x;
    mouseY = y;
    mouseKnown = true;
    if (screen == PLAYING)
    {
        if (mouseAimValid())
            throwDistraction();
        return;
    }
    if (screen == DIALOGUE)
    {
        advanceDialogue();
        return;
    }
    int i = buttonAt(viewX(x), viewY(y));
    if (i >= 0)
        activateItem(i);
}

// ============================================================
// TIMER: about 60 frames per second, game logic at 120 Hz
// ============================================================

void timer(int value)
{
    (void)value;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float frame = (now - lastTime) / 1000.0f;
    lastTime = now;
    if (frame > 0.25f)
        frame = 0.25f; // after a pause, don't jump far ahead

    timeLeftOver += frame;
    while (timeLeftOver >= FIXED_DT)
    {
        updateGame(FIXED_DT);
        timeLeftOver -= FIXED_DT;
    }

    if (quitGame)
        exit(0);

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// ============================================================
// MAIN
// ============================================================

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowSize(1000, 600);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Museum Detective Case");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(my_keyboard);
    glutKeyboardUpFunc(my_keyboard_up);
    glutSpecialFunc(spe_key);
    glutSpecialUpFunc(spe_key_up);
    glutMouseFunc(my_mouse);
    glutPassiveMotionFunc(my_mouse_move);
    glutMotionFunc(my_mouse_move);
    glutIgnoreKeyRepeat(1); // we keep track of held keys ourselves

    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
