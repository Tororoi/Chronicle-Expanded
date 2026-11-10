#pragma once

namespace expanded {

// The Inferno's flame over Goro as his pull begins: the dungeons' shared torch flame (torch_fire.hpp: CFireOmni's, its
// flicker and its glow, of its own step, the same in every dungeon) kRisingFlameSize units square at kRisingFlameAlpha (of 0x80), centred kRisingFlameLift above his feet.
// It rises out of the floor beneath him over kRisingFlameRise frames, cut off at his feet, and sinks back the same way
// once the pull ends; it draws over him, at the depth of a point kRisingFlameNear toward the camera. Times are game
// frames.
inline constexpr float         kRisingFlameSize = 30.0f;
inline constexpr float         kRisingFlameLift = 7.0f;
inline constexpr unsigned char kRisingFlameAlpha = 0x80;
inline constexpr int           kRisingFlameRise = 14;
inline constexpr float         kRisingFlameNear = 15.0f;

// One dungeon step: rising while up, sinking otherwise.
void RisingFlameStep(bool up);
// How far it has risen (0 under the floor, 1 over him).
float RisingFlameRisen();
// The draw, after the dungeon's torches.
void RisingFlameDraw();
void RisingFlameReset();

} // namespace expanded
