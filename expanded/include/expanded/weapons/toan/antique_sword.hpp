#pragma once

namespace dc::game {
class CDungeonMap;
} // namespace dc::game

namespace expanded {

// Antique Sword "Treasure Sense": a shop sells the sword for kAntiqueSwordBuyPrice and pays kAntiqueSwordSellPrice for
// it (its attachments on top, as for every weapon), and for an Antique Sword SynthSphere too. With the sword in hand, or
// Super Steve carrying its SynthSphere, the minimap shows every closed chest on the floor, explored or not, as the
// Magical Crystal does, and marks the real ones apart from the mimics: a yellow circle with a pale yellow centre, a pixel
// wider for a big chest.
inline constexpr int kAntiqueSwordBuyPrice = 12000;
inline constexpr int kAntiqueSwordSellPrice = 3000;

// An item's price as a shop reads it, the table's as price.
int AntiqueSwordPrice(int item_no, int sell, int price);

// What a shop pays for an attachment, a SynthSphere's by its weapon, the table's as price.
int SphereSellPrice(int item_no, int sphere_weapon_no, int price);

// Whether the minimap shows every closed chest.
bool TreasureSenseActive();

// Whether the chest in the slot is a real one, not a mimic's.
bool RealChest(const CDungeonMap &map, int box);

// Hook: DrawMiniMap, after a closed chest's 8x8 icon is drawn at x, y: a real chest's mark over it.
void TreasureSenseMark(const CDungeonMap &map, int box, int x, int y);

} // namespace expanded
