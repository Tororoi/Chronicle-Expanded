#pragma once

struct COLLISION_HIT;

namespace expanded {

// Baselard "Heavy Hand": whether a hit is Toan's swing with it, or Super Steve's pellet with its sphere. The Claymore
// throws as it does (claymore.hpp).
bool HeavyHandHit(const COLLISION_HIT &record);

} // namespace expanded
