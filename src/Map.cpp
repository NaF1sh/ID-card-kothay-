#include "Map.h"

#include <algorithm>
#include <cmath>

#include <GL/freeglut.h>

#include "Graphics.h"

using namespace gfx;

namespace {

// ---- palette: soft pastel campus ------------------------------------------
const Color GRASS        = hex(0x9ccc72);
const Color GRASS_DARK   = hex(0x6fa64f);
const Color GRASS_LIGHT  = hex(0xb9de8e);
const Color ASPHALT      = hex(0x6c717d);
const Color ASPHALT_EDGE = hex(0x5b606b);
const Color CURB         = hex(0xdcd5c4);
const Color ROAD_PAINT   = hex(0xf6ecbf);
const Color PAVE         = hex(0xeadfc9);
const Color PAVE_JOINT   = hex(0xcdbea2);
const Color PLAZA_A      = hex(0xecdcc1);
const Color PLAZA_B      = hex(0xdac5a2);
const Color BRICK        = hex(0xc4735c);
const Color BRICK_DARK   = hex(0x9b5442);
const Color BRICK_CAP    = hex(0xe6c0a7);
const Color STONE        = hex(0xbbb4a6);
const Color STONE_LIGHT  = hex(0xe4dfd4);
const Color SOIL         = hex(0x7c5b43);
const Color SHADOW       = {0.10f, 0.16f, 0.08f, 0.22f};
const Color SHADOW_SOFT  = {0.10f, 0.16f, 0.08f, 0.0f};
const Color WOOD         = hex(0xb9875a);
const Color WOOD_DARK    = hex(0x83593a);
const Color IRON         = hex(0x434a54);
const Color GLASS        = hex(0x9fd0e6);
const Color GLASS_DARK   = hex(0x5d8fae);
const Color FRAME        = hex(0x4f5c69);
const Color UNI_BLUE    = hex(0x2f5f8c);
const Color UNI_GOLD    = hex(0xe9bb4d);
const Color WHITE        = hex(0xffffff);
const Color INK          = hex(0x2e3440);
const Color LAMP_WARM    = hex(0xffe39a);

struct BuildingStyle {
    Color wall, wallDark, roof, roofDark, parapet, accent;
};

BuildingStyle styleFor(BuildingKind k) {
    switch (k) {
        case BuildingKind::Academic:
            return {hex(0xf3e9d6), hex(0xd6c6a8), hex(0xd8d4cb), hex(0xc3beb2), hex(0xaea595), UNI_BLUE};
        case BuildingKind::Admin:
            return {hex(0xedcfb0), hex(0xcda680), hex(0xcfb49c), hex(0xb79a80), hex(0xa27f66), hex(0x8c4b3b)};
        case BuildingKind::Canteen:
            return {hex(0xf7e2a9), hex(0xd9be7f), hex(0xe89c70), hex(0xcf8158), hex(0xb66a4b), hex(0xd8524a)};
        case BuildingKind::GuardBooth:
            return {hex(0xe4ecf3), hex(0xbcc8d5), hex(0x4472a8), hex(0x365e8f), hex(0x2a4d78), hex(0x4472a8)};
    }
    return {};
}

// Two-layer offset shadow: light comes from the top-left, so shadows fall
// to the bottom-right everywhere on the map.
void dropShadow(const Rect& r, float off) {
    setColor(withAlpha(SHADOW, 0.10f));
    roundedRect(r.x + off * 0.4f, r.y - off * 1.1f, r.w + off * 0.9f, r.h, 4.0f);
    setColor(withAlpha(SHADOW, 0.16f));
    rect(r.x + off * 0.6f, r.y - off * 0.6f, r.w, r.h);
}

// ===========================================================================
// Ground layer
// ===========================================================================
void drawGrass() {
    setColor(GRASS);
    rect(0, 0, WORLD_W, WORLD_H);

    // Mowed-lawn stripes (alpha blended bands).
    setColor(withAlpha(GRASS_LIGHT, 0.22f));
    for (float x = 0; x < WORLD_W; x += 80.0f) rect(x, 0, 40.0f, WORLD_H);

    // Grass tufts: a batch of small "V" shapes made from GL_LINES.
    static std::vector<Vec2> tufts;
    static std::vector<Vec2> blossoms;
    if (tufts.empty()) {
        for (int i = 0; i < 2600; ++i) {
            float x = hash01(i, 1) * WORLD_W, y = hash01(i, 2) * WORLD_H;
            float s = 2.5f + hash01(i, 3) * 3.0f;
            tufts.push_back({x, y});
            tufts.push_back({x - 1.8f, y + s});
            tufts.push_back({x, y});
            tufts.push_back({x + 1.6f, y + s * 0.85f});
        }
        for (int i = 0; i < 360; ++i) blossoms.push_back({hash01(i, 7) * WORLD_W, hash01(i, 8) * WORLD_H});
    }
    setColor(withAlpha(GRASS_DARK, 0.55f));
    lines(tufts, 1.2f);

    // Tiny wild flowers: GL_POINTS.
    setColor(withAlpha(WHITE, 0.85f));
    points(blossoms, 3.0f);
}

void drawPavement(const Rect& r) {
    setColor(PAVE_JOINT);
    rect(r.expanded(2.0f));
    setColor(PAVE);
    rect(r);
    // Running-bond paving joints.
    std::vector<Vec2> j;
    const float rowH = 12.0f, tileW = 22.0f;
    int row = 0;
    for (float y = r.y + rowH; y < r.top(); y += rowH, ++row) {
        j.push_back({r.x, y});
        j.push_back({r.right(), y});
    }
    row = 0;
    for (float y = r.y; y < r.top(); y += rowH, ++row) {
        float off = (row % 2) ? tileW * 0.5f : 0.0f;
        float y2 = std::min(y + rowH, r.top());
        for (float x = r.x + off + tileW; x < r.right(); x += tileW) {
            j.push_back({x, y});
            j.push_back({x, y2});
        }
    }
    setColor(withAlpha(PAVE_JOINT, 0.8f));
    lines(j, 1.0f);
}

void drawPlaza(const Rect& r) {
    setColor(PAVE_JOINT);
    rect(r.expanded(3.0f));
    const float t = 20.0f;
    int ix = 0;
    for (float x = r.x; x < r.right(); x += t, ++ix) {
        int iy = 0;
        for (float y = r.y; y < r.top(); y += t, ++iy) {
            setColor(((ix + iy) % 2) ? PLAZA_A : PLAZA_B);
            rect(x, y, std::min(t, r.right() - x), std::min(t, r.top() - y));
        }
    }
    setColor(withAlpha(WHITE, 0.35f));
    rectOutline(r.x + 4, r.y + 4, r.w - 8, r.h - 8, 1.5f);
}

void drawPaths(const std::vector<PathSeg>& paths, const std::vector<Rect>& crossings) {
    for (const auto& p : paths) {
        if (p.kind == PathKind::Pavement) drawPavement(p.r);
        else if (p.kind == PathKind::Plaza) drawPlaza(p.r);
    }
    // Roads: all curbs first, then all asphalt, so junctions merge cleanly.
    setColor(CURB);
    for (const auto& p : paths)
        if (p.kind == PathKind::Road) rect(p.r.expanded(4.0f));
    for (const auto& p : paths) {
        if (p.kind != PathKind::Road) continue;
        setColor(ASPHALT);
        rect(p.r);
    }
    // Asphalt grain + markings.
    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& p = paths[i];
        if (p.kind != PathKind::Road) continue;
        std::vector<Vec2> grain;
        int n = static_cast<int>(p.r.w * p.r.h / 140.0f);
        for (int k = 0; k < n; ++k)
            grain.push_back({p.r.x + hash01(k, 31 + int(i)) * p.r.w, p.r.y + hash01(k, 47 + int(i)) * p.r.h});
        setColor(withAlpha(ASPHALT_EDGE, 0.7f));
        points(grain, 2.0f);

        // Centre dashes; skipped where another road or a crossing overlaps.
        bool horizontal = p.r.w >= p.r.h;
        float len = horizontal ? p.r.w : p.r.h;
        for (float s = 8.0f; s + 18.0f < len; s += 34.0f) {
            Rect d = horizontal ? Rect{p.r.x + s, p.r.center().y - 1.5f, 18.0f, 3.0f}
                                : Rect{p.r.center().x - 1.5f, p.r.y + s, 3.0f, 18.0f};
            bool blocked = false;
            for (size_t k = 0; k < paths.size(); ++k)
                if (k != i && paths[k].kind == PathKind::Road && paths[k].r.expanded(4).intersects(d)) blocked = true;
            for (const Rect& c : crossings)
                if (c.expanded(6).intersects(d)) blocked = true;
            if (!blocked) {
                setColor(ROAD_PAINT);
                rect(d);
            }
        }
    }
    // Zebra crossings.
    for (const Rect& c : crossings) {
        setColor(withAlpha(WHITE, 0.9f));
        // Bars run along the traffic direction of the road underneath.
        bool alongX = true;
        for (const auto& p : paths)
            if (p.kind == PathKind::Road && p.r.contains(c.center())) alongX = p.r.w >= p.r.h;
        if (alongX)
            for (float x = c.x; x + 5 <= c.right(); x += 10.0f) rect(x, c.y + 2, 6.0f, c.h - 4);
        else
            for (float y = c.y; y + 5 <= c.top(); y += 10.0f) rect(c.x + 2, y, c.w - 4, 6.0f);
    }
}

