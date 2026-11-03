#include "expanded/weapons/goro/battle_ax.hpp"

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

bool Felled(const CMonstorUnit &unit, int monster) {
    return WeaponActive(ITEM_WEAPON_BATTLE_AX) && unit.model[unit.monster[monster].base_model].attachment_kind == MONSTER_FAMILY_PLANT;
}

} // namespace

float WoodcutterDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    // The wielder's own weapon hits: they carry its anti-monster table.
    bool wielders = hit.vs_monster != nullptr && UserStatus != nullptr && hit.owner == UserStatus->cur_chara;
    return wielders && Felled(unit, monster) ? damage * kWoodcutterDamage : damage;
}

int WoodcutterAbs(const CMonstorUnit &unit, int monster, int abs) {
    return Felled(unit, monster) ? abs * kWoodcutterAbs : abs;
}

} // namespace expanded
