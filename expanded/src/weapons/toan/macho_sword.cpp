#include "expanded/weapons/toan/macho_sword.hpp"

#include <algorithm>

#include "dngmessageman.hpp"
#include "expanded/weapons/ownership.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

// The ABS MAX line Step shows as a kill fills a weapon.
constexpr int kAbsMaxMessage = 150;
constexpr int kAbsMaxFrames = 480;

void ShowAbsMax(const WEAPON_HAVE &weapon) {
    DngMessMan.insert_mes_1 = GetCommonItemDataSystemMsg(weapon.item_no);
    DngMessMan.insert_value_1 = weapon.level;
    DngMessMan.message = kAbsMaxMessage;
    DngMessMan.timer = kAbsMaxFrames;
    DngMessMan.steev_window = false;
}

} // namespace

bool OvertrainingAbs(WEAPON_HAVE &weapon, int abs, int max_abs) {
    if (!WeaponOwned(ITEM_WEAPON_MACHO_SWORD)) {
        return false;
    }
    int limit = max_abs * kOvertrainingLimit;
    int updated = std::min(weapon.experience + abs, limit);
    if (weapon.experience < max_abs && updated >= max_abs) {
        ShowAbsMax(weapon);
    }
    weapon.experience = static_cast<s16>(std::max<int>(weapon.experience, updated));
    return true;
}

int OvertrainingCarry(const WEAPON_HAVE &weapon, int max_abs) {
    return std::max(0, weapon.experience - max_abs);
}

int OvertrainingGaugeAbs(int abs, int max_abs) {
    return std::min(abs, max_abs);
}

} // namespace expanded
