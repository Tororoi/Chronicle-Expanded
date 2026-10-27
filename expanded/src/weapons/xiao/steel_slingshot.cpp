#include "expanded/weapons/xiao/steel_slingshot.hpp"

#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

float EnduranceUpShotWear(float wear) {
    if (UserStatus == nullptr || !WeaponActive(ITEM_WEAPON_STEEL_SLINGSHOT)) {
        return wear;
    }
    // The equipped record's WHP, which the wear comes off (NowWeaponHave is a copy with the attachments' values).
    const WEAPON_HAVE *weapon = EquippedWeapon(*UserStatus, UserStatus->cur_chara);
    if (weapon == nullptr) {
        return wear;
    }
    bool low = weapon->durability > 0 && weapon->durability_f <= kEnduranceUpWarning * weapon->durability;
    return low ? wear * kEnduranceUpShot : wear;
}

int EnduranceUpEndurance(const WEAPON_HAVE &weapon, int add) {
    return weapon.item_no == ITEM_WEAPON_STEEL_SLINGSHOT ? kEnduranceUpEndurance : add;
}

int EnduranceUpDurability(const WEAPON_HAVE &weapon, int roll) {
    return weapon.item_no == ITEM_WEAPON_STEEL_SLINGSHOT ? roll * kEnduranceUpDurability : roll;
}

} // namespace expanded
