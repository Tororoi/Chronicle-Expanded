#pragma once

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// The Crystal Gemron (expanded/docs/gemrons.md): Holy Gemron's stats and script (its range widened) under its own model
// and record, weak to ice and immune to the rest. It fires the Ice Queen's homing ice arrow, which can freeze, and
// encases a frozen player in her ice prison; its crystal balls shatter as it dies; its eyes glow.

// Hook: the dungeon step after the shot effects (its arrows and prison).
void CrystalGemronStep(CMonstorUnit &unit);

// Hook: the monster draw before a unit draws (its eyes' glow).
void CrystalGemronDrawing(CMonstorUnit &unit, int monster);

} // namespace expanded
