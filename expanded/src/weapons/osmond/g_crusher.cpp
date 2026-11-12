#include "expanded/weapons/osmond/g_crusher.hpp"

#include <algorithm>
#include <format>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

bool GuardCrusherActive() {
    return WeaponActive(ITEM_WEAPON_G_CRUSHER) || WeaponActive(ITEM_WEAPON_STAR_BREAKER);
}

bool GiantCrusherActive() {
    return WeaponActive(ITEM_WEAPON_G_CRUSHER);
}

bool GuardCrusherBreaks(const CMonstorUnit &unit, int monster, COLLISION_HIT &hit) {
    if ((hit.weapon_flags & kGuardRolledMark) != 0) {
        return (hit.weapon_flags & kGuardBrokenMark) != 0;
    }
    if (UserStatus == nullptr || hit.owner != UserStatus->cur_chara || !GuardCrusherActive()) {
        return false;
    }
    hit.weapon_flags |= kGuardRolledMark;
    if (!ModChance(kGuardCrusherChance)) {
        return false;
    }
    hit.weapon_flags |= kGuardBrokenMark;
    LogEvent("guard_crush", std::format("\"chara\":{},\"monster\":{},\"kind\":{}", hit.owner, monster, ModelKind(unit.monster[monster].base_model)));
    return true;
}

float GiantCrusherRate(const CMonstorUnit &unit, int monster) {
    const MONSTOR_MODEL &model = unit.model[unit.monster[monster].base_model];
    if (model.attachment_kind == MONSTER_FAMILY_PLANT) {
        return 1.0f;
    }
    return 1.0f + (kGiantCrusherDamage - 1.0f) * (1.0f - std::clamp(model.knockback_scale, 0.0f, 1.0f));
}

float GiantCrusherDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    // The wielder's own weapon hits.
    bool wielders = hit.vs_monster != nullptr && UserStatus != nullptr && hit.owner == UserStatus->cur_chara;
    return wielders && GiantCrusherActive() ? damage * GiantCrusherRate(unit, monster) : damage;
}

} // namespace expanded
