#pragma once

namespace dc::game {
class CDungeonMap;
} // namespace dc::game

namespace expanded {

// Sax "Fine Fare": while the active character holds it, item chests give better fare. Each heir adds a tier:
// - Sax: water (Regular, Tasty) -> Premium Water, Bread and Cheese -> Premium Chicken.
// - Dusack: also Treasure Key -> Gold Bullion.
// - 7 Branch Sword, Atlamillia Sword, Chronicle Sword: also Repair Powder -> Auto Repair Powder, one chest in
//   kFineFareRepairOneIn, the same chests each time the floor is played.
// Super Steve carrying one of the five's spheres has that sword's tier.
inline constexpr int kFineFareRepairOneIn = 4;

// The tier of the weapon out, -1 for none.
int FineFareTier();

// Hook: BtGetTreasureboxSmall_Init: the item box_no of map gives.
int FineFareItem(const CDungeonMap &map, int box_no);

} // namespace expanded
