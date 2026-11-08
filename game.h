// ============================================================
// MUSEUM DETECTIVE CASE
// 2D top-down stealth game - GLUT / OpenGL
// game.h : constants, structures, global variables and
//          function prototypes shared by every .cpp file
// ============================================================

#ifndef GAME_H
#define GAME_H

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// freeglut sends Shift as a special key with these codes
// (they are in freeglut_ext.h, not in glut.h)
#ifndef GLUT_KEY_SHIFT_L
#define GLUT_KEY_SHIFT_L 0x0070
#define GLUT_KEY_SHIFT_R 0x0071
#endif

// ============================================================
// SCREEN AND WORLD SIZE
// ============================================================
// glOrtho(-500, 500, -300, 300) gives a 1000 x 600 view.
// The museum (world) is 1000 x 478. The HUD uses the band
// at the bottom (58 units) and at the top (64 units).

#define VIEW_W 1000.0f
#define VIEW_H 600.0f
#define WORLD_W 1000.0f
#define WORLD_H 478.0f
#define PLAY_BOTTOM 58.0f
#define PLAY_TOP 536.0f

#define PI 3.14159265f

// ============================================================
// BASIC STRUCTURES
// ============================================================

struct Vec2
{
    float x, y;
};

// x, y is the bottom-left corner, y grows upward
struct Rect
{
    float x, y, w, h;
};

struct Color
{
    float r, g, b, a;
};

// ============================================================
// GAME STATES
// ============================================================

enum GameState
{
    MAIN_MENU,
    NAME_ENTRY,
    SCOREBOARD,
    INSTRUCTIONS,
    BRIEFING,
    PLAYING,
    PAUSED,
    DIALOGUE,
    CAUGHT,
    TIMED_OUT,
    LEVEL_COMPLETE,
    WIN
};

enum Direction
{
    DIR_DOWN,
    DIR_UP,
    DIR_LEFT,
    DIR_RIGHT
};

// ============================================================
// MAP OBJECTS
// ============================================================

enum BuildingKind
{
    HALL,     // big museum hall (blue)
    OFFICE,   // office building (brown)
    GALLERY,  // gallery with striped awning
    BOOTH     // small security desk
};

enum PathKind
{
    ROAD,
    PAVEMENT,
    PLAZA
};

enum TreeKind
{
    RAIN_TREE,
    FLAME_TREE,
    MANGO_TREE
};

struct Building
{
    Rect r;
    int kind;
    const char *name;
    float doorX;
};

struct PathSeg
{
    Rect r;
    int kind;
};

struct Tree
{
    Vec2 p;
    float radius;
    int kind;
};

struct Bush
{
    Vec2 p;
    float radius;
};

struct Bench
{
    Vec2 p;
    bool vertical;
};

struct Railing
{
    Vec2 a, b;
};

struct Table
{
    Vec2 p;
    int colorIndex;
};

#define MAX_OBJ 16
#define MAX_SOLID 80

struct Map
{
    PathSeg paths[MAX_OBJ];
    int pathCount;
    Rect crossings[MAX_OBJ];
    int crossingCount;
    Building buildings[MAX_OBJ];
    int buildingCount;
    Rect walls[MAX_OBJ];
    int wallCount;
    Rect lakes[MAX_OBJ];
    int lakeCount;
    Rect fields[MAX_OBJ];
    int fieldCount;
    Tree trees[MAX_OBJ];
    int treeCount;
    Bush bushes[MAX_OBJ];
    int bushCount;
    Rect flowerBeds[MAX_OBJ];
    int flowerBedCount;
    Bench benches[MAX_OBJ];
    int benchCount;
    Vec2 lamps[MAX_OBJ];
    int lampCount;
    Railing railings[MAX_OBJ];
    int railingCount;
    Table tables[MAX_OBJ];
    int tableCount;
    Vec2 flags[MAX_OBJ];
    int flagCount;
    Rect signs[MAX_OBJ];
    int signCount;

    bool hasGate;
    float gateX0, gateX1;

