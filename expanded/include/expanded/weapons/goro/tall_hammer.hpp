#pragma once

#include "shot_utils.hpp"

namespace dc::game {
class CCameraFollow;
class CCharacter;
struct COLLISION_HIT;
class CSHOT_EFFECT;
} // namespace dc::game

namespace expanded {

// Tall Hammer "Colossus": with the Tall Hammer in Goro's hand, holding a full charge grows Goro (and the hammer with
// him) evenly from his size to kColossusMost times it over kColossusGrowFrames; letting go keeps the size he reached,
// and charging again grows him on. kColossusFrames after he first grew he shrinks to kColossusLeast times his size over
// kColossusShrinkFrames, and kColossusSpentFrames after the shrink began he pops back to his size as the Cactus's copy
// rises (out of the summon smoke, turquoise, holding small until the smoke's frame kColossusPopFrom, past his size to
// kColossusPopPeak of it by kColossusPopPeakFrame and settled by kColossusPopFrame); from the shrink to the pop he
// cannot grow. At any size but his own:
//   - his attack is his size times his weapon's;
//   - his defense rises by up to kColossusDefense at kColossusMost and falls by as much at kColossusLeast;
//   - his hammer's hits reach his size times as far, and throw monsters his size times as far (their first step's speed
//     times its square root: a throw carries speed^2 over twice its decay);
//   - the height monsters' hits and shots find him at is his size times his own, his body (where monsters stop) his
//     size times as wide, and a hit reaches him (his size - 1) times that width further out;
//   - at kColossusMost, a monster he walks into takes kColossusTrampleShare of a hammer hit (his attack at his size),
//     wearing no weapon HP, once every kColossusTrampleFrames;
//   - at kColossusMost, monsters' hits land a tier lighter: a guard-breaking hit as a knockback, a knockback as a light
//     hit, and a light hit blocked as his guard would block it, without taking him out of what he was doing;
//   - at kColossusSwitchSize or more, stepping onto a hammer switch sets it off;
//   - his cape drapes at his size;
//   - his stride is his size times his own and his walk and run play at his size to the -kColossusStrideRate (a
//     little slower big, a little quicker small), so a step carries him his size to the 1 - kColossusStrideRate;
//   - the dungeon camera keeps his size times what it keeps from him (its follow point's rise above him, its distance
//     and height, and the limits the camera pass keeps them in), so he fills the screen as at his own size.
// Times are game frames, counted while the dungeon plays (not paused, no event). A change of floor puts him back.
inline constexpr float kColossusMost = 2.0f, kColossusLeast = 0.5f, kColossusSwitchSize = 1.5f;
inline constexpr int   kColossusGrowFrames = 300; // from his size to kColossusMost
inline constexpr int   kColossusFrames = 1800, kColossusShrinkFrames = 120, kColossusSpentFrames = 720;
inline constexpr float kColossusPopFrom = 6.5f, kColossusPopPeakFrame = 10.6f, kColossusPopFrame = 12.0f; // the smoke's
inline constexpr float kColossusPopPeak = 3.4f / 3.0f;
inline constexpr float kColossusPopSmokeScale = 0.5f;
inline constexpr float kColossusSmokeTint[3] = {0.3f, 1.6f, 1.4f}; // the smoke's pink to turquoise
inline constexpr int   kColossusDefense = 15;
inline constexpr int   kColossusTrampleFrames = 30;
inline constexpr float kColossusTrampleShare = 0.25f;
inline constexpr float kColossusStrideRate = 0.25f;
inline constexpr float kColossusBodyRadius = 6.0f; // the player's body, as monsters stop at it (MoveCheck2)

// Goro's size now (1: his own).
float ColossusScale();

// The size the camera keeps to (his model's: 1 with another member out), and the camera at it: its distance and height
// rescaled as his size changes, and its follow point's offset from him (at pos, his feet) his size times the game's.
// Hook: the dungeon camera follows him (movechara), before the camera pass (autoCamTrial) keeps it in its limits.
float ColossusCameraScale();
void  ColossusCamera(CCameraFollow &camera, const float *pos);

// How far the stick moves him (movechara, times its velocity), and his walk's play rate at his size (a motion step,
// PlayerMotionStepping).
float ColossusMoveScale();
void  ColossusMotion(CCharacter &chara, int chara_no);

// What his size changes, each the game's own as the argument (Goro in play; anyone else's as it is):
int   ColossusAttack(int attack);
int   ColossusDefense(int chara, int defense);
float ColossusHitRadius(float radius);
float ColossusKickSpeed(float speed);
float ColossusHeight(float height);
float ColossusHurtReach();
float ColossusBlockRadius(float radius);
void  ColossusShotPoint(float *point);
// A monster's hit on him: whether it is blocked, and the kind (COLLISION_HIT::kind) it lands as, kind its own. A hit he
// shrugs off leaves him in what he was doing: ColossusShrugsOff, asked before the game takes him out of his action,
// keeps it, and ColossusShrugged, once the game has reacted (blocked: the guard's push-back, sound and spark), puts it
// back.
bool ColossusShrugsOff(int kind);
void ColossusShrugged(bool blocked);
int  ColossusHitKind(int kind);
// At his full size, each attack hits him once: a monster's sphere that hit him passes him by until its hit window
// closes or its motion starts over (its next swing hits), the monster's other spheres and other monsters' hitting as
// ever, and a shot hits nothing more and, still in flight, meets him as a wall on its next step (its impact, or its
// burst).
// Hooks: whether a hit passes him by (BtCheckDamageProc), a hit reached him (after the game reacts to it), a shot set a
// hit record (ShotEffectHitSet) and a shot's collision test (ShotEffectCollision).
bool                  ColossusPassesBy(const COLLISION_HIT &hit);
void                  ColossusHitTaken(const COLLISION_HIT &hit);
void                  ColossusShotHitSet(CSHOT_EFFECT &effect, int slot, const COLLISION_HIT &hit);
SHOT_COLLISION_RESULT ColossusShotCollision(const CSHOT_EFFECT &effect, int slot, SHOT_COLLISION_RESULT result);

// Whether a hit is one of his walk-in hits (it wears no weapon HP).
bool ColossusTrample(const COLLISION_HIT &hit);

// Whether he stands big enough to set off a hammer switch, and on the one for chara_no at pos, radius its own (within
// its radius across, its height within his).
bool ColossusTriggersSwitches();
bool ColossusStepsOn(int chara_no, const float *pos, float radius);

// Hooks: a dungeon step (DungeonMonstersTick); the floor changed (tests too).
void ColossusStep();
void ColossusReset();

} // namespace expanded
