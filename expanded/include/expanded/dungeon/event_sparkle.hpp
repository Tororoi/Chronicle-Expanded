#pragma once

namespace expanded {

// The event sparkle: the blue-white four-pointed glint a town draws over the event points the player walks up to
// (EdEventPointDraw's SystemEffect, cell (0, 0) of s_ef01 in s_eff.img of gedit/system/esys_cmn.pak), entered into
// kFloorCharacterBlock on a dungeon floor the first time it is wanted there.
inline constexpr const char *kSparklePack = "gedit/system/esys_cmn.pak";
inline constexpr const char *kSparkleImage = "s_eff.img";
inline constexpr const char *kSparklePicture = "s_ef01";
inline constexpr int         kSparkleCell = 32;

// Whether the picture is in on this floor, entering it when it is not; false when it cannot be read or has no room.
bool EventSparkleReady();

// Tests replace the read of the picture's image file (null puts the disc's back): it gives the file, or null.
using SparkleImageRead = unsigned int *(*) ();
void SetSparkleImageRead(SparkleImageRead read);

// Draws the sparkle centred at at, half_width and half_height as the game's 3D sprites take them (C3DSprite: the height
// in the screen's half-height rows), brightness 0-128, in the blend and depth state of the pass it is called from (the
// dungeon's additive one, DungeonShotsDrawn).
void DrawEventSparkle(const float *at, float half_width, float half_height, int brightness);

// A round glint's half_width over its half_height (the town's: 2 x scale by scale).
inline constexpr float kSparkleWidth = 2.0f;

} // namespace expanded
