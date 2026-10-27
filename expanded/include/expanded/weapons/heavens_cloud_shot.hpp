#pragma once

namespace dc::game {
class CSHOT;
} // namespace dc::game

namespace expanded {

// Super Steve carrying Heaven's Cloud's SynthSphere: "Typhoon" from Xiao's slingshot. Holding the shot drawn charges
// it over kTyphoonShotFrames in two stages, each with the game's charge-complete flash: from kTyphoonShotEmpowered of
// the way the shot is empowered, and at kTyphoonShotFull it is full. An empowered shot's pellet grows (to
// kTyphoonPellet times its size at full) and hits harder (up to kTyphoonShotDamage times), and where it lands among
// the monsters (on one, or with one within kTyphoonBurstNear) it bursts in a wind blast: the Wind Gem's burst, grown
// from kTyphoonBurstLook to kTyphoonBurstLookFull times, its reach from kTyphoonBlastReach to kTyphoonBlastReachFull.
// Every monster in it takes kTyphoonBlastDamage times the attack times (0.5 + 0.5 x the charge), in the selected
// element, a guard stopping it, and is thrown out to the blast's edge from a point kTyphoonBlastPullback short of it
// towards Xiao. Its Quick Draw (small_sword.hpp) and Moonlit Focus (tsukikage.hpp) live with those weapons.
// Times are game frames; the charge is 0 at the first stage and 1 at the second.
inline constexpr int   kTyphoonShotFrames = 180;
inline constexpr float kTyphoonShotEmpowered = 0.125f;
inline constexpr float kTyphoonShotFull = 0.98f;
inline constexpr float kTyphoonPellet = 8.0f;
inline constexpr float kTyphoonShotDamage = 1.5f;
inline constexpr float kTyphoonBurstNear = 40.0f;
inline constexpr float kTyphoonBurstLook = 2.0f;
inline constexpr float kTyphoonBurstLookFull = 5.0f;
inline constexpr float kTyphoonBurstRise = 5.0f; // the burst's model rises this far from its origin per 1x of size
inline constexpr float kTyphoonBlastReach = 60.0f;
inline constexpr float kTyphoonBlastReachFull = 160.0f;
inline constexpr float kTyphoonBlastDamage = 0.75f;
inline constexpr float kTyphoonBlastPullback = 5.0f;

bool TyphoonShotActive();

// The charge a hold of frames gives: 0 short of the first stage, up to 1 at the second.
float TyphoonShotCharge(int frames);

// Hooks: a dungeon step; a pellet let go (from the shot's pool, its slot); a pellet ended at point (struck: on a
// monster).
void TyphoonShotStep();
void TyphoonShotFired(CSHOT &shots, int slot);
void TyphoonShotEnded(const CSHOT &shots, int slot, const float *point, bool struck);

} // namespace expanded