void drawFlowerBed(const FlowerBed& f, float t, int seed) {
    const Rect& r = f.r;
    setColor(withAlpha(SHADOW, 0.18f));
    roundedRect(r.x + 2, r.y - 4, r.w + 2, r.h, 5);
    setColor(hex(0xb88f70));  // brick edging
    roundedRect(r.x - 3, r.y - 3, r.w + 6, r.h + 6, 6);
    setColor(SOIL);
    roundedRect(r.x, r.y, r.w, r.h, 4);

    static const Color petals[] = {hex(0xf38fac), hex(0xf6d55c), hex(0xffffff), hex(0xb48be2), hex(0xf27b57)};
    int cols = std::max(1, static_cast<int>(r.w / 10.0f));
    int rows = std::max(1, static_cast<int>(r.h / 10.0f));
    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            int id = seed * 977 + i * 31 + j;
            float px = r.x + (i + 0.5f) * r.w / cols + (hash01(id, 1) - 0.5f) * 3.0f;
            float py = r.y + (j + 0.5f) * r.h / rows + (hash01(id, 2) - 0.5f) * 3.0f;
            setColor(hex(0x4f8e40));
            circle(px, py, 4.2f, 10);
            setColor(hex(0x6eae55));
            circle(px - 1.0f, py + 1.0f, 2.6f, 8);
            // flowers gently "breathe" (scaling animation)
            float s = 1.0f + 0.10f * std::sin(t * 2.2f + id);
            setColor(petals[static_cast<int>(hash01(id, 3) * 5) % 5]);
            circle(px + 0.8f, py + 1.2f, 2.3f * s, 8);
            setColor(hex(0xffd84a));
            circle(px + 0.8f, py + 1.2f, 0.8f, 6);
        }
    }
}

void drawTreeShadow(const Tree& tr) {
    softEllipse(tr.p.x + tr.radius * 0.45f, tr.p.y + tr.radius * 0.05f, tr.radius * 1.1f, tr.radius * 0.6f,
                withAlpha(SHADOW, 0.32f), SHADOW_SOFT);
}

// ===========================================================================
// Prop layer
// ===========================================================================
void drawBush(const Bush& b, int seed) {
    float r = b.radius;
    softEllipse(b.p.x + r * 0.35f, b.p.y - r * 0.35f, r * 1.1f, r * 0.7f, withAlpha(SHADOW, 0.28f), SHADOW_SOFT);
    Vec2 lobes[5];
    for (int k = 0; k < 5; ++k) {
        float a = degToRad(k * 72.0f + hash01(seed, k) * 30.0f);
        lobes[k] = {b.p.x + std::cos(a) * r * 0.45f, b.p.y + std::sin(a) * r * 0.40f};
    }
    setColor(hex(0x3c6b35));  // outline pass
    for (auto& l : lobes) circle(l.x, l.y, r * 0.6f + 1.5f);
    setColor(hex(0x5a9a49));  // fill pass
    for (auto& l : lobes) circle(l.x, l.y, r * 0.6f);
    circle(b.p.x, b.p.y, r * 0.6f);
    setColor(hex(0x7dba60));  // highlight
    circle(b.p.x - r * 0.18f, b.p.y + r * 0.18f, r * 0.42f);
    setColor(withAlpha(hex(0xb4e08c), 0.8f));
    circle(b.p.x - r * 0.3f, b.p.y + r * 0.32f, r * 0.15f);
}

