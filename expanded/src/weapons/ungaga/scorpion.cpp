#include "expanded/weapons/ungaga/scorpion.hpp"

#include <algorithm>

#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "menu_inventory.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kWeaponSlots = 11;

} // namespace

bool VenomActive() {
    return UserStatus != nullptr && WeaponActive(ITEM_WEAPON_SCORPION);
}

void VenomPoisoned(const MONSTOR &target) {
    if (!VenomActive()) {
        return;
    }
    CDngStatusData *status = static_cast<CDngStatusData *>(UserStatus);
    int             chara = status->cur_chara;
    status->ailments[chara] &= ~AILMENT_POISON;

    // The weapon equipped, as a kill's ABS reaches it (CMonstorUnit::Step).
    int gain = target.exp / kVenomAbsShare;
    int slot = status->equipped_weapon_slot[chara];
    if (gain <= 0 || slot < 0 || slot >= kWeaponSlots || status->CheckDefaultWeapon(chara) == 0) {
        return;
    }
    WEAPON_HAVE &weapon = status->chara_weapons[chara][slot];
    int          max_exp = GetWeaponMaxExp(&weapon);
    weapon.experience = static_cast<s16>(std::max<int>(weapon.experience, std::min(max_exp, weapon.experience + gain)));
}

} // namespace expanded
