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
//   With none in reach, a wolf farther than kPackLeash walks to the player (its script finding him far past him, so it
//   walks on and never strikes). Within kPackLeash its move of its own plays out (kPackSettle frames at most), then its
//   script waits and it heels as the Pocklekul's pockle does: to its own place kPackHeel behind the player, the pack fanned
//   kPackSpread apart there by slot, walked to at a pace by how far it is (kPackPace of the way, kPackMinPace to
//   kPackMaxPace a frame, its walk clip at kPackWalkRate of it), idled at once within kPackArrived, turned the player's way.
//   A monster coming in reach starts its script afresh.
// - Lamb form: a wolf's blow on the guard wears the weapon as kLambGuardWear swings do, toward its wolf form.
inline constexpr float kPackReach = 300.0f;
inline constexpr float kPackLeash = 300.0f;
inline constexpr float kPackWalkOn = 32.0f;
inline constexpr int   kPackSettle = 180;
inline constexpr float kPackHeel = 35.0f, kPackSpread = 0.8f, kPackArrived = 4.0f;
inline constexpr float kPackPace = 0.08f, kPackMinPace = 0.5f, kPackMaxPace = 2.4f, kPackWalkRate = 0.12f, kPackTurnRate = 0.3f;
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

// Hook: after the monsters step: each wolf with no prey near the player parked and heeled, or released.
void PackStep(int pause);

// Whether the wolf in slot is parked heeling.
bool PackParked(int slot);

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