void drawBench(const Bench& b) {
    pushTransform(b.p.x, b.p.y, b.vertical ? 90.0f : 0.0f);  // rotation transform
    setColor(withAlpha(SHADOW, 0.25f));
    roundedRect(-17.0f, -10.0f, 38.0f, 13.0f, 3.0f);
    setColor(IRON);
    rect(-16.0f, -7.0f, 3.0f, 15.0f);
    rect(13.0f, -7.0f, 3.0f, 15.0f);
    for (int k = 0; k < 3; ++k) {
        setColor(k % 2 ? WOOD : lighter(WOOD, 0.12f));
        roundedRect(-18.0f, -6.0f + k * 4.0f, 36.0f, 3.3f, 1.2f);
    }
    setColor(WOOD_DARK);
    roundedRect(-18.0f, 6.5f, 36.0f, 3.0f, 1.2f);  // backrest
    popTransform();
}

void drawLampBase(const LampPost& l) {
    setColor(STONE);
    circle(l.p.x, l.p.y, 5.0f);
    setColor(IRON);
    circle(l.p.x, l.p.y, 3.2f);
    rect(l.p.x - 1.5f, l.p.y, 3.0f, 30.0f);
    setColor(withAlpha(WHITE, 0.35f));
    rect(l.p.x - 1.0f, l.p.y + 2.0f, 0.8f, 26.0f);
}

void drawRailing(const Railing& rl) {
    Vec2 d = rl.b - rl.a;
    float len = d.length();
    int posts = std::max(2, static_cast<int>(len / 12.0f) + 1);
    setColor(withAlpha(SHADOW, 0.2f));
    thickLine(rl.a.x + 3, rl.a.y - 3, rl.b.x + 3, rl.b.y - 3, 2.0f);
    setColor(IRON);
    for (int i = 0; i < posts; ++i) {
        Vec2 p = rl.a + d * (static_cast<float>(i) / (posts - 1));
        rect(p.x - 1.0f, p.y, 2.0f, 9.0f);
    }
    thickLine(rl.a.x, rl.a.y + 8.0f, rl.b.x, rl.b.y + 8.0f, 2.0f);
    setColor(hex(0x5d6773));
    thickLine(rl.a.x, rl.a.y + 4.0f, rl.b.x, rl.b.y + 4.0f, 1.2f);
}

void drawTableBase(const UmbrellaTable& u) {
    softEllipse(u.p.x + 6, u.p.y - 6, 22, 14, withAlpha(SHADOW, 0.3f), SHADOW_SOFT);
    setColor(WOOD_DARK);
    for (int k = 0; k < 4; ++k) {
        float a = degToRad(45.0f + k * 90.0f);
        roundedRect(u.p.x + std::cos(a) * 15 - 4, u.p.y + std::sin(a) * 12 - 4, 8, 8, 2);
    }
    setColor(WOOD);
    circle(u.p.x, u.p.y, 9.0f);
}

void drawWall(const Rect& w) {
    setColor(withAlpha(SHADOW, 0.25f));
    rect(w.x + 4, w.y - 4, w.w, w.h);
    setColor(BRICK);
    rect(w);
    std::vector<Vec2> mortar;
    bool horizontal = w.w >= w.h;
    if (horizontal) {
        // front face (lower 55%) shows brick courses, top is the capstone
        float faceTop = w.y + w.h * 0.55f;
        int row = 0;
        for (float y = w.y; y < faceTop; y += 3.8f, ++row) {
            mortar.push_back({w.x, y});
            mortar.push_back({w.right(), y});
            float off = (row % 2) ? 7.0f : 0.0f;
            for (float x = w.x + off; x < w.right(); x += 14.0f) {
                mortar.push_back({x, y});
                mortar.push_back({x, std::min(y + 3.8f, faceTop)});
            }
        }
        setColor(withAlpha(BRICK_DARK, 0.7f));
        lines(mortar, 1.0f);
        setColor(BRICK_CAP);
        rect(w.x, faceTop, w.w, w.top() - faceTop);
        setColor(withAlpha(WHITE, 0.4f));
        line(w.x, w.top() - 1.0f, w.right(), w.top() - 1.0f, 1.0f);
    } else {
        // vertical walls are seen from above: capstone with joints
        setColor(BRICK_CAP);
        rect(w.x + 2, w.y, w.w - 4, w.h);
        for (float y = w.y; y < w.top(); y += 20.0f) {
            mortar.push_back({w.x + 2, y});
            mortar.push_back({w.right() - 2, y});
        }
        setColor(withAlpha(BRICK_DARK, 0.6f));
        lines(mortar, 1.0f);
    }
}

void drawPillar(float cx, float cy, float size) {
    float h = size * 0.5f;
    setColor(withAlpha(SHADOW, 0.3f));
    rect(cx - h + 5, cy - h - 5, size, size);
    setColor(hex(0xa69e90));
    rect(cx - h, cy - h, size, size);
    setColor(STONE_LIGHT);
    rect(cx - h + 3, cy - h + 3, size - 6, size - 6);
    setColor(withAlpha(hex(0xa69e90), 0.7f));
    line(cx - h + 3, cy - h + 3, cx + h - 3, cy + h - 3, 1.0f);
    line(cx - h + 3, cy + h - 3, cx + h - 3, cy - h + 3, 1.0f);
    setColor(UNI_GOLD);
    circle(cx, cy, size * 0.18f);
}

void drawGatePanel(float hx, float hy, float deg, float len) {
    pushTransform(hx, hy, deg);  // panel rotates around its hinge
    setColor(withAlpha(SHADOW, 0.25f));
    rect(2.0f, -5.0f, len, 4.0f);
    setColor(IRON);
    rect(0.0f, -1.5f, len, 3.0f);
    setColor(hex(0x6c7684));
    for (float x = 4.0f; x < len; x += 6.0f) rect(x - 0.8f, -3.0f, 1.6f, 6.0f);
    setColor(UNI_GOLD);
    circle(len, 0.0f, 2.0f);
    popTransform();
}

