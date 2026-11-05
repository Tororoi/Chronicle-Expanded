#pragma once

namespace expanded {

// Pull of the Underworld's sparks (satans_ax.hpp): while the pull is on, the pink hit spark (basefx00's first cell)
// lies on the ground around Goro, kSparkNear to kSparkFar from him, and is drawn in toward him: the pull a monster of
// weight 1 takes where each is, times the pull's strength, on top of kSparkCreep, each wandering across its path on a
// sway of its own. kSparkHoldPerFrame start a frame while he holds the charge, and kSparkSmashPerFrame, eight times as
// many, once he lets it go and the pull is at its strongest. Each starts unseen and fades in over its first
// kSparkFadeIn units drawn in, twinkles, shrinks and fades as it comes within kSparkNear of him (each fade no more
// than a quarter of its way in), and fades over its last kSparkFadeOut frames; when the pull ends they fade where
// they are.
// Each monster the pull draws gives off sparks of its own too, kBodySparkPerFrame a frame times the pull's strength,
// from anywhere on it (one of its meshes' vertices as it stands, or failing those inside its frames' bounds or its
// hurt spheres), drawn in as the ground's are, through the air toward the height on his body nearest the one each
// started at (his feet to CharaHeight above them); one starting inside the monster is hidden by it until it leaves.
inline constexpr int   kSparkCount = 384;
inline constexpr float kBodySparkPerFrame = 0.5f;
inline constexpr float kSparkHoldPerFrame = 0.375f;
inline constexpr float kSparkSmashPerFrame = 3.0f;
inline constexpr float kSparkNear = 12.0f;
inline constexpr float kSparkFar = 90.0f;
inline constexpr float kSparkCreep = 0.15f;
inline constexpr int   kSparkLife = 120;
inline constexpr float kSparkFadeIn = 6.0f; // units drawn in
inline constexpr int   kSparkFadeOut = 20;  // frames
inline constexpr int   kSparkAlpha = 0x80;  // its full colour, added to the floor's
inline constexpr float kSparkLift = 1.0f;   // above the ground
inline constexpr float kSparkSize = 1.2f;   // away from him (a hit's sparks are 0.1 to 1.3)

// One dungeon step: sparks started, drawn in and aged.
void UnderworldSparksStep();
// Drawn just after the run dust.
void UnderworldSparksDraw();
// The sparks in flight (tests).
int UnderworldSparksLive();
// Where spark i of the live ones is (tests); false past the last.
bool UnderworldSparkAt(int i, float *position);
// How opaque spark i of the live ones is, before its twinkle (tests); -1 past the last.
int UnderworldSparkAlpha(int i);
// Every spark gone (the floor changed; tests too).
void UnderworldSparksReset();

} // namespace expanded
