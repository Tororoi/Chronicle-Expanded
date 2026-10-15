#pragma once

struct WEAPON_DATA;

namespace expanded {

// Lamb's Sword transforms and powers up at this share of its WHP (retail: 0.2).
inline constexpr float kLambSwordThreshold = 0.5f;

// Applies the weapon stat changes to the game's weapon table, the first time it is called.
void WeaponStatsApply();

// Applies the weapon stat changes to a weapon table indexed from ITEM_WEAPON_START.
void WeaponStatsApply(WEAPON_DATA *list);

} // namespace expanded
