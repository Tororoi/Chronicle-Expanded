#include "expanded/weapons/ruby/bandits_ring.hpp"

#include <algorithm>
#include <format>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

int steals[kFloorMonsters] = {};

void OnFloor() {
    std::ranges::fill(steals, 0);
}

void OnSpawn(CMonstorUnit &, int monster) {
    steals[monster] = 0;
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), AddEnemySpawnListener(OnSpawn), true);

bool MonsterSlot(int monster) {
    return monster >= 0 && monster < kFloorMonsters;
}

bool RingsHit(const COLLISION_HIT &hit) {
    return UserStatus != nullptr && hit.owner == UserStatus->cur_chara && WeaponActive(ITEM_WEAPON_BANDIT_S_RING);
}

} // namespace

int MonsterSteals(int monster) {
    return MonsterSlot(monster) ? steals[monster] : 0;
}

// Unrobbed, the game's own item stays; robbed twice, the game left none.
void DoubleTakeHitTaken(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit) {
    if (!MonsterSlot(monster) || steals[monster] != 1) {
        return;
    }
    MONSTOR &live = unit.monster[monster];
    live.steal_item = RingsHit(hit) ? unit.model[live.base_model].steal_item : -1;
}

void DoubleTakeStolen(const CMonstorUnit &unit, int monster, int item) {
    if (!MonsterSlot(monster)) {
        return;
    }
    steals[monster]++;
    LogEvent("steal", std::format("\"monster\":{},\"kind\":{},\"item\":{},\"steals\":{}", monster, ModelKind(unit.monster[monster].base_model), item,
                                  steals[monster]));
}

} // namespace expanded
