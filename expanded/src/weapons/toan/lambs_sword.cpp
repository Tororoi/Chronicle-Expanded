#include "expanded/weapons/toan/lambs_sword.hpp"

#include <cmath>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/status/confuse_state.hpp"
#include "expanded/weapons/stats.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/wear.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

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

bool PackTarget(CMonstorUnit &unit, int slot, float *position) {
    if (!PackWolf(unit, slot) || !confuse::InPlay(unit, slot)) {
        return false;
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
    if (best >= 0) {
        MonsterAimPoint(unit, best, self[0], self[2], position);
        return true;
    }
    // Heel: kPackHeel from the player, on the wolf's side.
    float dx = self[0] - CharaMain.pos[0], dz = self[2] - CharaMain.pos[2];
    float length = std::hypot(dx, dz);
    if (length < 0.01f) {
        dx = 0.0f, dz = 1.0f, length = 1.0f;
    }
    position[0] = CharaMain.pos[0] + dx / length * kPackHeel;
    position[1] = CharaMain.pos[1];
    position[2] = CharaMain.pos[2] + dz / length * kPackHeel;
    position[3] = 1.0f;
    return true;
}

bool PackHitPassesBy(const COLLISION_HIT &hit) {
    return NowMonstorUnit != nullptr && PackWolf(*NowMonstorUnit, confuse::Attacker(hit));
}

void LambGuardBlocked(bool blocked, const COLLISION_HIT &hit) {
    int attacker = confuse::Attacker(hit);
    if (!blocked || attacker < 0 || NowMonstorUnit == nullptr || !Wolf(*NowMonstorUnit, attacker) || !PackLeaderLamb()) {
        return;
    }
    WearWeapon(kSwingWear);
}

} // namespace expanded
