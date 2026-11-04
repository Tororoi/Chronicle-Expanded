#pragma once

namespace expanded {

// Tall Hammer "Colossus"'s look: as Goro grows past his own size, his body sheets fade from their own colours to their
// copies with the shirt in turquoise (goro_shirt.hpp, built into his pack), and he and the hammer in his hand take the
// game's character tint, kColossusTint, each by his share of the way from his size to kColossusMost (none at his size or
// smaller, all of it at kColossusMost). The sheets are drawn from their baked pictures, so the fade is written into those
// (a sheet at another size, a texture pack's, is left as it is).
inline constexpr float kColossusTint[3] = {56.0f, 48.0f, 28.0f}; // light gold, added to the ambient (0-255) at kColossusMost

// His share of the look at a size (1: his own).
float ColossusLookShare(float size);

// A dungeon step at his size (1 with another member out): the sheets and the tints brought to its share.
void ColossusLookStep(float size);

} // namespace expanded
