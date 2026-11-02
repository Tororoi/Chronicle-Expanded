#pragma once

namespace dc::game {
class CFrame;
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Serpent Sword, with it in hand or Super Steve carrying its SynthSphere.
//
// "Snake Slayer": the weapon's hit kills Auntie Medu (Enhanced too) outright, whatever its damage.
//
// "Snake Charmer": the snake's spirit in the sword. While the guard is up, the Killer Snake's head (the Wise Owl Forest
// event's giant snake: kKillerSnakeFile's head, set up from kKillerSnakeCfg) hovers over the blade at kSnakeScale,
// kSnakeLift above its middle and facing as the player faces. It comes with its appearance (kSnakeAppear, played
// through), then idles (kSnakeIdle), hissing (kSnakeHiss, its mouth opening, played through) every kSnakeHissPeriod
// steps and whenever a monster comes near. A monster in play within kSnakeRetreatRadius of the player backs away until
// it is past kSnakeCalmRadius: every step it takes toward the player is turned straight away, and its script finds the
// player far behind it. The head goes with the guard.
//
// The appearance ends with the head turned as the event's body held it; the model takes the turn back to the head's
// rest (the pose its skin was made in) so that it ends upright and facing ahead, and after it the neck is held where
// the appearance left it, so the idle and the hiss only turn the head and open the mouth. kSnakeBack is how far ahead
// of its neck the head's middle lies.
inline constexpr const char *kKillerSnakeFile = "gedit/s97/chara/e117_0.chr";
inline constexpr const char *kKillerSnakeCfg = "e117e54a.cfg";
inline constexpr int         kSnakeAppear = 8;
inline constexpr int         kSnakeIdle = 7;
inline constexpr int         kSnakeHiss = 5;
inline constexpr int         kSnakeHissPeriod = 120;
inline constexpr float       kSnakeScale = 0.5f;
inline constexpr float       kSnakeLift = 8.0f;
inline constexpr float       kSnakeBack = 11.0f;
inline constexpr float       kSnakeRetreatRadius = 80.0f;
inline constexpr float       kSnakeCalmRadius = 120.0f;

bool SerpentSwordActive();

// Hook: CheckDmg, the HP a hit takes off the monster: all of it for Snake Slayer.
int SnakeSlayerAmount(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);

// Hook: each dungeon step: the snake up while the guard is, its motion chosen, placed and stepped.
void SnakeCharmerStep();
// Hook: a model posed by its motion (MotionPlay, before its skin is built): the snake's neck held after its appearance.
void SnakeCharmerPosed(CFrame *frame);
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
