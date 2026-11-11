#pragma once

namespace dc::game {
class CCharacter;
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// The Inferno, in Goro's hand: Satan's Ax's Pull of the Underworld (satans_ax.hpp) and its sparks, orange red, half of
// the ground's starting up to twice his height and those off the ground coming in lower (satans_ax/underworld_sparks.hpp),
// and:
//
// Fireproof: a monster's fire hit (its fire element bit) passes him by.
//
// "Burning Aura": once each second the pull has been on (his charge held, then the smash until it lands), every monster
// it is drawing takes a fire hit of kBurningAuraStep of his weapon's attack for each second so far (1% at the first, 2%
// at the second, 3% at the third: 6% over three seconds), through any guard and throwing nothing; one immune to fire
// takes none of it. A monster it draws from farther than kBurningAuraBrave is afraid: one whose kind backs off (as from
// the Serpent Sword's snake: serpent_sword.hpp) backs off, and a kind with none goes on as ever; within
// kBurningAuraBrave it is itself again, and comes at him. The floor's fog reddens toward kBurningAuraFog as the pull
// goes on and thickens (its near and far pulled in to kBurningAuraFogPull of their own), the background past the
// floor's drawn cells with it, and its light toward kBurningAuraLight (each colour scaled), fully at
// kBurningAuraFogFrames, and all fade back over kBurningAuraFogFade once it ends; from kBurningAuraHazeFrom of the way,
// a heat shimmer (Mirage's: heat_haze.hpp) comes up over the whole screen, at kBurningAuraHazeSway times a fire's sway
// when the red is full; the Inferno itself is drawn lighter by kBurningAuraTint in red, green and blue as the red comes
// (in place of Satan's Ax's pink: satans_ax/underworld_tint.hpp). A fireball rises out of the floor around him as the
// pull begins, lighting him (inferno/rising_flame.hpp), its wind flapping his cape (inferno/cape_flap.hpp), and Mr.
// Blare's blast goes off where his smash lands, its shockwave reaching farther (inferno/smash_blast.hpp). Times are
// game frames.
inline constexpr int   kBurningAuraFrames = 60;
inline constexpr float kBurningAuraStep = 0.01f;
inline constexpr float kBurningAuraBrave = 30.0f;
inline constexpr float kBurningAuraFog[3] = {220.0f, 40.0f, 20.0f};
inline constexpr int   kBurningAuraFogFrames = 180;
inline constexpr int   kBurningAuraFogFade = 60;
inline constexpr float kBurningAuraFogPull = 0.75f;
inline constexpr float kBurningAuraLight[3] = {1.15f, 0.62f, 0.45f};
inline constexpr float kBurningAuraHazeFrom = 0.5f;
inline constexpr float kBurningAuraHazeSway = 2.25f;
inline constexpr float kBurningAuraTint[3] = {150.0f, 25.0f, 0.0f};
inline constexpr int   kFireElementBit = 1; // a hit's flags: the fire element (element_tbl's first)

// Whether Goro is in play with the Inferno in hand.
bool InfernoWielded();

// One dungeon step: the pull's time counted, the aura's hits planted on the second, the fog reddened or faded.
void BurningAuraStep();
// The seconds the pull has been on, and how red the fog is (0 to 1).
int   BurningAuraSeconds();
float BurningAuraRed();
// How strong the shimmer is (0 to 1).
float BurningAuraHaze();

// Hooks: whether a monster's hit passes him by (BtCheckDamageProc); whether a monster is afraid this step
// (SnakeCharmerMonsterStep); the light, fog and background the floor hands the renderer, reddened in place (DunMainDraw);
// the shimmer, with the dungeon's other rasters; the flame around him (inferno/rising_flame.hpp), with the dungeon's effects.
bool InfernoFireproof(const COLLISION_HIT &hit);
bool BurningAuraFeared(CMonstorUnit &unit, int monster);
void BurningAuraLight(float *ambient, float (*colours)[4]);
void BurningAuraFog(float *fog_rate, unsigned char *fog_colour);
void BurningAuraBackground(unsigned char *background);
void BurningAuraHazeDraw();
void BurningAuraFlameDraw();
// The tint added to the Inferno as it draws (CharacterLight).
void BurningAuraWeaponLight(const CCharacter &character, float *ambient);

// The floor changed (tests too).
void BurningAuraReset();

} // namespace expanded
