#pragma once

namespace expanded {

// Tall Hammer "Colossus"'s look: as Goro grows past his own size, his body sheets fade from their own colours to their
// gold-and-turquoise copies (goro_gold.hpp, built into his pack), and his gold parts (the gold frame and the cape)
// take the game's frame tint, kColossusTint, each by his share of the way from his size to kColossusMost (none at his
// size or smaller, all of it at kColossusMost). The sheets are drawn from their baked pictures, so the fade is written
// into those (a sheet at another size, a texture pack's, is left as it is).
inline constexpr float kColossusTint[3] = {70.0f, 60.0f, 35.0f}; // light gold, added to the ambient (0-255) at kColossusMost

// His share of the look at a size (1: his own).
float ColossusLookShare(float size);

// A dungeon step with Goro in play at his size: the sheets and the tint brought to its share.
void ColossusLookStep(float size);

} // namespace expanded
