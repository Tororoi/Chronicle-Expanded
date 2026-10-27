#include "expanded/weapons/xiao/angel_gear/shield_ring.hpp"
#include "sound_effect.h"

#include <cmath>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "hitmark.hpp"
#include "monstorunit.hpp"
#include "snd.hpp"

extern CHitPointMark MyHitPointMark[16];

namespace expanded::angel_gear {

namespace {

constexpr int kHurtsPlayer = 1;

struct Ring {
    bool up = false;
    int  strike_wait = 0; // frames left in which a swing is spent silently
};

Ring ring;

void OnFloor() {
    RingReset();
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

} // namespace

void RingUp(bool up) {
    ring.up = up;
}

bool RingIsUp() {
    return ring.up;
}

bool SpendOnShield(bool quiet) {
    if (NowColData == nullptr || ring.strike_wait > 0) {
        return false; // a strike's wait: no swing is looked at
    }
    float root[3];
    PropRoot(root);
    for (int i = 0; i < kHitRecords; i++) {
        COLLISION_HIT &hit = NowColData->hit[i];
        if (NowColData->active[i] == 0 || (hit.target_mask & kHurtsPlayer) == 0 || hit.phase != hit.ready_phase) {
            continue;
        }
        float dx = hit.pos[0] - root[0], dz = hit.pos[2] - root[2];
        float reach = hit.radius + kPropHitRadius;
        if (dx * dx + dz * dz > reach * reach || hit.pos[1] + hit.radius < root[1] - kPropHitBelow || hit.pos[1] - hit.radius > root[1] + kPropHitHeight) {
            continue;
        }
        NowColData->active[i] = 0; // spent on the shield
        if (hit.scale_rate != 0.0f || quiet) {
            continue; // a shot's blast, or a swing while it clears
        }
        // The guard's spark where it touched (its surface toward the grip) and the guard's clink.
        float to[3] = {root[0] - hit.pos[0], root[1] - hit.pos[1], root[2] - hit.pos[2]};
        float length = std::sqrt(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
        float at[4] = {hit.pos[0], hit.pos[1], hit.pos[2], 1.0f};
        if (length > 1e-3f) {
            for (int c = 0; c < 3; c++) {
                at[c] += to[c] / length * hit.radius;
            }
        }
        MyHitPointMark[0].Set(at);
        SndSePlay(SE_HIT_BLOCKED, -1, 0);
        ring.strike_wait = kStrikeFrames;
        return true;
    }
    return false;
}

void RingStep() {
    if (ring.strike_wait > 0) {
        ring.strike_wait--;
    }
}

void RingReset() {
    ring = Ring{};
}

bool RingTarget(int monster, float *position) {
    if (!ring.up || NowMonstorUnit == nullptr || monster < 0 || monster >= kFloorMonsters) {
        return false;
    }
    sceVu0CopyVector(position, CharaMain.pos);
    const MONSTOR &live = NowMonstorUnit->monster[monster];
    if (live.state < 1 || live.hp <= 0) {
        return true; // not spawned, or without life
    }
    float at[4];
    NowMonstorUnit->chara[monster][0].GetPosition(at);
    float dx = at[0] - CharaMain.pos[0], dz = at[2] - CharaMain.pos[2];
    float d = std::sqrt(dx * dx + dz * dz);
    if (d > kRingRadius) { // past the shield: its own point of the ring
        position[0] = CharaMain.pos[0] + dx / d * kRingRadius;
        position[2] = CharaMain.pos[2] + dz / d * kRingRadius;
    }
    return true;
}

float RingBlockRadius(float radius) {
    return ring.up ? kRingRadius : radius;
}

} // namespace expanded::angel_gear
