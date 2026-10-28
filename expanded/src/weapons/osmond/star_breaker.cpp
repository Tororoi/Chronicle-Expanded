#include "expanded/weapons/osmond/star_breaker.hpp"

#include <format>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/osmond/star_breaker/shooting_stars.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

double ShootingStarChance(int owner) {
    if (UserStatus == nullptr || owner != UserStatus->cur_chara) {
        return 0.0;
    }
    if (WeaponWielded(ITEM_WEAPON_STAR_BREAKER)) {
        return kShootingStarChance;
    }
    return SuperSteveSphere() == ITEM_WEAPON_STAR_BREAKER ? kSuperSteveStarChance : 0.0;
}

void StarBreakerHitLanded(const CMonstorUnit &unit, int monster, COLLISION_HIT &hit, bool blocked) {
    double chance = ShootingStarChance(hit.owner);
    if (chance <= 0.0 || (hit.weapon_flags & kStarRolledMark) != 0) {
        return;
    }
    hit.weapon_flags |= kStarRolledMark;
    if (!ModChance(chance)) {
        return;
    }
    LogEvent("star", std::format("\"step\":\"proc\",\"chara\":{},\"monster\":{},\"kind\":{},\"blocked\":{}", hit.owner, monster,
                                 ModelKind(unit.monster[monster].base_model), blocked));
    SendShootingStar();
}

} // namespace expanded
