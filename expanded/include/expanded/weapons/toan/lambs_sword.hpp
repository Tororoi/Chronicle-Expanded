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
//   monster in play within kPackReach (its script's own approach and attacks), its blows hurting the monsters they meet
//   and none of them the player or another wolf; the monsters it hits turn on it. They draw no stars.
//   With none in reach a wolf has its own place kPackHeel behind the player, the pack fanned kPackSpread apart there by
//   slot, and its own script walks it there (finding the player far past its place, so it walks on rather than arriving
//   and striking; it is not hurried, and may fall behind). Within kPackArrived of it, or held up kPackStalledFrames
//   (a wall in its way: no kPackHeadway made), its script waits and it idles turned the player's way; once its place has
//   moved kPackRestart, its script goes on from where it waited. A monster coming in reach sets it going after it the
//   same way.
// - Lamb form: a wolf's blow on the guard wears the weapon as kLambGuardWear swings do, toward its wolf form.
inline constexpr float kPackReach = 300.0f;
inline constexpr float kPackWalkOn = 32.0f;
inline constexpr float kPackHeel = 35.0f, kPackSpread = 0.8f, kPackArrived = 6.0f, kPackRestart = 40.0f, kPackTurnRate = 0.3f;
inline constexpr float kPackHeadway = 2.0f;
inline constexpr int   kPackStalledFrames = 45;
inline constexpr float kLambGuardWear = 5.0f;

// Whether the Lamb's Sword is in hand (or on Super Steve's sphere), and in its wolf or its lamb form.
bool PackLeaderWolf();
bool PackLeaderLamb();

// Whether the monster in slot is a wolf (a Werewolf or a White Fang of either kind).
bool Wolf(const CMonstorUnit &unit, int slot);

// Whether the monster in slot runs with the player: a wolf, in wolf form, of the floor's own monsters.
bool PackWolf(const CMonstorUnit &unit, int slot);

// The nearest other monster in play within kPackReach of a pack wolf, or -1.
int PackPrey(CMonstorUnit &unit, int slot);

// Where a pack wolf's script finds the player: its prey's aim point, else far past the player on its way to him. False
// for a monster not in the pack.
bool PackTarget(CMonstorUnit &unit, int slot, float *position);

// Hook: after the monsters step: each wolf with no prey its place, and held there once it is there.
void PackStep(int pause);

// Whether the wolf in slot is waiting at its place; and its place behind the player (x, z) while it has one, else null.
bool         PackParked(int slot);
const float *PackPlace(int slot);

// Whether a monster's hit passes the player by: a pack wolf's.
bool PackHitPassesBy(const COLLISION_HIT &hit);

// A monster's hit has been reacted to: in lamb form, a wolf's blow the guard stopped wears the weapon.
void LambGuardBlocked(bool blocked, const COLLISION_HIT &hit);

// Whether the weapon record is the Lamb's Sword, or Super Steve carrying its SynthSphere; and whether it is in its wolf
// form (kLambSwordThreshold of its WHP or below).
bool LambWeapon(const WEAPON_HAVE &weapon);
bool LambWolfForm(const WEAPON_HAVE &weapon);

// Hook: MenuWeaponSpSet, a weapon's model shown in its form: Super Steve (his pack as wolf_steve.hpp builds it) as Steve,
// with a Lamb's Sword SynthSphere in its wolf form as Wolf Steve, or with a Macho Sword SynthSphere in Macho's hat (his
// hair hidden under it). Nothing for a pack without those frames.
void WolfSteveShow(CCharacter *chara, const WEAPON_HAVE &weapon);

} // namespace expanded
