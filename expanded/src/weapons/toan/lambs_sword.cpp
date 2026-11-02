#include "expanded/weapons/toan/lambs_sword.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/wolf_steve.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/status/confuse_state.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/stats.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/wear.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"

namespace expanded {

namespace {

// The weapon in play's form: wolf at the threshold of its WHP or below.
bool WolfForm() {
    const WEAPON_HAVE *weapon = NowWeaponHave;
    return weapon != nullptr && weapon->durability_f <= kLambSwordThreshold * weapon->durability;
}

bool Active() {
    return WeaponActive(ITEM_WEAPON_LAMB_S_SWORD);
}

} // namespace

bool PackLeaderWolf() {
    return Active() && WolfForm();
}

bool PackLeaderLamb() {
    return Active() && !WolfForm();
}

bool Wolf(const CMonstorUnit &unit, int slot) {
    int kind = ModelKind(unit.monster[slot].base_model);
    return kind == No(Monster::WEREWOLF) || kind == No(Monster::WHITE_FANG) || kind == No(Monster::WHITE_FANG_ENHANCED);
}

bool PackWolf(const CMonstorUnit &unit, int slot) {
    return slot >= 0 && slot < kFloorMonsters && !AllyUnit(unit) && Wolf(unit, slot) && PackLeaderWolf();
}

int PackPrey(CMonstorUnit &unit, int slot) {
    if (!PackWolf(unit, slot) || !confuse::InPlay(unit, slot)) {
        return -1;
    }
    float self[4];
    confuse::Position(unit, slot, self);
    int   best = -1;
    float best_distance = kPackReach;
    for (int i = 0; i < kFloorMonsters; i++) {
        if (i == slot || !confuse::InPlay(unit, i) || PackWolf(unit, i)) {
            continue;
        }
        float other[4];
        confuse::Position(unit, i, other);
        float distance = std::hypot(other[0] - self[0], other[2] - self[2]);
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return best;
}

bool PackTarget(CMonstorUnit &unit, int slot, float *position) {
    if (!PackWolf(unit, slot) || !confuse::InPlay(unit, slot)) {
        return false;
    }
    float self[4];
    confuse::Position(unit, slot, self);
    if (int prey = PackPrey(unit, slot); prey >= 0) {
        MonsterAimPoint(unit, prey, self[0], self[2], position);
        return true;
    }
    // None: far past the player along its way to him, so its script walks on rather than arriving and striking.
    for (int i : {0, 2}) {
        position[i] = self[i] + kPackWalkOn * (CharaMain.pos[i] - self[i]);
    }
    position[1] = CharaMain.pos[1];
    position[3] = 1.0f;
    return true;
}

namespace {

// Where a parked wolf's script waits: resumed each step, it yields at once (a hit's or its fall's label still runs).
vmcode_t pack_wait[] = {
    {RS_OP_WAIT, 0, 0},
    {RS_OP_WAIT, 0, 0},
    {RS_OP_WAIT, 0, 0},
    {RS_OP_END,  0, 0}
};

constexpr int kIdleClip = 0, kWalkClip = 1; // every wolf's

struct Pack {
    const CMonstorUnit *unit = nullptr;
    bool                parked[kFloorMonsters] = {};
    int                 settling[kFloorMonsters] = {};
    // Each parked wolf's script as it was when it parked, given back as it goes again: a Werewolf's script started afresh
    // from its main label never strikes.
    CRunScript script[kFloorMonsters];
    int        running[kFloorMonsters] = {};
};

Pack pack;

void Park(CMonstorUnit &unit, int slot) {
    unit.interpreter[slot].pc = pack_wait;
    unit.script_state[slot] = 1;
    unit.monster[slot].movement_speed = 0.0f;
}

// Its script on from where it parked, its clips at their own rate.
void Release(CMonstorUnit &unit, int slot) {
    pack.parked[slot] = false;
    pack.settling[slot] = 0;
    unit.interpreter[slot] = pack.script[slot];
    unit.script_state[slot] = pack.running[slot];
    unit.chara[slot][0].motion_speed = -1.0f;
}

void Clip(CCharacter &body, int clip, float rate) {
    if (body.motion_no != clip) {
        body.motion_no = clip;
        body.motion_flags = 0;
    }
    body.motion_speed = rate;
}

// A parked wolf to its place: kPackHeel behind the player, the pack fanned kPackSpread apart by rank (rank of count),
// walked to (its walk clip, pace by how far it is) or idled at turned the way the player faces.
void Heel(CMonstorUnit &unit, int slot, int rank, int count) {
    CCharacter &body = unit.chara[slot][0];
    MONSTOR    &live = unit.monster[slot];
    float       yaw = PlayerYaw();
    float       angle = yaw + 3.14159265f + (float(rank) - float(count - 1) / 2.0f) * kPackSpread;
    float       place[2] = {CharaMain.pos[0] + std::sin(angle) * kPackHeel, CharaMain.pos[2] + std::cos(angle) * kPackHeel};
    float       at[4];
    body.GetPosition(at);
    float dx = place[0] - at[0], dz = place[1] - at[2];
    float distance = std::hypot(dx, dz);
    float face[2];
    if (!(kPackArrived < distance)) {
        Clip(body, kIdleClip, -1.0f);
        face[0] = at[0] + std::sin(yaw) * 10.0f;
        face[1] = at[2] + std::cos(yaw) * 10.0f;
    } else {
        float pace = std::clamp(distance * kPackPace, kPackMinPace, kPackMaxPace);
        float step = std::min(distance / 6.0f, pace);
        at[0] += dx * (step / distance);
        at[2] += dz * (step / distance);
        body.SetPosition(at);
        Clip(body, kWalkClip, pace * kPackWalkRate);
        face[0] = place[0];
        face[1] = place[1];
    }
    live.turn_target[0] = face[0];
    live.turn_target[1] = at[1];
    live.turn_target[2] = face[1];
    live.turn_speed = kPackTurnRate;
}

} // namespace

void PackStep(int pause) {
    CMonstorUnit *unit = NowMonstorUnit;
    if (unit == nullptr || AllyUnit(*unit)) {
        return;
    }
    if (pack.unit != unit) {
        pack = Pack{};
        pack.unit = unit;
    }
    if (pause != 0) {
        return;
    }
    std::vector<int> heeling;
    for (int slot = 0; slot < kFloorMonsters; slot++) {
        bool wolf = PackWolf(*unit, slot) && confuse::InPlay(*unit, slot);
        if (!wolf || unit->monster[slot].hp <= 0) {
            if (pack.parked[slot]) {
                Release(*unit, slot);
            }
            continue;
        }
        float at[4];
        unit->chara[slot][0].GetPosition(at);
        bool near = std::hypot(at[0] - CharaMain.pos[0], at[2] - CharaMain.pos[2]) < kPackLeash;
        if (PackPrey(*unit, slot) >= 0 || !near) {
            if (pack.parked[slot]) {
                Release(*unit, slot);
            }
            continue;
        }
        if (!pack.parked[slot]) {
            // A move of its own plays out first (its script walking on past the player).
            int clip = unit->chara[slot][0].motion_no;
            if (clip != kIdleClip && clip != kWalkClip && ++pack.settling[slot] < kPackSettle) {
                continue;
            }
            pack.parked[slot] = true;
            pack.settling[slot] = 0;
            pack.script[slot] = unit->interpreter[slot];
            pack.running[slot] = unit->script_state[slot];
        }
        heeling.push_back(slot);
    }
    for (std::size_t k = 0; k < heeling.size(); k++) {
        Park(*unit, heeling[k]);
        Heel(*unit, heeling[k], int(k), int(heeling.size()));
    }
}

bool PackParked(int slot) {
    return slot >= 0 && slot < kFloorMonsters && pack.parked[slot];
}

bool PackHitPassesBy(const COLLISION_HIT &hit) {
    return NowMonstorUnit != nullptr && PackWolf(*NowMonstorUnit, confuse::Attacker(hit));
}

bool LambWeapon(const WEAPON_HAVE &weapon) {
    return weapon.item_no == ITEM_WEAPON_LAMB_S_SWORD || (weapon.item_no == ITEM_WEAPON_SUPER_STEVE && CarriesWeapon(weapon, ITEM_WEAPON_LAMB_S_SWORD));
}

bool LambWolfForm(const WEAPON_HAVE &weapon) {
    return weapon.durability_f <= kLambSwordThreshold * weapon.durability;
}

void WolfSteveShow(CCharacter *chara, const WEAPON_HAVE &weapon) {
    if (chara == nullptr || chara->frame == nullptr || weapon.item_no != ITEM_WEAPON_SUPER_STEVE) {
        return;
    }
    bool wolf = LambWeapon(weapon) && LambWolfForm(weapon);
    for (auto [name, shown] : {
             std::pair{kSteveParts, !wolf},
             std::pair{kSteveHands, !wolf},
             std::pair{kWolfParts,  wolf },
             std::pair{kWolfHands,  wolf }
    }) {
        if (CFrame *frame = chara->frame->SearchFrame(const_cast<char *>(name))) {
            frame->attr.draw_on = shown ? 1 : 2;
        }
    }
}

void LambGuardBlocked(bool blocked, const COLLISION_HIT &hit) {
    int attacker = confuse::Attacker(hit);
    if (!blocked || attacker < 0 || NowMonstorUnit == nullptr || !Wolf(*NowMonstorUnit, attacker) || !PackLeaderLamb()) {
        return;
    }
    WearWeapon(kSwingWear * kLambGuardWear);
}

} // namespace expanded
