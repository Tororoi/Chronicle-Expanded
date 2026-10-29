#pragma once

namespace expanded {

// Magical Hammer "Bait Ward": while a Magical Hammer is owned (any bag or the storage), a fish with no liking for the
// bait last on the hook (FishInfo::food_affinity 0) turns away from the hook every kBaitWardFrames frames, the spot's
// six slots kBaitWardFrames / 6 frames apart, and is never turned toward the player (fish_steering.hpp). Before any
// bait none is turned.
inline constexpr int kBaitWardFrames = 300;

// Whether a Magical Hammer is owned.
bool BaitWard();

} // namespace expanded
