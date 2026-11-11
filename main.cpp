#include "game.h"

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

bool letterHeld[4];
bool arrowHeld[4];
bool shiftHeld = false;
bool spaceHeld = false;

int mouseX = 0;
int mouseY = 0;
bool mouseKnown = false;

int winW = 1000;
int winH = 600;
int vpX = 0;
int vpY = 0;
int vpW = 1000;
int vpH = 600;
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

int lastTime = 0;
float timeLeftOver = 0;


#define FIXED_DT (1.0f / 120.0f)

#define KEY_BACKSPACE 8
#define KEY_ENTER 13
#define KEY_ESCAPE 27

#define DIR_KEY_UP 0
#define DIR_KEY_DOWN 1
#define DIR_KEY_LEFT 2
#define DIR_KEY_RIGHT 3

void init()
{
    glClearColor(0.11f, 0.14f, 0.19f, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-500, 500, -300, 300, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    loadScores();
    startMusic();
    levelIndex = 0;
    restartLevel();
    setScreen(MAIN_MENU);
}

static bool isMenuScreen()
{
    return screen == MAIN_MENU || screen == NAME_ENTRY ||
           screen == SCOREBOARD || screen == INSTRUCTIONS;
}

static void drawCurrentScreen()
{
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
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    bool inMenu = isMenuScreen();

    glPushMatrix();
    glTranslatef(-500, -300 + PLAY_BOTTOM, 0);
    drawWorld(!inMenu);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-500, -300, 0);
    if (!inMenu && screen != WIN)
        drawHud();
    beginButtons();
    drawCurrentScreen();
    glPopMatrix();

    glutSwapBuffers();
}

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

static int letterToDir(unsigned char key)
{
    if (key == 'w' || key == 'W')
        return DIR_KEY_UP;
    if (key == 's' || key == 'S')
        return DIR_KEY_DOWN;
    if (key == 'a' || key == 'A')
        return DIR_KEY_LEFT;
    if (key == 'd' || key == 'D')
        return DIR_KEY_RIGHT;
    return -1;
}

static int arrowToDir(int key)
{
    if (key == GLUT_KEY_UP)
        return DIR_KEY_UP;
    if (key == GLUT_KEY_DOWN)
        return DIR_KEY_DOWN;
    if (key == GLUT_KEY_LEFT)
        return DIR_KEY_LEFT;
    if (key == GLUT_KEY_RIGHT)
        return DIR_KEY_RIGHT;
    return -1;
}

static void readShift()
{
    shiftHeld = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
}

static bool isShiftKey(int key)
{
    return key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R;
}

static bool isNameChar(unsigned char key)
{
    if (key >= 'a' && key <= 'z')
        return true;
    if (key >= 'A' && key <= 'Z')
        return true;
    if (key >= '0' && key <= '9')
        return true;
    return key == ' ' || key == '-' || key == '_';
}

static void selectPrevious()
{
    int count = menuItemCount();
    selected = (selected + count - 1) % count;
}

static void selectNext()
{
    int count = menuItemCount();
    selected = (selected + 1) % count;
}

static bool menuIsOpen()
{
    return screen != PLAYING && menuItemCount() > 0;
}

static void typeNameKey(unsigned char key)
{
    int length = strlen(nameDraft);
    if (key == KEY_ESCAPE)
    {
        setScreen(MAIN_MENU);
    }
    else if (key == KEY_ENTER)
    {
        activateItem(selected);
    }
    else if (key == KEY_BACKSPACE && length > 0)
    {
        nameDraft[length - 1] = '\0';
    }
    else if (length < 18 && isNameChar(key))
    {
        nameDraft[length] = key;
        nameDraft[length + 1] = '\0';
    }
}

static void dialogueKey(unsigned char key)
{
    if (key == KEY_ESCAPE)
        setScreen(PLAYING);
    else if (key == 'f' || key == 'F' || key == KEY_ENTER || key == ' ')
        advanceDialogue();
}

