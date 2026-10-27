#pragma once

struct BT_SHOT_EFFECT;

namespace expanded::babels_spear {

// The beam that marks Curse of Babel's spot: the Dark Genie's small beam in cyan (data/prop_models.hpp's kBabelBeamFile)
// as one of Expanded's shot effects, the visual alone (it hits nothing), burst at the spot and held there at its form's
// scale. Its clips are played by hand at kBeamRate a frame: its rise (KEY kBeamRise) once, then its loop (KEY kBeamLoop)
// over and over, each moved on from kBeamClipLead frames before its end, and its vanish (KEY kBeamVanish) once the copy's
// time is up, which ends the shot at its last frame (it is the description's first motion, whose end the game retires a
// first-phase shot at). A shot the game ends before its vanish is taken up again.
inline constexpr float kBeamRate = 0.1f;
inline constexpr int   kBeamRise = 0, kBeamLoop = 1, kBeamVanish = 2;
inline constexpr float kBeamRiseStart = 10.0f, kBeamRiseEnd = 28.0f;
inline constexpr float kBeamLoopStart = 28.0f, kBeamLoopEnd = 48.0f;
inline constexpr float kBeamVanishStart = 48.0f, kBeamVanishEnd = 70.0f;
inline constexpr float kBeamClipLead = 0.5f;

// Its description, which the weapon registers (shot_effects.hpp).
BT_SHOT_EFFECT *BeamEffect();

// Burst at spot (x, ground, z), scale times its size, on its rise: false when it is not loaded on this floor.
bool BeamStart(const float *spot, float scale);
// One frame: held at spot on the clip its time calls for (vanishing once vanish is set).
void BeamStep(const float *spot, bool vanish);
// Put out now.
void BeamStop();
// Whether it plays, the KEY it is on (-1 when none) and its clip's frame.
bool  BeamPlaying();
int   BeamKey();
float BeamFrame();

} // namespace expanded::babels_spear
