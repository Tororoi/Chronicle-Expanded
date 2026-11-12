#pragma once

#include <cstdint>

#include "expanded/dungeon/own_sounds.hpp"

namespace dc::game {
enum class MonsterDamageResult : int;
class CCharacter;
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Big Bucks Hammer's Midas Touch: Goro's hit with Big Bucks Hammer in hand (and Xiao's with its SynthSphere on Super
// Steve, the same in every way) turns a monster (not a boss) to gold when it
// kills it, or when it lands while the monster has less than kMidasLowLife in 100 of its life left (the hit made killing).
// Its kill and ABS are as any other's, and its drop, but for a kMidasBarChance in 100 chance of a gold bar in its place
// (never in place of an item the floor gave it beforehand, as a key). It turns with kMidasTurnSound and a gold flash
// (kMidasFlash added to its light, fading out over kMidasFlashFrames); it stands where it was struck, held in the pose it was in, its fade
// held, drawn as polished gold, reflecting its floor tile's surroundings (ReflectionCubeAt) scaled by kMidasGoldTint with
// kMidasGoldLift added (the gold of data/gold_statue.hpp while the tile is not captured), and lit kMidasShine brighter,
// and stays standing, as solid as the monster was, until the floor ends. What drops inside it slides kMidasPull a step
// toward Goro while he is against it (within kMidasTouch of where it stops him), out to where he picks it up.
inline constexpr int      kMidasLowLife = 25;
inline constexpr int      kMidasBarChance = 10;
inline constexpr int      kMidasFlashFrames = 15;                 // a quarter second
inline constexpr float    kMidasFlash[3] = {90.0f, 75.0f, 35.0f}; // the turning flash, added to its light at its start
inline constexpr float    kMidasPull = 2.0f;
inline constexpr float    kMidasTouch = 2.0f;
inline constexpr float    kMidasShine[3] = {90.0f, 70.0f, 20.0f}; // added to its light: polished, not dull
inline constexpr uint32_t kMidasGoldTint = 0x40'40'40'40u;        // the reflection as it is (RGBA8, 0x40 as 1.0)
// Gold's clink (a short bright metal ring from the Moon Sea's bank, o05a_g.hd program 53 note 36): a hit meeting gold Goro's
// guard, and the player's hit on a statue, which is not heard again from that statue for kMidasClinkFrames. A melee
// swing's (Toan's, Goro's, Ungaga's) also throws the player back from it: kMidasRebound, falling by kMidasReboundDecay
// a step.
inline constexpr OwnSound kMidasClink = {.path = "sound/set/snd309.snd", .bank = "o05a_g.hd", .program = 53, .note = 36, .frames = 36};
// Turning to gold: a short bright metal ring (the Sun & Moon Temple's bank, s16a_g.hd program 47 note 91).
inline constexpr OwnSound kMidasTurnSound = {.path = "sound/set/snd26.snd", .bank = "s16a_g.hd", .program = 47, .note = 91, .frames = 12};
inline constexpr float    kMidasRebound = 2.0f;
inline constexpr float    kMidasReboundDecay = 0.3f;
inline constexpr int      kMidasClinkFrames = 20;
inline constexpr uint32_t kMidasGoldLift = 0x00'0A'1A'1Fu; // and a gold body under it (RGBA8): 0.12, 0.1, 0.04

// Whether Goro is in play with Big Bucks Hammer in hand, or Xiao with its SynthSphere on Super Steve, under its curse
// (midas_curse.hpp).
bool MidasTouchWielded();

// Draws what follows as polished gold standing at at (its floor tile's reflection, else the gold ball), until
// MidasUnpolish.
void MidasPolish(const float *at);
// The gold ball alone, for a model shown away from the floor (the item menu's).
void MidasPolishBall();
void MidasUnpolish();

// Hook: a chest's item about to be drawn at at: the Gold Bullion polished and lit kMidasShine brighter as the statues
// are, true when it was, for MidasUnpolish (and the light put back) after.
bool GoldBullionDrawing(int item_no, const float *at);

// Whether monster of unit is a gold statue.
bool MidasStatue(const CMonstorUnit &unit, int monster);

// Hook: the player's step after the monsters blocked it (PlayerMoveChecked), before the walls: thrown back off a statue.
void MidasPlayerMove(float *velocity);

// Whether it is a statue the game is done with, still stopping the player (CMonstorUnit::MoveCheck).
bool MidasSolid(const CMonstorUnit &unit, int monster);

// A drop's step (CRandomItem::CheckPosition, before the player's reach is measured): drawn toward the player while it
// lies inside a statue he is against.
void MidasPullDrop(float *drop);

// A statue's step of one of its characters (FasterMotionStep): held in the motion it was struck in, as the freeze
// status's stop timer holds one. False (nothing done) for any other monster.
bool MidasHold(CMonstorUnit &unit, int monster, CCharacter &character);

// A hit meeting gold Goro's guard sounds (BtCheckDamageProc): the clink; false (the game's own) when he is not gold.
bool MidasGuardSound();

// A monster's drop as it dies (MonsterDropItem): a statue's, at kMidasBarChance in 100, a gold bar.
int MidasDropItem(const CMonstorUnit &unit, int monster, int drop_item);

// Something just turned to gold: its sound, and its flash's frames to come (the light added, MidasFlashLight).
void MidasTurnEffect(int &flash);
// The flash's light added to ambient, frames of it left.
void MidasFlashLight(int frames, float *ambient);

// Hooks: a hit monster of unit takes (MonsterHitTaken), turned killing when it turns it to gold; a statue's
// motion held (FasterMotionStep, MidasHold); a monster drawing and drawn (MonsterDrawing, MonsterDrawn: in gold);
// a dungeon step (the statues watched, their gold bars rolled); the statues the game no longer draws, drawn
// (DungeonEffectsDrawn).
void MidasHitTaken(CMonstorUnit &unit, int monster, COLLISION_HIT &hit);
// The hit's damage dealt (MonsterDamaged): its own damage back for any other monster it reaches, and one it could not
// kill left alive, not a statue.
void MidasDamaged(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit);
void MidasDrawing(CMonstorUnit &unit, int monster);
void MidasDrawn(CMonstorUnit &unit, int monster);
void MidasStep();
void MidasDraw();
// The damage check's result for the monster it checked (CheckDmg): a statue's kill finished at once, as its death
// script would end it (_STATUS_SET_DEAD: its slot let go, one off the live count, its rewards with it), and handed on as
// no result, so neither that script nor a revive (enemies/revive.hpp) moves it.
MonsterDamageResult MidasCheckDmg(CMonstorUnit &unit, MonsterDamageResult result);

// The light a statue draws with, its shine added (CharacterLight).
void MidasLight(const CCharacter &character, float *ambient);

void MidasReset();

} // namespace expanded
