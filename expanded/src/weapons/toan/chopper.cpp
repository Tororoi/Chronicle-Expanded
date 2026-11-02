#include "expanded/weapons/toan/chopper.hpp"

#include <cmath>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

bool ButchersEdgeActive() {
    return WeaponActive(ITEM_WEAPON_CHOPPER);
}

bool Organic(const CMonstorUnit &unit, int monster) {
    int family = unit.model[unit.monster[monster].base_model].attachment_kind;
    return family == MONSTER_FAMILY_BEAST || family == MONSTER_FAMILY_PLANT;
}

float ButchersEdgeDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    // The wielder's own weapon hits: they carry its anti-monster table.
    bool wielders = hit.vs_monster != nullptr && UserStatus != nullptr && hit.owner == UserStatus->cur_chara;
    return wielders && ButchersEdgeActive() && Organic(unit, monster) ? damage * kButchersDamage : damage;
}

int ButchersEdgeAbs(const CMonstorUnit &unit, int monster, int abs) {
    return ButchersEdgeActive() && Organic(unit, monster) ? static_cast<int>(std::lround(abs * kButchersAbs)) : abs;
}

} // namespace expanded
