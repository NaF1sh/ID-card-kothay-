#include "Graphics.h"

#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

namespace gfx {

namespace {
int g_winW = 1, g_winH = 1;
int g_vpX = 0, g_vpY = 0, g_vpW = 1, g_vpH = 1;
float g_ppu = 1.0f;  // window pixels per world unit

int autoSegments(float r) {
    // More segments for bigger on-screen circles keeps edges smooth but cheap.
    int s = static_cast<int>(r * g_ppu * 0.6f) + 10;
    return std::clamp(s, 10, 72);
}

void* glutFont(Font f) {
    switch (f) {
        case Font::Small:  return GLUT_BITMAP_HELVETICA_10;
        case Font::Normal: return GLUT_BITMAP_HELVETICA_12;
        case Font::Medium: return GLUT_BITMAP_HELVETICA_18;
        case Font::Large:  return GLUT_BITMAP_TIMES_ROMAN_24;
        case Font::Title:  return GLUT_BITMAP_TIMES_ROMAN_24;
    }
    return GLUT_BITMAP_HELVETICA_12;
}

void ellipseVertex(float cx, float cy, float rx, float ry, float rad) {
    glVertex2f(cx + std::cos(rad) * rx, cy + std::sin(rad) * ry);
}
}  // namespace

// ---- colour --------------------------------------------------------------
Color mix(const Color& a, const Color& b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}
Color darker(const Color& c, float amount) { return {c.r * (1 - amount), c.g * (1 - amount), c.b * (1 - amount), c.a}; }
Color lighter(const Color& c, float amount) { return mix(c, {1, 1, 1, c.a}, amount); }
Color withAlpha(const Color& c, float a) { return {c.r, c.g, c.b, a}; }

// ---- projection ----------------------------------------------------------
void initRenderState() {
    glEnable(GL_BLEND);                                 // alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);                           // MSAA anti-aliasing (if available)
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glDisable(GL_DEPTH_TEST);                           // painter's algorithm layering
}

void setupProjection(int winW, int winH) {
    g_winW = std::max(1, winW);
    g_winH = std::max(1, winH);
    float s = std::min(g_winW / VIEW_W, g_winH / VIEW_H);
    g_vpW = static_cast<int>(VIEW_W * s);
    g_vpH = static_cast<int>(VIEW_H * s);
    g_vpX = (g_winW - g_vpW) / 2;
    g_vpY = (g_winH - g_vpH) / 2;
    g_ppu = s;
}

void beginFrame(const Color& letterbox) {
    // Clear the whole window (letterbox bars), then restrict drawing to the
    // aspect-correct viewport and install the orthographic projection
    // (one screen = VIEW_W x VIEW_H units).
    glViewport(0, 0, g_winW, g_winH);
    glClearColor(letterbox.r, letterbox.g, letterbox.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(g_vpX, g_vpY, g_vpW, g_vpH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, VIEW_W, 0.0, VIEW_H, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

float pixelsPerUnit() { return g_ppu; }

void worldViewport(bool enabled) {
    glViewport(g_vpX, g_vpY + (enabled ? int(PLAY_BOTTOM * g_ppu) : 0),
               g_vpW, enabled ? int(PLAY_H * g_ppu) : g_vpH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, VIEW_W, 0, enabled ? PLAY_H : VIEW_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

Vec2 windowToView(int px, int py) {
    // Undo the letterbox offset and scale; flip y (GLUT measures from the top).
    return {(px - g_vpX) / g_ppu, (g_winH - py - g_vpY) / g_ppu};
}

// ---- primitives ------------------------------------------------------------
void setColor(const Color& c) { glColor4f(c.r, c.g, c.b, c.a); }

void rect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void rect(const Rect& r) { rect(r.x, r.y, r.w, r.h); }

void rectOutline(float x, float y, float w, float h, float lineWidth) {
    glLineWidth(lineWidth);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void gradientRectV(float x, float y, float w, float h, const Color& bottom, const Color& top) {
    glBegin(GL_QUADS);
    setColor(bottom);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    setColor(top);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

static void roundedRectVerts(float x, float y, float w, float h, float r) {
    r = std::min(r, std::min(w, h) * 0.5f);
    const int seg = std::max(3, autoSegments(r) / 4);
    const float cx[4] = {x + w - r, x + w - r, x + r, x + r};
    const float cy[4] = {y + r, y + h - r, y + h - r, y + r};
    const float start[4] = {-90.0f, 0.0f, 90.0f, 180.0f};
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i <= seg; ++i) {
            float a = degToRad(start[c] + 90.0f * i / seg);
            glVertex2f(cx[c] + std::cos(a) * r, cy[c] + std::sin(a) * r);
        }
    }
}

void roundedRect(float x, float y, float w, float h, float radius) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x + w * 0.5f, y + h * 0.5f);
    roundedRectVerts(x, y, w, h, radius);
    // close the fan on the first rim vertex
    float r = std::min(radius, std::min(w, h) * 0.5f);
    glVertex2f(x + w - r, y);
    glEnd();
}

void roundedRectOutline(float x, float y, float w, float h, float radius, float lineWidth) {
    glLineWidth(lineWidth);
    glBegin(GL_LINE_LOOP);
    roundedRectVerts(x, y, w, h, radius);
    glEnd();
}

void circle(float cx, float cy, float r, int segments) { ellipse(cx, cy, r, r, segments); }

void circleOutline(float cx, float cy, float r, float lineWidth, int segments) {
    ellipseOutline(cx, cy, r, r, lineWidth, segments);
}

void ellipse(float cx, float cy, float rx, float ry, int segments) {
    int seg = segments > 0 ? segments : autoSegments(std::max(rx, ry));
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; ++i) ellipseVertex(cx, cy, rx, ry, 2.0f * PI_F * i / seg);
    glEnd();
}

void ellipseOutline(float cx, float cy, float rx, float ry, float lineWidth, int segments) {
    int seg = segments > 0 ? segments : autoSegments(std::max(rx, ry));
    glLineWidth(lineWidth);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < seg; ++i) ellipseVertex(cx, cy, rx, ry, 2.0f * PI_F * i / seg);
    glEnd();
}

void softEllipse(float cx, float cy, float rx, float ry, const Color& inner, const Color& outer, int segments) {
    int seg = segments > 0 ? segments : autoSegments(std::max(rx, ry));
    glBegin(GL_TRIANGLE_FAN);
    setColor(inner);
    glVertex2f(cx, cy);
    setColor(outer);
    for (int i = 0; i <= seg; ++i) ellipseVertex(cx, cy, rx, ry, 2.0f * PI_F * i / seg);
    glEnd();
}

void arc(float cx, float cy, float r, float startDeg, float endDeg, float lineWidth, int segments) {
    int seg = segments > 0 ? segments : std::max(4, autoSegments(r) * static_cast<int>(std::fabs(endDeg - startDeg)) / 360);
    glLineWidth(lineWidth);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= seg; ++i) {
        float a = degToRad(startDeg + (endDeg - startDeg) * i / seg);
        glVertex2f(cx + std::cos(a) * r, cy + std::sin(a) * r);
    }
    glEnd();
}

void sector(float cx, float cy, float r, float startDeg, float endDeg, int segments) {
    int seg = segments > 0 ? segments : std::max(4, autoSegments(r) * static_cast<int>(std::fabs(endDeg - startDeg)) / 360);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; ++i) {
        float a = degToRad(startDeg + (endDeg - startDeg) * i / seg);
        glVertex2f(cx + std::cos(a) * r, cy + std::sin(a) * r);
    }
    glEnd();
}