void drawBuilding(const Building& b, float t) {
    const Rect& r = b.r;
    const BuildingStyle st = styleFor(b.kind);
    const bool booth = b.kind == BuildingKind::GuardBooth;
    const float faceH = booth ? r.h * 0.42f : std::min(r.h * 0.36f, 58.0f);
    const float roofY = r.y + faceH;
    const float doorW = booth ? 12.0f : (b.kind == BuildingKind::Academic ? 40.0f : 30.0f);
    const float doorH = faceH * (booth ? 0.8f : 0.72f);

    // Entrance steps (outside the footprint, walkable).
    if (!booth) {
        for (int k = 2; k >= 0; --k) {
            float w = doorW + 16.0f + k * 8.0f;
            setColor(k % 2 ? STONE : STONE_LIGHT);
            rect(b.doorX - w * 0.5f, r.y - 4.0f * (k + 1), w, 4.0f * (k + 1));
        }
    }

    // ---- front facade ----
    gradientRectV(r.x, r.y, r.w, faceH, st.wallDark, st.wall);
    setColor(darker(st.wallDark, 0.15f));
    rect(r.x, r.y, r.w, 4.0f);  // plinth

    if (b.kind == BuildingKind::Canteen) {
        // striped awning with scalloped edge
        float aw = r.w - 10.0f, ay = r.y + faceH * 0.45f;
        int stripes = static_cast<int>(aw / 12.0f);
        float sw = aw / stripes;
        for (int i = 0; i < stripes; ++i) {
            setColor(i % 2 ? WHITE : st.accent);
            float x = r.x + 5.0f + i * sw;
            rect(x, ay, sw, faceH * 0.5f);
            sector(x + sw * 0.5f, ay, sw * 0.5f, 180.0f, 360.0f);
        }
        setColor(WOOD);
        rect(r.x + 14, r.y + 4, r.w - 28, faceH * 0.32f);  // serving counter
        setColor(WOOD_DARK);
        rect(r.x + 14, r.y + 4 + faceH * 0.32f - 3, r.w - 28, 3);
    } else {
        int bays = std::max(1, static_cast<int>(r.w / (booth ? 30.0f : 44.0f)));
        float bayW = r.w / bays;
        for (int i = 0; i < bays; ++i) {
            float cx = r.x + bayW * (i + 0.5f);
            if (std::fabs(cx - b.doorX) < doorW * 0.5f + bayW * 0.35f) continue;
            float ww = bayW * 0.56f, wh = faceH * 0.46f, wy = r.y + faceH * 0.30f;
            setColor(FRAME);
            rect(cx - ww * 0.5f - 2, wy - 2, ww + 4, wh + 4);
            gradientRectV(cx - ww * 0.5f, wy, ww, wh, GLASS_DARK, GLASS);
            setColor(withAlpha(WHITE, 0.35f));  // reflection
            quad({cx - ww * 0.5f, wy + wh * 0.2f}, {cx - ww * 0.1f, wy + wh}, {cx + ww * 0.1f, wy + wh},
                 {cx - ww * 0.5f + ww * 0.15f, wy});
            setColor(FRAME);
            line(cx, wy, cx, wy + wh, 1.2f);
            line(cx - ww * 0.5f, wy + wh * 0.5f, cx + ww * 0.5f, wy + wh * 0.5f, 1.2f);
            setColor(lighter(st.wall, 0.4f));
            rect(cx - ww * 0.5f - 3, wy - 4, ww + 6, 2.5f);  // sill
            if (!booth && i > 0) {  // pilaster between bays
                setColor(withAlpha(st.wallDark, 0.7f));
                rect(r.x + bayW * i - 2.0f, r.y + 4, 4.0f, faceH - 4);
            }
        }
        // door
        setColor(st.accent);
        rect(b.doorX - doorW * 0.5f - 3, r.y, doorW + 6, doorH + 3);
        gradientRectV(b.doorX - doorW * 0.5f, r.y, doorW, doorH, GLASS_DARK, lighter(GLASS, 0.2f));
        setColor(FRAME);
        line(b.doorX, r.y, b.doorX, r.y + doorH, 1.5f);
        setColor(UNI_GOLD);
        circle(b.doorX - 3, r.y + doorH * 0.45f, 1.3f, 8);
        circle(b.doorX + 3, r.y + doorH * 0.45f, 1.3f, 8);
        if (!booth) {  // entrance canopy
            setColor(withAlpha(SHADOW, 0.3f));
            rect(b.doorX - doorW * 0.5f - 8, r.y + doorH - 2, doorW + 18, 5);
            setColor(st.accent);
            rect(b.doorX - doorW * 0.5f - 10, r.y + doorH + 3, doorW + 20, 5);
        }
    }

    // ---- roof ----
    const float roofH = r.top() - roofY;
    setColor(st.roof);
    rect(r.x, roofY, r.w, roofH);
    // parapet rim
    const float pw = booth ? 3.0f : 5.0f;
    setColor(st.parapet);
    rect(r.x, roofY, r.w, pw);
    rect(r.x, r.top() - pw, r.w, pw);
    rect(r.x, roofY, pw, roofH);
    rect(r.right() - pw, roofY, pw, roofH);
    setColor(withAlpha(WHITE, 0.25f));
    rectOutline(r.x + pw, roofY + pw, r.w - 2 * pw, roofH - 2 * pw, 1.0f);
    setColor(darker(st.parapet, 0.3f));
    rect(r.x, roofY - 3.0f, r.w, 3.0f);  // cornice line separating roof and facade

    if (b.kind == BuildingKind::Academic || b.kind == BuildingKind::Admin) {
        // roof slab joints
        setColor(withAlpha(st.roofDark, 0.6f));
        for (float x = r.x + 40.0f; x < r.right() - pw; x += 40.0f) line(x, roofY + pw, x, r.top() - pw, 1.0f);

        // solar panels (Academic) / skylight (Admin)
        if (b.kind == BuildingKind::Academic) {
            for (int k = 0; k < 2; ++k) {
                float px = r.x + 24.0f + k * 92.0f, py = roofY + roofH * 0.42f;
                setColor(withAlpha(SHADOW, 0.35f));
                rect(px + 3, py - 3, 76, 38);
                setColor(hex(0x2c4f7c));
                rect(px, py, 76, 38);
                setColor(withAlpha(hex(0x9cc4ef), 0.7f));
                for (float x = px + 12.7f; x < px + 76; x += 12.7f) line(x, py, x, py + 38, 1.0f);
                line(px, py + 19, px + 76, py + 19, 1.0f);
                setColor(withAlpha(WHITE, 0.25f));
                triangle({px, py + 38}, {px + 30, py + 38}, {px, py + 14});
            }
        } else {
            float sx = r.x + r.w * 0.18f, sy = roofY + roofH * 0.35f;
            setColor(st.roofDark);
            rect(sx - 3, sy - 3, 96, 46);
            gradientRectV(sx, sy, 90, 40, GLASS_DARK, GLASS);
            setColor(FRAME);
            for (float x = sx + 15; x < sx + 90; x += 15) line(x, sy, x, sy + 40, 1.0f);
            line(sx, sy + 20, sx + 90, sy + 20, 1.0f);
        }

        // water tank (cylinder seen from above)
        float tx = r.right() - 42.0f, ty = r.top() - 30.0f;
        softEllipse(tx + 6, ty - 5, 20, 16, withAlpha(SHADOW, 0.4f), SHADOW_SOFT);
        setColor(hex(0x3f4a57));
        circle(tx, ty, 15.0f);
        setColor(hex(0x5a6b7d));
        circle(tx, ty, 12.5f);
        setColor(hex(0x6f8296));
        circle(tx - 2, ty + 2, 6.0f);

        // AC units with spinning fans (rotation animation)
        for (int k = 0; k < 3; ++k) {
            float ax = r.right() - 100.0f - k * 34.0f, ay = r.top() - 30.0f;
            if (ax < r.x + 20) break;
            setColor(withAlpha(SHADOW, 0.35f));
            rect(ax - 10, ay - 15, 24, 24);
            setColor(hex(0xeef0f2));
            roundedRect(ax - 12, ay - 12, 24, 24, 3);
            setColor(hex(0x9aa3ad));
            circle(ax, ay, 9.0f);
            pushTransform(ax, ay, std::fmod(t * 360.0f * (1.0f + k * 0.3f), 360.0f));
            setColor(hex(0x5c6670));
            for (int bl = 0; bl < 3; ++bl) sector(0, 0, 8.0f, bl * 120.0f, bl * 120.0f + 40.0f, 6);
            popTransform();
        }

        // stair room (raised box with its own shadow)
        Rect sr{r.x + r.w - 150.0f, roofY + 14.0f, 40.0f, 30.0f};
        if (sr.x > r.x + 200.0f) {
            dropShadow(sr, 6.0f);
            setColor(st.wallDark);
            rect(sr);
            setColor(st.roof);
            rect(sr.x + 2, sr.y + 6, sr.w - 4, sr.h - 8);
        }
    }

    // name banner on the roof edge
    if (!b.name.empty()) {
        Font f = booth ? Font::Small : Font::Normal;
        float tw = textWidth(b.name, f) + (booth ? 6.0f : 18.0f);
        float bh = booth ? 10.0f : 16.0f;
        float by = booth ? r.top() - bh - 3.0f : roofY + 8.0f;
        setColor(withAlpha(SHADOW, 0.35f));
        roundedRect(r.center().x - tw * 0.5f + 2, by - 2, tw, bh, 3);
        setColor(booth ? WHITE : st.accent);
        roundedRect(r.center().x - tw * 0.5f, by, tw, bh, 3);
        setColor(booth ? st.accent : WHITE);
        textCentered(r.center().x, by + (booth ? 2.0f : 4.5f), b.name, f);
    }
}

