#pragma once

#include "expanded/weapons/kick_back.hpp"

namespace dc::game {
class CCharacter;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Thorn Armlet "Thorn Guard": with the Thorn Armlet in hand, or Super Steve carrying its SynthSphere, the guard raises
// thorns around the wielder. Ruby's (the armlet in hand) are a giant copy of the armlet: the armlet's own model (its
// ring's axis, the model's x, stood upright and pointing down), kThornScale times its size, its middle kThornLift above
// the wielder's waist (halfway between the kThornWaistJoints as posed, so it goes where she goes; kThornHeightShare of
// her height above her feet for a skeleton without them), fading in over kThornFadeFrames while the guard is on (its
// action, from the raise) and out once it drops, and turning about her once every kThornTurnFrames, tinted
// kThornArmletTint, with a gentle green glow on its middle.
//
// Super Steve's are kPricklyCount Pricklys (the fishing bait's model, data/prop_models.hpp), kPricklyScale times their
// size, evenly round the wielder kPricklyRadius out from her middle (kThornLift above kThornHeightShare of her height),
// each facing out, turning together as the armlet turns and fading as it fades, tinted kPricklyTint, with a gentle
// purple-to-teal glow (glow_sprite.hpp's kPrickly) on her middle.
//
// While the guard is on (raising it included: the game counts the raise as a guard for a blow too), a monster's melee
// hit (one its attack plants itself: no shot's) that finds the wielder passes her by, guard-breaking blows too, and
// turns on the monster: a hit of the wielder's on its body (its largest sphere, at least kThornHitRadius wide) at the
// damage its own hit carried (before her defense), with the armlet's flags, thrown away from the wielder as a Baselard
// swing throws (kThornKick), the monster flinching as from any hit of hers. It wears no weapon HP. An attack's hit is
// planted afresh every frame its window is open: a monster's hits pass her by while they keep coming, and one more
// turns on it only once kThornRearmFrames have gone by with none (its next swing).
//
// The action gauge is the armlet's health, as the Angel Gear's shield's: it stands only on a full gauge, and while it
// stands the gauge holds (no refill), each hit it turns taking a kThornHits'th; the last breaks it, with the
// weapon-break sound, and it clears over kThornBreakFrames. Broken, it does not stand again until the gauge has
// refilled, and the gauge refills only once the guard is let go (at her own rate). Lowered unbroken, the gauge refills
// from what is left, and the armlet stands again only on a full one. Without the armlet standing, her guard is the
// game's.
// Times are game frames.
inline constexpr int         kThornHits = 3;
inline constexpr int         kThornBreakFrames = 9;
inline constexpr float       kThornFullGauge = 99.5f;
inline constexpr float       kThornScale = 7.0f;
inline constexpr float       kThornLift = 1.5f;
inline constexpr float       kThornHeightShare = 0.5f;
inline constexpr const char *kThornWaistJoints[2] = {"hip", "spine"};
// The ring's middle along its axis, in the model's x (it runs -0.43 to 0.26).
inline constexpr float kArmletAxisMiddle = -0.085f;
inline constexpr int   kPricklyCount = 5;
inline constexpr float kPricklyScale = 1.5f;
inline constexpr float kPricklyRadius = 4.25f;
// The Prickly's middle, in its model's y (it sits on its base: -0.35 to 1.37).
inline constexpr float kPricklyMiddle = 0.51f;
// The copies' tints, added to the light they are drawn in (red, green, blue).
inline constexpr float kThornArmletTint[3] = {10.0f, 18.0f, 10.0f};
// The thorns' glow: a gentle disc (glow_sprite.hpp's kThorn for the armlet, kPrickly for the Pricklys) kThornGlowSize
// across on their middle, growing from nothing over kThornGlowFrames as they rise and shrinking back over as many once
// they fade or break, tested against depth from kThornGlowPull nearer.
inline constexpr float    kThornGlowSize = 16.0f;
inline constexpr float    kThornGlowPull = 6.0f;
inline constexpr int      kThornGlowFrames = 15;
inline constexpr float    kPricklyTint[3] = {15.0f, 15.0f, 15.0f};
inline constexpr int      kThornFadeFrames = 8;
inline constexpr int      kThornTurnFrames = 240;
inline constexpr float    kThornHitRadius = 6.0f;
inline constexpr KickBack kThornKick = kHeavyKickBack;
inline constexpr int      kThornRearmFrames = 4;

// Whether the wielder carries the thorns (the armlet in hand, or its sphere on Super Steve).
bool ThornGuardActive();

// Whether the armlet stands around the wielder, how solid it is (0 to 1), its glow's size (0 to 1) and its turn
// (radians, falling as it spins); whether it stands to
// turn hits (raised, not breaking), the hits it has left, and whether it is broken and waiting on a full gauge.
bool  ThornArmletStanding();
int   ThornArmletHits();
bool  ThornArmletCooling();
bool  ThornArmletUp();
float ThornArmletAlpha();
float ThornArmletGlow();
float ThornArmletTurn();

// Hook: a monster's hit found the player (PlayerHitPassesBy): true when the thorns take it (she is untouched).
bool ThornGuardHit(const COLLISION_HIT &hit);

// Hook: motionDrive's refill of the action gauge (ActionGaugeGain): none while the armlet stands, none while it is broken
// and the guard still held.
float ThornGuardGaugeGain(float gain);

// Whether a hit is the thorns' (it wears no weapon HP).
bool ThornHit(const COLLISION_HIT &hit);

// Hooks: a step (once a frame the monsters step unpaused); the draw, with the monsters'.
void ThornArmletStep();
void ThornArmletDraw();
// Hook: the effects' draw (glows draw after the monsters): the thorns' glow.
void ThornArmletGlowDraw();

// The copy, for tests (null before it is loaded).
CCharacter *ThornArmletCharacter();

// Taken down (the floor changed), for tests too.
void ThornArmletReset();

} // namespace expanded
