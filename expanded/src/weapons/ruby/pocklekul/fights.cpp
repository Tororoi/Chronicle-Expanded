#include "expanded/weapons/ruby/pocklekul/fights.hpp"

#include <libvu0.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <iterator>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/status/confuse_state.hpp"
#include "expanded/weapons/weapon_hit.hpp"
#include "hitmark.hpp"
#include "hitvalue.hpp"
#include "mathutil.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "snd.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

namespace expanded::pocklekul {

namespace {

// A planted hit lasts into the floor's next step, whose monster takes it; free records it leaves the game.
constexpr int kPlantedLife = 2;
constexpr int kHitReserve = 16;

// The mark a hit planted for it carries, with the monster it is planted on in its low half (COLLISION_HIT::attribute,
// which the game never reads).
constexpr int kOwnHitMark = 0x504B0000; // "PK"
constexpr int kMarkMask = static_cast<int>(0xFFFF0000u);

bool Marked(const COLLISION_HIT &hit) {
    return (hit.attribute & kMarkMask) == kOwnHitMark;
}

struct Fights {
    int  clock = 0;
    int  on[kFloorMonsters];            // the frame a hit of its was last planted on each monster
    int  by[kFloorMonsters];            // the frame each monster's hit last reached it
    bool provoked[kFloorMonsters] = {}; // going after it
    bool felled[kFloorMonsters] = {};   // killed by its hit, its ABS not yet logged

