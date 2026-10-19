#include "expanded/weapons/toan/dusack.hpp"

#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

// Whether each monster's invincibility is a wake.
bool waking[kFloorMonsters] = {};

void OnFloor() {
    for (bool &wake : waking) {
        wake = false;
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

bool NoFoolsGoldWeapon(int weapon_no) {
    return weapon_no == ITEM_WEAPON_DUSACK || weapon_no == ITEM_WEAPON_BRAVE_ARK;
}

} // namespace

bool NoFoolsGoldActive() {
    return WeaponWielded(ITEM_WEAPON_DUSACK) || WeaponWielded(ITEM_WEAPON_BRAVE_ARK) || NoFoolsGoldWeapon(SuperSteveSphere());
}

void NoFoolsGoldInvincible(int monster_no, int steps) {
    if (monster_no >= 0 && monster_no < kFloorMonsters) {
        waking[monster_no] = steps >= kWakeInvincibleSteps;
    }
}

bool NoFoolsGold(const CMonstorUnit &unit, int monster_no) {
    if (monster_no < 0 || monster_no >= kFloorMonsters || !waking[monster_no]) {
        return false;
    }

    int kind = unit.model[unit.monster[monster_no].base_model].kind;
    return (kind == MONSTER_KIND_MIMIC_SMALL || kind == MONSTER_KIND_MIMIC_LARGE) && NoFoolsGoldActive();
}

} // namespace expanded
