#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Serpent Sword, with it in hand or Super Steve carrying its SynthSphere.
//
// "Snake Slayer": the weapon's hit kills Auntie Medu (Enhanced too) outright, whatever its damage.
//
// "Snake Charmer": while the guard is up, the Killer Snake (the Wise Owl Forest event's giant snake: kKillerSnakeFile's
// model, set up from kKillerSnakeCfg) stands kSnakeSide to the player's side at kSnakeScale, facing as they face and
// swaying (its motion kSnakeSway). A monster in play within kSnakeRetreatRadius of the player has the snake rear and lean
// at it (kSnakeRear, played through once, turned to it; again while one is still that near) and backs away until it is
// past kSnakeCalmRadius: every step it takes toward the player is turned straight away, and its script finds the player
// far behind it. The snake goes with the guard.
inline constexpr const char *kKillerSnakeFile = "gedit/s97/chara/e115.chr";
inline constexpr const char *kKillerSnakeCfg = "e115e54a.cfg";
inline constexpr int         kSnakeSway = 3;
inline constexpr int         kSnakeRear = 2;
inline constexpr float       kSnakeScale = 1.0f;
inline constexpr float       kSnakeSide = 20.0f;
inline constexpr float       kSnakeRetreatRadius = 80.0f;
inline constexpr float       kSnakeCalmRadius = 120.0f;

bool SerpentSwordActive();

// Hook: CheckDmg, the HP a hit takes off the monster: all of it for Snake Slayer.
int SnakeSlayerAmount(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);

// Hook: each dungeon step: the snake up while the guard is, its motion chosen, placed and stepped.
void SnakeCharmerStep();
// Hook: the monsters drawn: the snake.
void SnakeCharmerDraw();
// Whether the snake is up.
bool SnakeCharmerUp();
// Hook: where the monster's script finds the player: true, with a point well behind the monster, while it backs away.
bool SnakeCharmerTarget(int monster, float *position);
// Hook: a monster's step after the other monsters blocked it: a step toward the player turned straight away while it
// backs away.
void SnakeCharmerMonsterMove(CMonstorUnit &unit, int monster);
// Hook: the floor ending: the snake forgotten (its model goes with the floor).
void SnakeCharmerReset();

} // namespace expanded
