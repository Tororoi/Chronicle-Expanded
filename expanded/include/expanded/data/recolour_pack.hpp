#pragma once

// A character pack with one of its 8-bit pictures recoloured texel by texel and renamed, so it is not taken for the
// pack's own when both are loaded: Satan's Ax's pink fire (pink_fire.hpp), the Midas Touch's gold (gold_statue.hpp).
// Includes no game header, so dcdata can build with it.

#include <array>
#include <functional>
#include <span>

#include "expanded/data/bytes.hpp"

namespace expanded {

// source's picture (in bank, named picture) with each texel's colour (0-255) mapped by colour_of, the colours merged
// back to 256 as the look pages merge them, and renamed to renamed, mds's meshes' materials with it.
Bytes RecolourPackPicture(std::span<const unsigned char> source, const char *bank, const char *picture, const char *mds, const char *renamed,
                          const std::function<std::array<int, 3>(const std::array<int, 3> &)> &colour_of);

} // namespace expanded
