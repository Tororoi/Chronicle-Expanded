#include "expanded/baselard.hpp"

#include "collisiondata.hpp"
#include "expanded/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

// Hit owners: Toan's swings, and Xiao's pellets (attack 0).
constexpr int kToan = 0;
constexpr int kXiao = 1;

} // namespace

bool HeavyHandHit(const COLLISION_HIT &record) {
    if (record.owner == kToan) {
        return WeaponWielded(ITEM_WEAPON_BASELARD);
    }

    return record.owner == kXiao && record.attack_no == 0 && SuperSteveSphere() == ITEM_WEAPON_BASELARD;
}

} // namespace expanded
