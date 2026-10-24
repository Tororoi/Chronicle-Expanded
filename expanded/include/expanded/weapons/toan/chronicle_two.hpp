#pragma once

struct WEAPON_HAVE;

namespace expanded {

// Chronicle 2's ownership passive: while the save owns Chronicle 2, the SynthSpheres attached to a weapon can take
// its attack and magic past their usual max (WEAPON_DATA's attack_max and magic_max) in what the weapon shows and
// fights with. Only while they are attached: a level-up that folds them into the weapon still stops at the max, and
// other attachments stop at it too. Its own max attack of 999 is in stats.cpp.
inline constexpr int kStatAttack = 0;
inline constexpr int kStatMagic = 3;
inline constexpr int kStatMost = 999;

// The most weapon's attack (kStatAttack) or magic (kStatMagic) may show, usual its max: that, or while the save owns
// Chronicle 2, that plus what the SynthSpheres attached to weapon add, up to kStatMost.
int ChronicleTwoLimit(const WEAPON_HAVE &weapon, int stat, int usual);

} // namespace expanded