void ringSector(float cx, float cy, float innerR, float outerR, float startDeg, float endDeg, int segments) {
    int seg = segments > 0 ? segments : std::max(4, autoSegments(outerR) * static_cast<int>(std::fabs(endDeg - startDeg)) / 360);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= seg; ++i) {
        float a = degToRad(startDeg + (endDeg - startDeg) * i / seg);
        float c = std::cos(a), s = std::sin(a);
        glVertex2f(cx + c * innerR, cy + s * innerR);
        glVertex2f(cx + c * outerR, cy + s * outerR);
    }
    glEnd();
}

void line(float x1, float y1, float x2, float y2, float lineWidth) {
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

void lines(const std::vector<Vec2>& pairs, float lineWidth) {
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
    for (const Vec2& p : pairs) glVertex2f(p.x, p.y);
    glEnd();
}

void thickLine(float x1, float y1, float x2, float y2, float width) {
    Vec2 d = Vec2{x2 - x1, y2 - y1}.normalized();
    Vec2 n{-d.y * width * 0.5f, d.x * width * 0.5f};
    glBegin(GL_QUADS);
    glVertex2f(x1 + n.x, y1 + n.y);
    glVertex2f(x2 + n.x, y2 + n.y);
    glVertex2f(x2 - n.x, y2 - n.y);
    glVertex2f(x1 - n.x, y1 - n.y);
    glEnd();
}

void triangle(Vec2 a, Vec2 b, Vec2 c) {
    glBegin(GL_TRIANGLES);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glVertex2f(c.x, c.y);
    glEnd();
}

void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
    glBegin(GL_QUADS);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glVertex2f(c.x, c.y);
    glVertex2f(d.x, d.y);
    glEnd();
}

