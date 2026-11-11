#include "game.h"

#define STROKE_FONT_HEIGHT 119.05f

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
    float length = vlength(a);
    if (length > 0.000001f)
        return makeVec(a.x / length, a.y / length);
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

float minf(float a, float b)
{
    if (a < b)
        return a;
    return b;
}

float maxf(float a, float b)
{
    if (a > b)
        return a;
    return b;
}

float degToRad(float d) { return d * PI / 180.0f; }
float radToDeg(float r) { return r * 180.0f / PI; }

bool rectContains(Rect r, Vec2 p)
{
    bool insideX = p.x >= r.x && p.x <= r.x + r.w;
    bool insideY = p.y >= r.y && p.y <= r.y + r.h;
    return insideX && insideY;
}

bool rectsOverlap(Rect a, Rect b)
{
    bool overlapX = a.x < b.x + b.w && b.x < a.x + a.w;
    bool overlapY = a.y < b.y + b.h && b.y < a.y + a.h;
    return overlapX && overlapY;
}

Rect rectExpand(Rect r, float margin)
{
    Rect bigger = {r.x - margin, r.y - margin, r.w + 2 * margin, r.h + 2 * margin};
    return bigger;
}

Vec2 rectCenter(Rect r)
{
    return makeVec(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

float hash01(int a, int b)
{
    unsigned int hash = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    hash = (hash ^ (hash >> 13)) * 1274126177u;
    hash ^= hash >> 16;
    return (hash & 0xFFFFFF) / (float)0x1000000;
}

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
    float keep = 1 - amount;
    Color dark = {c.r * keep, c.g * keep, c.b * keep, c.a};
    return dark;
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

static int autoSegments(float radius)
{
    int segments = (int)(radius * pixelsPerUnit * 0.6f) + 10;
    if (segments < 10)
        segments = 10;
    if (segments > 72)
        segments = 72;
    return segments;
}

static void ellipseVertex(float cx, float cy, float rx, float ry, int i, int segments)
{
    float theta = 2.0f * PI * float(i) / float(segments);
    float x = cx + rx * cosf(theta);
    float y = cy + ry * sinf(theta);
    glVertex2f(x, y);
}

void DrawCircle(float cx, float cy, float rx, float ry, int num_segments)
{
    if (num_segments <= 0)
        num_segments = autoSegments(maxf(rx, ry));

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= num_segments; i++)
        ellipseVertex(cx, cy, rx, ry, i, num_segments);
    glEnd();
}

void circle(float cx, float cy, float r)
{
    DrawCircle(cx, cy, r, r, 0);
}

void DrawCircleOutline(float cx, float cy, float rx, float ry, float width)
{
    int segments = autoSegments(maxf(rx, ry));
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++)
        ellipseVertex(cx, cy, rx, ry, i, segments);
    glEnd();
}

void DrawSoftCircle(float cx, float cy, float rx, float ry, Color inner, Color outer)
{
    int segments = autoSegments(maxf(rx, ry));
    glBegin(GL_TRIANGLE_FAN);
    setColor(inner);
    glVertex2f(cx, cy);
    setColor(outer);
    for (int i = 0; i <= segments; i++)
        ellipseVertex(cx, cy, rx, ry, i, segments);
    glEnd();
}

static void rectVertices(float x, float y, float w, float h)
{
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
}

void DrawRect(float x, float y, float w, float h)
{
    glBegin(GL_QUADS);
    rectVertices(x, y, w, h);
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
    rectVertices(x, y, w, h);
    glEnd();
}

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

static float cornerRadius(float w, float h, float radius)
{
    float halfShortSide = minf(w, h) * 0.5f;
    return minf(radius, halfShortSide);
}

static void roundRectVertices(float x, float y, float w, float h, float radius)
{
    float r = cornerRadius(w, h, radius);
    int segments = autoSegments(r) / 4;
    if (segments < 3)
        segments = 3;

    float cornerX[4] = {x + w - r, x + w - r, x + r, x + r};
    float cornerY[4] = {y + r, y + h - r, y + h - r, y + r};
    float startAngle[4] = {-90, 0, 90, 180};

    for (int c = 0; c < 4; c++)
    {
        for (int i = 0; i <= segments; i++)
        {
            float angle = degToRad(startAngle[c] + 90.0f * i / segments);
            glVertex2f(cornerX[c] + cosf(angle) * r, cornerY[c] + sinf(angle) * r);
        }
    }
}

