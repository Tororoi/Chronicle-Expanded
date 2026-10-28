#pragma once

namespace expanded {

// Skunk "Longer Flame": with the Skunk in Osmond's hand, or Super Steve throwing its SynthSphere's flame
// (xiao/flame_gun.hpp), the flame gun (CSHOT_FIREBAR, the Blessing Gun's and the Skunk's) lays its 24 particles
// kLongerFlame times as far apart along the aim, and draws each puff and plants its hit kFlamePuffScale times the size,
// so the flame and its hits reach 92 from the muzzle instead of 46, the hits radius 8 instead of 4. The Blessing Gun
// keeps the game's flame.
inline constexpr float kLongerFlame = 2.0f;
inline constexpr float kFlamePuffScale = 2.0f;

// Hook: CSHOT_FIREBAR::Init and Set: the distance between neighbouring flame particles, spacing the game's.
float FlameSpacing(float spacing);

// Hook: CSHOT_FIREBAR::Step and Draw: a puff's hit radius and drawn size, size the game's.
float FlamePuffSize(float size);

} // namespace expanded
