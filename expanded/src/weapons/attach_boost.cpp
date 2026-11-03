#include "expanded/weapons/attach_boost.hpp"

#include "expanded/weapons/osmond/blessing_gun.hpp"
#include "expanded/weapons/toan/buster_sword.hpp"
#include "itemdata.hpp"

namespace expanded {

void AttachBoostWeapon(WEAPON_HAVE &weapon) {
    BusterBoostWeapon(weapon);
    BlessingBoostWeapon(weapon);
}

void AttachBoostPlaced(WEAPON_HAVE *weapon, ATTACH_LIST &entry) {
    BusterBoostPlaced(weapon, entry);
    if (weapon != nullptr) {
        BlessingBoostWeapon(*weapon);
    }
    BlessingBoostEntry(entry, BlessingBoosted(weapon));
}

void AttachBoostSwapped(WEAPON_HAVE &weapon, ATTACH_LIST &placed, ATTACH_LIST &taken) {
    BusterBoostSwapped(weapon, placed, taken);
    BlessingBoostWeapon(weapon);
    BlessingBoostEntry(placed, BlessingBoosted(&weapon));
    BlessingBoostEntry(taken, false);
}

} // namespace expanded
