#pragma once

namespace expanded {

// Tall Hammer "Colossus"'s look: as Goro grows past his own size, his body sheets fade from their own colours to their
// gold-and-turquoise copies (goro_gold.hpp, built into his pack), his gold parts (the gold frame and the cape) take the
// game's frame tint, kColossusTint, and the hammer the character tint, kColossusHammerTint, each by his share of the way
// from his size to kColossusMost (none at his size or smaller, all of it at kColossusMost). The sheets are drawn from
// their baked pictures, so the fade is written into those (a sheet at another size, a texture pack's, is left as it is).
inline constexpr float kColossusTint[3] = {140.0f, 120.0f, 70.0f}; // light gold, added to the ambient (0-255) at kColossusMost
// The hammer in his hand, lit whole by the game's character tint (its ambient_offset) the same way: kColossusHammerTint at
// kColossusMost.
inline constexpr float kColossusHammerTint[3] = {100.0f, 100.0f, 100.0f};

// His share of the look at a size (1: his own).
float ColossusLookShare(float size);

// A dungeon step at his size (1 with another member out): the sheets and the tints brought to its share.
void ColossusLookStep(float size);

} // namespace expanded
