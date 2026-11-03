#pragma once

#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/ungaga/mirage/heat_haze.hpp"

namespace dc::game {
class CCharacter;
class CFrame;
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Serpent Sword, with it in hand or Super Steve carrying its SynthSphere, once the Killer Snake is beaten (game flag
// kKillerSnakeBeaten, "hebikiri": until then the sword is the event's, taking no wear and gaining no ABS).
//
// "Snake Slayer": the weapon's hit kills Auntie Medu (Enhanced too) outright, whatever its damage.
//
// "Snake Charmer": the snake's spirit in the sword. While the guard is up, the Killer Snake's head (the Wise Owl Forest
// event's giant snake: kKillerSnakeFile's head, set up from kKillerSnakeCfg) hovers over the blade at kSnakeScale,
// kSnakeLift above where the guard holds its middle (kSnakeAhead ahead of the player and kSnakeHeight up, on the
// player, so the head stays still as the guard's pose comes and goes) and facing as the player faces. It rises with its
// appearance (kSnakeAppear) and holds the appearance's last pose; from there it idles on its own: its mouth opens by
// kSnakeMouthAngle every kSnakeMouthPeriod steps (and when a monster comes near), easing open over kSnakeMouthEase,
// held kSnakeMouthHold and easing shut, while the head stretches forward and back by kSnakeStretch every
// kSnakeStretchPeriod steps and bobs up and down by kSnakeBob every kSnakeBobPeriod. A monster in play near the player
// keeps off as its own AI does, values in its script raised for its own step only (the program is the kind's, shared,
// so they go back after it). Most scripts have a retreat (a routine of its own that hops it back on its back-off
// motion: SnakeRetreatOf) that their AI loop (label 100) rolls for when the player is within some distance, its
// cooldown between: within kSnakeRetreatRadius that distance is the reach, the roll its own. The routine's own limits
// are raised: where it stops once the player is so far it goes on to kSnakeRetreatTo, and where it stops after so far
// from where it started, or so many hops, it goes on (to kSnakeRetreatHops hops) while the monster is nearer than
// kSnakeRetreatTo and stops at its next check once it is that far. Within kSnakeApproachRadius (so that a walk-in begun
// out there is kept too) its approach comes only to kSnakeApproachTo (what the loop calls only while the player is more
// than so far is called only beyond it, and a walk-in to within so far walks in to it), and it holds its shots (what
// the loop calls while the player is within, or beyond, so far is never called when it fires a shot). So it comes in to
// about kSnakeApproachTo, rolls its retreat now and then, backs off to the edge and comes back; everything else is its
// script's. An ordinary monster whose AI has no retreat to trigger is taken from its script instead while it is near:
// once between actions it staggers (its damage clip, as a blinding staggers it: BlindClipsOf, cross-faded in) and then
// stands idle (kSnakeIdle) where it is, staggering again if struck, until the guard comes down. A mimic retreats once
// it is out of its chest and done with its wake (its appear clip, kMimicWake, a king mimic's kKingMimicWake). A monster
// near the player that neither retreats nor is held (a boss, a mimic still waking) keeps to its script but takes no
// step nearer the player. An Expanded species (the Crystal, Atla and Bomb Gemrons) runs its stand-in's script, and so
// its retreat. The head fades in over kSnakeFadeSteps as the guard goes up and out over as long as it comes down, the
// head and the weapon glowing white (kSnakeGlow added to their light) as it shows. It is a ghost: drawn kSnakeGhost
// see-through (its nearest surface only, never its inside), shimmering as the Mirage's copy does, less (kSnakeHaze at
// kSnakeHazeGain, its wave at kSnakeHazeWave, from under its face, kSnakeFace, down over the blade), a
// periwinkle-to-fuchsia glow disc kSnakeGlowDisc across on its face (depth-tested from kSnakeGlowPull nearer) growing
// in as it comes and shrinking away as it goes, over kSnakeGlowSteps (slower than its fade: the disc stays where the
// face last was).
//
// It hisses as it comes (kSnakeHiss: the Killer Snake's own hiss from its event's bank, SE 393 there, a little quieter
// than the game plays it).
//
// The appearance ends with the head turned as the event's body held it; the model takes the turn back to the head's
// rest (the pose its skin was made in) so that it ends upright and facing ahead. Its jaw (kSnakeJaw) is shut at its
// rest turn, and the appearance leaves it part open: the idle turns it about the same axis, from shut. kSnakeBack is
// how far ahead of its neck the head's middle lies.
inline constexpr int              kKillerSnakeBeaten = 0x30;
inline constexpr OwnSound         kSnakeHiss = {"sound/set/snd401.snd", "d02c_g.hd", 44, 93, 150, 1, nullptr, 0, 100};
inline constexpr const char      *kKillerSnakeFile = "gedit/s97/chara/e117_0.chr";
inline constexpr const char      *kKillerSnakeCfg = "e117e54a.cfg";
inline constexpr const char      *kSnakeJaw = "null59";
inline constexpr int              kSnakeAppear = 8;
inline constexpr int              kSnakeFadeSteps = 15;
inline constexpr float            kSnakeGlow = 80.0f;
inline constexpr float            kSnakeGhost = 0.45f;
inline constexpr const char      *kSnakeFace = "face";
inline constexpr float            kSnakeHazeGain = 1.0f;
inline constexpr float            kSnakeGlowDisc = 21.0f, kSnakeGlowPull = 8.0f;
inline constexpr int              kSnakeGlowSteps = 30;
inline constexpr mirage::HazeLook kSnakeHaze = {-17.0f, 10.0f, 16.0f, 2.0f};
inline constexpr float            kSnakeHazeWave = mirage::kHazeWaveStep * 0.5f;
inline constexpr int              kSnakeMouthPeriod = 180, kSnakeMouthEase = 40, kSnakeMouthHold = 10;
inline constexpr float            kSnakeMouthAngle = 0.5f; // radians
inline constexpr float            kSnakeStretch = 1.2f;
inline constexpr int              kSnakeStretchPeriod = 200;
inline constexpr float            kSnakeBob = 0.6f;
inline constexpr int              kSnakeBobPeriod = 120;
inline constexpr float            kSnakeScale = 0.5f;
inline constexpr float            kSnakeAhead = 10.0f;
inline constexpr float            kSnakeHeight = 14.0f;
inline constexpr float            kSnakeLift = 2.0f;
inline constexpr float            kSnakeBack = 11.0f;
inline constexpr float            kSnakeRetreatRadius = 100.0f;
inline constexpr float            kSnakeRetreatTo = 110.0f;
inline constexpr float            kSnakeApproachRadius = 200.0f;
inline constexpr float            kSnakeApproachTo = 70.0f;
inline constexpr float            kSnakeRetreatUnlimited = 10000.0f;
inline constexpr int              kSnakeRetreatHops = 40;
inline constexpr int              kSnakeIdle = 0;
inline constexpr int              kMimicWake = 18, kKingMimicWake = 21;

bool SerpentSwordActive();

// Hook: CheckDmg, the HP a hit takes off the monster: all of it for Snake Slayer.
int SnakeSlayerAmount(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);

// Hook: each dungeon step: the snake up while the guard is, faded in or out, placed and stepped.
void SnakeCharmerStep();
// Hook: a model posed by its motion (MotionPlay, before its skin is built): the snake's jaw set after its appearance.
void SnakeCharmerPosed(CFrame *frame);
// Hook: the monsters drawn: the snake, as faded in as it is.
void SnakeCharmerDraw();
// Hook: the effects drawn: the snake's glow disc.
void SnakeCharmerGlowDraw();
// Hook: the map's raster drawn: the snake's shimmer.
void SnakeCharmerHazeDraw();
// Hook: a character's light for its draw: the snake's and the weapon's white as the snake shows.
void SnakeCharmerLight(const CCharacter &character, float *ambient);
// Whether the snake is up (the guard is), how far it has faded in (0..1), and how far its mouth is open (0..1).
bool  SnakeCharmerUp();
float SnakeCharmerShown();
float SnakeCharmerMouth();
// How far the glow disc has grown (0..1).
float SnakeCharmerGlow();

// A kind's retreat: the function of its script (its record, as a CALL names it, in script) that steps it back on its
// motion, and how many arguments it takes.
struct SnakeRetreat {
    Monster     kind;
    const char *script;
    int         record;
    int         motion;
    int         argc = 0;
};

// A kind's retreat, or null when its script has none.
const SnakeRetreat *SnakeRetreatOf(int kind);
// Hook: a monster's step: true when it was held in place of its script's step (a kind whose AI has no retreat to
// trigger); a monster that retreats has its script's trigger and limits raised for its step instead.
bool SnakeCharmerMonsterStep(CMonstorUnit &unit, int monster);
// Hook: after a monster's script step: the values raised for it put back as its script has them.
void SnakeCharmerScriptStepped(CMonstorUnit &unit, int monster);
// Hook: a monster's step after the other monsters blocked it: what takes a step nearer the player taken off it while
// it is near and not retreating.
void SnakeCharmerMonsterMove(CMonstorUnit &unit, int monster);
// Hook: the floor ending: the snake forgotten (its model goes with the floor).
void SnakeCharmerReset();

} // namespace expanded