    // filled by finishMap(): what blocks feet and what blocks sight
    Rect solids[MAX_SOLID];
    int solidCount;
    Rect blockers[MAX_SOLID];
    int blockerCount;
    Vec2 circlePos[MAX_SOLID];
    float circleR[MAX_SOLID];
    int circleCount;
};

// ============================================================
// PLAYER
// ============================================================

#define PLAYER_RADIUS 8.0f
#define PLAYER_SPEED 115.0f
#define SPRINT_MULT 1.7f
#define STAMINA_DRAIN 0.4f
#define STAMINA_REGEN 0.22f

struct Player
{
    Vec2 pos;
    Vec2 vel;
    int facing;
    float walkPhase;
    float moveBlend;
    float stamina;
    bool sprinting;
    bool exhausted;
};

// ============================================================
// GUARD
// ============================================================

#define GUARD_RADIUS 9.0f
#define MAX_GUARDS 4
#define MAX_WAYPOINTS 4
#define MAX_PATH 300

enum GuardState
{
    PATROLLING,
    SEARCHING,
    ALERT,
    GUARD_CAUGHT
};

struct Waypoint
{
    Vec2 p;
    float searchTime;
};

struct Guard
{
    // settings from the level
    Waypoint route[MAX_WAYPOINTS];
    int routeCount;
    float speed;
    float fov;
    float range;
    float searchSweep;
    float startAngle;
    float detectionTimeScale;
    float chaseSpeed;
    float hearingRadius;
    float forgetTime;

    // live state
    Vec2 pos;
    float angle;
    int state;
    int target;
    float stateTimer;
    float searchBase;
    Vec2 lastSeen;
    float detection;
    bool seeing;
    float walkPhase;
    float moveBlend;
    float clock;

    // A* path
    Vec2 path[MAX_PATH];
    int pathCount;
    int pathIndex;
    Vec2 pathTarget;
    float pathTimer;
};

// ============================================================
// LEVEL
// ============================================================

struct Level
{
    const char *name;
    const char *objective;
    const char *briefing;
    const char *reward;
    const char *passNames[3];
    int passCount;
    Vec2 spawn;
    Rect goal;
    Vec2 checkpoints[3];
    float timeLimit;
};

#define LEVEL_COUNT 4

// ============================================================
// NPCs AND CATS
// ============================================================

enum NpcKind
{
    CONSULTANT,
    AGENT,
    SELLER,
    POET,
    CURATOR,
    GAMER
};

#define NPC_COUNT 6
#define MAX_LINES 8

struct Npc
{
    Vec2 position;
    const char *name;
    const char *role;
    int kind;
    const char *lines[MAX_LINES];
    int lineCount;
    int visits;
    char lastTask[300];
};

struct CatPose
{
    Vec2 position;
    Vec2 direction;
    bool moving;
    bool happy;
};

#define MAX_ROUTE 300
#define CROWD_COUNT 6
#define CAT_COUNT 4

// ============================================================
// SCOREBOARD
// ============================================================

struct ScoreEntry
{
    char name[20];
    int score;
    float seconds;
};

struct LevelResult
{
    float time;
    int timeBonus;
    int stealthBonus;
    int retryPenalty;
    int levelScore;
    int totalScore;
    float totalTime;
};

// ============================================================
// FONTS
// ============================================================

enum FontSize
{
    FONT_SMALL,
    FONT_NORMAL,
    FONT_MEDIUM,
    FONT_LARGE
};

// ============================================================
// GLOBAL VARIABLES (defined in main.cpp)
// ============================================================

extern int screen;
extern int selected;
extern float screenTime;
extern float uiTime;
extern float worldTime;
extern bool quitGame;

extern Map map;
extern Level level;
extern int levelIndex;
extern Player player;
extern Guard guards[MAX_GUARDS];
extern int guardCount;
extern Npc npcs[NPC_COUNT];

