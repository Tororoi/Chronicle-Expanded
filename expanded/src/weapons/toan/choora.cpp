#include "expanded/weapons/toan/choora.hpp"

#include <algorithm>

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

constexpr Monster kArmored[] = {
    Monster::CRABBY_HERMIT,
    Monster::CRABBY_HERMIT_ENHANCED,
    Monster::LIVING_ARMOR,
    Monster::LIVING_ARMOR_ENHANCED,
    Monster::BLACK_KNIGHT,
    Monster::BLACK_KNIGHT_MOUNT,
    Monster::ARTHUR,
    Monster::ARTHUR_ENHANCED,
    Monster::ALEXANDER,
    Monster::ALEXANDER_ENHANCED,
    Monster::STEEL_GIANT,
    Monster::STEEL_GIANT_ENHANCED,
    Monster::PIRATES_CHARIOT,
    Monster::PIRATES_CHARIOT_ENHANCED,
    Monster::MOON_BUG,
    Monster::STATUE,
    Monster::STATUE_DOG,
    Monster::STATUE_DOG_ENHANCED,
    Monster::GOLEM,
    Monster::TITAN,
    Monster::TITAN_ENHANCED,
    Monster::GOL,
    Monster::GOL_ENHANCED,
    Monster::SIL,
    Monster::SIL_ENHANCED,
    Monster::BLIZZARD,
    Monster::ROCKANOFF,
    Monster::ROCKANOFF_ENHANCED,
    Monster::NIKAPOUS,
};

bool Active() {
    return WeaponActive(ITEM_WEAPON_CHOORA);
}

} // namespace

bool Armored(const CMonstorUnit &unit, int monster) {
    int kind = ModelKind(unit.monster[monster].base_model);
    return std::ranges::any_of(kArmored, [kind](Monster armored) { return No(armored) == kind; });
}

float ArmorPiercerDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    // The wielder's own weapon hits: they carry its anti-monster table.
    bool wielders = hit.vs_monster != nullptr && UserStatus != nullptr && hit.owner == UserStatus->cur_chara;
    return wielders && Active() && Armored(unit, monster) ? damage * kArmorPiercerDamage : damage;
}

int ArmorPiercerAbs(const CMonstorUnit &unit, int monster, int abs) {
    return Active() && Armored(unit, monster) ? abs * kArmorPiercerAbs : abs;
}

} // namespace expanded
