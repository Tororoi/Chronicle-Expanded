#pragma once

namespace expanded {

// Antique Sword "Treasure Sense": a shop pays kAntiqueSwordSellPrice for the sword (its attachments on top, as for every
// weapon), and with it in hand, or Super Steve carrying its SynthSphere, the minimap shows every closed chest on the floor,
// explored or not, as the Magical Crystal does.
inline constexpr int kAntiqueSwordSellPrice = 3000;

// An item's price as a shop reads it, the table's as price.
int AntiqueSwordPrice(int item_no, int sell, int price);

// Whether the minimap shows every closed chest.
bool TreasureSenseActive();

} // namespace expanded
