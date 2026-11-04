#pragma once

#include "shot_utils.hpp"

namespace dc::game {
class CCamera;
class CCameraFollow;
class CCharacter;
class CDungeonEventData;
class CFrame;
struct COLLISION_HIT;
class CSHOT;
class CSHOT_EFFECT;
} // namespace dc::game

namespace expanded {

// Tall Hammer "Colossus": with the Tall Hammer in Goro's hand, holding a full charge grows Goro (and the hammer with
// him) evenly from his size to kColossusMost times it over kColossusGrowFrames; letting go keeps the size he reached,
// and charging again grows him on. kColossusFrames after he first grew he shrinks to kColossusLeast times his size over
// kColossusShrinkFrames, and kColossusSpentFrames after the shrink began he pops back to his size as the Cactus's copy
// rises (in a turquoise explosion kColossusPopBurstDelay frames in, kColossusPopBurst of its size, kColossusPopRise of
// his height up him, with a kColossusPopFlash flash on him and the hammer; holding small until kColossusPopFrom frames
// into the pop, past his size to kColossusPopPeak of it by kColossusPopPeakFrame and settled by kColossusPopFrame); from
// the shrink to the pop he cannot grow. At any size but his own:
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
//   - his sounds (his voice, and what his motions and his attacks play) sound kColossusPitch semitones lower for each
//     doubling of his size, as much higher for each halving;
//   - his shirt turns turquoise and he and his hammer are lit a light gold, all the more the bigger he is
//     (colossus_look.hpp);
//   - his charge attack's ground impact is his size times as large, and reaches his size times as far;
//   - his stride is his size times his own and his walk and run play at his size to the -kColossusStrideRate (a
//     little slower big, a little quicker small), so a step carries him his size to the 1 - kColossusStrideRate;
//   - the dungeon camera keeps his size times what it keeps from him (its follow point's rise above him, its distance
//     and height, and the limits the camera pass keeps them in), so he fills the screen as at his own size; a script
//     event's camera looks at what it would of him his size times as high, from his size times as far;
//   - a script event's motion (the chasm jump's leap) travels the game's own way, his body at his size.
// Xiao grown to kColossusMost leaps Osmond's gap in one bound, from the marker she stands at: her chasm's jump made
// one hop (its first, twice as far and as high, blending into the second's landing on the far side; its script
// skipping the second, its effort unvoiced), the event's camera following her and holding its view once she lands.
// Xiao shrunk below her size cannot jump her own gap: she is told, as anyone else is, that she cannot cross.
// Times are game frames, counted while the dungeon plays (not paused, no event). A change of floor puts him back.
inline constexpr float kColossusMost = 2.0f, kColossusLeast = 0.5f, kColossusSwitchSize = 1.5f;
inline constexpr int   kColossusGrowFrames = 300; // from his size to kColossusMost
inline constexpr int   kColossusFrames = 1800, kColossusShrinkFrames = 120, kColossusSpentFrames = 720;
inline constexpr float kColossusPopFrom = 9.0f, kColossusPopPeakFrame = 16.0f, kColossusPopFrame = 18.0f; // since it began
inline constexpr float kColossusPopPeak = 3.4f / 3.0f;
inline constexpr float kColossusPopBurst = 0.3f;   // the turquoise explosion's size, of its own
inline constexpr float kColossusPopOpacity = 0.5f; // of its own
inline constexpr int   kColossusPopBurstDelay = 2; // frames into the pop
inline constexpr float kColossusPopRise = 0.25f;   // where it bursts, of his height over his feet
// His flash in the burst, his and the hammer's, on the game's character tint (ambient_offset, 0-255), as the burst
// lights him: up as it bursts out over him, all of it once it covers him, fading as it fades.
inline constexpr float kColossusPopFlash[3] = {100.0f, 200.0f, 200.0f};
inline constexpr int   kColossusDefense = 15;
inline constexpr int   kColossusTrampleFrames = 30;
inline constexpr float kColossusTrampleShare = 0.25f;
inline constexpr float kColossusStrideRate = 0.25f;
inline constexpr float kColossusPitch = 2.0f;      // semitones per doubling of his size
inline constexpr float kColossusBodyRadius = 6.0f; // the player's body, as monsters stop at it (MoveCheck2)

// Goro's size now (1: his own).
float ColossusScale();

// The size the camera keeps to (his model's: 1 with another member out), and the camera at it: its distance and height
// rescaled as his size changes, and its follow point's offset from him (at pos, his feet) his size times the game's.
// Hook: the dungeon camera follows him (movechara), before the camera pass (autoCamTrial) keeps it in its limits.
float ColossusCameraScale();
void  ColossusCamera(CCameraFollow &camera, const float *pos);

// How many semitones his sounds are shifted by at his size (0 with another member out, or at his own); his voice
// shifted every dungeon step (ColossusStep) and not as the floor is left (ColossusQuiet, DungeonMonstersCleared).
float ColossusSoundPitch();
void  ColossusQuiet();

// His charge attack's ground impact (his effect pack's slot the smash set, GoroKey_Play) at his size: its model, and the
// hits it plants reaching his size times as far (ShotEffectHitSet).
void ColossusImpact(CSHOT_EFFECT &effect, int slot);

// Xiao's pellet at her size: its damage and its size her size times her own (PelletFired).
void ColossusPelletFired(CSHOT &shots, int slot);

// How far the stick moves him (movechara, times its velocity), and his walk's play rate at his size (a motion step,
// PlayerMotionStepping).
float ColossusMoveScale();
void  ColossusMotion(CCharacter &chara, int chara_no);

// A dungeon script event's motion at the member's size (motionDrive, once it has stepped): its root's travel (the
// chasm jump's leap, moved by its root bone, not her position) at the game's own length, her body at her size.
void ColossusMotionStepped(CCharacter &chara);
// A scene camera's key during a dungeon script event at the member's size (the motion's step, its value local to
// frame): what it looks at her size times as high, and the camera her size times as far from it, the same shot of
// her at her size; false (the game sets the key) at her own size or out of an event.
bool ColossusSceneCamera(CCamera &camera, CFrame &frame, const float *value, bool target);

// Osmond's gap (his part's two markers, each running the script that tells anyone else they cannot cross): Xiao at
// kColossusMost leaps it in one bound. The script an event at the marker she stands at runs (event null for none):
// her chasm's jump, from that marker towards the other, where she is at kColossusMost; script_no otherwise.
int ColossusGapScript(int script_no, const CDungeonEventData *event);
// Who the event's script is told is in play (_GET_NOW_USER_ID), chara the one who is: at her own gap's marker, Xiao
// shrunk below her size is told as another member, its script telling her she cannot cross; and the line an event
// shows (_MES_MAKE), message_no its own: there, the line it would show that member swapped for her own.
int ColossusUserId(int chara);
int ColossusMessageNo(int message_no);
// The frame the jump's script looks up by name for where it leaps from (all the floor's frames), while the gap's
// leap is set; null otherwise.
CFrame *ColossusGapMarker(const char *name);
// The event loop, before it resumes the event's script: the frames its script is to go on by too (the gap's leap
// skipping its second hop; 0 for none); then, once they have run, done. Its sounds are unplayed while they run.
int  ColossusEventSkip();
void ColossusEventSkipped();
bool ColossusEventMuted();

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
