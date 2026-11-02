#pragma once

namespace dc::game {
class CCharacter;
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
// kSnakeLift above its middle and facing as the player faces. It rises with its appearance (kSnakeAppear) and holds the
// appearance's last pose; from there it idles on its own: its mouth opens by kSnakeMouthAngle every kSnakeMouthPeriod
// steps (and when a monster comes near), easing open over kSnakeMouthEase, held kSnakeMouthHold and easing shut, while
// the head stretches forward and back by kSnakeStretch every kSnakeStretchPeriod steps and bobs up and down by kSnakeBob
// every kSnakeBobPeriod. A monster in play within kSnakeRetreatRadius of the player backs away until it is past
// kSnakeCalmRadius: every step it takes toward the player is turned straight away, and its script finds the player far
// behind it. The head fades in over kSnakeFadeSteps as the guard goes up and out over as long as it comes down, the
// head and the weapon glowing white (kSnakeGlow added to their light) as it shows.
//
// The appearance ends with the head turned as the event's body held it; the model takes the turn back to the head's
// rest (the pose its skin was made in) so that it ends upright and facing ahead. Its jaw (kSnakeJaw) is shut at its
// rest turn, and the appearance leaves it part open: the idle turns it about the same axis, from shut. kSnakeBack is
// how far ahead of its neck the head's middle lies.
inline constexpr const char *kKillerSnakeFile = "gedit/s97/chara/e117_0.chr";
inline constexpr const char *kKillerSnakeCfg = "e117e54a.cfg";
inline constexpr const char *kSnakeJaw = "null59";
inline constexpr int         kSnakeAppear = 8;
inline constexpr int         kSnakeFadeSteps = 15;
inline constexpr float       kSnakeGlow = 32.0f;
inline constexpr int         kSnakeMouthPeriod = 180, kSnakeMouthEase = 20, kSnakeMouthHold = 20;
inline constexpr float       kSnakeMouthAngle = 1.0f; // radians
inline constexpr float       kSnakeStretch = 1.2f;
inline constexpr int         kSnakeStretchPeriod = 200;
inline constexpr float       kSnakeBob = 0.6f;
inline constexpr int         kSnakeBobPeriod = 120;
inline constexpr float       kSnakeScale = 0.5f;
inline constexpr float       kSnakeLift = 8.0f;
inline constexpr float       kSnakeBack = 11.0f;
inline constexpr float       kSnakeRetreatRadius = 80.0f;
inline constexpr float       kSnakeCalmRadius = 120.0f;

bool SerpentSwordActive();

// Hook: CheckDmg, the HP a hit takes off the monster: all of it for Snake Slayer.
int SnakeSlayerAmount(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);

// Hook: each dungeon step: the snake up while the guard is, faded in or out, placed and stepped.
void SnakeCharmerStep();
// Hook: a model posed by its motion (MotionPlay, before its skin is built): the snake's jaw set after its appearance.
void SnakeCharmerPosed(CFrame *frame);
// Hook: the monsters drawn: the snake, as faded in as it is.
void SnakeCharmerDraw();
// Hook: a character's light for its draw: the snake's and the weapon's white as the snake shows.
void SnakeCharmerLight(const CCharacter &character, float *ambient);
// Whether the snake is up (the guard is), how far it has faded in (0..1), and how far its mouth is open (0..1).
bool  SnakeCharmerUp();
float SnakeCharmerShown();
float SnakeCharmerMouth();
// Hook: where the monster's script finds the player: true, with a point well behind the monster, while it backs away.
bool SnakeCharmerTarget(int monster, float *position);
// Hook: a monster's step after the other monsters blocked it: a step toward the player turned straight away while it
// backs away.
void SnakeCharmerMonsterMove(CMonstorUnit &unit, int monster);
// Hook: the floor ending: the snake forgotten (its model goes with the floor).
void SnakeCharmerReset();

} // namespace expanded
