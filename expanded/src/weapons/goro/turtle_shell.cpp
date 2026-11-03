#include "expanded/weapons/goro/turtle_shell.hpp"

#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

float HardShellWear(const WEAPON_HAVE &weapon, float amount) {
    return CarriesWeapon(weapon, ITEM_WEAPON_TURTLE_SHELL) ? amount * kHardShellWear : amount;
}

int HardShellDefense(int chara, int defense) {
    const WEAPON_HAVE *weapon = UserStatus != nullptr ? EquippedWeapon(*UserStatus, chara) : nullptr;
    return weapon != nullptr && CarriesWeapon(*weapon, ITEM_WEAPON_TURTLE_SHELL) ? defense + kHardShellDefense : defense;
}

} // namespace expanded
