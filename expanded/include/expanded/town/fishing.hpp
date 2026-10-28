#pragma once

namespace dc::game {
struct FishInfo;
} // namespace dc::game

namespace expanded {

// The game's fishing hooks, each calling the features that act there in turn.

// The kind of fish FishingLoadFish loads in a spot's slot: kind, the game's roll (-1 for none), as the weapons change it;
// rare_interval is the slot's one-in-n chance of a Garayan for the time of day.
int FishKind(int kind, int rare_interval);

// A fish's size as CFish::SetScale makes it: size, the game's roll for a kind of info, as the weapons change it.
float FishSize(float size, const FishInfo &info);

// The distance from which fish notice the bait on the hook (FishingStepFish): radius, the game's for bait item_no, as
// the weapons' passives change it.
float BaitNoticeRadius(int item_no, float radius);

} // namespace expanded
