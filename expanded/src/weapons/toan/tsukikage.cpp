#include "expanded/weapons/toan/tsukikage.hpp"

#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

bool MoonlitFocusWeapon(int weapon_no) {
    return weapon_no == ITEM_WEAPON_TSUKIKAGE || weapon_no == ITEM_WEAPON_HEAVEN_S_CLOUD;
}

} // namespace

bool MoonlitFocusSword() {
    return NowWeaponHave != nullptr && MoonlitFocusWeapon(NowWeaponHave->item_no);
}

bool MoonlitFocusSphere() {
    return MoonlitFocusWeapon(SuperSteveSphere());
}

float MoonlitFocusCharge(float step) {
    return MoonlitFocusSword() ? step * kMoonlitFocusRate : step;
}

void MoonlitFocusShot(float *vector) {
    if (!MoonlitFocusSphere()) {
        return;
    }

    for (int axis = 0; axis < 3; axis++) {
        vector[axis] *= kMoonlitFocusRate;
    }
}

} // namespace expanded
