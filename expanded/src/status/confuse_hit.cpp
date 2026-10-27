#include <libvu0.h>

#include <cstdio>
#include <format>

#include "collisiondata.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/status/confuse.hpp"
#include "expanded/status/confuse_state.hpp"
#include "hitmark.hpp"
#include "hitvalue.hpp"
#include "mathutil.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "snd.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

namespace expanded {

namespace {

using namespace confuse;

// How often the same attacker may hurt the same monster.
constexpr int kContactSteps = kStepsPerSecond / 2;

// Deals another monster's hit to sphere i: blocked by a guard window the monster can guard with, else damage less defence
// (at least 1).
int FriendlyHit(CMonstorUnit &unit, int i, int hit) {
    int            slot = unit.current_monster;
    MONSTOR       &monster = unit.monster[slot];
    COLLISION_HIT *record = NowColData->Get(hit);
    ConfuseLanded(unit, hit, slot);

    for (int j = 0; j < 3; j++) {
        float time = unit.chara[slot][0].motion_type.state.time;

        if (MonsterCanGuard(slot) && unit.guard[slot].active[j] != 0 && unit.guard[slot].motion_start[j] <= time &&
            !(unit.guard[slot].motion_end[j] < time)) {
            sceVu0FVECTOR guard_direction = {0, 2.5f, 0, 1};
            HitMark[hitCnt].Set(unit.effect[slot].position[i], guard_direction, HIT_MARK_GUARD, 0.8f, 0.005f, 0.02f, 1.3f, 32, monster.ground_y);
            HitPointMark[hitCnt].Set(unit.effect[slot].position[i]);
            SndSePlay(SE_HIT_BLOCKED, -1, 0);
            return MONSTER_DMG_NONE;
        }
    }

    int result = MONSTER_DMG_HIT;
    unit.effect[slot].hit_slot = i;
    unit.effect[slot].hit_attributes = record->weapon_flags;
    monster.last_hit_id = -1;
    monster.hit_element = WEAPON_ELEMENT_NONE;

    if (record->knockback_mode == kKnockbackFromOrigin) {
        sceVu0FVECTOR knockback_position, knockback_origin;
        unit.chara[slot][0].GetPosition(knockback_position);
        sceVu0CopyVector(knockback_origin, record->knockback_origin);
        knockback_position[1] = 0;
        knockback_origin[1] = 0;
        sceVu0SubVector(monster.knockback_direction, knockback_position, knockback_origin);
        sceVu0Normalize(monster.knockback_direction, monster.knockback_direction);
        monster.knockback[0] = record->knockback_speed * monster.knockback[2];
        monster.knockback[1] = record->knockback_decay * monster.knockback[2];
    }

    sceVu0FVECTOR hit_direction = {0, 1.1f, 0, 1};
    HitMark[hitCnt].Set(unit.effect[slot].position[i], hit_direction, HIT_MARK_HIT, 0.5f, 0.01f, 0.02f, 1.0f, 32, monster.ground_y);
    HitPointMark[hitCnt].Set(unit.effect[slot].position[i]);
    hitCnt = hitCnt == 15 ? 0 : hitCnt + 1;

    float damage = (float) record->damage - (float) monster.defense;

    if (damage <= 0.0f) {
        damage = 1.0f;
    }

    damage *= 0.01f * (float) monster.item_damage_rate;

    if (monster.anger_timer > 0) {
        damage /= 2.0f;
    }

    if (damage <= 0.0f) {
        damage = 0;
    }

    int amount = (int) damage;

    if (!(damage - (float) amount <= 0.0f)) {
        amount++;
    }

    monster.hp -= amount;
    LogEvent("confuse", std::format("\"step\":\"hit\",\"by\":{},\"monster\":{},\"amount\":{},\"life\":{}", Attacker(*record), slot, amount, monster.hp));

    if (monster.hp <= 0) {
        monster.last_attacker = -1;
        monster.hp = 0;
        unit.alive_count--;
        result = MONSTER_DMG_KILLED;
        ((CDngStatusData *) UserStatus)->AddKills();

        if (monster.event_flag2 != -1) {
            monster.event_flag2_pending = 1;
        }
    }

    monster.palette_target[0] = 255;
    monster.palette_target[1] = amount > 0 ? 0 : 255;
    monster.palette_target[2] = amount > 0 ? 0 : 255;
    monster.palette_cycles = 1;
    monster.palette_step = 0.08f;
    monster.palette_blend = 0;

    if (amount > 0) {
        HitValueEntry(NowHitValue, unit.effect[slot].position[i], amount, HIT_VALUE_MONSTER, NULL);
        SndSePlay(SE_MONSTER_HIT, -1, 0);
    } else {
        HitValueEntry(NowHitValue, unit.effect[slot].position[i], 0, HIT_VALUE_ZERO, NULL);
        SndSePlay(SE_HIT_NO_DAMAGE, -1, 0);
    }

    return result;
}

} // namespace

int ConfuseFriendlyHit(CMonstorUnit &unit, int victim, const float *position, float radius) {
    Confusion &state = State();
    if (state.unit != &unit || NowColData == nullptr) {
        return -1;
    }
    for (int i = 0; i < kHitRecords; i++) {
        const COLLISION_HIT &hit = NowColData->hit[i];
        if (NowColData->active[i] == 0 || hit.phase != hit.ready_phase) {
            continue;
        }
        int attacker = Attacker(hit);
        if (attacker < 0 || attacker == victim || !InPlay(unit, attacker)) {
            continue;
        }
        // A confused monster hurts any other; a provoked one only the monster it goes after.
        if (!Confused(unit, attacker) && Provoker(unit, attacker) != victim) {
            continue;
        }
        if (state.contact[attacker][victim] != 0 && state.tick - state.contact[attacker][victim] < kContactSteps) {
            continue;
        }
        sceVu0FVECTOR at;
        sceVu0CopyVector(at, const_cast<float *>(position));
        if (DistVector(at, const_cast<float *>(hit.pos)) <= radius + hit.radius) {
            return i;
        }
    }
    return -1;
}

void ConfuseLanded(CMonstorUnit &unit, int hit, int victim) {
    Confusion &state = State();
    int        attacker = Attacker(NowColData->hit[hit]);
    if (attacker < 0) {
        return;
    }
    state.contact[attacker][victim] = state.tick;
    if (NowColData->hit[hit].monster_no >= 0) {
        std::fprintf(stderr, "confuse: monster %d's shot hits monster %d\n", attacker, victim);
        ShotEffectStrikeHit(hit);
        NowColData->active[hit] = 0;
    } else {
        std::fprintf(stderr, "confuse: monster %d hits monster %d\n", attacker, victim);
    }
    if (!Confused(unit, attacker)) {
        return;
    }
    // The victim remembers it, most recent first.
    int *list = state.hit_by[victim];
    if (list[0] == attacker) {
        return;
    }
    int end = 0;
    while (end < kFloorMonsters - 1 && list[end] >= 0 && list[end] != attacker) {
        end++;
    }
    for (int i = end; i > 0; i--) {
        list[i] = list[i - 1];
    }
    list[0] = attacker;
    if (!Confused(unit, victim) && Provoker(unit, victim) == attacker) {
        std::fprintf(stderr, "confuse: monster %d turns on monster %d\n", victim, attacker);
    }
}

int ConfuseMonsterHit(CMonstorUnit &unit, int sphere) {
    int slot = unit.current_monster;
    int hit = ConfuseFriendlyHit(unit, slot, unit.effect[slot].position[sphere], unit.effect[slot].radius[sphere]);
    return hit == -1 ? -1 : FriendlyHit(unit, sphere, hit);
}

} // namespace expanded
