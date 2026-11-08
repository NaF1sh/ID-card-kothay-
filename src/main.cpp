// main.cpp - FreeGLUT setup, callback registration and the fixed-timestep
// main loop. All game behaviour lives in Game.
//
// Optional: `IDCardKothay.exe --screenshot out.bmp [seconds]` renders the scene
// after the given simulated time, saves a BMP and exits (used for testing).
// `--hold wd` additionally keeps those keys pressed from the start, and
// `--start x,y` places the student at a world position, `--level N` starts
// at level N (1-4).
#include <GL/freeglut.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Game.h"
#include "Graphics.h"

#ifndef GL_BGR_EXT
#define GL_BGR_EXT 0x80E0
#endif

namespace {
Game g_game;
int g_lastMs = 0;
float g_accumulator = 0.0f;
constexpr float FIXED_DT = 1.0f / 120.0f;  // simulation runs at 120 Hz
constexpr float MAX_FRAME = 0.25f;         // clamp after stalls (window drag etc.)

std::string g_shotPath;
std::string g_holdKeys;
int g_startLevel = -1;
bool g_hasStart = false;
float g_startX = 0.0f, g_startY = 0.0f;
float g_shotTime = 1.0f;
int g_winW = 1280, g_winH = 768;

void saveBmp(const std::string& path, int w, int h) {
    int rowBytes = (w * 3 + 3) & ~3;
    std::vector<unsigned char> px(static_cast<size_t>(rowBytes) * h);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, px.data());
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    unsigned int fileSize = 54 + static_cast<unsigned int>(px.size());
    unsigned char hdr[54] = {'B', 'M'};
    auto put32 = [&](int off, unsigned int v) { std::memcpy(hdr + off, &v, 4); };
    put32(2, fileSize);
    put32(10, 54);
    put32(14, 40);
    put32(18, static_cast<unsigned int>(w));
    put32(22, static_cast<unsigned int>(h));
    hdr[26] = 1;
    hdr[28] = 24;
    put32(34, static_cast<unsigned int>(px.size()));
    std::fwrite(hdr, 1, 54, f);
    std::fwrite(px.data(), 1, px.size(), f);
    std::fclose(f);
}

void display() {
    g_game.render();
    if (!g_shotPath.empty() && g_game.time() >= g_shotTime) {
        glFinish();
        saveBmp(g_shotPath, g_winW, g_winH);
        glutLeaveMainLoop();
        return;
    }
    glutSwapBuffers();
}

void reshape(int w, int h) {
    g_winW = w;
    g_winH = h;
    g_game.reshape(w, h);
}

void tick(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    float frame = (now - g_lastMs) / 1000.0f;
    g_lastMs = now;
    if (!g_shotPath.empty()) frame = 1.0f / 60.0f;  // deterministic in screenshot mode
    if (frame > MAX_FRAME) frame = MAX_FRAME;

    // Fixed timestep: simulation speed is independent of the frame rate.
    g_accumulator += frame;
    while (g_accumulator >= FIXED_DT) {
        g_game.update(FIXED_DT);
        g_accumulator -= FIXED_DT;
    }
    if (g_game.wantsQuit()) {
        glutLeaveMainLoop();
        return;
    }
    // Hidden screenshot windows do not receive paint events on Windows.
    // Render explicitly once the deterministic capture time has elapsed.
    if (!g_shotPath.empty() && g_game.time() >= g_shotTime) {
        display();
        return;
    }
    glutPostRedisplay();
    glutTimerFunc(16, tick, 0);
}

void keyboard(unsigned char k, int, int) { g_game.keyDown(k); }
void keyboardUp(unsigned char k, int, int) { g_game.keyUp(k); }
void special(int k, int, int) { g_game.specialDown(k); }
void specialUp(int k, int, int) { g_game.specialUp(k); }
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) g_game.mouseClick(x, y);
}
void motion(int x, int y) { g_game.mouseMove(x, y); }
}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            g_shotPath = argv[++i];
            if (i + 1 < argc && argv[i + 1][0] != '-') g_shotTime = static_cast<float>(std::atof(argv[++i]));
        } else if (std::strcmp(argv[i], "--hold") == 0 && i + 1 < argc) {
            g_holdKeys = argv[++i];
        } else if (std::strcmp(argv[i], "--level") == 0 && i + 1 < argc) {
            g_startLevel = std::atoi(argv[++i]) - 1;
        } else if (std::strcmp(argv[i], "--start") == 0 && i + 1 < argc) {
            g_hasStart = std::sscanf(argv[++i], "%f,%f", &g_startX, &g_startY) == 2;
        }
    }

    glutInit(&argc, argv);
    glutSetOption(GLUT_MULTISAMPLE, 8);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowSize(g_winW, g_winH);
    glutCreateWindow("Museum Detective Case");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    gfx::initRenderState();
    g_game.init();
    g_game.reshape(g_winW, g_winH);
    if (g_startLevel >= 0) g_game.startAtLevel(g_startLevel);
    if (g_hasStart) g_game.placePlayer({g_startX, g_startY});
    for (char k : g_holdKeys) g_game.keyDown(static_cast<unsigned char>(k));

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(motion);
    glutMotionFunc(motion);  // keep aiming accurate while a mouse button is held
    glutIgnoreKeyRepeat(1);  // we track key state ourselves

    g_lastMs = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, tick, 0);
    glutMainLoop();
    return 0;
}
