#pragma once

class CMonstorUnit;

namespace expanded {

// DeSanga "Vampire": with the DeSanga in hand, or Super Steve carrying its SynthSphere, every monster the active
// character kills restores kVampireHeal WHP to the weapon in hand, up to its max.
inline constexpr float kVampireHeal = 5.0f;

// A monster's life just ran out (CheckDmg): the heal, when the game credits the kill to the active character.
void VampireKill(const CMonstorUnit &unit, int monster);

} // namespace expanded
