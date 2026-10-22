#include "expanded/weapons/toan/cross_hinder.hpp"

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/revive.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

bool Undead(const CMonstorUnit &unit, int monster) {
    return unit.model[unit.monster[monster].base_model].attachment_kind == MONSTER_FAMILY_UNDEAD;
}

// An undead the Cross Hinder kills stays down; the revive layer asks only about monsters that can get back up.
bool Sanctified(const CMonstorUnit &unit, int monster) {
    return SanctifierActive() && Undead(unit, monster);
}

[[maybe_unused]] const bool kSanctified = (AddStaysDownRule(Sanctified), true);

} // namespace

bool SanctifierActive() {
    return WeaponActive(ITEM_WEAPON_CROSS_HINDER);
}

bool HolyReachActive() {
    return WeaponActive(ITEM_WEAPON_CROSS_HINDER) || WeaponActive(ITEM_WEAPON_BIG_BANG) || WeaponActive(ITEM_WEAPON_SWORD_OF_ZEUS);
}

float SanctifierDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    // The wielder's own weapon hits: they carry its anti-monster table.
    bool wielders = hit.vs_monster != nullptr && UserStatus != nullptr && hit.owner == UserStatus->cur_chara;
    return wielders && SanctifierActive() && Undead(unit, monster) ? damage * kSanctifierDamage : damage;
}

int SanctifierAbs(const CMonstorUnit &unit, int monster, int abs) {
    return SanctifierActive() && Undead(unit, monster) ? abs * kSanctifierAbs : abs;
}

float HolyReachRate(float rate) {
    return HolyReachActive() ? rate * kHolyReach : rate;
}

} // namespace expanded
