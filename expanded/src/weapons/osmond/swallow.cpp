#include "expanded/weapons/osmond/swallow.hpp"

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

bool Flying(const CMonstorUnit &unit, int monster) {
    return unit.model[unit.monster[monster].base_model].attachment_kind == MONSTER_FAMILY_SKY;
}

bool StoppingPowerWeapon(int weapon_no) {
    return weapon_no == ITEM_WEAPON_SWALLOW || weapon_no == ITEM_WEAPON_G_CRUSHER || weapon_no == ITEM_WEAPON_STAR_BREAKER;
}

} // namespace

bool WingClipperActive() {
    return WeaponActive(ITEM_WEAPON_SWALLOW);
}

float WingClipperDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    // The wielder's own weapon hits: they carry its anti-monster table.
    bool wielders = hit.vs_monster != nullptr && UserStatus != nullptr && hit.owner == UserStatus->cur_chara;
    return wielders && WingClipperActive() && Flying(unit, monster) ? damage * kWingClipperDamage : damage;
}

int WingClipperAbs(const CMonstorUnit &unit, int monster, int abs) {
    return WingClipperActive() && Flying(unit, monster) ? abs * kWingClipperAbs : abs;
}

bool StoppingPowerHit(const COLLISION_HIT &record) {
    if (record.owner == CHARA_OSMOND) {
        return StoppingPowerWeapon(HeldWeapon());
    }
    // Super Steve's pellets (attack 0) with the sphere of one of them.
    return record.owner == CHARA_XIAO && record.attack_no == 0 && StoppingPowerWeapon(SuperSteveSphere());
}

} // namespace expanded