void drawSign(const Rect& r) {
    setColor(hex(0x8f8879));
    roundedRect(r.x - 4, r.y - 4, r.w + 8, r.h + 8, 6);
    setColor(STONE_LIGHT);
    roundedRect(r.x, r.y, r.w, r.h, 5);
    setColor(UNI_BLUE);
    rect(r.x, r.y, r.w, 6);  // blue base band
    // emblem: shield circle with an open book
    float ex = r.x + 22, ey = r.y + r.h * 0.55f;
    setColor(UNI_BLUE);
    circle(ex, ey, 15);
    setColor(UNI_GOLD);
    circleOutline(ex, ey, 13, 1.5f);
    setColor(WHITE);
    quad({ex - 9, ey - 3}, {ex - 1, ey - 5}, {ex - 1, ey + 5}, {ex - 9, ey + 7});
    quad({ex + 1, ey - 5}, {ex + 9, ey - 3}, {ex + 9, ey + 7}, {ex + 1, ey + 5});
    setColor(UNI_BLUE);
    text(r.x + 44, r.y + r.h - 18, "CITY CAMPUS", Font::Normal);
    text(r.x + 44, r.y + r.h - 31, "UNIVERSITY", Font::Small);
    setColor(hex(0x7b7468));
    text(r.x + 44, r.y + 12, "Main Gate  -  Visitors Welcome", Font::Small);
}

void drawMinar(Vec2 p) {
    // stepped platform
    setColor(STONE);
    rect(p.x - 56, p.y, 112, 8);
    setColor(lighter(STONE, 0.25f));
    rect(p.x - 50, p.y + 8, 100, 6);
    setColor(STONE_LIGHT);
    rect(p.x - 46, p.y + 14, 92, 44);
    setColor(withAlpha(STONE, 0.6f));
    rectOutline(p.x - 46, p.y + 14, 92, 44, 1.0f);

    // red sun behind the columns
    setColor(hex(0xd9363e));
    circle(p.x, p.y + 62, 21);
    setColor(withAlpha(hex(0xff6b6b), 0.5f));
    circle(p.x - 4, p.y + 66, 12);

    auto column = [&](float cx, float w, float h) {
        float y0 = p.y + 28;
        setColor(withAlpha(SHADOW, 0.35f));
        rect(cx - w * 0.5f + 3, y0 - 4, w, 6);
        setColor(WHITE);
        rect(cx - w * 0.5f, y0, w, h);
        setColor(INK);
        for (float y = y0 + 5; y < y0 + h - 2; y += 5) line(cx - w * 0.5f + 1.5f, y, cx + w * 0.5f - 1.5f, y, 1.0f);
        setColor(withAlpha(hex(0x9aa0a8), 0.8f));
        rectOutline(cx - w * 0.5f, y0, w, h, 1.0f);
    };
    column(p.x - 34, 7, 24);
    column(p.x + 34, 7, 24);
    column(p.x - 19, 9, 34);
    column(p.x + 19, 9, 34);
    column(p.x, 16, 46);
    // bent top of the central column
    setColor(hex(0xeceef0));
    quad({p.x - 8, p.y + 74}, {p.x + 8, p.y + 74}, {p.x + 6, p.y + 84}, {p.x - 10, p.y + 84});
    setColor(INK);
    line(p.x - 7, p.y + 79, p.x + 6, p.y + 79, 1.0f);
}