void polygon(const std::vector<Vec2>& pts) {
    glBegin(GL_POLYGON);
    for (const Vec2& p : pts) glVertex2f(p.x, p.y);
    glEnd();
}

void polygonOutline(const std::vector<Vec2>& pts, float lineWidth) {
    glLineWidth(lineWidth);
    glBegin(GL_LINE_LOOP);
    for (const Vec2& p : pts) glVertex2f(p.x, p.y);
    glEnd();
}

void points(const std::vector<Vec2>& pts, float size) {
    glPointSize(size);
    glBegin(GL_POINTS);
    for (const Vec2& p : pts) glVertex2f(p.x, p.y);
    glEnd();
}

// ---- transformations -------------------------------------------------------
void pushTransform(float x, float y, float deg, float sx, float sy) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    if (deg != 0.0f) glRotatef(deg, 0.0f, 0.0f, 1.0f);
    if (sx != 1.0f || sy != 1.0f) glScalef(sx, sy, 1.0f);
}

void popTransform() { glPopMatrix(); }

// ---- text --------------------------------------------------------------------
void text(float x, float y, const std::string& s, Font f) {
    glRasterPos2f(x, y);
    glutBitmapString(glutFont(f), reinterpret_cast<const unsigned char*>(s.c_str()));
}

void textCentered(float cx, float y, const std::string& s, Font f) { text(cx - textWidth(s, f) * 0.5f, y, s, f); }

float textWidth(const std::string& s, Font f) {
    return glutBitmapLength(glutFont(f), reinterpret_cast<const unsigned char*>(s.c_str())) / g_ppu;
}

float textHeight(Font f) { return glutBitmapHeight(glutFont(f)) / g_ppu; }

void strokeText(float x, float y, float height, const std::string& s, float lineWidth) {
    // Stroke fonts are line-segment geometry: they scale with glScalef.
    const float scale = height / 119.05f;
    glLineWidth(lineWidth);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);
    for (unsigned char ch : s) glutStrokeCharacter(GLUT_STROKE_ROMAN, ch);
    glPopMatrix();
}

float strokeTextWidth(const std::string& s, float height) {
    return glutStrokeLength(GLUT_STROKE_ROMAN, reinterpret_cast<const unsigned char*>(s.c_str())) * height / 119.05f;
}

float hash01(int a, int b) {
    uint32_t h = static_cast<uint32_t>(a) * 374761393u + static_cast<uint32_t>(b) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFFFF) / static_cast<float>(0x1000000);
}

}  // namespace gfx
