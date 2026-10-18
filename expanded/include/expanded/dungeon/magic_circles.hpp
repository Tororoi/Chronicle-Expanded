#pragma once

struct WEAPON_HAVE;

namespace expanded {

// The magic circles' strength: retail's (1), or amplified (the Crysknife and Magical Hammer's "Circle Amplifier":
// 2 while one of the two is owned, 3 while both are). An amplified circle multiplies what it does by it:
// - attack up lasts 30 s times it, and every monster's rage 5 s times it;
// - money up gives it times retail's gain, money down takes 20 % times it;
// - each WHP and stat roll is multiplied by it, elements' and antis' losses too;
// - the WHP loss takes WHP to 1;
// - ABS full and WHP cure also give it - 1 Powerup Powders and Auto Repair Powders.
int CircleMagnitude();

// Hook: Run_TrapCircle, with the circle's kind (a default weapon's stamina already in): runs an amplified circle on
// the active character's weapon, its sound too; false leaves the circle to retail.
bool AmplifiedCircle(int kind, WEAPON_HAVE &weapon);

} // namespace expanded
