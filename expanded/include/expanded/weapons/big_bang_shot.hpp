#pragma once

namespace dc::game {
class CCharacter;
class CSHOT;
} // namespace dc::game

namespace expanded {

// Super Steve carrying Big Bang's SynthSphere: "Detonate" from Xiao's slingshot. Locked on, a guard hangs a kHoverBomb
// times bomb over the target from its first frame, fading in red with a red-orange glow beside it over the
// kBombChargeFrames the charge takes; charged, it pulses red to black once a second and the slingshot whitens. Her
// next shot drops it for the blast (which no guard stops), the explosion and Big Bang's flash and blinding (kBombDropWhp); with nothing
// hanging, that shot is the flash alone from where she stands. Every pellet flies as a bomb: one held
// kChargedShotFrames before it is let go is a kChargedBomb times bomb whose blast is half the drop's, and a guard
// stops it (kChargedShotWhp); a plain one is a bomb at its own size that bursts where it lands, its own hit its damage. A
// charge unused for kBombPrimedFrames is spent. Times are game frames.

inline constexpr int   kBombChargeFrames = 180;
inline constexpr int   kBombPrimedFrames = 600;
inline constexpr int   kChargedShotFrames = 60;
inline constexpr float kHoverBomb = 4.0f;
inline constexpr float kChargedBomb = 2.0f;
inline constexpr float kPlainBomb = 1.0f;
inline constexpr float kBombDropWhp = 20.0f;
inline constexpr float kBombFlashWhp = 5.0f;
inline constexpr float kChargedShotWhp = 10.0f;
inline constexpr float kChargedBlast = 0.5f; // of the drop's damage, reach and throw
inline constexpr float kHoverMargin = 10.0f; // the hanging bomb's bottom over the monster's height
inline constexpr float kBombGlowSize = 36.0f;

bool BigBangShotActive();
// A bomb hangs over the target, or falls on it.
bool BigBangShotHanging();

// One step (once a frame the monsters step unpaused); the floor's monsters put back; the hanging bomb's glow, with
// the dungeon's effects.
void BigBangShotStep();
void BigBangShotReset();
void BigBangShotDraw();

// Hooks: Xiao fired a pellet into slot of shots; a pellet ended at point; the white the charge puts on her and the
// slingshot and the hanging bomb's tint.
void BigBangShotFired(CSHOT &shots, int slot);
void BigBangShotEnded(const CSHOT &shots, int slot, const float *point);
void BigBangShotLight(const CCharacter &character, float *ambient);
// Whether a pellet out is a charged bomb (its blast is its hit, so it hurts nothing itself).
bool BigBangShotBlasts(const CSHOT &shots, int slot);

} // namespace expanded
