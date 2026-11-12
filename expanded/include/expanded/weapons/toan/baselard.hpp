#pragma once

namespace dc::game {
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Baselard "Heavy Hand": whether a hit is Toan's swing with it, or Super Steve's pellet with its sphere. The Claymore
// throws as it does (claymore.hpp), and Super Steve's pellets with the Steel Hammer's sphere (xiao/steel_hammer_shot.hpp).
bool HeavyHandHit(const COLLISION_HIT &record);

} // namespace expanded
