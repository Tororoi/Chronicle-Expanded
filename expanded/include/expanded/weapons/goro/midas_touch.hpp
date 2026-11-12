#pragma once

namespace dc::game {
enum class MonsterDamageResult : int;
class CCharacter;
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Big Bucks Hammer's Midas Touch: each of Goro's hits that lands with Big Bucks Hammer in hand has a kMidasChance in
// 100 chance to turn the monster (not a boss) to gold. The hit kills it, through the game's own damage, so its kill,
// ABS and drops are as any other's; but it stands where it was struck, frozen in the pose it was in, its fade held,
// drawn with the gold of data/gold_statue.hpp in place of its own pictures and lit kMidasShine brighter, and stays
// standing once the game is done with it, until the floor ends. The first time Goro comes within kMidasNear of a statue
// it has a kMidasBarChance in 100 chance to drop a gold bar there, as a monster drops an item.
inline constexpr int   kMidasChance = 5;
inline constexpr int   kMidasBarChance = 10;
inline constexpr float kMidasNear = 15.0f;
inline constexpr float kMidasShine[3] = {90.0f, 70.0f, 20.0f}; // added to its light: polished, not dull

// Whether Goro is in play with Big Bucks Hammer in hand.
bool MidasTouchWielded();

// Whether monster of unit is a gold statue.
bool MidasStatue(const CMonstorUnit &unit, int monster);

// Hooks: a hit monster of unit takes (MonsterHitTaken), turned killing when it turns it to gold; whether a statue's
// motion steps (FasterMotionStep: it does not); a monster drawing and drawn (MonsterDrawing, MonsterDrawn: in gold);
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
