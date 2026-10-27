#include "expanded/dungeon/floor_height.hpp"

#include <algorithm>
#include <vector>

#include "collision.hpp"
#include "dun/gameloop.hpp"
#include "dungeonparts.hpp"
#include "gameutil.hpp"

namespace expanded {

namespace {

constexpr float kFloorReach = 16.0f;
constexpr int   kFloorPolys = 8192;

} // namespace

bool FloorUnder(float x, float z, float from, float *ground) {
    if (NowDngMap == nullptr) {
        return false;
    }
    static std::vector<CCPoly> polys(kFloorPolys);
    float                      centre[4] = {x, from - kFloorDepth / 2.0f, z, 1.0f};
    int                        count = std::clamp(setCollisionData(NowDngMap, polys.data(), centre, kFloorReach, kFloorDepth / 2.0f / kFloorReach), 0, kFloorPolys);
    float                      start[4] = {x, from, z, 1.0f}, hit[4];
    if (CheckHitVertical(polys.data(), count, start, -kFloorDepth, hit, 0) < 0) {
        return false;
    }
    *ground = hit[1];
    return true;
}

} // namespace expanded
