#include "expanded/weapons/toan/chopper.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// The sea monsters that are organic.
constexpr Monster kOrganicSea[] = {Monster::GYON, Monster::GYON_ENHANCED, Monster::SPACE_GYON, Monster::SPACE_GYON_ENHANCED, Monster::OPAR};

} // namespace

bool ButchersEdgeActive() {
    return WeaponActive(ITEM_WEAPON_CHOPPER);
}

bool Organic(const CMonstorUnit &unit, int monster) {
    int family = unit.model[unit.monster[monster].base_model].attachment_kind;
    if (family == MONSTER_FAMILY_BEAST || family == MONSTER_FAMILY_PLANT) {
        return true;
    }
    // The slot is released as its kill is paid: its kind, not whether it is alive.
    int kind = ModelKind(unit.monster[monster].base_model);
    return family == MONSTER_FAMILY_SEA && std::ranges::any_of(kOrganicSea, [&](Monster sea) { return kind == No(sea); });
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
