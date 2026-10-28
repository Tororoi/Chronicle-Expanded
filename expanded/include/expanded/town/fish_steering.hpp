#pragma once

namespace dc::game {
class CFish;
} // namespace dc::game

namespace expanded {

// Fish steering: at Matataki Waterfall and East Harbor every fish turns toward the player every kSteerFrames frames,
// which keeps them within reach without locking them on: their own swimming turns them again as it changes course.
// While a Mardan sword is owned its fish turn toward the player too, at their own pace (MardanSteerFrames), by the bait
// last on the hook since the spot's fish loaded.
inline constexpr int kSteerFrames = 600;

// Hooks: FishingLoadFish having loaded a spot's fish, and FishingStepFish about to step them (Fish, turned toward
// Chara, by esa_type's bait).
void FishSteeringLoaded(int spot);
void FishSteeringStep();

// Turns fish to swim toward position, unless it is there.
void TurnFishToward(CFish &fish, const float *position);

} // namespace expanded
