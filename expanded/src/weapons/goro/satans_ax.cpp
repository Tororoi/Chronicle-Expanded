#include "expanded/weapons/goro/satans_ax.hpp"

#include <algorithm>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "charaheight.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_shot.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kChargeAction = 13; // GoroKey_Play's charge, held
constexpr int kSmashAction = 14;  // and his smash, his full charge let go

// The pull a monster's step last took, and the step it left (its movement times its speed): taken back off before the
// next unless its script has set another since, so it draws the monster on without gathering speed.
struct Pulled {
    bool  on = false;
    float pull[3] = {};
    float left[3] = {};
};

Pulled pulled[kFloorMonsters];

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(UnderworldReset), true);

void Step(const MONSTOR &live, float *step) {
    for (int i = 0; i < 3; i++) {
        step[i] = live.movement[i] * live.movement_speed;
    }
}

void SetStep(MONSTOR &live, const float *step) {
    float speed = std::sqrt(step[0] * step[0] + step[1] * step[1] + step[2] * step[2]);
    if (speed < 1e-4f) {
        live.movement_speed = 0.0f;
        return;
    }
    for (int i = 0; i < 3; i++) {
        live.movement[i] = step[i] / speed;
    }
    live.movement_speed = speed;
}

} // namespace

float UnderworldPullAt(float distance) {
    if (distance > kUnderworldReach) {
        return 0.0f;
    }
    return kUnderworldPullNear * std::exp(-std::max(distance, 0.0f) / kUnderworldFalloff);
}

float UnderworldStrength() {
    if (UnderworldShotAt(nullptr)) {
        return 1.0f;
    }
    if (!Playing(CHARA_GORO) || !WeaponWielded(ITEM_WEAPON_SATAN_S_AX) || BtActStatus.action_on == 0) {
        return 0.0f;
    }
    if (BtActStatus.action_no == kSmashAction) {
        // Until it hits the ground, not until he can act again.
        return CharaMain.motion_type.state.time < kUnderworldSmashLands ? 1.0f : 0.0f;
    }
    return BtActStatus.action_no == kChargeAction ? kUnderworldChargeShare : 0.0f;
}

bool UnderworldCentreNow(UnderworldCentre &centre) {
    float at[3];
    if (UnderworldShotAt(at)) {
        centre = {.x = at[0], .z = at[2], .ground = CharaMain.pos[1], .low = at[1], .high = at[1], .body = 0.0f, .pull = kUnderworldShotPull};
        return true;
    }
    if (UnderworldStrength() <= 0.0f) {
        return false;
    }
    float top = UserStatus != nullptr ? CharaHeight(UserStatus) : 16.0f;
    centre = {.x = CharaMain.pos[0],
              .z = CharaMain.pos[2],
              .ground = CharaMain.pos[1],
              .low = CharaMain.pos[1],
              .high = CharaMain.pos[1] + top,
              .body = ColossusBlockRadius(kColossusBodyRadius)};
    return true;
}

void UnderworldMonsterMove(CMonstorUnit &unit, int monster) {
    if (monster < 0 || monster >= kFloorMonsters) {
        return;
    }
    MONSTOR &live = unit.monster[monster];
    Pulled  &last = pulled[monster];
    float    step[3];
    Step(live, step);
    if (last.on && std::fabs(step[0] - last.left[0]) < 1e-4f && std::fabs(step[1] - last.left[1]) < 1e-4f &&
        std::fabs(step[2] - last.left[2]) < 1e-4f) {
        for (int i = 0; i < 3; i++) {
            step[i] -= last.pull[i];
        }
        SetStep(live, step);
    }
    last.on = false;
    float            weight = live.knockback[2] * UnderworldStrength();
    UnderworldCentre centre;
    if (weight <= 0.0f || !MonsterInPlay(live) || !UnderworldCentreNow(centre)) {
        return;
    }
    float position[4];
    unit.chara[monster][0].GetPosition(position);
    float dx = centre.x - position[0], dz = centre.z - position[2];
    float distance = std::hypot(dx, dz);
    // As far as its body is from what it is drawn to: no nearer than where it meets it.
    float room = MonsterBodyEdge(unit, monster, centre.x, centre.z) - centre.body;
    float pull = std::min(UnderworldPullAt(distance) * weight * centre.pull, room);
    if (pull <= 0.0f || distance < 0.01f) {
        return;
    }
    last.pull[0] = dx / distance * pull;
    last.pull[1] = 0.0f;
    last.pull[2] = dz / distance * pull;
    for (int i = 0; i < 3; i++) {
        step[i] += last.pull[i];
    }
    SetStep(live, step);
    Step(live, last.left);
    last.on = true;
}

bool UnderworldPulling(int monster) {
    return monster >= 0 && monster < kFloorMonsters && pulled[monster].on;
}

void UnderworldReset() {
    std::fill(std::begin(pulled), std::end(pulled), Pulled{});
}

} // namespace expanded
