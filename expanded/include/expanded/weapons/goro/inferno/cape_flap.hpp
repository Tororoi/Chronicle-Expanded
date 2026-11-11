#pragma once

namespace expanded {

// The Inferno's flame (or Satan's Ax's) flapping Goro's cape as it rises (inferno/rising_flame.hpp), as the Divine
// Beast Title's cat's cape flies (xiao/spirit_cape.hpp): not a wind but the cape's rest shape blown back off his back,
// which its springs follow. Each column bends up from its collar, link by link: each row hangs from the one above by
// its own length, turned up off his back by kCapeFlapLift (radians) at the hem eased by kCapeFlapEase along the hang
// (the shoulders stay down, the tail flies), with a swell of up to kCapeFlapSwell running collar to hem every
// kCapeFlapRippleFrames, a wavelength kCapeFlapRippleRows rows long, so it never stretches. Its springs hold it there
// as the cat's do: its gravity faded out and its stiffness raised to kCapeFlapStiffness. All of it times how far the
// flame has risen; once it has sunk the cape is its own again. Times are game frames.
inline constexpr float kCapeFlapLift = 1.8f;
inline constexpr float kCapeFlapEase = 1.2f;
inline constexpr float kCapeFlapSwell = 0.3f;
inline constexpr int   kCapeFlapRippleFrames = 30;
inline constexpr float kCapeFlapRippleRows = 10.0f;
inline constexpr float kCapeFlapStiffness = 0.25f;

// One dungeon step: his cape's rest shape blown by the flame, or put back.
void CapeFlapStep();
// How far, in radians, row b of a cape hang rows long swings up this step at a full flame (tests).
float CapeFlapSwing(int b, int hang);
void  CapeFlapReset();

} // namespace expanded
