#include "expanded/weapons/osmond/snail.hpp"

#include <string>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/status/monster_ailments.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

void SnailMonsterDamaged(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount) {
    if (amount <= 0 || unit.monster[monster].hp <= 0 || UserStatus == nullptr || hit.owner != UserStatus->cur_chara ||
        !WeaponActive(ITEM_WEAPON_SNAIL) || !ModChance(kSlimeTrailChance)) {
        return;
    }
    MONSTOR &live = unit.monster[monster];
    bool     gooed = live.slow_timer > 0;
    GiveMonsterAilments(live, monster, kMonsterGoo);
    if (!gooed && live.slow_timer > 0) {
        LogEvent("goo", "\"monster\":" + std::to_string(monster));
    }
}

} // namespace expanded