void drawFlagPole(Vec2 p) {
    setColor(STONE);
    circle(p.x, p.y, 7);
    setColor(STONE_LIGHT);
    circle(p.x, p.y, 4.5f);
    setColor(hex(0xc9cdd2));
    rect(p.x - 1.3f, p.y, 2.6f, 82);
    setColor(UNI_GOLD);
    circle(p.x, p.y + 83, 2.4f);
}

// Waving flag: a GL_QUAD_STRIP whose vertices are displaced by a sine wave.
void drawFlagCloth(Vec2 p, float t) {
    const float w = 36, h = 22, top = p.y + 80;
    const int n = 14;
    auto wave = [&](float u) { return std::sin(u * 6.0f - t * 5.0f) * 2.2f * u; };
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= n; ++i) {
        float u = static_cast<float>(i) / n;
        float shade = 0.88f + 0.12f * std::cos(u * 6.0f - t * 5.0f);
        Color c = hex(0x006a4e);
        glColor4f(c.r * shade, c.g * shade, c.b * shade, 1.0f);
        glVertex2f(p.x + 1 + u * w, top - h + wave(u));
        glVertex2f(p.x + 1 + u * w, top + wave(u));
    }
    glEnd();
    float u = 0.45f;
    setColor(hex(0xf42a41));
    circle(p.x + 1 + u * w, top - h * 0.5f + wave(u), h * 0.3f);
}

void drawUmbrella(const UmbrellaTable& u, float t) {
    static const Color tops[] = {hex(0xe8574e), hex(0x3f8fb5), hex(0xf2b33d)};
    Color c = tops[u.colorIndex % 3];
    float cy = u.p.y + 10.0f;
    float sway = std::sin(t * 1.5f + u.p.x) * 3.0f;
    pushTransform(u.p.x, cy, 22.5f + sway);
    softEllipse(6, -6, 24, 22, withAlpha(SHADOW, 0.25f), SHADOW_SOFT);
    for (int k = 0; k < 8; ++k) {
        setColor(k % 2 ? c : WHITE);
        triangle({0, 0}, {std::cos(degToRad(k * 45.0f)) * 21, std::sin(degToRad(k * 45.0f)) * 21},
                 {std::cos(degToRad((k + 1) * 45.0f)) * 21, std::sin(degToRad((k + 1) * 45.0f)) * 21});
    }
    setColor(darker(c, 0.3f));
    std::vector<Vec2> rim;
    for (int k = 0; k < 8; ++k) rim.push_back({std::cos(degToRad(k * 45.0f)) * 21, std::sin(degToRad(k * 45.0f)) * 21});
    polygonOutline(rim, 1.2f);
    setColor(WOOD_DARK);
    circle(0, 0, 2.5f);
    popTransform();
}

// ===========================================================================
// Canopy layer
// ===========================================================================
void drawTreeTrunk(const Tree& tr) {
    float r = tr.radius;
    setColor(hex(0x6b4a32));
    quad({tr.p.x - 5, tr.p.y - 2}, {tr.p.x + 5, tr.p.y - 2}, {tr.p.x + 3, tr.p.y + r * 0.6f}, {tr.p.x - 3, tr.p.y + r * 0.6f});
    setColor(hex(0x8a6444));
    rect(tr.p.x - 2.5f, tr.p.y, 1.5f, r * 0.5f);
}

void drawTreeCanopy(const Tree& tr, float t, int seed) {
    float r = tr.radius;
    Color outline, base, mid, light;
    switch (tr.kind) {
        case TreeKind::Rain:         outline = hex(0x355f31); base = hex(0x4f8a43); mid = hex(0x68a853); light = hex(0x8fc56c); break;
        case TreeKind::Krishnachura: outline = hex(0x3a6131); base = hex(0x5b8f45); mid = hex(0x76aa55); light = hex(0x9acb6e); break;
        case TreeKind::Mango:        outline = hex(0x2c5230); base = hex(0x3f7a47); mid = hex(0x559459); light = hex(0x78b571); break;
    }
    // gentle sway: rotation + scaling about the canopy centre
    float sway = std::sin(t * 1.2f + seed * 1.7f) * 1.8f;
    float breathe = 1.0f + 0.015f * std::sin(t * 1.6f + seed);
    pushTransform(tr.p.x, tr.p.y + r * 0.6f, sway, breathe, breathe);

    const int N = 7;
    Vec2 lobe[N];
    float lr[N];
    for (int k = 0; k < N; ++k) {
        float a = degToRad(k * 360.0f / N + hash01(seed, k) * 25.0f);
        float d = r * (0.42f + hash01(seed, k + 10) * 0.12f);
        lobe[k] = {std::cos(a) * d, std::sin(a) * d * 0.92f};
        lr[k] = r * (0.48f + hash01(seed, k + 20) * 0.12f);
    }
    setColor(outline);
    circle(0, 0, r * 0.62f + 2.0f);
    for (int k = 0; k < N; ++k) circle(lobe[k].x, lobe[k].y, lr[k] + 2.0f);
    setColor(base);
    circle(0, 0, r * 0.62f);
    for (int k = 0; k < N; ++k) circle(lobe[k].x, lobe[k].y, lr[k]);
    setColor(mid);
    for (int k = 0; k < N; ++k) circle(lobe[k].x * 0.75f - r * 0.08f, lobe[k].y * 0.75f + r * 0.08f, lr[k] * 0.72f);
    setColor(light);
    circle(-r * 0.22f, r * 0.25f, r * 0.3f);
    circle(r * 0.1f, r * 0.38f, r * 0.18f);
    setColor(withAlpha(lighter(light, 0.3f), 0.8f));
    circle(-r * 0.3f, r * 0.34f, r * 0.1f);

    if (tr.kind == TreeKind::Krishnachura) {  // flame-tree blossoms
        static const Color fl[] = {hex(0xe8463a), hex(0xf27a35), hex(0xf9a43f)};
        for (int k = 0; k < 22; ++k) {
            float a = hash01(seed, 40 + k) * 2.0f * PI_F;
            float d = std::sqrt(hash01(seed, 80 + k)) * r * 0.85f;
            setColor(fl[k % 3]);
            circle(std::cos(a) * d, std::sin(a) * d, 2.0f + hash01(seed, 120 + k) * 1.4f, 8);
        }
    } else if (tr.kind == TreeKind::Mango) {
        for (int k = 0; k < 6; ++k) {
            float a = hash01(seed, 50 + k) * 2.0f * PI_F;
            float d = std::sqrt(hash01(seed, 90 + k)) * r * 0.7f;
            setColor(hex(0xc9d65a));
            ellipse(std::cos(a) * d, std::sin(a) * d, 2.0f, 2.8f, 8);
        }
    }
    popTransform();
}

