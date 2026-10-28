#pragma once

// Yellow Drops' west bank as the mod's ISO bake reshapes it, so the town's fishing spot fishes from its shore: the
// bank's edge bulges west along its section (70 at most, by a sine, the section's ends kept) at twice its stations, in
// s1301's visual meshes (grid10, grid11) and its collision (the _a crown wall and floor, the _c camera wall), with the
// factory's camera ring (doumu_c) pulled in to the dome and the torii pillars given camera hulls; and the town's water
// raised to the bank's notch. Includes no game header, so dcdata can build with it.

#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/town_scene.hpp"

namespace expanded {

// The water's height, raised from 0.
inline constexpr float kYellowDropsWater = 4.25f;

// The scene with its water sheet (s1302) raised and s1301 rebuilt with the bulged bank (appended, its directory entry
// moved to it); the disc's s1301, or it throws. cfg places the pillars and the dome.
Bytes YellowDropsWestBank(std::span<const unsigned char> scene, std::span<const unsigned char> cfg);

// The mapinfo with its water surface raised.
Bytes YellowDropsWater(std::span<const unsigned char> cfg);

// The walls along the bulged waterline, from the bank's top down to under the fish.
std::vector<scene::Triangle> YellowDropsBankWalls();

} // namespace expanded
