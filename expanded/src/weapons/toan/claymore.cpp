#include "expanded/weapons/toan/claymore.hpp"

#include "dun/gameloop.hpp"
#include "expanded/weapons/blade_scale.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// Whether the sword in hand was grown, so it is put back at its own size once.
bool grown = false;

} // namespace

bool GreatswordActive() {
    return Playing(CHARA_TOAN) && NowWeapon == &MainWeapon &&
           WeaponWielded(ITEM_WEAPON_CLAYMORE);
}

void GreatswordStep() {
    if (GreatswordActive()) {
        ScaleSwordInHand(kGreatswordScale);
        grown = true;
    } else if (grown) {
        ScaleSwordInHand(1.0f);
        grown = false;
    }
}

void GreatswordHits(const float *hit_pos) {
    if (GreatswordActive()) {
        PlantAlongBlade(hit_pos, kGreatswordScale);
    }
}

} // namespace expanded