    Fights() {
        std::fill(std::begin(on), std::end(on), -kPockleContactFrames);
        std::fill(std::begin(by), std::end(by), -kPockleContactFrames);
    }
};

Fights fights;

int FreeRecords() {
    int free = 0;
    for (int active : NowColData->active) {
        free += active == 0 ? 1 : 0;
    }
    return free;
}

// CheckDmg's ranged distance factor for a hit of the summoner's at at (Xiao's, Ruby's and Osmond's).
float RangedFactor(const float *at) {
    int owner = UserStatus != nullptr ? UserStatus->cur_chara : -1;
    if (owner != CHARA_XIAO && owner != CHARA_RUBY && owner != CHARA_OSMOND) {
        return 1.0f;
    }
    sceVu0FVECTOR player, point = {at[0], at[1], at[2], 1.0f};
    sceVu0CopyVector(player, CharaMain.pos);
    float distance = DistVector(player, point);
    if (distance <= 20.0f) {
        return 1.5f;
    }
    if (!(distance < 50.0f)) {
        return std::max(0.5f, (100.0f - (distance - 50.0f)) / 100.0f);
    }
    return 1.0f;
}

// A hit of the summoner's planted on victim's body for damage before the ranged factor, with its reaction and push.
bool Plant(CMonstorUnit &floor, int victim, int damage, int reaction, const COLLISION_HIT *push) {
    if (NowColData == nullptr || FreeRecords() <= kHitReserve) {
        return false;
    }
    float centre[4];
    MonsterBodyCentre(floor, victim, centre);
    int            planted = std::max(1, static_cast<int>(std::nearbyint(static_cast<float>(damage) / RangedFactor(centre))));
    COLLISION_HIT *record = PlantWeaponHit(centre, planted, kPlantedLife, kPockleHitRadius);
    if (record == nullptr) {
        return false;
    }
    record->kind = reaction;
    record->attribute = kOwnHitMark | victim;
    if (push != nullptr) {
        sceVu0CopyVector(record->knockback_origin, const_cast<float *>(push->knockback_origin));
        record->knockback_speed = push->knockback_speed;
        record->knockback_decay = push->knockback_decay;
        record->knockback_mode = push->knockback_mode;
    }
    fights.on[victim] = fights.clock;
    LogEvent("pockle", std::format("\"step\":\"hit\",\"monster\":{},\"damage\":{}", victim, damage));
    return true;
}

// Its money, what the monster would drop, now the pockle's.
void Steal(CMonstorUnit &floor, CMonstorUnit &ally, int victim) {
    MONSTOR &from = floor.monster[victim];
    if (from.money <= 0) {
        return;
    }
    MONSTOR &pockle = ally.monster[kAllyMonster];
    pockle.stolen_money += from.money;
    LogEvent("pockle", std::format("\"step\":\"steal\",\"monster\":{},\"gold\":{},\"carried\":{}", victim, from.money, pockle.stolen_money));
    from.money = 0;
    from.money_chance = 0;
}

void Provoke(int monster, const char *why) {
    CMonstorUnit *floor = AllyFloor();
    if (floor == nullptr || fights.provoked[monster] || !MonsterLiving(floor->monster[monster]) || confuse::Confused(*floor, monster)) {
        return;
    }
    fights.provoked[monster] = true;
    std::fprintf(stderr, "pocklekul: monster %d %s the pockle goes after it\n", monster, why);
    LogEvent("pockle", std::format("\"step\":\"provoked\",\"monster\":{}", monster));
}

void Release(int monster) {
    fights.provoked[monster] = false;
    std::fprintf(stderr, "pocklekul: monster %d back on the player\n", monster);
}

// A shot's element as CheckDmg takes one: a single element bit and no status, else none.
int ShotElement(int flags) {
    if ((flags & 0xFF00) != 0) {
        return WEAPON_ELEMENT_NONE;
    }
    switch (flags & 0x1F) {
        case HIT_ELEMENT_FIRE:
            return WEAPON_ELEMENT_FIRE;
        case HIT_ELEMENT_COLD:
            return WEAPON_ELEMENT_COLD;
        case HIT_ELEMENT_THUNDER:
            return WEAPON_ELEMENT_THUNDER;
        case HIT_ELEMENT_WIND:
            return WEAPON_ELEMENT_WIND;
        case HIT_ELEMENT_HOLY:
            return WEAPON_ELEMENT_HOLY;
        default:
            return WEAPON_ELEMENT_NONE;
    }
}

// Monster attacker's hit (record index) on the pockle's sphere, as CheckDmg takes one of nobody's (owner -1).
int Take(CMonstorUnit &unit, int sphere, int index, int attacker) {
    COLLISION_HIT        &hit = NowColData->hit[index];
    int                   slot = unit.current_monster;
    MONSTOR              &live = unit.monster[slot];
    MONSTOR_EFFECT_STATE &body = unit.effect[slot];
    bool                  shot = hit.monster_no >= 0;
    int                   element = shot ? ShotElement(hit.flags) : WEAPON_ELEMENT_NONE;
    int                   damage_in = hit.damage;
    fights.by[attacker] = fights.clock;
    sceVu0FVECTOR origin, from;
    sceVu0CopyVector(origin, hit.knockback_origin);
    float speed = hit.knockback_speed, decay = hit.knockback_decay;
    int   mode = hit.knockback_mode;
    if (shot) {
        ShotEffectStrikeHit(index);
        NowColData->active[index] = 0;
    }

    float time = unit.chara[slot][0].motion_type.state.time;
    for (int j = 0; j < 3; j++) {
        if (unit.guard[slot].active[j] != 0 && unit.guard[slot].motion_start[j] <= time && !(unit.guard[slot].motion_end[j] < time)) {
            sceVu0FVECTOR guard_direction = {0, 2.5f, 0, 1};
            HitMark[hitCnt].Set(body.position[sphere], guard_direction, HIT_MARK_GUARD, 0.8f, 0.005f, 0.02f, 1.3f, 32, live.ground_y);
            HitPointMark[hitCnt].Set(body.position[sphere]);
            SndSePlay(SE_HIT_BLOCKED, -1, 0);
            LogEvent("pockle", std::format("\"step\":\"guarded\",\"by\":{}", attacker));
            return MONSTER_DMG_NONE;
        }
    }

    body.hit_slot = sphere;
    body.hit_attributes = 0;
    live.last_hit_id = -1;
    live.hit_element = element;
    if (mode == kKnockbackFromOrigin) {
        unit.chara[slot][0].GetPosition(from);
        from[1] = 0;
        origin[1] = 0;
        sceVu0SubVector(live.knockback_direction, from, origin);
        sceVu0Normalize(live.knockback_direction, live.knockback_direction);
        live.knockback[0] = speed * live.knockback[2];
        live.knockback[1] = decay * live.knockback[2];
    }
    sceVu0FVECTOR hit_direction = {0, 1.1f, 0, 1};
    HitMark[hitCnt].Set(body.position[sphere], hit_direction, HIT_MARK_HIT, 0.5f, 0.01f, 0.02f, 1.0f, 32, live.ground_y);
    HitPointMark[hitCnt].Set(body.position[sphere]);
    hitCnt = hitCnt == 15 ? 0 : hitCnt + 1;

    int   immune = 0;
    float damage = static_cast<float>(damage_in) - static_cast<float>(live.defense);
    if (damage <= 0.0f) {
        damage = 1.0f;
    }
    if (element < WEAPON_ELEMENT_NONE) {
        float before = damage;
        damage *= 0.01f * static_cast<float>(live.attachment_weight[element]);
        if (damage <= 0.0f && !(before <= 0.0f)) {
            immune = 1;
        }
    }
    if (live.anger_timer > 0) {
        damage /= 2.0f;
    }
    damage *= 0.01f * static_cast<float>(live.item_damage_rate);
    if (damage <= 0.0f) {
        damage = 0;
    }
    int amount = static_cast<int>(damage);
    if (!(damage - static_cast<float>(amount) <= 0.0f)) {
        amount++;
    }

    live.hp -= amount;
    int result = MONSTER_DMG_HIT;
    if (live.hp <= 0) {
        live.last_attacker = -1;
        live.hp = 0;
        unit.alive_count--;
        result = MONSTER_DMG_KILLED;
        if (live.event_flag2 != -1) {
            live.event_flag2_pending = 1;
        }
    }
    live.palette_target[0] = 255;
    live.palette_target[1] = immune == 0 ? 0 : 255;
    live.palette_target[2] = immune == 0 ? 0 : 255;
    live.palette_cycles = 1;
    live.palette_step = 0.08f;
    live.palette_blend = 0;
    if (immune == 0) {
        HitValueEntry(NowHitValue, body.position[sphere], amount, HIT_VALUE_MONSTER, NULL);
        SndSePlay(SE_MONSTER_HIT, -1, 0);
    } else {
        HitValueEntry(NowHitValue, body.position[sphere], 0, HIT_VALUE_ZERO, NULL);
        SndSePlay(SE_HIT_NO_DAMAGE, -1, 0);
    }
    LogEvent("pockle", std::format("\"step\":\"hurt\",\"by\":{},\"{}\":{},\"amount\":{},\"life\":{}", attacker, shot ? "shot" : "swing",
                                   damage_in, amount, live.hp));
    return result;
}

} // namespace

void FightsBegin() {
    fights = Fights{};
}

void FightsEnd() {
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (fights.provoked[monster]) {
            Release(monster);
        }
    }
    fights = Fights{};
}

