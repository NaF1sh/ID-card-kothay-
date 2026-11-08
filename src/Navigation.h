#pragma once
#include <vector>
#include "Types.h"
class Map;
namespace navigation {
bool clearPath(Vec2 from, Vec2 to, float radius, const Map& map);
std::vector<Vec2> findPath(Vec2 from, Vec2 to, float radius, const Map& map);
}
