#pragma once

namespace expanded {

// The floor's height under (x, z): the highest floor at or below from, searched kFloorDepth down (setCollisionData's
// polygons there, CheckHitVertical). False where there is none, or no dungeon map.
inline constexpr float kFloorDepth = 200.0f;

bool FloorUnder(float x, float z, float from, float *ground);

} // namespace expanded
