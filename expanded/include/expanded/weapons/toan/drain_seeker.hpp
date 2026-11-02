#pragma once

namespace dc::game {
class CDungeonMap;
} // namespace dc::game

namespace expanded {

// Drain Seeker "Blood Scent": with the Drain Seeker in hand, or Super Steve carrying its SynthSphere, the minimap shows
// every monster on the floor, as the Magical Crystal does, and a mimic still hiding in its chest as the monster it is:
// the monster's mark at the chest, not the chest's.
bool BloodScentActive();

// Hook: DrawMiniMap, for a closed chest: whether it is a mimic's that Blood Scent marks as a monster.
bool BloodScentMimic(const CDungeonMap &map, int box);

} // namespace expanded
