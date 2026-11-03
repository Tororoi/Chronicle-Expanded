#pragma once

namespace dc::game {
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Plate Hammer "Tempering": each time it breaks and Auto Repair Powder mends it, its max WHP rises by kTemperingGain
// (to kTemperingMost, the most a level-up gives) before the powder fills it back up. Super Steve carrying its
// SynthSphere is tempered the same way.
inline constexpr int kTemperingGain = 1;
inline constexpr int kTemperingMost = 99;

// Hook: Auto Repair Powder about to mend the broken weapon (the equipped record): its max WHP raised when it is tempered.
void TemperingRepaired(WEAPON_HAVE &weapon);

} // namespace expanded
