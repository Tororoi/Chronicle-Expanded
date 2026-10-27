#pragma once

class CMonstorUnit;
struct COLLISION_HIT;

namespace expanded::pocklekul {

// The Pocklekul's pockle in the floor's fights, which the engine never has one monster take from another:
//   - Its swings never hit the summoner: each attack sphere it sets (CheckDmg) goes into the collision table striking no
//     one, after a living floor monster whose body (its posed hurt spheres, across the ground) the sphere reaches is
//     given a hit of the summoner's (PlantWeaponHit's, kPockleHitRadius wide at the monster's largest posed sphere, the
//     swing's reaction and push, landing on that monster alone): the summoner's kill and ABS, the swing's damage over the
//     summoner's ranged distance factor, so CheckDmg's factor gives it back whole. One a monster every
//     kPockleContactFrames.
//   - Its shots (its unit's, marked kAllyShooter) do the same on the first living floor monster whose hurt sphere one
//     meets in flight, and strike there; they hurt the summoner as any monster's do.
//   - A stealing swing (flag kPockleStealGold: every pockle's melee but Friday's kick and Hell Pockle's second) that is
//     given takes the monster's money kPockleStealPercent of the time: the monster drops none, the pockle the lot as it
//     falls.
//   - A floor monster its hit takes life from, or that its body stops, goes after it (its script finds the player at the
//     pockle) until either falls or goes, or confusion takes the monster.
//   - A living floor monster's swing or shot that reaches its hurt spheres hurts it as one of nobody's does (CheckDmg's
//     owner -1: less its defence, at least 1, the shot's element weighed; its guard windows block it), one an attacker
//     every kPockleContactFrames; a shot strikes there. The summoner's own hits hurt it as they hurt any monster.
//   - It and the floor's monsters stop each other as monsters do (within both collision radii, below the other's top,
//     moving toward it), and it stops the player as a floor monster does.
inline constexpr int   kPockleContactFrames = 30;
inline constexpr float kPockleHitRadius = 1.0f;
inline constexpr int   kPockleStealGold = 0x40000;
inline constexpr int   kPockleStealPercent = 20;

// A pockle just stood (its fights afresh), and it went (the monsters it provoked back on the player).
void FightsBegin();
void FightsEnd();

// Once a step it stands unpaused: the frame count its contacts keep, and the monsters it provoked let go once they are
// gone or confused.
void FightsStep();

// Hooks: an attack sphere CheckDmg just set for it (hit its record, -1 for none); a floor monster's hit on its sphere
// (CheckDmg), CheckDmg's result or -1 for none; whether its unit refuses a hit (one it planted on a monster); a floor
// monster hit for amount (CheckDmg); the shots stepped (its own in flight); a monster's step after the monsters of its
// unit blocked it (both ways); where a floor monster it provoked finds the player, true when it set it; a floor
// monster's kill ABS (logged when its hit felled it). A hit planted for it lands on its monster alone: a floor monster
// refuses one meant for another (PockleHitMeantElsewhere).
void PockleSwingSet(CMonstorUnit &unit, int hit);
int  PockleHitOn(CMonstorUnit &unit, int sphere);
bool PockleRefuses(const COLLISION_HIT &hit);
bool PockleHitMeantElsewhere(int monster, const COLLISION_HIT &hit);
void PockleMonsterDamaged(int monster, const COLLISION_HIT &hit, int amount);
void PockleShots();
void PockleMoveChecked(CMonstorUnit &unit, int monster);
bool PockleProvoked(int monster, float *position);
void PockleKillAbs(int monster, int abs);

} // namespace expanded::pocklekul
