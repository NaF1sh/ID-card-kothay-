// ============================================================
// drawing.cpp
// Small math helpers, colours, and the basic shapes used by
// every other file (circle, rectangle, line, triangle, text).
// Everything is drawn with glBegin / glEnd primitives.
// ============================================================

#include "game.h"

// ============================================================
// VECTOR MATH
// ============================================================

Vec2 makeVec(float x, float y)
{
    Vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

Vec2 vadd(Vec2 a, Vec2 b) { return makeVec(a.x + b.x, a.y + b.y); }
Vec2 vsub(Vec2 a, Vec2 b) { return makeVec(a.x - b.x, a.y - b.y); }
Vec2 vscale(Vec2 a, float s) { return makeVec(a.x * s, a.y * s); }
float vlength(Vec2 a) { return sqrtf(a.x * a.x + a.y * a.y); }
float vdot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
float vcross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

Vec2 vnormal(Vec2 a)
{
    float l = vlength(a);
    if (l > 0.000001f)
        return makeVec(a.x / l, a.y / l);
    return makeVec(0, 0);
}

float clampf(float v, float lo, float hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

float minf(float a, float b) { return a < b ? a : b; }
float maxf(float a, float b) { return a > b ? a : b; }
float degToRad(float d) { return d * PI / 180.0f; }
float radToDeg(float r) { return r * 180.0f / PI; }

bool rectContains(Rect r, Vec2 p)
{
    return p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h;
}

bool rectsOverlap(Rect a, Rect b)
{
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

Rect rectExpand(Rect r, float m)
{
    Rect e = {r.x - m, r.y - m, r.w + 2 * m, r.h + 2 * m};
    return e;
}

Vec2 rectCenter(Rect r)
{
    return makeVec(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

// The same "random" number every frame for the same (a, b).
// Used to scatter grass and flowers without storing them.
float hash01(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFFFF) / (float)0x1000000;
}

// ============================================================
// COLOURS
// ============================================================

// hexColor(0xff8800) -> orange
Color hexColor(unsigned int rgb, float a)
{
    Color c;
    c.r = ((rgb >> 16) & 0xFF) / 255.0f;
    c.g = ((rgb >> 8) & 0xFF) / 255.0f;
    c.b = (rgb & 0xFF) / 255.0f;
    c.a = a;
    return c;
}

Color mixColor(Color a, Color b, float t)
{
    Color c;
    c.r = a.r + (b.r - a.r) * t;
    c.g = a.g + (b.g - a.g) * t;
    c.b = a.b + (b.b - a.b) * t;
    c.a = a.a + (b.a - a.a) * t;
    return c;
}

Color darker(Color c, float amount)
{
    Color d = {c.r * (1 - amount), c.g * (1 - amount), c.b * (1 - amount), c.a};
    return d;
}

Color lighter(Color c, float amount)
{
    Color white = {1, 1, 1, c.a};
    return mixColor(c, white, amount);
}

Color withAlpha(Color c, float a)
{
    c.a = a;
    return c;
}

void setColor(Color c)
{
    glColor4f(c.r, c.g, c.b, c.a);
}

void setHex(unsigned int rgb, float a)
{
    setColor(hexColor(rgb, a));
}

// ============================================================
// CIRCLE (filled ellipse with GL_TRIANGLE_FAN)
// ============================================================

int autoSegments(float r)
{
    int s = (int)(r * pixelsPerUnit * 0.6f) + 10;
    if (s < 10)
        s = 10;
    if (s > 72)
        s = 72;
    return s;
}

void DrawCircle(float cx, float cy, float rx, float ry, int num_segments)
{
    if (num_segments <= 0)
        num_segments = autoSegments(maxf(rx, ry));

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= num_segments; i++)
    {
        float theta = 2.0f * PI * float(i) / float(num_segments);
        float x = rx * cosf(theta);
        float y = ry * sinf(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

void circle(float cx, float cy, float r)
{
    DrawCircle(cx, cy, r, r, 0);
}

void DrawCircleOutline(float cx, float cy, float rx, float ry, float width)
{
    int n = autoSegments(maxf(rx, ry));
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < n; i++)
    {
        float theta = 2.0f * PI * float(i) / float(n);
        glVertex2f(cx + rx * cosf(theta), cy + ry * sinf(theta));
    }
    glEnd();
}

// Centre colour fading to the rim colour: soft shadows and glows
void DrawSoftCircle(float cx, float cy, float rx, float ry, Color inner, Color outer)
{
    int n = autoSegments(maxf(rx, ry));
    glBegin(GL_TRIANGLE_FAN);
    setColor(inner);
    glVertex2f(cx, cy);
    setColor(outer);
    for (int i = 0; i <= n; i++)
    {
        float theta = 2.0f * PI * float(i) / float(n);
        glVertex2f(cx + rx * cosf(theta), cy + ry * sinf(theta));
    }
    glEnd();
}

// ============================================================
// RECTANGLES
// ============================================================

void DrawRect(float x, float y, float w, float h)
{
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void DrawRectR(Rect r)
{
    DrawRect(r.x, r.y, r.w, r.h);
}

void DrawRectOutline(float x, float y, float w, float h, float width)
{
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// Two colours: bottom edge and top edge (smooth shading)
void DrawGradientRect(float x, float y, float w, float h, Color bottom, Color top)
{
    glBegin(GL_QUADS);
    setColor(bottom);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    setColor(top);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// The corner points of a rounded rectangle (4 quarter circles)
void roundRectVertices(float x, float y, float w, float h, float r)
{
    r = minf(r, minf(w, h) * 0.5f);
    int seg = autoSegments(r) / 4;
    if (seg < 3)
        seg = 3;
    float cx[4] = {x + w - r, x + w - r, x + r, x + r};
    float cy[4] = {y + r, y + h - r, y + h - r, y + r};
    float start[4] = {-90, 0, 90, 180};
    for (int c = 0; c < 4; c++)
    {
        for (int i = 0; i <= seg; i++)
        {
            float a = degToRad(start[c] + 90.0f * i / seg);
            glVertex2f(cx[c] + cosf(a) * r, cy[c] + sinf(a) * r);
        }
    }
}

void DrawRoundRect(float x, float y, float w, float h, float radius)
{
    float r = minf(radius, minf(w, h) * 0.5f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x + w * 0.5f, y + h * 0.5f);
    roundRectVertices(x, y, w, h, radius);
    glVertex2f(x + w - r, y);
    glEnd();
}

void DrawRoundRectOutline(float x, float y, float w, float h, float radius, float width)
{
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    roundRectVertices(x, y, w, h, radius);
    glEnd();
}

// ============================================================
// ARC, SECTOR, LINES, TRIANGLE, QUAD
// ============================================================

void DrawArc(float cx, float cy, float r, float startDeg, float endDeg, float width)
{
    int n = autoSegments(r) * (int)fabsf(endDeg - startDeg) / 360;
    if (n < 4)
        n = 4;
    glLineWidth(width);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= n; i++)
    {
        float a = degToRad(startDeg + (endDeg - startDeg) * i / n);
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

void DrawSector(float cx, float cy, float r, float startDeg, float endDeg)
{
    int n = autoSegments(r) * (int)fabsf(endDeg - startDeg) / 360;
    if (n < 4)
        n = 4;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= n; i++)
    {
        float a = degToRad(startDeg + (endDeg - startDeg) * i / n);
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

void DrawLine(float x1, float y1, float x2, float y2, float width)
{
    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

// A line drawn as a thin quad, so its width scales with the window
void DrawThickLine(float x1, float y1, float x2, float y2, float width)
{
    Vec2 d = vnormal(makeVec(x2 - x1, y2 - y1));
    float nx = -d.y * width * 0.5f;
    float ny = d.x * width * 0.5f;
    glBegin(GL_QUADS);
    glVertex2f(x1 + nx, y1 + ny);
    glVertex2f(x2 + nx, y2 + ny);
    glVertex2f(x2 - nx, y2 - ny);
    glVertex2f(x1 - nx, y1 - ny);
    glEnd();
}

void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    glBegin(GL_TRIANGLES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}

void DrawQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glVertex2f(x4, y4);
    glEnd();
}

// ============================================================
// TEXT
// ============================================================

void *fontFor(int font)
{
    switch (font)
    {
    case FONT_SMALL:
        return GLUT_BITMAP_HELVETICA_10;
    case FONT_MEDIUM:
        return GLUT_BITMAP_HELVETICA_18;
    case FONT_LARGE:
        return GLUT_BITMAP_TIMES_ROMAN_24;
    default:
        return GLUT_BITMAP_HELVETICA_12;
    }
}

void DrawText(float x, float y, const char *s, int font)
{
    glRasterPos2f(x, y);
    for (int i = 0; s[i] != '\0'; i++)
        glutBitmapCharacter(fontFor(font), s[i]);
}

// Bitmap fonts have a fixed size in pixels, so divide by the
// window scale to get the width in view units.
float TextWidth(const char *s, int font)
{
    int w = 0;
    for (int i = 0; s[i] != '\0'; i++)
        w += glutBitmapWidth(fontFor(font), s[i]);
    return w / pixelsPerUnit;
}

void DrawTextCentered(float cx, float y, const char *s, int font)
{
    DrawText(cx - TextWidth(s, font) * 0.5f, y, s, font);
}

// Stroke text is made of lines, so it can be scaled and rotated
void DrawStrokeText(float x, float y, float height, const char *s, float width)
{
    float scale = height / 119.05f;
    glLineWidth(width);
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    for (int i = 0; s[i] != '\0'; i++)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, s[i]);
    glPopMatrix();
}

float StrokeTextWidth(const char *s, float height)
{
    float w = 0;
    for (int i = 0; s[i] != '\0'; i++)
        w += glutStrokeWidth(GLUT_STROKE_ROMAN, s[i]);
    return w * height / 119.05f;
}

// 75.4 seconds -> "1:15"
void formatTime(float seconds, char *out)
{
    int s = (int)seconds;
    sprintf(out, "%d:%02d", s / 60, s % 60);
}
