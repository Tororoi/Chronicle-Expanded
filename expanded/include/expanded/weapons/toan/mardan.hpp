#pragma once

namespace dc::game {
struct FishInfo;
} // namespace dc::game

namespace expanded {

// The Mardan swords' fishing, while one is owned (any bag or the storage), the best one owned counting. Mardan Twei and
// Arise Mardan give each fish a spot loads a second, independent chance of being a Garayan: the one-in-n chance the
// time of day gives the spots that have them, a Baron Garayan one time in kBaronOdds, sized as the game sizes a fish.
// Arise Mardan also grows every fish (AriseSize).
inline constexpr int kBaronOdds = 5;

// The best Mardan sword owned, Arise Mardan before Mardan Twei before Mardan Eins, or 0.
int MardanSword();

// Hook: FishingLoadFish: the kind a spot's slot loads, kind the game's roll (-1 for none) and rare_interval its one-in-n
// chance of a Garayan for the time of day.
int MardanFishKind(int kind, int rare_interval);

// Arise Mardan's size: a size the game rolled (smoothed into the kind's largest, max_size) is scaled from 1 time at
// the roll's smallest (half usual_size) to kAriseGrowth times at the largest, then lifted over the scaled range by
// kAriseSmoothing * range * t^kAriseSmoothingExponent * (1 - t), t how far up it is, so the sizes ramp into the new
// largest; never past kAriseGrowth times max_size, which only the game's largest roll reaches.
inline constexpr float kAriseGrowth = 2.0f;
inline constexpr float kAriseSmoothing = 0.72f;
inline constexpr float kAriseSmoothingExponent = 2.0f;

float AriseSize(float size, float usual_size, float max_size);

// Hook: CFish::SetScale: a fish's size, size the game's for a kind of info, grown while Arise Mardan is owned.
float AriseFishSize(float size, const FishInfo &info);

// The fish the Mardan swords steer: while one is owned, a Mardan or Baron Garayan or an Umadakara turns toward the
// player every kMardanSteerFrames frames for the sword (Mardan Eins, Mardan Twei, Arise Mardan) divided by its liking
// for the bait last on the hook (FishInfo::food_affinity); one with no liking for it is left alone.
inline constexpr int kUmadakara = 9;
inline constexpr int kMardanSteerFrames[] = {240, 180, 120};

// The frames between sword's turns of a fish of fish_kind with affinity for the bait, or 0 for none.
int MardanSteerFrames(int sword, int fish_kind, float affinity);

} // namespace expanded
