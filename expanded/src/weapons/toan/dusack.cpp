#include "expanded/weapons/toan/dusack.hpp"

#include "expanded/data/monster_scripts.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

bool NoFoolsGoldWeapon(int weapon_no) {
    return weapon_no == ITEM_WEAPON_DUSACK || weapon_no == ITEM_WEAPON_BRAVE_ARK;
}

} // namespace

bool NoFoolsGoldActive() {
    return WeaponWielded(ITEM_WEAPON_DUSACK) || WeaponWielded(ITEM_WEAPON_BRAVE_ARK) || NoFoolsGoldWeapon(SuperSteveSphere());
}

bool NoFoolsGoldOpens(const CMonstorUnit &unit, int monster_no, int window) {
    if (monster_no < 0 || monster_no >= kFloorMonsters || window < 0 || window >= 3) {
        return false;
    }
    int                         kind = unit.model[unit.monster[monster_no].base_model].kind;
    const MONSTOR_GUARD_WINDOW &guard = unit.guard[monster_no];
    bool                        wake = guard.motion_start[window] == kMimicWakeGuardStart &&
                                       (guard.motion_end[window] == kMimicWakeGuardEnd || guard.motion_end[window] == kKingMimicWakeGuardEnd);
    return wake && (kind == MONSTER_KIND_MIMIC_SMALL || kind == MONSTER_KIND_MIMIC_LARGE) && NoFoolsGoldActive();
}

} // namespace expanded
