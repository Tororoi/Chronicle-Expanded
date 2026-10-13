#pragma once

#include <cstdint>

class CMonstorUnit;
struct PortDecodedTexture;

// The Confuse weapon ability: bit 14 of a weapon's flags (WEAPON_HAVE::flags, the ability word every hit
// carries as COLLISION_HIT::weapon_flags), beside Poison, Stop, Drain and the rest. Babel's Spear has it
// from its weapon data; a SynthSphere made from a weapon that has it carries it on as it carries any ability.
//
// A hit that carries it confuses the monster 5% of the time, flat, as Critical's 1% is: status resistance
// does not lower it, and bosses (MONSTER_KIND_NO_LOCK_ON) are never confused. A confused monster goes after
// whatever is nearest, another monster or the player, for 20 seconds of play (a fresh proc starts the time
// again; the time runs while the monster is out of view, and the confusion lasts until it dies, the time is
// up or the floor is left): its scripts' player position and distance (_GET_POSITION, _GET_DISTANCE) give
// its target's, and its attack spheres and projectiles hurt the monsters they touch as well as the player. A
// monster it hits turns on it: it goes after the confused monster that hit it last, and hurts only that one,
// until that one comes to its senses or dies, then after the one before, then the player again. The Terra
// nut's spinning stars (gedit/s04/chara/e114ex.chr) turn over the heads of the eight confused monsters in
// play nearest the player.
inline constexpr int kWeaponFlagConfuse = 0x4000;
// The option bit and its name, the system bank's message 0x45 (Big bucks, bit 1, is 0x46).
inline constexpr int kConfuseOptionBit = 14;
inline constexpr int kConfuseNameMessage = 0x45;

// The menus' option name of a bit, as the system bank numbers them.
int ConfuseOptionMessage(int bit);

// A step of play: the monsters stepped (CMonstorUnit::Step without its hold). Confusion counts down.
void ConfuseTick();

// The floor's monsters were set up afresh (BtLoadMonstor): every confusion ends and the stars are loaded into
// the monsters' memory and texture block.
void ConfuseFloorLoaded();

// The floor's monsters were put back, or their memory taken for an event: every confusion ends, and the
// stars too while drop_stars (their memory and textures are gone).
void ConfuseFloorReset(bool drop_stars);

// Every step a monster in play is checked for damage (CMonstorUnit::CheckDmg), before anything else.
void ConfuseStep(CMonstorUnit &unit, int slot);

// The roll of a hit that carries kWeaponFlagConfuse and landed on the monster.
void ConfuseRoll(CMonstorUnit &unit, int slot);

// Where a monster's script sees the player: false for the player itself, else the position of what a
// confused or provoked monster goes after.
bool ConfuseTarget(CMonstorUnit &unit, int slot, float *position);

// A hit another monster's attack plants that may hurt monster victim through its sphere at position: the
// hit's index, or -1. ConfuseLanded records that it did; a projectile's ends there as on any target.
int  ConfuseFriendlyHit(CMonstorUnit &unit, int victim, const float *position, float radius);
void ConfuseLanded(CMonstorUnit &unit, int hit, int victim);

// The stars, drawn with the dungeon's effects.
void ConfuseStarsDraw();

// The Confuse icon laid into the charaface sheet's empty tile beside Steal, as the sheet is decoded.
void ConfuseIconTexture(const char *name, PortDecodedTexture &texture);
