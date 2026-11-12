#include "expanded/weapons/osmond/jackal.hpp"

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

bool Beast(const CMonstorUnit &unit, int monster) {
    return unit.model[unit.monster[monster].base_model].attachment_kind == MONSTER_FAMILY_BEAST;
}

} // namespace

bool PredatorActive() {
    return WeaponActive(ITEM_WEAPON_JACKAL);
}

float PredatorDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    // The wielder's own weapon hits: they carry its anti-monster table.
    bool wielders = hit.vs_monster != nullptr && UserStatus != nullptr && hit.owner == UserStatus->cur_chara;
    return wielders && PredatorActive() && Beast(unit, monster) ? damage * kPredatorDamage : damage;
}

int PredatorAbs(const CMonstorUnit &unit, int monster, int abs) {
    return PredatorActive() && Beast(unit, monster) ? abs * kPredatorAbs : abs;
}

} // namespace expanded
