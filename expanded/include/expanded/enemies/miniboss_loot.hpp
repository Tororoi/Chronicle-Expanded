#pragma once

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

// A miniboss's drop: an item, and the boost its record takes if it is a boosted weapon.
struct MinibossDrop {
    int                item = -1;
    const WeaponBoost *boost = nullptr;
};

// Rolls the drop of a miniboss of a dungeon's floor (UserStatus->cur_floor) by its name (MONSTOR::name_no):
// its dungeon's rare drop 5% of the time and common drop 30% of the time where it has them, otherwise the
// back floor's key 35%, else a weapon of the dungeon 15%, else an attachment 80%, else an item.
MinibossDrop RollMinibossDrop(int dungeon, int floor, int name_no);

// Writes the boost into a weapon record.
void BoostWeapon(WEAPON_HAVE &weapon, const WeaponBoost &boost);

// A defeated miniboss's drop, a new floor, and picking up a dropped item (DunMoveChara: in place of
// BtGetAttach_Init), which boosts the weapon if one of the floor's minibosses dropped it.
void MinibossDropDropped(const MinibossDrop &drop);
void MinibossDropsCleared();
void MinibossDropPickUp(int dungeon, int item_no);

// The percentage roll the drop makes, 0-99, and the index into a list of n; null restores the random ones
// (for tests).
using MinibossLootRoll = int (*)(int n);
void SetMinibossLootRoll(MinibossLootRoll roll);

} // namespace expanded
