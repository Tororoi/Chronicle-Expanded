#pragma once

#include "expanded/dungeon/own_sounds.hpp"

namespace dc::game {
class CCharacter;
class CSHOT;
struct COLLISION_HIT;
struct MONSTOR_EFFECT_STATE;
} // namespace dc::game

namespace expanded {

// Divine Beast Title "Spirit Beast": with the Divine Beast Title, the Angel Shooter or the Angel Gear in
// hand, or Super Steve carrying one's SynthSphere, a shot held drawn kSpiritBeastFrames (the charge-complete flash) lets
// Xiao's cat (the model built at extraction, data/spirit_cat.hpp) go with its pellet, taking away the cat that was out. Each
// weapon's cat has a look of its own: the Title's is plain and glows blue; the Angel Shooter's wears the wings, glows white,
// is kAngelShooterCatScale times the size (its walk, head and glow with it), crouches from kSpiritBeastPounceWinged
// (kSpiritBeastPounce by a mimic still in its chest) and steers its pounce on past the apex, until it has fallen halfway to
// the floor (landing, until kSpiritBeastCommitHorizon frames out); the Angel Gear's is that cat glowing gold, a warmer white,
// kAngelGearCatScale times the size; Super Steve's with either one's sphere is that cat at that size wearing the cape and
// mask in the selected element's colour (spirit_cape.hpp), its glow the element's. A change of look takes the cat away. Its step runs once a frame after the pellets step:
//   - Riding: from the pellet's first step it is held, through everything, and shrinks away as the cat grows over
//     kSpiritBeastGrowFrames with its head on it; at full size the cat breaks away with the pellet's velocity. A pellet
//     that ends before then leaves the cat full size where it was, fading.
//   - Falling: it moves, then falls kSpiritBeastGravity faster; the land clip cuts in kSpiritBeastLandLead frames before
//     the predicted touchdown.
//   - Landing: it keeps its momentum while in the air or until the clip's kSpiritBeastLandTouch, and walks once the clip
//     is at its end.
//   - Walking: kSpiritBeastWalk units a frame, straight at the monster she has (or would have) locked on to, else the
//     nearest to her within kSpiritBeastSight, taking a new lock-on every kSpiritBeastRetarget frames; a wall ahead stops it
//     standing; with no monster it sits, making kSpiritBeastSitSound as it sits down. A mimic still in its chest is a
//     target too, the chest its aim.
//   - Within kSpiritBeastPounce it crouches until the crouch clip ends, held while the monster cannot be hit (a mimic in
//     its chest, or one in its untouchable frames), then winds up in the float-up and leaps as the clip's feet leave the
//     floor: an arc through the height of the monster's aim point in kSpiritBeastLeapFrames (kSpiritBeastLeapFramesTall for
//     a large one), re-aimed at it until the apex, the float-up held until kSpiritBeastFallBlend steps of fade into the fall
//     pose end as the land clip starts. A monster more than twice that far by then is walked to instead.
//   - Falling or landing, the monster its head touches (as a pellet does) is planted one hit: the pellet's damage plus the
//     weapon's attack, in the weapon's element, through a guard, staggering it and shoving it (kSpiritBeastKickSpeed,
//     kSpiritBeastKickDecay) away from the head. Only a hit the monster takes spends the cat, which then fades over
//     kSpiritBeastFadeFrames as it goes on; one refused, or not taken within kSpiritBeastHitWait frames, lets it touch again.
// kSpiritBeastLifeFrames after it was bound it fades where it stands. Another character or weapon in hand, or a script
// event, takes it away at once. It glows from its fifth frame. Times are game frames, distances units, clip frames
// the cat's motion's.
inline constexpr int   kSpiritBeastFrames = 30;
inline constexpr int   kSpiritBeastGrowFrames = 6;
inline constexpr float kSpiritBeastGravity = 0.08f;
inline constexpr float kSpiritBeastLandLead = (219.0f - 215.0f) / 0.36f; // the land clip's frames to the paws' touch
inline constexpr float kSpiritBeastLandTouch = 219.0f, kSpiritBeastLandEnd = 227.0f;
inline constexpr float kSpiritBeastWalk = 0.7f;
inline constexpr float kSpiritBeastWalkRate = 0.85f; // the town's walk mapping for this rig at that speed
inline constexpr float kSpiritBeastSight = 300.0f;
inline constexpr int   kSpiritBeastRetarget = 30;
inline constexpr float kSpiritBeastPounce = 30.0f;
inline constexpr float kSpiritBeastPounceWinged = 50.0f;
inline constexpr float kSpiritBeastCommitHorizon = 3.0f;
inline constexpr float kSpiritBeastReadyStart = 95.0f, kSpiritBeastReadyEnd = 105.0f;     // the crouch clip
inline constexpr float kSpiritBeastFloatStart = 285.0f, kSpiritBeastFloatLaunch = 293.0f; // the float-up, its feet off
inline constexpr float kSpiritBeastFallBlend = 16.0f;
inline constexpr int   kSpiritBeastLeapFrames = 32;
inline constexpr int   kSpiritBeastLeapFramesTall = 40;
inline constexpr float kSpiritBeastTouch = 2.0f; // a pellet's reach
inline constexpr float kSpiritBeastHitRadius = 3.0f;
inline constexpr int   kSpiritBeastHitWait = 4;
inline constexpr float kSpiritBeastKickSpeed = 2.0f;
inline constexpr float kSpiritBeastKickDecay = 0.3f;
inline constexpr int   kSpiritBeastFadeFrames = 30;
inline constexpr int   kSpiritBeastLifeFrames = 1200;

// Norune's cat sound (its SE 387), at about half the game's full volume.
inline constexpr OwnSound kSpiritBeastSitSound = {
    .path = "sound/set/snd0.snd", .bank = "e01a_g.hd", .program = 44, .note = 87, .frames = 72, .volume = 60};

// The Angel Shooter's cat and the Angel Gear's (and Super Steve's with their spheres) at these sizes.
inline constexpr float kAngelShooterCatScale = 1.1f;
inline constexpr float kAngelGearCatScale = 1.2f;

bool SpiritBeastActive();

// The cat let go last: its size, and whether it wears the wings or the cape and mask.
float SpiritBeastScale();
bool  SpiritBeastWinged();
bool  SpiritBeastCaped();

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

// The cat's model, for tests.
CCharacter *SpiritBeastModel();

// Hooks: a dungeon step, after the pellets'; a pellet let go into slot; the cat's draw; its light (CCharacter::Draw);
// whether a hit is the cat's (a guard lets it through, and as a hit of Xiao's it staggers); a monster took a hit
// (CheckDmg, past its guard and invincibility); the percentage of a hit a monster's sphere takes, for one of Xiao's whose
// kick the sphere names (its body parameters 1 and 0: data/monster_scripts.hpp).
void SpiritBeastStep();
void SpiritBeastFired(CSHOT &shots, int slot);
void SpiritBeastDraw();
void SpiritBeastLight(const CCharacter &character, float *ambient);
bool SpiritBeastHit(const COLLISION_HIT &hit);
void SpiritBeastHitTaken(const COLLISION_HIT &hit);
int  SpiritBeastSpherePercent(const MONSTOR_EFFECT_STATE &effect, int sphere, int owner, const COLLISION_HIT &hit, int percent);

// The cat taken down (the floor changed).
void SpiritBeastReset();

} // namespace expanded
