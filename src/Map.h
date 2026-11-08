// Map.h - campus description (pure data) + layered rendering.
// A Map only stores *what* is on the campus; Level.cpp decides *where*.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Types.h"

enum class BuildingKind { Academic, Admin, Canteen, GuardBooth };
enum class PathKind { Road, Pavement, Plaza };
enum class TreeKind { Rain, Krishnachura, Mango };

struct Building {
    Rect r;              // footprint (also the collision box)
    BuildingKind kind;
    std::string name;
    float doorX;         // world x of the front (south-facing) door
};
struct PathSeg   { Rect r; PathKind kind; };
struct Tree      { Vec2 p; float radius; TreeKind kind; };  // p = trunk base
struct Bush      { Vec2 p; float radius; };
struct FlowerBed { Rect r; };
struct Bench     { Vec2 p; bool vertical; };
struct LampPost  { Vec2 p; };
struct Railing   { Vec2 a, b; };
struct UmbrellaTable { Vec2 p; int colorIndex; };
struct Gate      { float x0 = 0, x1 = 0; };  // opening in the bottom boundary wall

// Something drawn in the depth-sorted layer; larger y = further away = drawn first.
struct DepthItem {
    float y;
    std::function<void()> draw;
};

class Map {
public:
    std::vector<PathSeg> paths;
    std::vector<Rect> crossings;      // zebra crossings painted on roads
    std::vector<Building> buildings;
    std::vector<Rect> walls;
    std::vector<Rect> lakes;
    std::vector<Rect> footballCourts;
    std::vector<Tree> trees;
    std::vector<Bush> bushes;
    std::vector<FlowerBed> flowerBeds;
    std::vector<Bench> benches;
    std::vector<LampPost> lamps;
    std::vector<Railing> railings;
    std::vector<UmbrellaTable> tables;
    std::vector<Vec2> minars;         // Shaheed Minar monuments (platform bottom-centre)
    std::vector<Vec2> flags;          // flag pole bases
    std::vector<Rect> signs;          // university name monuments
    Gate gate;
    bool hasGate = false;

    // Build collision / sight-blocking lists from the object data.
    void finalize();
    const std::vector<Rect>& solidRects() const { return solids_; }
    const std::vector<Circle>& solidCircles() const { return circles_; }
    const std::vector<Rect>& sightBlockers() const { return blockers_; }

    // Rendering is split into layers so characters can be drawn in between.
    void drawGround(float t) const;    // grass, paths, flower beds, ground shadows
    // Upright objects (buildings, walls, trunks, benches...) to be y-sorted
    // together with the characters. Items reference this Map, so draw them
    // before the Map changes.
    void collectDepthItems(float t, std::vector<DepthItem>& out) const;
    void drawCanopies(float t) const;  // tree tops, lamp glow, gate arch, clouds
    void drawCollisionDebug() const;

private:
    std::vector<Rect> solids_;
    std::vector<Rect> blockers_;
    std::vector<Circle> circles_;
};
