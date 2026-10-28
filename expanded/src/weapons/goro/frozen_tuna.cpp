#include "expanded/weapons/goro/frozen_tuna.hpp"

#include <algorithm>
#include <format>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/goro/frozen_tuna/ice_prisons.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_freefuncs.hpp"
#include "userstatus.hpp"

namespace expanded {

using namespace frozen_tuna;

namespace {

// The kinds the stop passes by.
constexpr Monster kIceKinds[] = {Monster::BLIZZARD, Monster::SAM, Monster::GEMRON_ICE};

// A wielder's pool, watching the weapon record and the member it was seeded on: the WHP last seen, the life banked, the
// part of a point drained toward the next, and the frames toward the next drain.
struct Pool {
    const WEAPON_HAVE *weapon = nullptr;
    int                chara = -1;
    float              whp = 0.0f;
    float              banked = 0.0f;
    float              drained = 0.0f;
    int                frames = 0;
};

Pool goro_pool;
Pool sphere_pool;

// The life the member has, the gauge's target while it moves.
int Life(const CUserStatus &status, int chara) {
    return status.life_step[chara] != 0 ? status.next_hp[chara] : status.hp[chara];
}

void Log(const Pool &pool, const char *what) {
    LogEvent("pool", std::format("\"chara\":{},\"what\":\"{}\",\"banked\":{:.2f}", pool.chara, what, pool.banked));
}

void Empty(Pool &pool) {
    if (pool.banked > 0.0f) {
        pool.banked = 0.0f;
        Log(pool, "reset");
    }
    pool.drained = 0.0f;
    pool.frames = 0;
}

void StepPool(Pool &pool, bool active) {
    WEAPON_HAVE *weapon = active && UserStatus != nullptr ? EquippedWeapon(*UserStatus, UserStatus->cur_chara) : nullptr;
    if (weapon == nullptr) {
        Empty(pool);
        pool = Pool{};
        return;
    }
    int chara = UserStatus->cur_chara;
    int life = Life(*UserStatus, chara);
    if (pool.weapon != weapon || pool.chara != chara) {
        Empty(pool);
        pool = {.weapon = weapon, .chara = chara, .whp = weapon->durability_f};
        return;
    }
    if (weapon->durability_f < pool.whp) {
        pool.banked += (pool.whp - weapon->durability_f) * kColdStorageHealPerWhp;
        Log(pool, "bank");
    } else if (weapon->durability_f > pool.whp) {
        Empty(pool);
    }
    pool.whp = weapon->durability_f;
    if (pool.banked > 0.0f && life > 0 && life < UserStatus->max_hp[chara] && MonstersStepping() && ++pool.frames >= kColdStorageDrainFrames) {
        pool.frames = 0;
        float take = std::min(1.0f, pool.banked);
        pool.banked -= take;
        pool.drained += take;
        if (pool.drained >= 1.0f) {
            pool.drained -= 1.0f;
            UserStatus->AddNowLife(chara, 1, 255.0f);
            Log(pool, "heal");
        }
    }
    if (pool.banked <= 0.0f) {
        pool.banked = 0.0f;
        pool.frames = 0;
    }
}

bool IceKind(const CMonstorUnit &unit, int monster) {
    return std::any_of(std::begin(kIceKinds), std::end(kIceKinds), [&](Monster kind) { return IsKind(unit, monster, kind); });
}

// The game's freeze on a monster (CheckDmg's for a hit with Freeze), for kColdStorageMonsterFrames; one frozen already
// is held anew.
void Stop(MONSTOR &live) {
    live.stop_timer = kColdStorageMonsterFrames;
    live.poison_timer = 0;
    live.slow_timer = 0;
    live.anger_timer = 0;
    live.movement_speed = 0.0f;
}

} // namespace

float ColdStorageBanked(bool sphere) {
    return sphere ? sphere_pool.banked : goro_pool.banked;
}

void ColdStorageStep() {
    StepPool(goro_pool, WeaponWielded(ITEM_WEAPON_FROZEN_TUNA));
    StepPool(sphere_pool, SuperSteveSphere() == ITEM_WEAPON_FROZEN_TUNA);
    IcePrisonsStep();
}

void ColdStorageMonsterDamaged(CMonstorUnit &unit, int, const COLLISION_HIT &hit, int amount) {
    if (amount <= 0 || UserStatus == nullptr || hit.owner != UserStatus->cur_chara || !WeaponActive(ITEM_WEAPON_FROZEN_TUNA) ||
        !ModChance(kColdStorageChance)) {
        return;
    }
    int chara = UserStatus->cur_chara;
    int stopped = 0;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        MONSTOR &live = unit.monster[monster];
        if (live.state != 2 || !MonsterInPlay(live) || IceKind(unit, monster)) {
            continue;
        }
        Stop(live);
        EncaseMonster(monster);
        stopped++;
    }
    BtSetStatusErr(AILMENT_FREEZE);
    bool frozen = (UserStatus->ailments[chara] & AILMENT_FREEZE) != 0;
    if (frozen) {
        UserStatus->ailment_frames[chara] = kColdStorageWielderFrames;
        EncaseMember(chara);
    }
    PlayIceFreeze();
    LogEvent("stop", std::format("\"chara\":{},\"monsters\":{},\"wielder\":{}", chara, stopped, frozen));
}

bool ColdStorageHitRefused(const COLLISION_HIT &hit) {
    return UserStatus != nullptr && hit.owner == UserStatus->cur_chara && PlayerFrozen() && WeaponActive(ITEM_WEAPON_FROZEN_TUNA);
}

void ColdStorageDraw() {
    IcePrisonsDraw();
}

void ColdStorageCleared() {
    IcePrisonsClear();
}

void ColdStorageVisitBegins() {
    goro_pool = Pool{};
    sphere_pool = Pool{};
}

} // namespace expanded
