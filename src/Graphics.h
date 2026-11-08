// Graphics.h - reusable geometric drawing helpers built only on OpenGL
// primitives (points, lines, triangles, quads, fans, polygons).
// No textures or image files are used anywhere in the project.
#pragma once

#include <string>
#include <vector>

#include "Types.h"

namespace gfx {

enum class Font { Small, Normal, Medium, Large, Title };

// ---- colour helpers ----------------------------------------------------
constexpr Color hex(uint32_t rgb, float a = 1.0f) {
    return {((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f, (rgb & 0xFF) / 255.0f, a};
}
Color mix(const Color& a, const Color& b, float t);
Color darker(const Color& c, float amount);   // amount 0..1 towards black
Color lighter(const Color& c, float amount);  // amount 0..1 towards white
Color withAlpha(const Color& c, float a);

// ---- projection / frame ------------------------------------------------
void initRenderState();
// Letterboxed orthographic projection: the 1000x600 view keeps its aspect
// ratio for any window size (no stretching).
void setupProjection(int winW, int winH);
void worldViewport(bool enabled);
void beginFrame(const Color& letterbox);
float pixelsPerUnit();
// Window pixel (GLUT mouse coords, y down) -> view units (y up).
Vec2 windowToView(int px, int py);

// ---- primitives ----------------------------------------------------------
void setColor(const Color& c);
void rect(float x, float y, float w, float h);
void rect(const Rect& r);
void rectOutline(float x, float y, float w, float h, float lineWidth = 1.0f);
void gradientRectV(float x, float y, float w, float h, const Color& bottom, const Color& top);
void roundedRect(float x, float y, float w, float h, float radius);
void roundedRectOutline(float x, float y, float w, float h, float radius, float lineWidth = 1.0f);
void circle(float cx, float cy, float r, int segments = 0);
void circleOutline(float cx, float cy, float r, float lineWidth = 1.0f, int segments = 0);
void ellipse(float cx, float cy, float rx, float ry, int segments = 0);
void ellipseOutline(float cx, float cy, float rx, float ry, float lineWidth = 1.0f, int segments = 0);
// Radial gradient (centre colour -> rim colour) used for soft shadows / glows.
void softEllipse(float cx, float cy, float rx, float ry, const Color& inner, const Color& outer, int segments = 0);
void arc(float cx, float cy, float r, float startDeg, float endDeg, float lineWidth = 1.0f, int segments = 0);
void sector(float cx, float cy, float r, float startDeg, float endDeg, int segments = 0);
void ringSector(float cx, float cy, float innerR, float outerR, float startDeg, float endDeg, int segments = 0);
void line(float x1, float y1, float x2, float y2, float lineWidth = 1.0f);
void lines(const std::vector<Vec2>& pairs, float lineWidth = 1.0f);  // GL_LINES batch
void thickLine(float x1, float y1, float x2, float y2, float width);  // quad-based
void triangle(Vec2 a, Vec2 b, Vec2 c);
void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d);
void polygon(const std::vector<Vec2>& pts);  // convex
void polygonOutline(const std::vector<Vec2>& pts, float lineWidth = 1.0f);
void points(const std::vector<Vec2>& pts, float size);

// ---- matrix transformations ---------------------------------------------
// pushTransform = glPushMatrix + glTranslatef + glRotatef + glScalef
void pushTransform(float x, float y, float deg = 0.0f, float sx = 1.0f, float sy = 1.0f);
void popTransform();

// ---- text (GLUT bitmap fonts for UI, stroke fonts for scalable world text)
void text(float x, float y, const std::string& s, Font f = Font::Normal);
void textCentered(float cx, float y, const std::string& s, Font f = Font::Normal);
float textWidth(const std::string& s, Font f = Font::Normal);
float textHeight(Font f);
void strokeText(float x, float y, float height, const std::string& s, float lineWidth = 1.5f);
float strokeTextWidth(const std::string& s, float height);

// Deterministic pseudo-random value in [0,1) - used to scatter decorative
// details identically every frame without storing them.
float hash01(int a, int b = 0);

}  // namespace gfx
