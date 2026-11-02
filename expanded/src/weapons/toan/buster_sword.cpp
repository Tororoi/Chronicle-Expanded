#include "expanded/weapons/toan/buster_sword.hpp"

#include <cstring>

#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "menu_inventory.hpp"

namespace expanded {

namespace {

constexpr int kAntiStats = 10;

bool Holds(const ATTACH_LIST &entry) {
    return entry.item_no >= ITEM_ATTACH_START;
}

bool BusterSphere(const ATTACH_LIST &entry) {
    return entry.item_no == ITEM_ATTACH_SYNTHESIS_SPHERE && entry.sphere_weapon_no == ITEM_WEAPON_BUSTER_SWORD;
}

// The weapon's own slots (its data's).
int OwnHoles(const WEAPON_HAVE &weapon) {
    return GetWeaponHoleNum(weapon.item_no);
}

// Whether the extra slot is open: the sphere on (with the weapon abilities), or the slot still holding something.
bool ExtraOpen(const WEAPON_HAVE &weapon) {
    int own = OwnHoles(weapon);
    return own > 0 && own < 6 && (SphereWeapon(weapon) == ITEM_WEAPON_BUSTER_SWORD || Holds(weapon.attach[own]));
}

// An empty regular slot of the weapon's own, or -1.
int EmptyRegular(const WEAPON_HAVE &weapon) {
    const WEAPON_DATA *data = GetWeaponData(weapon.item_no);
    for (int i = 0; data != nullptr && i < OwnHoles(weapon); i++) {
        if (data->hole[i] == 1 && !Holds(weapon.attach[i])) {
            return i;
        }
    }
    return -1;
}

void Move(ATTACH_LIST &to, ATTACH_LIST &from) {
    std::memcpy(&to, &from, sizeof(to));
    std::memset(&from, 0, sizeof(from));
}

} // namespace

bool BusterBoosted(const WEAPON_HAVE *weapon) {
    if (weapon == nullptr) {
        return false;
    }
    return weapon->item_no == ITEM_WEAPON_BUSTER_SWORD ||
           (weapon->item_no == ITEM_WEAPON_SUPER_STEVE && SphereWeapon(*weapon) == ITEM_WEAPON_BUSTER_SWORD);
}

void BusterBoostEntry(ATTACH_LIST &entry, bool boosted) {
    if (!Holds(entry)) {
        return;
    }
    if (entry.item_no == ITEM_ATTACH_SYNTHESIS_SPHERE) {
        // Its values are its own: the point added to (or taken from) each anti stat it raises, once.
        bool holds = (entry.unk_1F & kBusterBoostedSphere) != 0;
        if (holds == boosted) {
            return;
        }
        for (s8 &anti : entry.vs_monster) {
            if (anti > 0) {
                anti = static_cast<s8>(anti + (boosted ? kBusterBoost : -kBusterBoost));
            }
        }
        entry.unk_1F = static_cast<u8>(boosted ? entry.unk_1F | kBusterBoostedSphere : entry.unk_1F & ~kBusterBoostedSphere);
        return;
    }
    const ATTACH_DATA *data = GetAttachData(entry.item_no);
    if (data == nullptr) {
        return;
    }
    for (int anti = 0; anti < kAntiStats; anti++) {
        if (data->vs_monster[anti] > 0) {
            entry.vs_monster[anti] = static_cast<s8>(data->vs_monster[anti] + (boosted ? kBusterBoost : 0));
        }
    }
}

void BusterBoostWeapon(WEAPON_HAVE &weapon) {
    bool boosted = BusterBoosted(&weapon);
    for (ATTACH_LIST &entry : weapon.attach) {
        BusterBoostEntry(entry, boosted);
    }
    int own = OwnHoles(weapon);
    if (own > 0 && own < 6) {
        weapon.attach_kind[own] = weapon.item_no == ITEM_WEAPON_SUPER_STEVE && ExtraOpen(weapon) ? 3 : 0;
    }
}

void BusterBoostPlaced(WEAPON_HAVE *weapon, ATTACH_LIST &entry) {
    if (weapon != nullptr) {
        BusterBoostWeapon(*weapon);
    }
    BusterBoostEntry(entry, BusterBoosted(weapon));
}

void BusterBoostSwapped(WEAPON_HAVE &weapon, ATTACH_LIST &placed, ATTACH_LIST &taken) {
    BusterBoostWeapon(weapon);
    BusterBoostEntry(placed, BusterBoosted(&weapon));
    BusterBoostEntry(taken, false);
}

int BusterHoles(const WEAPON_HAVE &weapon, int holes) {
    return ExtraOpen(weapon) ? holes + 1 : holes;
}

int BusterHoleKind(const WEAPON_HAVE &weapon, int hole, int kind) {
    if (kind <= 0 && hole == OwnHoles(weapon) && ExtraOpen(weapon)) {
        return 1;
    }
    return kind;
}

bool BusterSwapRefused(const WEAPON_HAVE &weapon, int hole, const ATTACH_LIST &held) {
    int own = OwnHoles(weapon);
    if (hole < 0 || hole >= 6 || !BusterSphere(weapon.attach[hole]) || BusterSphere(held) || own <= 0 || own >= 6) {
        return false;
    }
    return Holds(weapon.attach[own]) && EmptyRegular(weapon) < 0;
}

void BusterSphereLeft(WEAPON_HAVE &weapon, const ATTACH_LIST &taken, int vacated) {
    int own = OwnHoles(weapon);
    if (BusterSphere(taken) && own > 0 && own < 6 && Holds(weapon.attach[own]) && SphereWeapon(weapon) != ITEM_WEAPON_BUSTER_SWORD) {
        int to = EmptyRegular(weapon);
        if (to < 0) {
            to = vacated;
        }
        if (to >= 0 && !Holds(weapon.attach[to])) {
            Move(weapon.attach[to], weapon.attach[own]);
        }
    }
    BusterBoostWeapon(weapon);
}

bool BusterCancelled(WEAPON_HAVE &weapon, ATTACH_LIST &held, short &held_no) {
    int own = OwnHoles(weapon);
    if (held_no < ITEM_ATTACH_START || own <= 0 || own >= 6 || Holds(weapon.attach[own]) || SphereWeapon(weapon) != ITEM_WEAPON_BUSTER_SWORD) {
        return false;
    }
    held.item_no = held_no;
    Move(weapon.attach[own], held);
    held_no = 0;
    BusterBoostWeapon(weapon);
    return true;
}

} // namespace expanded
