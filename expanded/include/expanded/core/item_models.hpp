#pragma once

namespace expanded {

// An item model Expanded builds for an item the game has none for (its .mds and .img at this path, no extension), or
// null: the Gold Bullion's bar (data/gold_bar.hpp), once dcdata has built it. The game's item model paths take it in
// place of the stand-in they would draw.
const char *ExpandedItemModel(int item_no);

} // namespace expanded
