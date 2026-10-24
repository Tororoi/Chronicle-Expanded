#include "expanded/weapons/xiao/flamingo.hpp"

#include <algorithm>
#include <initializer_list>

#include "expanded/weapons/ownership.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

bool LockOnDistanceActive() {
    for (int weapon : {ITEM_WEAPON_FLAMINGO, ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_DIVINEBEASTTITLE, ITEM_WEAPON_ANGEL_SHOOTER, ITEM_WEAPON_ANGE_GEAR}) {
        if (WeaponActive(weapon)) {
            return true;
        }
    }
    return false;
}

float LockOnDistanceRate(float rate) {
    return LockOnDistanceActive() ? rate * kLockOnDistance : rate;
}

float BaitNoticeRadius(float radius) {
    return radius + kBaitFlamboyance * static_cast<float>(std::min(WeaponsOwned(ITEM_WEAPON_FLAMINGO), kBaitFlamboyanceMost));
}

} // namespace expanded
