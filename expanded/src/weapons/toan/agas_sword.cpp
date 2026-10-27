#include "expanded/weapons/toan/agas_sword.hpp"

#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

int DefensiveLegacyDefense(int chara, int defense) {
    const WEAPON_HAVE *weapon = UserStatus != nullptr ? EquippedWeapon(*UserStatus, chara) : nullptr;
    return weapon != nullptr && CarriesWeapon(*weapon, ITEM_WEAPON_AGA_S_SWORD) ? defense + kDefensiveLegacy : defense;
}

} // namespace expanded
