#pragma once

// The fishing collision the mod authored, where a spot's own geometry has none: as world triangles. Includes no game
// header, so dcdata can build with it.

#include <vector>

#include "expanded/data/town_scene.hpp"

namespace expanded {

// Queens: the canal's containment walls along both banks.
std::vector<scene::Triangle> QueensCanalWalls();

} // namespace expanded