extern Vec2 crowdRoute[CROWD_COUNT][MAX_ROUTE];
extern int crowdLength[CROWD_COUNT];
extern int crowdCount;
extern Vec2 catRoute[CAT_COUNT][MAX_ROUTE];
extern int catLength[CAT_COUNT];
extern int catCount;
extern float catClock[CAT_COUNT];
extern float catAffection[CAT_COUNT];

// keyboard state (filled by key down / key up callbacks)
extern bool letterHeld[4];
extern bool arrowHeld[4];
extern bool shiftHeld;
extern bool spaceHeld;

// mouse
extern int mouseX, mouseY;
extern bool mouseKnown;

// window / letterbox
extern int winW, winH;
extern int vpX, vpY, vpW, vpH;
extern float pixelsPerUnit;

// gameplay
extern float levelTime;
extern float totalTime;
extern int totalScore;
extern int retries;
extern bool awaitingDeparture;
extern bool spotted;
extern bool alarmed;
extern bool collected[3];
extern float pickupTime;
extern Vec2 pickupPos;
extern float petTime;
extern int petCat;
extern float alertCueCooldown;
extern int distractions;
extern float distractionCooldown;
extern float noiseTime;
extern Vec2 noisePos;
extern float radioCooldown;
extern char message[300];
extern float messageTime;
extern int catcher;
extern bool showDebug;

extern char playerName[20];
extern char nameDraft[20];
extern bool rankedRun;
extern char scoreStatus[120];
extern LevelResult result;

extern char dialogueText[300];
extern const char *speakerName;
extern const char *speakerRole;
extern int dialogueCharacter;
extern float dialogueTime;

extern ScoreEntry scores[11];
extern int scoreCount;

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================

// ---- vector math (drawing.cpp) ----
Vec2 makeVec(float x, float y);
Vec2 vadd(Vec2 a, Vec2 b);
Vec2 vsub(Vec2 a, Vec2 b);
Vec2 vscale(Vec2 a, float s);
float vlength(Vec2 a);
Vec2 vnormal(Vec2 a);
float vdot(Vec2 a, Vec2 b);
float vcross(Vec2 a, Vec2 b);
float clampf(float v, float lo, float hi);
float minf(float a, float b);
float maxf(float a, float b);
float degToRad(float d);
float radToDeg(float r);
bool rectContains(Rect r, Vec2 p);
bool rectsOverlap(Rect a, Rect b);
Rect rectExpand(Rect r, float m);
Vec2 rectCenter(Rect r);
float hash01(int a, int b);

// ---- colours (drawing.cpp) ----
Color hexColor(unsigned int rgb, float a = 1.0f);
Color mixColor(Color a, Color b, float t);
Color darker(Color c, float amount);
Color lighter(Color c, float amount);
Color withAlpha(Color c, float a);
void setColor(Color c);
void setHex(unsigned int rgb, float a = 1.0f);

// ---- primitives (drawing.cpp) ----
void DrawCircle(float cx, float cy, float rx, float ry, int num_segments);
void circle(float cx, float cy, float r);
void DrawCircleOutline(float cx, float cy, float rx, float ry, float width);
void DrawSoftCircle(float cx, float cy, float rx, float ry, Color inner, Color outer);
void DrawRect(float x, float y, float w, float h);
void DrawRectR(Rect r);
void DrawRectOutline(float x, float y, float w, float h, float width);
void DrawGradientRect(float x, float y, float w, float h, Color bottom, Color top);
void DrawRoundRect(float x, float y, float w, float h, float radius);
void DrawRoundRectOutline(float x, float y, float w, float h, float radius, float width);
void DrawArc(float cx, float cy, float r, float startDeg, float endDeg, float width);
void DrawSector(float cx, float cy, float r, float startDeg, float endDeg);
void DrawLine(float x1, float y1, float x2, float y2, float width);
void DrawThickLine(float x1, float y1, float x2, float y2, float width);
void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3);
void DrawQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);

// ---- text (drawing.cpp) ----
void DrawText(float x, float y, const char *s, int font);
void DrawTextCentered(float cx, float y, const char *s, int font);
float TextWidth(const char *s, int font);
void DrawStrokeText(float x, float y, float height, const char *s, float width);
float StrokeTextWidth(const char *s, float height);
void formatTime(float seconds, char *out);

