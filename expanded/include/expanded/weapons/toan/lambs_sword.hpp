#pragma once

namespace dc::game {
class CCharacter;
class CMonstorUnit;
struct COLLISION_HIT;
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Lamb's Sword "Pack Leader", in hand or on Super Steve's SynthSphere, by the form the game gives the weapon (wolf at
// kLambSwordThreshold of its WHP or below, lamb above):
// - Wolf form: the floor's wolves (Werewolves and White Fangs) run with the player. Each goes for the nearest other
//   monster in play within kPackReach, its blows hurting the monsters they meet and none of them the player or another
//   wolf; the monsters it hits turn on it. With none in reach it heels kPackHeel from the player. They draw no stars.
// - Lamb form: a wolf's blow on the guard wears the weapon as a swing does, toward its wolf form.
inline constexpr float kPackReach = 300.0f;
inline constexpr float kPackHeel = 30.0f;

// Whether the Lamb's Sword is in hand (or on Super Steve's sphere), and in its wolf or its lamb form.
bool PackLeaderWolf();
bool PackLeaderLamb();

// Whether the monster in slot is a wolf (a Werewolf or a White Fang of either kind).
bool Wolf(const CMonstorUnit &unit, int slot);

// Whether the monster in slot runs with the player: a wolf, in wolf form, of the floor's own monsters.
bool PackWolf(const CMonstorUnit &unit, int slot);

// Where a pack wolf goes: the aim point of the nearest other monster in play within kPackReach, else kPackHeel from the
// player on its side. False for a monster not in the pack.
bool PackTarget(CMonstorUnit &unit, int slot, float *position);

// Whether a monster's hit passes the player by: a pack wolf's.
bool PackHitPassesBy(const COLLISION_HIT &hit);

// A monster's hit has been reacted to: in lamb form, a wolf's blow the guard stopped wears the weapon.
void LambGuardBlocked(bool blocked, const COLLISION_HIT &hit);

// Whether the weapon record is the Lamb's Sword, or Super Steve carrying its SynthSphere; and whether it is in its wolf
// form (kLambSwordThreshold of its WHP or below).
bool LambWeapon(const WEAPON_HAVE &weapon);
bool LambWolfForm(const WEAPON_HAVE &weapon);

// Hook: MenuWeaponSpSet, a weapon's model shown in its form: Super Steve (his pack as wolf_steve.hpp builds it) as Steve,
// or with a Lamb's Sword SynthSphere in its wolf form as Wolf Steve. Nothing for a pack without the wolf's frames.
void WolfSteveShow(CCharacter *chara, const WEAPON_HAVE &weapon);

} // namespace expanded