void FightsStep() {
    fights.clock++;
    CMonstorUnit *floor = AllyFloor();
    for (int monster = 0; floor != nullptr && monster < kFloorMonsters; monster++) {
        if (fights.provoked[monster] && (!MonsterLiving(floor->monster[monster]) || confuse::Confused(*floor, monster))) {
            Release(monster);
        }
    }
}

void PockleSwingSet(CMonstorUnit &unit, int hit) {
    if (hit < 0 || NowColData == nullptr) {
        return;
    }
    COLLISION_HIT &swing = NowColData->hit[hit];
    CMonstorUnit  *floor = AllyFloor();
    if (floor != nullptr && swing.damage > 0 && swing.radius > 0.0f) {
        for (int victim = 0; victim < kFloorMonsters; victim++) {
            if (!MonsterLiving(floor->monster[victim]) || MonsterBodyEdge(*floor, victim, swing.pos[0], swing.pos[2]) > swing.radius ||
                fights.clock - fights.on[victim] < kPockleContactFrames) {
                continue;
            }
            if (!Plant(*floor, victim, swing.damage, swing.kind, &swing)) {
                break;
            }
            if ((swing.flags & kPockleStealGold) != 0 && ModRoll(100) < kPockleStealPercent) {
                Steal(*floor, unit, victim);
            }
        }
    }
    // It strikes no one itself: not the summoner, and no one takes it for a floor monster's.
    swing.target_mask = 0;
    NowColData->active[hit] = 0;
}

int PockleHitOn(CMonstorUnit &unit, int sphere) {
    CMonstorUnit *floor = AllyFloor();
    if (floor == nullptr || NowColData == nullptr) {
        return -1;
    }
    int    slot = unit.current_monster;
    float *at = unit.effect[slot].position[sphere];
    float  radius = unit.effect[slot].radius[sphere];
    for (int i = 0; i < kHitRecords; i++) {
        COLLISION_HIT &hit = NowColData->hit[i];
        if (NowColData->active[i] == 0 || hit.phase != hit.ready_phase || (hit.target_mask & kHitsPlayer) == 0) {
            continue;
        }
        int attacker = confuse::Attacker(hit);
        if (attacker < 0 || !MonsterLiving(floor->monster[attacker]) || fights.clock - fights.by[attacker] < kPockleContactFrames ||
            DistVector(at, hit.pos) > radius + hit.radius) {
            continue;
        }
        return Take(unit, sphere, i, attacker);
    }
    return -1;
}

bool PockleRefuses(const COLLISION_HIT &hit) {
    return Marked(hit);
}

