#pragma once

namespace expanded {

// What one swing wears off a weapon with no Endurance (BattleSubWeaponDmg's 1.5 - 0.01 x Endurance at a factor of 1).
inline constexpr float kSwingWear = 1.5f;

// A weapon effect's cost, worn off the active character's equipped weapon through the game's own wear (SwordDmgCheck1)
// as base_whp / kSwingWear swings: Endurance, Durable and Fragile apply, an Auto Repair Powder mends it at nothing, and
// it breaks there as any weapon does. Nothing outside a dungeon floor.
void WearWeapon(float base_whp);

} // namespace expanded
