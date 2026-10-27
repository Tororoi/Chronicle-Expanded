#include "expanded/status/confuse.hpp"

#include <libvu0.h>

#include "collisiondata.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "hitmark.hpp"
#include "hitvalue.hpp"
#include "monstorunit.hpp"
#include "snd.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

namespace expanded {

namespace {

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

    if (record->knockback_mode == 2) {
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

int ConfuseMonsterHit(CMonstorUnit &unit, int sphere) {
    int slot = unit.current_monster;
    int hit = ConfuseFriendlyHit(unit, slot, unit.effect[slot].position[sphere], unit.effect[slot].radius[sphere]);
    return hit == -1 ? -1 : FriendlyHit(unit, sphere, hit);
}

} // namespace expanded
