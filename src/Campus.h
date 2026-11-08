#pragma once
#include <string>
#include <vector>
#include "Map.h"

enum class CampusRole { Topper, Captain, Seller, Poet, Professor, Gamer };
struct CampusNpc {
    Vec2 position;
    std::string name, role;
    CampusRole kind;
    std::vector<std::vector<std::string>> conversations;
    int visits = 0;
    std::string lastTask = {};
};
std::vector<CampusNpc> campusCharacters(int levelIndex = 0);
void drawCampusNpc(const CampusNpc& npc, float time);
struct CampusCatPose {
    Vec2 position, direction;
    bool moving = false;
    bool happy = false;
};
void drawCampusCat(const CampusCatPose& pose, float time, int index);
