#pragma once

namespace expanded {

// A weapon's abilities as the game builds it: its table flags plus those it gains, the chance ones rolled now.
int WeaponAbilities(int weapon_no, int flags);

// The same, with a roll of 0-99 deciding the chance abilities.
int WeaponAbilities(int weapon_no, int flags, int roll);

} // namespace expanded
