#include "expanded/weapons/toan/chronicle_two.hpp"

#include <algorithm>

#include "expanded/weapons/ownership.hpp"
#include "itemdata.hpp"

namespace expanded {

int ChronicleTwoLimit(const WEAPON_HAVE &weapon, int stat, int usual) {
    const WEAPON_DATA *data = GetWeaponData(weapon.item_no);
    if (data == nullptr || !WeaponOwned(ITEM_WEAPON_CHRONICLE2)) {
        return usual;
    }
    int spheres = 0;
    for (int i = 0; i < 6; i++) {
        const ATTACH_LIST &attach = weapon.attach[i];
        if (data->hole[i] > 0 && attach.item_no == ITEM_ATTACH_SYNTHESIS_SPHERE) {
            spheres += attach.status[stat];
        }
    }
    return std::min(kStatMost, usual + std::max(0, spheres));
}

} // namespace expanded
