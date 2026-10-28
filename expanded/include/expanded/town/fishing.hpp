#pragma once

namespace expanded {

// The game's fishing hooks, each calling the features that act there in turn.

// The distance from which fish notice the bait on the hook (FishingStepFish): radius, the game's for bait item_no, as
// the weapons' passives change it.
float BaitNoticeRadius(int item_no, float radius);

} // namespace expanded
