#pragma once

namespace expanded {

// Skunk "Longer Flame": with the Skunk in Osmond's hand, his flame gun (CSHOT_FIREBAR, the Blessing Gun's and the
// Skunk's) lays its 24 particles kLongerFlame times as far apart along the aim, so the flame and the hits its particles
// plant reach that much farther: 92 from the muzzle instead of 46. The Blessing Gun keeps the game's spacing.
inline constexpr float kLongerFlame = 2.0f;

// Hook: CSHOT_FIREBAR::Init and Set: the distance between neighbouring flame particles, spacing the game's.
float FlameSpacing(float spacing);

} // namespace expanded
