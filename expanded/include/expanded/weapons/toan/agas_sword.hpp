#pragma once

namespace expanded {

// Aga's Sword "Defensive Legacy": the character holding Aga's Sword, or Super Steve carrying its SynthSphere, has
// kDefensiveLegacy more defense, on top of whatever food has raised it to.
inline constexpr int kDefensiveLegacy = 15;

// A character's defense, defense being their own, with the bonus their equipped weapon gives.
int DefensiveLegacyDefense(int chara, int defense);

} // namespace expanded