// ---- collision.cpp ----
bool circleVsRect(Vec2 c, float r, Rect rect);
bool overlapsSolid(Vec2 c, float r);
Vec2 clampToWorld(Vec2 c, float r);
Vec2 moveCircle(Vec2 from, Vec2 delta, float r);
float raycast(Vec2 origin, Vec2 dir, float maxDist);
bool lineOfSight(Vec2 a, Vec2 b);
bool clearPath(Vec2 from, Vec2 to, float radius);
int findPath(Vec2 from, Vec2 to, float radius, Vec2 out[], int maxOut);
Vec2 nearestFreeSpot(Vec2 p, float radius);

// ---- level.cpp ----
void loadLevel(int index);
void finishMap();
const char *levelObjective(int index);

// ---- map.cpp (scenery) ----
void drawGround(float t);
void drawCanopies(float t);
void drawCollisionDebug();
void drawBuilding(int i, float t);
void drawWall(int i);
void drawBush(int i);
void drawBench(int i);
void drawLampBase(int i);
void drawTreeTrunk(int i);
void drawTableBase(int i);
void drawRailing(int i);
void drawFlagPole(int i);
void drawSign(int i);
void drawGate();

// ---- characters.cpp ----
enum CharacterStyleId
{
    STYLE_DETECTIVE,
    STYLE_GUARD
};
void useStyle(int id);
void setShirt(unsigned int rgb);
void setBackpack(bool on);
void drawCharacter(int facing, float walkPhase, float moveBlend);
void drawCharacterShadow(Vec2 feet, float scale);
int directionFromAngle(float deg);
void setupNpcs();
void drawNpc(int i, float x, float y, float t);
void drawCat(CatPose pose, float t, int index);

// ---- player.cpp ----
void resetPlayer(Vec2 spawn);
void updatePlayer(float dt, float ax, float ay, bool sprint);
void drawPlayer(float crouch);

// ---- guard.cpp ----
void resetGuard(Guard &g);
void updateGuard(Guard &g, float dt, Vec2 playerPos, bool playerNoisy);
void guardHearNoise(Guard &g, Vec2 source, float radius);
void guardReceiveReport(Guard &g, Vec2 lastKnown);
void drawVisionCone(Guard &g);
void drawGuard(Guard &g);
void drawGuardIndicator(Guard &g, float t);
void drawSpeechBubble(Guard &g);
void drawGuardRoute(Guard &g);

// ---- gameplay.cpp ----
void setScreen(int s);
void startLevel(int index);
void restartLevel();
void retryLevel();
void goToMenu();
void updateGame(float dt);
void activateItem(int item);
int menuItemCount();
const char *menuItem(int i);
int collectedCount();
int nearbyPass();
int nearbyCharacter();
Vec2 distractionTarget();
bool mouseAimValid();
void throwDistraction();
void interact();
void advanceDialogue();
CatPose catPose(int i);
Vec2 routePose(Vec2 route[], int length, float distance, Vec2 *direction);
float remainingTime();
void drawWorld(bool showPlayer);

// ---- ui.cpp ----
void beginButtons();
int buttonAt(float x, float y);
void drawMainMenu();
void drawNameEntry();
void drawScoreboard();
void drawInstructions();
void drawBriefing();
void drawPauseMenu();
void drawDialogue();
void drawGameOver();
void drawTimeout();
void drawLevelComplete();
void drawVictory();
void drawHud();

// ---- scoreboard.cpp ----
void loadScores();
bool recordScore(const char *name, int score, float seconds);
void cleanName(const char *in, char *out);

// ---- sound.cpp ----
enum SoundCue
{
    SOUND_PICKUP,
    SOUND_COMPLETE,
    SOUND_DISTRACTION,
    SOUND_PET,
    SOUND_ALERT,
    SOUND_CAUGHT
};
void playSound(int cue);
void stopSound();

#endif
