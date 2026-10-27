#pragma once

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// Sun Sword "Solar Harvest": while the Sun Sword, Big Bang or Sword of Zeus is wielded, or Super Steve carries one's
// SynthSphere, each monster killed has a kSolarHarvestPercent chance to drop a Sun attachment in place of a regular
// drop. A monster that already carries a key or other set drop keeps it.
inline constexpr int kSolarHarvestPercent = 1;

// Whether one of those swords' effects is on.
bool SolarHarvestActive();

// Hook: CMonstorUnit::Step's death drop, before the game's rare-item roll: the item monster drops, a Sun attachment when
// the harvest wins, else drop_item as it is.
int SolarHarvestDrop(const CMonstorUnit &unit, int monster, int drop_item);

} // namespace expanded
