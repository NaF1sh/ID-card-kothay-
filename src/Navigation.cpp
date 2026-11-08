#include "Navigation.h"
#include "Collision.h"
#include "Map.h"
#include <algorithm>
#include <queue>
#include <limits>

bool navigation::clearPath(Vec2 from, Vec2 to, float radius, const Map& map) {
    int steps = std::max(1, static_cast<int>((to - from).length() / 5) + 1);
    for (int i = 0; i <= steps; ++i)
        if (collision::overlapsSolid(from + (to - from) * (float(i) / steps), radius, map)) return false;
    return true;
}

std::vector<Vec2> navigation::findPath(Vec2 from, Vec2 to, float radius, const Map& map) {
    if (clearPath(from, to, radius, map)) return {to};
    constexpr int W = 50, H = 24;
    constexpr float CELL = 20.0f;
    auto point = [](int id) { return Vec2{(id % W + 0.5f) * CELL, (id / W + 0.5f) * CELL}; };
    // Connect each endpoint to a nearby visible cell; never snap through walls.
    auto endpoint = [&](Vec2 p) {
        int best = -1;
        float distance = 110;
        int cx = int(p.x / CELL), cy = int(p.y / CELL);
        for (int y = std::max(0, cy - 3); y <= std::min(H - 1, cy + 3); ++y)
            for (int x = std::max(0, cx - 3); x <= std::min(W - 1, cx + 3); ++x) {
                int id = y * W + x;
                float d = (point(id) - p).length();
                if (d < distance && clearPath(p, point(id), radius, map)) { distance = d; best = id; }
            }
        return best;
    };
    int start = endpoint(from), goal = endpoint(to);
    if (start < 0 || goal < 0) return {};
    std::vector<float> cost(W * H, std::numeric_limits<float>::infinity());
    std::vector<int> parent(W * H, -1);
    std::vector<bool> closed(W * H, false);
    using Node = std::pair<float, int>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    cost[start] = 0;
    open.push({0, start});
    while (!open.empty()) {
        int current = open.top().second;
        open.pop();
        if (closed[current]) continue;
        if (current == goal) {
            std::vector<Vec2> path{to};
            for (int id = goal; id >= 0; id = parent[id]) path.push_back(point(id));
            std::reverse(path.begin(), path.end());
            return path;
        }
        closed[current] = true;
        int x = current % W, y = current / W;
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            if ((!dx && !dy) || x + dx < 0 || x + dx >= W || y + dy < 0 || y + dy >= H) continue;
            int next = (y + dy) * W + x + dx;
            float candidate = cost[current] + (dx && dy ? 35.35534f : CELL);
            if (closed[next] || candidate >= cost[next]) continue;
            if (!clearPath(point(current), point(next), radius, map)) continue;
            parent[next] = current;
            cost[next] = candidate;
            open.push({candidate + (point(next) - point(goal)).length(), next});
        }
    }
    return {};
}
