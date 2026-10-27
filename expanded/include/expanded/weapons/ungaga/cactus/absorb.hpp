#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Cactus "Absorb": with the Cactus in Ungaga's hand, or Super Steve carrying its SynthSphere, every hit of the wielder's
// that takes life off a monster restores the wielder's thirst by the life it took over kAbsorbDamagePerWater (100 damage
// = 10 = one drop on the gauge), up to the gauge's most; nothing once the gauge is full. The dry monsters, the rock, metal
// and undead kinds (CactusDryKind), give nothing.
inline constexpr float kAbsorbDamagePerWater = 10.0f;

// Whether a monster kind (MonstorTable) gives no water.
bool CactusDryKind(int kind);

// Hook: a monster's life taken by a hit (CheckDmg), amount the damage.
void CactusMonsterDamaged(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);

} // namespace expanded
