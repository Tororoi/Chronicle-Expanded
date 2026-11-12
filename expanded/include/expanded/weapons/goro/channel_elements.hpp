#pragma once

#include "collisiondata.hpp"

namespace dc::game {
struct CUserStatus;
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Magical Hammer "Channel Elements" (and Super Steve's with its SynthSphere, the same for Xiao's shots): no one element
// is chosen on the hammer; every hit carries all five (kChannelFlags),
// and each of their effects shows on the monster at once, the hit's sound the element's the hammer has most points
// in. The damage's magic part is reckoned once for each element, at the monster's resistance to it, and the five are
// added: the damage after defense, at the monster's weakest resistance (its highest), plus for each element the damage
// times (0.005 x Magic + 0.004 x the element's points) at its resistance. An element the monster is immune to adds
// nothing; one immune to all five takes nothing ("no effect"). The weapon menu shows all five chosen and refuses a
// change (kChannelMessage), the element picker does not open for it, and its build-up has no element chosen.
inline constexpr int kChannelFlags = HIT_ELEMENT_FIRE | HIT_ELEMENT_COLD | HIT_ELEMENT_THUNDER | HIT_ELEMENT_WIND | HIT_ELEMENT_HOLY;
inline constexpr int kChannelMessage = 0xA1; // the weapon menu's warning 0, as the Crystal Ring's

// Whether weapon channels the elements: a Magical Hammer, or Super Steve with its SynthSphere (the abilities on).
bool ChannelElements(const WEAPON_HAVE &weapon);

// The element flags of an attack of the weapon in hand at element (its own, or the one a feature gives it): all five
// for one that channels them, else the element's.
int AttackElementFlags(int element);

// Whether a hit's element flags are a channeled hit's.
bool ChannelHit(int element_flags);

// The damage of a channeled hit after defense, damage, from a weapon of magic and elem points on a monster of
// resist (percent for each element).
float ChannelDamage(float damage, int magic, const s8 *elem, const s16 *resist);

// The element weapon has most points in (the first of a tie).
int ChannelStrongest(const WEAPON_HAVE &weapon);

// Whether the weapon menu shows element as chosen on weapon, and refuses choosing.
bool ChannelChosen(const WEAPON_HAVE &weapon, int element);
bool ChannelRefused(const WEAPON_HAVE &weapon);

// Every channeling weapon of status's, and the one in hand, with no one element chosen.
void HoldChannelElements(CUserStatus &status);

// Hooks: a dungeon step; a frame of the weapon menu; a weapon built up from from_item (no element chosen on one built
// from a channeling hammer).
void ChannelElementsStep();
void ChannelElementsMenuStep();
void ChannelElementsBuiltUp(WEAPON_HAVE &weapon, int from_item);

} // namespace expanded
