#pragma once

namespace expanded {

// The game's fishing hooks, each calling the features that act there in turn.

// FishingLoadFish's spots, and the fish kinds (fish_info's index) Expanded's pools trade.
inline constexpr int kMatatakiWaterfall = 2;
inline constexpr int kEastHarbor = 3;
inline constexpr int kGobbler = 1;
inline constexpr int kGummy = 6;
inline constexpr int kNiler = 7;
inline constexpr int kPiccoly = 11;

// The distance from which fish notice the bait on the hook (FishingStepFish): radius, the game's for bait item_no, as
// the weapons' passives change it.
float BaitNoticeRadius(int item_no, float radius);

// The kind of a fish FishingLoadFish puts in spot, the game's drawn kind: the Matataki waterfall's Gummy is a Niler and
// East Harbor's Piccoly a Gobbler.
int PoolFish(int spot, int kind);

} // namespace expanded
