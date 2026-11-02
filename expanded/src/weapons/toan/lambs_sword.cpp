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
    // None: far past its place by the player (the player, before it has one) on the line there, rise and all (its
    // script's step checks follow that line over the floor), so its script walks on rather than arriving and striking;
    // PackStep holds it once it is there.
    const float *place = PackPlace(slot);
    float        to[3] = {place != nullptr ? place[0] : CharaMain.pos[0], CharaMain.pos[1], place != nullptr ? place[1] : CharaMain.pos[2]};
    for (int i = 0; i < 3; i++) {
        position[i] = self[i] + kPackWalkOn * (to[i] - self[i]);
    }
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

constexpr int kIdleClip = 0; // every wolf's

struct Pack {
    const CMonstorUnit *unit = nullptr;
    bool                parked[kFloorMonsters] = {};
    bool                placed[kFloorMonsters] = {};
    float               place[kFloorMonsters][2] = {};
    float               held_at[kFloorMonsters][2] = {}; // its place when it began to wait
    float               last[kFloorMonsters][2] = {};    // where it last made headway, and the frames since
    int                 stalled[kFloorMonsters] = {};
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

// A wolf at its place: its script waiting, idling there turned the way the player faces (its own turn).
void Hold(CMonstorUnit &unit, int slot) {
    Park(unit, slot);
    CCharacter &body = unit.chara[slot][0];
    Clip(body, kIdleClip, -1.0f);
    float at[4];
    body.GetPosition(at);
    float    yaw = PlayerYaw();
    MONSTOR &live = unit.monster[slot];
    live.turn_target[0] = at[0] + std::sin(yaw) * 10.0f;
    live.turn_target[1] = at[1];
    live.turn_target[2] = at[2] + std::cos(yaw) * 10.0f;
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
    std::vector<int> following;
    for (int slot = 0; slot < kFloorMonsters; slot++) {
        bool wolf = PackWolf(*unit, slot) && confuse::InPlay(*unit, slot) && unit->monster[slot].hp > 0;
        pack.placed[slot] = false;
        if (wolf && PackPrey(*unit, slot) < 0) {
            following.push_back(slot);
        } else if (pack.parked[slot]) {
            Release(*unit, slot);
        }
    }
    // Each its place kPackHeel from the player on its own side of him (none crosses him to get there), the pack fanned
    // at least kPackSpread apart round him.
    std::vector<std::pair<float, int>> bearings;
    for (int slot : following) {
        float at[4];
        unit->chara[slot][0].GetPosition(at);
        bearings.emplace_back(std::atan2(at[0] - CharaMain.pos[0], at[2] - CharaMain.pos[2]), slot);
    }
    std::sort(bearings.begin(), bearings.end());
    // Round the circle from its widest gap, so neighbours across the turn of the angle count as neighbours.
    std::size_t widest = 0;
    float       widest_gap = 0.0f;
    for (std::size_t k = 0; k < bearings.size(); k++) {
        float next = k + 1 < bearings.size() ? bearings[k + 1].first : bearings[0].first + 2.0f * 3.14159265f;
        if (next - bearings[k].first > widest_gap) {
            widest_gap = next - bearings[k].first;
            widest = k + 1;
        }
    }
    for (std::size_t k = 0; k < widest && widest < bearings.size(); k++) {
        bearings[k].first += 2.0f * 3.14159265f;
    }
    std::rotate(bearings.begin(), bearings.begin() + (widest % std::max<std::size_t>(bearings.size(), 1)), bearings.end());
    for (int pass = 0; pass < 8 && bearings.size() > 1; pass++) {
        for (std::size_t k = 0; k + 1 < bearings.size(); k++) {
            float short_by = kPackSpread - (bearings[k + 1].first - bearings[k].first);
            if (short_by > 0.0f) {
                bearings[k].first -= short_by / 2.0f;
                bearings[k + 1].first += short_by / 2.0f;
            }
        }
    }
    for (auto [angle, slot] : bearings) {
        pack.place[slot][0] = CharaMain.pos[0] + std::sin(angle) * kPackHeel;
        pack.place[slot][1] = CharaMain.pos[2] + std::cos(angle) * kPackHeel;
        pack.placed[slot] = true;
        float at[4];
        unit->chara[slot][0].GetPosition(at);
        float off = std::hypot(pack.place[slot][0] - at[0], pack.place[slot][1] - at[2]);
        if (std::hypot(at[0] - pack.last[slot][0], at[2] - pack.last[slot][1]) > kPackHeadway) {
            pack.last[slot][0] = at[0];
            pack.last[slot][1] = at[2];
            pack.stalled[slot] = 0;
        } else if (!pack.parked[slot]) {
            pack.stalled[slot]++;
        }
        if (pack.parked[slot] && std::hypot(pack.place[slot][0] - pack.held_at[slot][0], pack.place[slot][1] - pack.held_at[slot][1]) > kPackRestart) {
            Release(*unit, slot); // the player moved on: its script walks it there again
            pack.stalled[slot] = 0;
        } else if (!pack.parked[slot] && (off < kPackArrived || pack.stalled[slot] >= kPackStalledFrames)) {
            pack.parked[slot] = true;
            pack.held_at[slot][0] = pack.place[slot][0];
            pack.held_at[slot][1] = pack.place[slot][1];
            pack.script[slot] = unit->interpreter[slot];
            pack.running[slot] = unit->script_state[slot];
        }
        if (pack.parked[slot]) {
            Hold(*unit, slot);
        }
    }
}

bool PackParked(int slot) {
    return slot >= 0 && slot < kFloorMonsters && pack.parked[slot];
}

const float *PackPlace(int slot) {
    return slot >= 0 && slot < kFloorMonsters && pack.placed[slot] ? pack.place[slot] : nullptr;
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
