#pragma once

class CCharacter;
class CSHOT;
struct COLLISION_HIT;

namespace expanded {

// Divine Beast Title "Spirit Beast", as the mod has it: with the Divine Beast Title in hand, or Super Steve carrying its
// SynthSphere, a shot held drawn kSpiritBeastFrames (the charge-complete flash) lets Xiao's cat (the model built at
// extraction, data/spirit_cat.hpp) go with its pellet. The pellet is held where it was fired, passing through everything,
// and shrinks away as the cat grows over kSpiritBeastGrowFrames with its head on it; at full size the cat breaks away
// with the pellet's velocity and falls (kSpiritBeastGravity, as every flight of the cat does) to the floor; the land clip starts kSpiritBeastLandLead frames before
// the touchdown, its momentum stops as the paws touch, and kSpiritBeastLandFrames after the clip began it walks
// (kSpiritBeastWalk units a frame) at the monster she is locked on to, else the nearest to her within kSpiritBeastSight;
// with none it sits. Within kSpiritBeastPounce it crouches (kSpiritBeastReadyFrames), held while the monster cannot be
// hit, then winds up in the float-up and leaps: an arc reaching the height of the monster's aim point in kSpiritBeastLeapFrames
// (kSpiritBeastLeapFramesTall for a large one), its horizontal speed re-aimed at it until the apex. Falling or landing,
// the first monster its head touches (as a pellet does) takes one hit: the pellet's damage plus the weapon's attack, in the weapon's
// element, through a guard, staggering it and shoving it (kSpiritBeastKickSpeed, kSpiritBeastKickDecay) away from the cat.
// The cat then fades over kSpiritBeastFadeFrames as it goes on; kSpiritBeastLifeFrames after it was let go it fades where
// it stands. It glows blue. Times are game frames, distances units.
inline constexpr int   kSpiritBeastFrames = 30;
inline constexpr int   kSpiritBeastGrowFrames = 6;
inline constexpr float kSpiritBeastGravity = 0.08f;
inline constexpr float kSpiritBeastLandLead = 11.1f; // the land clip's frames to the paws' touch: (219 - 215) / 0.36
inline constexpr int   kSpiritBeastLandFrames = 27;
inline constexpr float kSpiritBeastWalk = 0.7f;
inline constexpr float kSpiritBeastWalkRate = 0.85f; // the town's walk mapping for this rig at that speed
inline constexpr float kSpiritBeastSight = 300.0f;
inline constexpr float kSpiritBeastPounce = 30.0f;
inline constexpr int   kSpiritBeastReadyFrames = 25;  // the crouch, 95..105 at 0.4
inline constexpr int   kSpiritBeastWindUpFrames = 11; // the float-up to the feet leaving the floor, 285..293 at 0.75
inline constexpr int   kSpiritBeastLeapFrames = 32;
inline constexpr int   kSpiritBeastLeapFramesTall = 40;
inline constexpr float kSpiritBeastTouch = 2.0f; // a pellet's reach
inline constexpr float kSpiritBeastHitRadius = 3.0f;
inline constexpr float kSpiritBeastKickSpeed = 2.0f;
inline constexpr float kSpiritBeastKickDecay = 0.3f;
inline constexpr int   kSpiritBeastFadeFrames = 30;
inline constexpr int   kSpiritBeastLifeFrames = 1200;

bool SpiritBeastActive();

// The cat's part of its life, for tests, and where it is.
enum class SpiritBeastPhase {
    kNone,
    kRiding,
    kFalling,
    kLanding,
    kWalking,
    kSitting,
    kReady,
    kWindUp,
    kLeaping
};
SpiritBeastPhase SpiritBeastPhaseNow();
bool             SpiritBeastFading();
const float     *SpiritBeastPosition();

// Hooks: a dungeon step; a pellet let go into slot; the cat's draw; its light (CCharacter::Draw); whether a hit is the
// cat's (a guard lets it through, and as a hit of Xiao's it staggers).
void SpiritBeastStep();
void SpiritBeastFired(CSHOT &shots, int slot);
void SpiritBeastDraw();
void SpiritBeastLight(const CCharacter &character, float *ambient);
bool SpiritBeastHit(const COLLISION_HIT &hit);

// The cat taken down (the floor changed).
void SpiritBeastReset();

} // namespace expanded
