#pragma once

#include <vector>

#include "expanded/enemies/monsters.hpp"

class CRandomItem;
struct WEAPON_HAVE;

namespace expanded {

// Stats a boosted weapon drop's record takes when picked up; 0 keeps the weapon's own. elem is fire, ice,
// thunder, wind, holy; anti is the anti-monster values in MonsterFamily order. abilities_low and abilities_high
// replace the low and high bytes of its ability flags, or -1 to keep them.
struct WeaponBoost {
    int weapon = -1;
    int attack = 0;
    int endurance = 0;
    int speed = 0;
    int magic = 0;
    int whp = 0;
    int elem[5] = {};
    int anti[10] = {};
    int abilities_low = -1;
    int abilities_high = -1;
};

// A miniboss's drop: an item, and the boost its record takes if it is a boosted drop.
struct MinibossDrop {
    int                item = -1;
    const WeaponBoost *boost = nullptr;
};

enum class MinibossTier { RARE, COMMON };

// The drops of a species' tier in a dungeon (0-6): those whose gate it has reached. Enhanced species take
// their base's.
std::vector<MinibossDrop> MinibossLoot(Monster species, int dungeon, MinibossTier tier);

// Rolls the drop of a species' miniboss on a dungeon's floor (UserStatus->cur_floor): a rare drop 5% of the
// time where the tier has one in the dungeon, else a common drop 30%, picked among the tier's drops there;
// when the common roll fails or finds none, the back floor's key 35%, else a weapon of the dungeon 15%, else
// an attachment 80%, else an item.
MinibossDrop RollMinibossDrop(int dungeon, int floor, Monster species);

// Writes the boost into a weapon record.
void BoostWeapon(WEAPON_HAVE &weapon, const WeaponBoost &boost);

// Hooks: a defeated miniboss's drop about to be put on the floor at position (before CRandomItem::Set), a
// new floor, an item's pickup event (in place of CRandomItem::checkEvent in DunMoveChara), and picking up
// the item (in place of BtGetAttach_Init), which boosts the weapon only if that item was a boosted drop.
void MinibossDropLanding(const MinibossDrop &drop, CRandomItem &items, const float *position);
void MinibossDropsCleared();
int  MinibossItemEvent(CRandomItem &items);
void MinibossDropPickUp(int dungeon, int item_no);

// The percentage roll the drop makes, 0-99, and the index into a list of n; null restores the random ones
// (for tests).
using MinibossLootRoll = int (*)(int n);
void SetMinibossLootRoll(MinibossLootRoll roll);

} // namespace expanded