bool PockleHitMeantElsewhere(int monster, const COLLISION_HIT &hit) {
    return Marked(hit) && (hit.attribute & ~kMarkMask) != monster;
}

void PockleMonsterDamaged(int monster, const COLLISION_HIT &hit, int amount) {
    CMonstorUnit *floor = AllyFloor();
    if (!Marked(hit) || floor == nullptr || monster < 0 || monster >= kFloorMonsters) {
        return;
    }
    if (amount > 0) {
        Provoke(monster, "hit by");
    }
    if (floor->monster[monster].hp <= 0) {
        fights.felled[monster] = true;
        LogEvent("pockle", std::format("\"step\":\"felled\",\"monster\":{}", monster));
    }
}

void PockleShots() {
    CMonstorUnit *floor = AllyFloor();
    if (floor == nullptr || NowShotEffect == nullptr) {
        return;
    }
    for (int index = 0; index < ShotEffectCount(); index++) {
        CSHOT_EFFECT   &effect = *ShotEffectAt(index);
        BT_SHOT_EFFECT *data = effect.effect_data;
        for (int slot = 0; data != nullptr && slot < effect.slot_count && slot < 8; slot++) {
            if (effect.active[slot] == 0 || effect.user_id_2[slot] != kAllyShooter || effect.phase[slot] >= 2) {
                continue;
            }
            sceVu0FVECTOR at;
            effect.chara[slot].GetPosition(at);
            float reach = data->radius[effect.phase[slot]];
            int   victim = -1;
            for (int monster = 0; monster < kFloorMonsters && victim < 0; monster++) {
                const MONSTOR &live = floor->monster[monster];
                if (!MonsterLiving(live) || live.revealed == 0) {
                    continue;
                }
                const MONSTOR_EFFECT_STATE &body = floor->effect[monster];
                for (int s = 0; s < kMonsterSpheres; s++) {
                    if (body.timer[s] != 0 && DistVector(const_cast<float *>(body.position[s]), at) <= reach + body.radius[s]) {
                        victim = monster;
                        break;
                    }
                }
            }
            if (victim < 0) {
                continue;
            }
            if (!Plant(*floor, victim, effect.damage[slot], data->hit_kind, nullptr)) {
                return;
            }
            ShotEffectStrike(effect, slot, at);
        }
    }
}

void PockleMoveChecked(CMonstorUnit &unit, int monster) {
    CMonstorUnit *ally = AllyStanding();
    CMonstorUnit *floor = AllyFloor();
    if (ally == nullptr || floor == nullptr || (&unit != ally && &unit != floor)) {
        return;
    }
    CMonstorUnit &other = &unit == ally ? *floor : *ally;
    MONSTOR      &moving = unit.monster[monster];
    float         at[4];
    unit.chara[monster][0].GetPosition(at);
    float step[3] = {moving.movement[0] * moving.movement_speed, moving.movement[1] * moving.movement_speed,
                     moving.movement[2] * moving.movement_speed};
    float next[3] = {at[0] + step[0], at[1] + step[1], at[2] + step[2]};
    for (int i = 0; i < kFloorMonsters; i++) {
        const MONSTOR &standing = other.monster[i];
        if (standing.state != 2) {
            continue;
        }
        float there[4];
        other.chara[i][0].GetPosition(there);
        float dx = there[0] - next[0], dz = there[2] - next[2];
        float reach = standing.collision_radius + moving.collision_radius;
        if (reach * reach < dx * dx + dz * dz || !(next[1] < there[1] + 2.0f * standing.collision_radius) ||
            !((there[0] - at[0]) * step[0] + (there[2] - at[2]) * step[2] > 0.0f)) {
            continue;
        }
        moving.movement[0] = moving.movement[1] = moving.movement[2] = 0.0f;
        moving.movement_speed = 0.0f;
        if (&unit == floor) {
            Provoke(monster, "stopped by");
        }
        return;
    }
}

bool PockleProvoked(int monster, float *position) {
    CMonstorUnit *ally = AllyStanding();
    if (ally == nullptr || monster < 0 || monster >= kFloorMonsters || !fights.provoked[monster]) {
        return false;
    }
    ally->chara[kAllyMonster][0].GetPosition(position);
    return true;
}

void PockleKillAbs(int monster, int abs) {
    if (monster < 0 || monster >= kFloorMonsters || !fights.felled[monster]) {
        return;
    }
    fights.felled[monster] = false;
    LogEvent("pockle", std::format("\"step\":\"abs\",\"monster\":{},\"abs\":{}", monster, abs));
}

} // namespace expanded::pocklekul