void drawLampHead(const LampPost& l, float t) {
    float glow = 0.22f + 0.05f * std::sin(t * 3.0f + l.p.x);
    softEllipse(l.p.x, l.p.y + 31, 26, 22, withAlpha(LAMP_WARM, glow), withAlpha(LAMP_WARM, 0.0f));
    setColor(IRON);
    ellipse(l.p.x, l.p.y + 31, 7.5f, 4.5f);
    setColor(LAMP_WARM);
    circle(l.p.x, l.p.y + 31, 2.8f);
}

void drawGateArch(const Gate& g, float t) {
    float x0 = g.x0 - 6, x1 = g.x1 + 6, y = 16.0f, h = 13.0f;
    setColor(withAlpha(SHADOW, 0.25f));
    rect(x0 + 8, y - 12, x1 - x0, h);  // arch casts a shadow on the road
    setColor(darker(UNI_BLUE, 0.25f));
    rect(x0, y - 2, x1 - x0, h + 4);
    setColor(UNI_BLUE);
    rect(x0, y, x1 - x0, h);
    setColor(UNI_GOLD);
    rect(x0, y + h - 2, x1 - x0, 2);
    setColor(WHITE);
    textCentered((x0 + x1) * 0.5f, y + 3.0f, "CITY CAMPUS UNIVERSITY", Font::Small);
    // gate pillar lamps
    for (float x : {g.x0 - 8.0f, g.x1 + 8.0f}) {
        float glow = 0.25f + 0.05f * std::sin(t * 2.5f + x);
        softEllipse(x, 9, 20, 18, withAlpha(LAMP_WARM, glow), withAlpha(LAMP_WARM, 0.0f));
        setColor(LAMP_WARM);
        circle(x, 9, 3.0f);
    }
}

void drawClouds(float t) {
    // Slow drifting cloud shadows give the scene life.
    for (int c = 0; c < 7; ++c) {
        float x = std::fmod(c * 610.0f + t * (7.0f + (c % 3) * 2.0f), WORLD_W + 400.0f) - 200.0f;
        float y = 120.0f + c * 160.0f;
        for (int k = 0; k < 4; ++k) {
            float ox = (k - 1.5f) * 45.0f, oy = std::sin(k * 1.9f + c) * 18.0f;
            softEllipse(x + ox, y + oy, 70, 42, {0.15f, 0.20f, 0.30f, 0.06f}, {0.15f, 0.20f, 0.30f, 0.0f});
        }
    }
}

}  // namespace

// ===========================================================================
// Map public API
// ===========================================================================
void Map::finalize() {
    solids_.clear();
    blockers_.clear();
    circles_.clear();
    for (const auto& lake : lakes) solids_.push_back(lake); // water blocks feet, never sight
    for (const auto& b : buildings) {
        solids_.push_back(b.r);
        blockers_.push_back(b.r);
    }
    for (const auto& w : walls) {
        solids_.push_back(w);
        blockers_.push_back(w);
    }
    if (hasGate) {
        solids_.push_back({gate.x0 - 17, 0, 18, 20});
        solids_.push_back({gate.x1 - 1, 0, 18, 20});
    }
    for (const auto& m : minars) {
        Rect r{m.x - 56, m.y, 112, 58};
        solids_.push_back(r);
        blockers_.push_back(r);
    }
    for (const auto& s : signs) {
        solids_.push_back(s.expanded(4));
        blockers_.push_back(s);  // the stone sign is tall enough to hide behind
    }
    for (const auto& b : benches)
        solids_.push_back(b.vertical ? Rect{b.p.x - 7, b.p.y - 18, 14, 36} : Rect{b.p.x - 18, b.p.y - 7, 36, 14});
    for (const auto& rl : railings) {
        Rect r{std::min(rl.a.x, rl.b.x) - 1, std::min(rl.a.y, rl.b.y) - 1, std::fabs(rl.b.x - rl.a.x) + 2,
               std::fabs(rl.b.y - rl.a.y) + 4};
        solids_.push_back(r);
    }
    for (const auto& t : trees) circles_.push_back({t.p, 7.0f});
    for (const auto& l : lamps) circles_.push_back({l.p, 4.0f});
    for (const auto& f : flags) circles_.push_back({f, 6.0f});
    for (const auto& u : tables) circles_.push_back({u.p, 12.0f});
}