void DrawRoundRect(float x, float y, float w, float h, float radius)
{
    float r = cornerRadius(w, h, radius);
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

static int arcSegments(float r, float startDeg, float endDeg)
{
    int degrees = (int)fabsf(endDeg - startDeg);
    int segments = autoSegments(r) * degrees / 360;
    if (segments < 4)
        segments = 4;
    return segments;
}

static void arcVertices(float cx, float cy, float r, float startDeg, float endDeg, int segments)
{
    for (int i = 0; i <= segments; i++)
    {
        float angle = degToRad(startDeg + (endDeg - startDeg) * i / segments);
        glVertex2f(cx + cosf(angle) * r, cy + sinf(angle) * r);
    }
}

void DrawArc(float cx, float cy, float r, float startDeg, float endDeg, float width)
{
    int segments = arcSegments(r, startDeg, endDeg);
    glLineWidth(width);
    glBegin(GL_LINE_STRIP);
    arcVertices(cx, cy, r, startDeg, endDeg, segments);
    glEnd();
}

void DrawSector(float cx, float cy, float r, float startDeg, float endDeg)
{
    int segments = arcSegments(r, startDeg, endDeg);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    arcVertices(cx, cy, r, startDeg, endDeg, segments);
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

void DrawThickLine(float x1, float y1, float x2, float y2, float width)
{
    Vec2 direction = vnormal(makeVec(x2 - x1, y2 - y1));
    float sideX = -direction.y * width * 0.5f;
    float sideY = direction.x * width * 0.5f;
    glBegin(GL_QUADS);
    glVertex2f(x1 + sideX, y1 + sideY);
    glVertex2f(x2 + sideX, y2 + sideY);
    glVertex2f(x2 - sideX, y2 - sideY);
    glVertex2f(x1 - sideX, y1 - sideY);
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

static void *fontFor(int font)
{
    if (font == FONT_SMALL)
        return GLUT_BITMAP_HELVETICA_10;
    if (font == FONT_MEDIUM)
        return GLUT_BITMAP_HELVETICA_18;
    if (font == FONT_LARGE)
        return GLUT_BITMAP_TIMES_ROMAN_24;
    return GLUT_BITMAP_HELVETICA_12;
}

void DrawText(float x, float y, const char *s, int font)
{
    void *glutFont = fontFor(font);
    glRasterPos2f(x, y);
    for (int i = 0; s[i] != '\0'; i++)
        glutBitmapCharacter(glutFont, s[i]);
}

float TextWidth(const char *s, int font)
{
    void *glutFont = fontFor(font);
    int widthInPixels = 0;
    for (int i = 0; s[i] != '\0'; i++)
        widthInPixels += glutBitmapWidth(glutFont, s[i]);
    return widthInPixels / pixelsPerUnit;
}

void DrawTextCentered(float cx, float y, const char *s, int font)
{
    float halfWidth = TextWidth(s, font) * 0.5f;
    DrawText(cx - halfWidth, y, s, font);
}

static const int fontsBigToSmall[4] = {FONT_LARGE, FONT_MEDIUM, FONT_NORMAL, FONT_SMALL};

static int firstFontToTry(int font)
{
    for (int i = 0; i < 3; i++)
    {
        if (fontsBigToSmall[i] == font)
            return i;
    }
    return 3;
}

static void drawShrunkStrokeText(float cx, float y, const char *s, float maxW)
{
    float height = 9;
    float width = StrokeTextWidth(s, height);
    if (width > maxW)
        height = height * maxW / width;
    float finalWidth = StrokeTextWidth(s, height);
    DrawStrokeText(cx - finalWidth * 0.5f, y, height, s, 1);
}

void DrawTextFit(float cx, float y, const char *s, int font, float maxW)
{
    for (int i = firstFontToTry(font); i < 4; i++)
    {
        if (TextWidth(s, fontsBigToSmall[i]) <= maxW)
        {
            DrawTextCentered(cx, y, s, fontsBigToSmall[i]);
            return;
        }
    }
    drawShrunkStrokeText(cx, y, s, maxW);
}

void DrawStrokeText(float x, float y, float height, const char *s, float width)
{
    float scale = height / STROKE_FONT_HEIGHT;
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
    float width = 0;
    for (int i = 0; s[i] != '\0'; i++)
        width += glutStrokeWidth(GLUT_STROKE_ROMAN, s[i]);
    return width * height / STROKE_FONT_HEIGHT;
}

void formatTime(float seconds, char *out)
{
    int totalSeconds = (int)seconds;
    int minutes = totalSeconds / 60;
    int secondsLeft = totalSeconds % 60;
    sprintf(out, "%d:%02d", minutes, secondsLeft);
}
