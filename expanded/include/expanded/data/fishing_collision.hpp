#pragma once

// The fishing collision the mod authored, where a spot's own geometry has none: as world triangles. Includes no game
// header, so dcdata can build with it.

#include <vector>

#include "expanded/data/town_scene.hpp"

namespace expanded {

// Brownboo: the pond's three rocks, simplified, every face a slope (the line's float and hook rest on them).
std::vector<scene::Triangle> BrownbooRocks();

// Queens: the canal's containment walls along both banks.
std::vector<scene::Triangle> QueensCanalWalls();

// Yellow Drops: the bases of the two pillars standing in the water west of the bank (the scene's own collision there
// stops above the bank).
std::vector<scene::Triangle> YellowDropsPillarBases();

} // namespace expanded