void Map::drawGround(float t) const {
    drawGrass();
    drawPaths(paths, crossings);
    for (const Rect& lake : lakes) {
        setColor(hex(0xb5a78b));
        roundedRect(lake.x - 8, lake.y - 8, lake.w + 16, lake.h + 16, 22);
        setColor(hex(0x377b92));
        roundedRect(lake.x, lake.y, lake.w, lake.h, 18);
        setColor(hex(0x63b5c5, 0.65f));
        for (int i = 0; i < 12; ++i) {
            float x = lake.x + 20 + std::fmod(i * 37.0f + t * 7, lake.w - 45);
            float y = lake.y + 18 + (i % 5) * 25;
            line(x, y, x + 16 + std::sin(t + i) * 5, y, 1.5f);
        }
        setColor(hex(0x83a64c));
        for (int i = 0; i < 4; ++i) ellipse(lake.x + 24 + i * 17, lake.y + lake.h - 23 - (i % 2) * 13, 8, 4);
        setColor(hex(0xfff1ce));
        float duckX = lake.x + lake.w * 0.6f + std::sin(t * 0.4f) * 30;
        ellipse(duckX, lake.y + 70, 9, 5);
        circle(duckX + 7, lake.y + 76, 4);
        setColor(hex(0xf2ac48));
        triangle({duckX + 10, lake.y + 76}, {duckX + 16, lake.y + 74}, {duckX + 10, lake.y + 73});
    }
    for (const Rect& field : footballCourts) {
        setColor(hex(0x477e45)); rect(field);
        for (int i = 0; i < 7; i += 2) {
            setColor(hex(0x528d4b)); rect(field.x + i * field.w / 7, field.y, field.w / 7, field.h);
        }
        setColor(hex(0xe2ecd1));
        rectOutline(field.x + 8, field.y + 8, field.w - 16, field.h - 16, 2);
        line(field.center().x, field.y + 8, field.center().x, field.top() - 8, 2);
        circleOutline(field.center().x, field.center().y, 32, 2);
        for (float x : {field.x + 8, field.right() - 55}) rectOutline(x, field.y + 50, 47, field.h - 100, 2);
        for (float x : {field.x - 5, field.right() - 8}) {
            rectOutline(x, field.center().y - 24, 13, 48, 2);
            for (int i = 1; i < 6; ++i) line(x, field.center().y - 24 + i * 8, x + 13, field.center().y - 24 + i * 8);
        }
    }
    for (size_t i = 0; i < flowerBeds.size(); ++i) drawFlowerBed(flowerBeds[i], t, static_cast<int>(i));
    // Shadows of tall objects lie flat on the ground, beneath every character.
    for (const auto& tr : trees) drawTreeShadow(tr);
    for (const auto& b : buildings) dropShadow(b.r, b.kind == BuildingKind::GuardBooth ? 8.0f : 16.0f);
    for (const auto& sg : signs) dropShadow(sg, 8.0f);
    for (const auto& m : minars) dropShadow({m.x - 56, m.y, 112, 58}, 10.0f);
    setColor(withAlpha(SHADOW, 0.25f));
    for (const auto& l : lamps) thickLine(l.p.x, l.p.y, l.p.x + 20.0f, l.p.y - 12.0f, 3.0f);
    for (const auto& f : flags) thickLine(f.x, f.y, f.x + 46, f.y - 26, 2.5f);
}

void Map::collectDepthItems(float t, std::vector<DepthItem>& out) const {
    // Each upright object is keyed by the y of its base (where it touches the
    // ground). Sorting all items by that key - together with the characters -
    // gives correct front/back overlap in the top-down 3/4 view.
    for (size_t i = 0; i < bushes.size(); ++i)
        out.push_back({bushes[i].p.y, [this, i] { drawBush(bushes[i], static_cast<int>(i) + 3); }});
    for (const auto& u : tables) out.push_back({u.p.y, [&u] { drawTableBase(u); }});
    for (const auto& b : benches) out.push_back({b.p.y - 6.0f, [&b] { drawBench(b); }});
    for (const auto& rl : railings) out.push_back({std::min(rl.a.y, rl.b.y), [&rl] { drawRailing(rl); }});
    for (const auto& l : lamps) out.push_back({l.p.y, [&l] { drawLampBase(l); }});
    for (const auto& f : flags) out.push_back({f.y, [&f] { drawFlagPole(f); }});
    for (const auto& sg : signs) out.push_back({sg.y, [&sg] { drawSign(sg); }});
    for (const auto& m : minars) out.push_back({m.y, [&m] { drawMinar(m); }});
    for (const auto& tr : trees) out.push_back({tr.p.y, [&tr] { drawTreeTrunk(tr); }});
    for (const auto& b : buildings) out.push_back({b.r.y, [&b, t] { drawBuilding(b, t); }});
    for (const auto& w : walls) {
        // Vertical side walls are seen from above: always behind everything.
        float key = (w.w >= w.h) ? w.y : WORLD_H;
        out.push_back({key, [&w] { drawWall(w); }});
    }
    if (hasGate) {
        out.push_back({0.0f, [this] {
            setColor(withAlpha(hex(0x9aa0a8), 0.8f));
            line(gate.x0, 7, gate.x1, 7, 2.0f);  // sliding-gate track
            drawGatePanel(gate.x0, 7, 78.0f, 46.0f);
            drawGatePanel(gate.x1, 7, 102.0f, 46.0f);
            drawPillar(gate.x0 - 8, 9, 20);
            drawPillar(gate.x1 + 8, 9, 20);
        }});
    }
}

void Map::drawCanopies(float t) const {
    for (size_t i = 0; i < trees.size(); ++i) drawTreeCanopy(trees[i], t, static_cast<int>(i) + 1);
    for (const auto& u : tables) drawUmbrella(u, t);
    for (const auto& f : flags) drawFlagCloth(f, t);
    for (const auto& l : lamps) drawLampHead(l, t);
    if (hasGate) drawGateArch(gate, t);
    drawClouds(t);
}

void Map::drawCollisionDebug() const {
    setColor({1.0f, 0.2f, 0.2f, 0.18f});
    for (const Rect& r : solids_) rect(r);
    setColor({1.0f, 0.1f, 0.1f, 0.9f});
    for (const Rect& r : solids_) rectOutline(r.x, r.y, r.w, r.h, 1.5f);
    for (const Circle& c : circles_) circleOutline(c.c.x, c.c.y, c.r, 1.5f);
    setColor({0.2f, 0.4f, 1.0f, 0.9f});
    for (const Rect& r : blockers_) rectOutline(r.x + 2, r.y + 2, r.w - 4, r.h - 4, 1.0f);
}
