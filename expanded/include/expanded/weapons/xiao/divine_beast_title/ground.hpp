#pragma once

#include <vector>

#include "collision.hpp"

namespace expanded::divine_beast_title {

// The ground around the cat, the way a thrown item finds it: the dungeon's polygons near it, as a character's floor and
// walls take them.
class CatGround {
public:
    // The floor under pos near its height: cast at its root, kProbeAhead along dir and kProbeBehind back, the highest hit
    // (so on stairs it rides its uphill end); floor is left as it is over a pit. The polygons gathered stay for WallAhead.
    void Probe(const float *pos, const float *dir, float &floor);

    // Whether a wall stands within kWallFrames of walking along move (x, z) from pos, over floor.
    bool WallAhead(const float *pos, float floor, const float *move);

private:
    std::vector<CCPoly> polys_ = std::vector<CCPoly>(8192);
    int                 count_ = 0;
};

} // namespace expanded::divine_beast_title
