#pragma once

namespace expanded {

// Crysknife "Crystal Affinity": a thrown elemental gem's burst deals twice its damage while the Crysknife is in hand,
// or Super Steve carries its SynthSphere.
inline constexpr int kCrystalAffinityDamage = 2;

// Hook: CMainItemModel::Step landing a thrown gem; the burst's damage.
int CrystalAffinityDamage(int damage);

} // namespace expanded
