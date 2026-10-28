#pragma once

namespace dc::game {
struct FishInfo;
} // namespace dc::game

namespace expanded {

// The game's fishing hooks, each calling the features that act there in turn.

// FishingLoadFish's spots, and the fish kinds (fish_info's index) Expanded names.
inline constexpr int kMatatakiWaterfall = 2;
inline constexpr int kEastHarbor = 3;
inline constexpr int kGobbler = 1;
inline constexpr int kGummy = 6;
inline constexpr int kNiler = 7;
inline constexpr int kUmadakara = 9;
inline constexpr int kPiccoly = 11;

// The kind of fish FishingLoadFish loads in a spot's slot: kind, the game's roll (-1 for none), as the weapons change it;
// rare_interval is the slot's one-in-n chance of a Garayan for the time of day.
int FishKind(int kind, int rare_interval);

// A fish's size as CFish::SetScale makes it: size, the game's roll for a kind of info, as the weapons change it.
float FishSize(float size, const FishInfo &info);

// FishingLoadFish having loaded a spot's fish (spot, the script's fish set), and FishingStepFish about to step them.
void FishSpotLoaded(int spot);
void FishStep();

// The distance from which fish notice the bait on the hook (FishingStepFish): radius, the game's for bait item_no, as
// the weapons' passives change it.
float BaitNoticeRadius(int item_no, float radius);

// The kind of a fish FishingLoadFish puts in spot, the game's drawn kind: the Matataki waterfall's Gummy is a Niler and
// East Harbor's Piccoly a Gobbler.
int PoolFish(int spot, int kind);

} // namespace expanded
