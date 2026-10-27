#include "expanded/weapons/ungaga/cactus/absorb.hpp"

#include <algorithm>
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

// The rock, metal and undead kinds: every kind sharing their name.
constexpr Monster kDryKinds[] = {
    Monster::MASTER_JACKET,
    Monster::SKELETON_SOLDIER,
    Monster::STATUE,
    Monster::PIRATES_CHARIOT,
    Monster::GOLEM,
    Monster::MR_BLARE,
    Monster::DUNE,
    Monster::TITAN,
    Monster::ARTHUR,
    Monster::LIVING_ARMOR,
    Monster::STEEL_GIANT,
    Monster::BILLY,
    Monster::VULCAN,
    Monster::ROCKANOFF,
    Monster::GOL,
    Monster::SIL,
    Monster::STATUE_DOG,
    Monster::GACIOUS,
    Monster::SILVER_GEAR,
    Monster::HORN_HEAD,
    Monster::ARTHUR_ENHANCED,
    Monster::SIL_ENHANCED,
    Monster::MASTER_JACKET_ENHANCED,
    Monster::VULCAN_ENHANCED,
    Monster::ROCKANOFF_ENHANCED,
    Monster::STEEL_GIANT_ENHANCED,
    Monster::GOL_ENHANCED,
    Monster::PIRATES_CHARIOT_ENHANCED,
    Monster::GACIOUS_ENHANCED,
    Monster::STATUE_DOG_ENHANCED,
    Monster::TITAN_ENHANCED,
    Monster::LIVING_ARMOR_ENHANCED,
};

bool Active() {
    return UserStatus != nullptr && WeaponActive(ITEM_WEAPON_CACTUS);
}

} // namespace

bool CactusDryKind(int kind) {
    return std::any_of(std::begin(kDryKinds), std::end(kDryKinds), [kind](Monster dry) { return No(dry) == kind; });
}

void CactusMonsterDamaged(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount) {
    if (!Active() || hit.owner != UserStatus->cur_chara) {
        return;
    }
    // The life it took: its life now is past the hit, under 0 for one that took more than it had.
    int taken = amount + std::min(0, unit.monster[monster].hp);
    if (taken <= 0 || CactusDryKind(ModelKind(unit.monster[monster].base_model))) {
        return;
    }
    int    chara = UserStatus->cur_chara;
    float &water = UserStatus->water_now[chara];
    float  most = UserStatus->water_max[chara];
    if (most > 0.0f && water >= most) {
        return;
    }
    float gained = static_cast<float>(taken) / kAbsorbDamagePerWater;
    water = most > 0.0f ? std::min(water + gained, most) : water + gained;
}

} // namespace expanded
