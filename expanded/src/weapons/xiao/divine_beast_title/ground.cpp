#include "expanded/weapons/xiao/divine_beast_title/ground.hpp"

#include "dun/gameloop.hpp"
#include "dungeonparts.hpp"

namespace expanded::divine_beast_title {

namespace {

constexpr float kProbeUp = 8.0f, kProbeDown = 40.0f;
constexpr float kProbeAhead = 3.0f, kProbeBehind = 2.0f;
constexpr int   kCharacterSkip = 1;                     // the polygons a character's floor and walls skip
constexpr float kWallHeight = 6.0f, kWallFrames = 3.0f; // the wall probe: this far up, this many frames of walking ahead

} // namespace

void CatGround::Probe(const float *pos, const float *dir, float &floor) {
    count_ = 0;
    if (NowDngMap == nullptr) {
        return;
    }
    count_ = setCollisionData(NowDngMap, polys_.data(), const_cast<float *>(pos), 20.0f, 1.5f);
    bool  found = false;
    float highest = 0.0f;
    for (float along : {0.0f, kProbeAhead, -kProbeBehind}) {
        float x = pos[0] + dir[0] * along, z = pos[2] + dir[1] * along;
        float from[4] = {x, pos[1] + kProbeUp, z, 1.0f}, to[4] = {x, pos[1] - kProbeDown, z, 1.0f}, hit[4];
        if (count_ > 0 && CheckHit(polys_.data(), count_, from, to, hit, 1, kCharacterSkip) >= 0 && (!found || hit[1] > highest)) {
            highest = hit[1];
            found = true;
        }
    }
    if (found) {
        floor = highest;
    }
}

bool CatGround::WallAhead(const float *pos, float floor, const float *move) {
    if (count_ <= 0) {
        return false;
    }
    float y = floor + kWallHeight;
    float from[4] = {pos[0], y, pos[2], 1.0f}, to[4] = {pos[0] + kWallFrames * move[0], y, pos[2] + kWallFrames * move[1], 1.0f}, hit[4];
    return CheckHit(polys_.data(), count_, from, to, hit, 1, kCharacterSkip) >= 0;
}

} // namespace expanded::divine_beast_title
