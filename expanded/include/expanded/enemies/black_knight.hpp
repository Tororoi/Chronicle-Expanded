#pragma once

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// The light of the Black Knight's defeat scene (event 508's e508_ex) for his death off his floor
// (expanded/docs/bosses.md): its KEY 1 on the ground at his feet, at the scene's 0.25 until its frame 36 hits him,
// then as Hercules' Wrath's sparkle plays it in the PS2 mod: 0.2, faded across its frames 60-68 and ended there.

// Brings the light down on the monster, with its sound; false when it is not loaded.
bool BlackKnightLight(CMonstorUnit &unit, int monster);

// Whether the light has hit him since it came down.
bool BlackKnightLit();

// Hook: the dungeon step after the shot effects.
void BlackKnightStep();

} // namespace expanded
