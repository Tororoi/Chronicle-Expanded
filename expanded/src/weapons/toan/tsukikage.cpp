#include "expanded/weapons/toan/tsukikage.hpp"

#include <algorithm>

#include "dun/gameloop.hpp"
#include "editloop.hpp"
#include "editloop3.hpp"
#include "expanded/weapons/ownership.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

bool MoonlitFocusWeapon(int weapon_no) {
    return weapon_no == ITEM_WEAPON_TSUKIKAGE || weapon_no == ITEM_WEAPON_HEAVEN_S_CLOUD;
}

} // namespace

bool MoonlitFocusSword() {
    return MoonlitFocusWeapon(HeldWeapon());
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

bool MoonlitBaitActive() {
    return EdGetTime(NowTime) == ED_TIME_NIGHT && WeaponOwned(ITEM_WEAPON_TSUKIKAGE);
}

float MoonlitBaitAffinity(float affinity) {
    return affinity > 0.0f && MoonlitBaitActive() ? std::max(affinity, kMoonlitBait) : affinity;
}

} // namespace expanded
