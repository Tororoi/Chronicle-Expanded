#pragma once

class CCharacter;
class CSHOT;
struct COLLISION_HIT;

namespace expanded {

// Divine Beast Title "Spirit Beast": with the Divine Beast Title in hand, or Super Steve carrying its SynthSphere, a
// shot held drawn kSpiritBeastFrames (the charge-complete flash) lets go Xiao's cat in place of the pellet. It grows out
// of the shot as it flies, falls to the floor, runs at the monster she is locked on to (else the nearest within
// kSpiritBeastSight of her), crouches within kSpiritBeastPounce of it and pounces. The first monster it touches takes one
// hit of kSpiritBeastDamage times the weapon's attack, in its selected element with its magic, through a guard, with a
// sword blow's stagger and a shove (kSpiritBeastKickSpeed, kSpiritBeastKickDecay) away from the cat; then the cat fades
// over kSpiritBeastFadeFrames. With nothing to chase it sits; it fades kSpiritBeastLifeFrames after it was let go. It
// glows blue. Times are game frames, distances units.
inline constexpr int   kSpiritBeastFrames = 60;
inline constexpr float kSpiritBeastDamage = 2.0f;
inline constexpr float kSpiritBeastSight = 300.0f;
inline constexpr float kSpiritBeastPounce = 30.0f;
inline constexpr float kSpiritBeastRun = 1.3f;      // units a frame
inline constexpr float kSpiritBeastFall = 0.08f;    // off the shot's line
inline constexpr float kSpiritBeastGravity = 0.05f; // in the pounce
inline constexpr int   kSpiritBeastLeapFrames = 32;
inline constexpr int   kSpiritBeastGrowFrames = 6;
inline constexpr int   kSpiritBeastFadeFrames = 30;
inline constexpr int   kSpiritBeastLifeFrames = 1200;
inline constexpr float kSpiritBeastTouch = 3.0f; // the cat's own reach in the touch
inline constexpr float kSpiritBeastHitRadius = 4.0f;
inline constexpr float kSpiritBeastKickSpeed = 2.0f;
inline constexpr float kSpiritBeastKickDecay = 0.3f;

bool SpiritBeastActive();

// The cat's state, for tests: where it is, and its part of the chase.
enum class SpiritBeastPhase {
    kNone,
    kFlying,
    kLanding,
    kRunning,
    kSitting,
    kReady,
    kLeaping,
    kFading
};
SpiritBeastPhase SpiritBeastPhaseNow();
const float     *SpiritBeastPosition();

// Hooks: a dungeon step; a pellet let go into slot; the cat's draw; its light (CCharacter::Draw); whether a monster's guard
// lets a hit through and whether a hit of Xiao's staggers (CheckDmg).
void SpiritBeastStep();
void SpiritBeastFired(CSHOT &shots, int slot);
void SpiritBeastDraw();
void SpiritBeastLight(const CCharacter &character, float *ambient);
bool SpiritBeastHit(const COLLISION_HIT &hit);

// The cat taken down (the floor changed).
void SpiritBeastReset();

} // namespace expanded
