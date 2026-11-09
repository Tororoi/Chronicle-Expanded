#pragma once

namespace expanded {

// The Inferno's flame over Goro as his pull begins: a dungeon torch's flame (CFireOmni's, its flicker and its glow, of
// its own step) kRisingFlameSize units square at kRisingFlameAlpha (of 0x80), centred kRisingFlameLift above his feet.
// It rises out of the floor beneath him over kRisingFlameRise frames, cut off at his feet, and sinks back the same way
// once the pull ends; it draws over him, at the depth of a point kRisingFlameNear toward the camera. Times are game
// frames.
inline constexpr float         kRisingFlameSize = 30.0f;
inline constexpr float         kRisingFlameLift = 13.0f;
inline constexpr unsigned char kRisingFlameAlpha = 0x40;
inline constexpr int           kRisingFlameRise = 8;
inline constexpr float         kRisingFlameNear = 15.0f;

// One dungeon step: rising while up, sinking otherwise.
void RisingFlameStep(bool up);
// How far it has risen (0 under the floor, 1 over him).
float RisingFlameRisen();
// The draw, after the dungeon's torches.
void RisingFlameDraw();
void RisingFlameReset();

} // namespace expanded
