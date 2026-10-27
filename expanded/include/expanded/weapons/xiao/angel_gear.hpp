#pragma once

class CCharacter;

namespace expanded {

// Angel Gear "Guardian Reflector": with the Angel Gear in hand, or Super Steve carrying its SynthSphere,
// while Xiao guards a giant copy of the slingshot in hand stands in front of her (angel_gear/shield_prop.hpp), fading in
// over kReflectorFadeIn with the change jingle once solid, and away over kReflectorFadeOut once she lets go. It turns about
// her to face the nearest shot coming at her, else the nearest monster, else ahead, and while it fires, the monster it fires
// at. It catches the shots coming at her and fires them back (angel_gear/shot_reflect.hpp): once none is coming (or the
// first has waited kReflectorFireWait), it draws its pouch back over kReflectorDraw, holds kReflectorHold, and lets go, the
// shot leaving as it does, back on its catch pose kReflectorShoot later. It is a shield the monsters walk at and swing at
// (angel_gear/shield_ring.hpp): kReflectorHits strikes break it, with the weapon-break sound, and it clears over
// kReflectorDispel. While it stands the action gauge is its health (each strike takes a kReflectorHits'th); after a break
// the gauge refills over kReflectorCooldown at the weapon's speed (slowed or quickened as the gauge's ailments make it),
// and it only stands on a full gauge, as her shot needs one. It glows a steady gold (kReflectorTint, the height of Xiao's
// Guardian Grace swell). Times are game frames.
inline constexpr int   kReflectorFadeIn = 15, kReflectorFadeOut = 30, kReflectorDispel = 9;
inline constexpr int   kReflectorFireWait = 60;
inline constexpr int   kReflectorDraw = 15, kReflectorHold = 9, kReflectorShoot = 12;
inline constexpr int   kReflectorHits = 5;
inline constexpr int   kReflectorCooldown = 240;
inline constexpr float kReflectorFullGauge = 99.5f;
inline constexpr float kReflectorTint[3] = {60.0f, 52.0f, 32.0f};

// Angel Gear's party heal: with it in Xiao's hand, or Super Steve carrying its sphere, while the weapon has the Heal ability
// (its own, or the sphere's), each tick of it heals the rest of the party 1 HP as it heals her (none past their most, none
// fallen); not while a chest opens, a talk or an event runs.

// Whether it is on: Xiao, with the Angel Gear in hand or Super Steve carrying its sphere, outside an event.
bool GuardianReflectorActive();

// Hooks: a dungeon step, after the monsters' shots stepped; the draw (beside the shots'); the player's hits about to be
// checked (BtCheckDamageProc), the swings on the shield spent first; the action gauge's refill, gain the game's; the point
// a shot meets as the player (checkCollision), the pouch's while the shield is solid; the Heal ability's tick.
void  GuardianReflectorStep();
void  GuardianReflectorDraw();
void  GuardianReflectorSpend();
float GuardianReflectorGaugeGain(float gain);
void  GuardianReflectorPlayerPoint(float *point);
void  AngelGearHealTick();
// The slingshot's gold (CCharacter::Draw).
void GuardianReflectorLight(const CCharacter &character, float *ambient);

// Whether the shield stands (fading included), how solid it is (0 to 1), the strikes it has left, whether the gauge is
// refilling after a break.
bool  GuardianReflectorUp();
float GuardianReflectorAlpha();
int   GuardianReflectorHits();
bool  GuardianReflectorCooling();

// Taken down (the floor changed), for tests too.
void GuardianReflectorReset();

} // namespace expanded
