#include "expanded/weapons/toan/agas_sword.hpp"

#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

int DefensiveLegacyDefense(int chara, int defense) {
    if (UserStatus == nullptr || chara < 0 || chara >= 6) {
        return defense;
    }
    int slot = UserStatus->equipped_weapon_slot[chara];
    if (slot < 0 || slot >= 11) {
        return defense;
    }
    const WEAPON_HAVE &weapon = UserStatus->chara_weapons[chara][slot];
    bool               legacy = (weapon.item_no == ITEM_WEAPON_AGA_S_SWORD && WeaponAbilitiesOn()) ||
                                (weapon.item_no == ITEM_WEAPON_SUPER_STEVE && SphereWeapon(weapon) == ITEM_WEAPON_AGA_S_SWORD);
    return legacy ? defense + kDefensiveLegacy : defense;
}

} // namespace expanded
