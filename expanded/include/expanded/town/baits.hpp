#pragma once

namespace dc::game {
struct EsaInfo;
} // namespace dc::game

namespace expanded {

// Expanded's baits, after esa_info's thirteen: their esa_type is kFirstExtraBait on, in the order baits.cpp lists them.
// The bait picker takes each from the bag, and it goes on the hook with its item's model and is spent as the game's
// baits are. A fish's affinity for one is the bait's for the fish's kind (BaitAffinity).
inline constexpr int kFirstExtraBait = 13;

// The esa_type of Expanded's bait made from item_no, or -1.
int ExtraBaitType(int item_no);

// The bait of esa_type type: esa_info's, or past its end Expanded's.
const EsaInfo &BaitInfo(int type);

// A fish of kind's affinity for Expanded's bait of esa_type type; 0 for another type or kind.
float ExtraBaitAffinity(int kind, int type);

// Hook: the bait picker (EventItemSelectKey) closing on item, which its list of the game's baits lacks: whether it takes
// it from the bag.
bool PickerTakesBait(int item);

} // namespace expanded
