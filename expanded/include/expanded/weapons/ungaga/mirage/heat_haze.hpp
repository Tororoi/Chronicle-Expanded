#pragma once

namespace expanded::mirage {

// The Mirage copy's heat shimmer: the game's own heat raster over a fire (CFireOmni::DrawRaster,
// the one framebuffer distortion it has), drawn once a frame at the copy's root, kHazeLift up (the raster sits high, built to
// rise over a flame; the draw adds the fire's own 3), kHazeWidth by kHazeHeight units (a fire's is 15 by 16), its sway
// kHazeSway times a fire's at the strength it is given (0 none to 1 full) and its wave stepping kHazeWaveStep at most a
// frame (a fire's 2). It blends the mask the dungeon's fires bring ("alpha01"): a dungeon without fires shows none.
inline constexpr float kHazeLift = -15.0f;
inline constexpr float kHazeWidth = 45.0f, kHazeHeight = 96.0f;
inline constexpr float kHazeSway = 3.0f;
inline constexpr float kHazeWaveStep = 1.0f;
inline constexpr float kHazeWaveCycle = 8.0f; // the wave's phase wraps at the eight steps of its table

// A shimmer's size and place: lift above its root, width by height units, sway times a fire's at full gain.
struct HazeLook {
    float lift, width, height, sway;
};

inline constexpr HazeLook kMirageHaze = {kHazeLift, kHazeWidth, kHazeHeight, kHazeSway};

// A frame unpaused: the wave stepped, at most step (the Mirage copy's by default).
void HazeStep(float step = kHazeWaveStep);
// The draw at root, at gain, as look has it (the Mirage copy's by default).
void HazeDraw(const float *root, float gain, const HazeLook &look = kMirageHaze);
// The wave's phase (tests).
float HazePhase();
void  HazeReset();

} // namespace expanded::mirage
