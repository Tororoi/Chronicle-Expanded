#pragma once

namespace dc::game {
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Atlamillia Sword "Atlamillia Insurance": while the save owns an Atlamillia Sword, a weapon that breaks on a dungeon
// floor (not a character's default weapon, which the game mends in place) leaves an atla on a random floor of that
// dungeon, as the game would place one, holding the weapon's SynthSphere: kInsuranceShare of its attack, endurance,
// speed, magic, elements and anti-monster values for each level from +0, up to kInsuranceShareMost at +4, with its
// abilities. Its attachments are lost. The atla comes back each time its floor is built until it is picked up, and
// waits in the save. Its Swift Strikes (shamshir.hpp) and Fine Fare (sax.hpp) live with those weapons.
inline constexpr float kInsuranceShare = 0.1f;
inline constexpr float kInsuranceShareMost = 0.5f;

// The share of a weapon's values its insured SynthSphere keeps.
float InsuranceShare(const WEAPON_HAVE &weapon);

// Hook: BattleSubWeaponDmg is about to wipe weapon, broken on the current floor.
void AtlamilliaWeaponBroken(const WEAPON_HAVE &weapon);

// Hooks: a floor loaded (its insured atlas are due) and a dungeon step (they are placed).
void AtlamilliaFloorLoaded();
void AtlamilliaStep();

// How many insured SynthSpheres wait in the save (for tests).
int InsuredSpheres();

} // namespace expanded