static void pressEscape()
{
    if (screen == MAIN_MENU)
        quitGame = true;
    else if (screen == INSTRUCTIONS)
        setScreen(MAIN_MENU);
    else if (screen == BRIEFING)
        setScreen(PLAYING);
    else
        goToMenu();
}

static void togglePause()
{
    if (screen == PLAYING)
        setScreen(PAUSED);
    else if (screen == PAUSED)
        setScreen(PLAYING);
}

static void tryRetry()
{
    if (screen == PLAYING || screen == PAUSED || screen == CAUGHT || screen == TIMED_OUT)
        retryLevel();
}

static void toggleCaseFile()
{
    if (screen == PLAYING)
        setScreen(BRIEFING);
    else if (screen == BRIEFING)
        setScreen(PLAYING);
}

static void actionKey(unsigned char key)
{
    if (key == KEY_ESCAPE)
        pressEscape();
    else if (key == 'p' || key == 'P')
        togglePause();
    else if (key == 'r' || key == 'R')
        tryRetry();
    else if (key == 'e' || key == 'E')
        throwDistraction();
    else if ((key == 'f' || key == 'F') && screen == PLAYING)
        interact();
    else if (key == 'j' || key == 'J')
        toggleCaseFile();
    else if (key == 'g' || key == 'G')
        showDebug = !showDebug;
    else if (key == 'm' || key == 'M')
        toggleMusic();
    else if (key == ' ')
        spaceHeld = true;
}

void my_keyboard(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    if (screen == NAME_ENTRY)
    {
        typeNameKey(key);
        return;
    }
    if (screen == DIALOGUE)
    {
        dialogueKey(key);
        return;
    }

    if (screen == PLAYING)
        readShift();

    int dir = letterToDir(key);
    if (dir >= 0)
        letterHeld[dir] = true;

    if (menuIsOpen())
    {
        if (dir == DIR_KEY_UP || dir == DIR_KEY_LEFT)
            selectPrevious();
        if (dir == DIR_KEY_DOWN || dir == DIR_KEY_RIGHT)
            selectNext();
        if (key == KEY_ENTER || key == ' ')
        {
            activateItem(selected);
            return;
        }
    }

    actionKey(key);
}

void my_keyboard_up(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    if (key == ' ')
        spaceHeld = false;
    int dir = letterToDir(key);
    if (dir >= 0)
        letterHeld[dir] = false;
    readShift();
}

void spe_key(int key, int x, int y)
{
    (void)x;
    (void)y;
    if (isShiftKey(key))
    {
        shiftHeld = true;
        return;
    }
    int dir = arrowToDir(key);
    if (dir < 0)
        return;
    arrowHeld[dir] = true;
    if (menuIsOpen())
    {
        if (dir == DIR_KEY_UP || dir == DIR_KEY_LEFT)
            selectPrevious();
        else
            selectNext();
    }
}

void spe_key_up(int key, int x, int y)
{
    (void)x;
    (void)y;
    if (isShiftKey(key))
    {
        shiftHeld = false;
        return;
    }
    int dir = arrowToDir(key);
    if (dir >= 0)
        arrowHeld[dir] = false;
}

static float viewX(int px)
{
    return (px - vpX) / pixelsPerUnit;
}

static float viewY(int py)
{
    return (winH - py - vpY) / pixelsPerUnit;
}

static void rememberMouse(int x, int y)
{
    mouseX = x;
    mouseY = y;
    mouseKnown = true;
}

void my_mouse_move(int x, int y)
{
    rememberMouse(x, y);
    if (screen == PLAYING)
        return;
    int button = buttonAt(viewX(x), viewY(y));
    if (button >= 0)
        selected = button;
}

void my_mouse(int button, int state, int x, int y)
{
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        return;
    rememberMouse(x, y);
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
    int clicked = buttonAt(viewX(x), viewY(y));
    if (clicked >= 0)
        activateItem(clicked);
}

void timer(int value)
{
    (void)value;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float frameSeconds = (now - lastTime) / 1000.0f;
    lastTime = now;
    if (frameSeconds > 0.25f)
        frameSeconds = 0.25f;

    timeLeftOver += frameSeconds;
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
    glutIgnoreKeyRepeat(1);

    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
