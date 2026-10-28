#include "expanded/weapons/ungaga/terra_sword/shockwave.hpp"

#include <algorithm>
#include <cstdio>

#include "bt_shot_effect.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/weapons/ungaga/terra_sword.hpp"
#include "shot_effect.hpp"

namespace expanded::terra_sword {

namespace {

// syougekiha.chr: its one clip playing once where it is put, no flight and no hit.
// clang-format off
BT_SHOT_EFFECT shock = {"syougekiha", 0, 0, {}, {}, -1, 0, 0, 3, kHitsMonsters, {0, -1, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

struct Burst {
    CSHOT_EFFECT *effect = nullptr;
    int           slot = -1;
    float         spot[3] = {};
};

Burst burst;

bool Live() {
    return burst.effect != nullptr && burst.slot >= 0 && burst.effect->effect_data == &shock && burst.effect->active[burst.slot] != 0;
}

void Hold() {
    CCharacter &model = burst.effect->chara[burst.slot];
    model.SetPosition(burst.spot[0], burst.spot[1], burst.spot[2]);
    model.SetScale(kShockScale, kShockScale, kShockScale);
    model.motion_speed = kShockRate;
}

} // namespace

BT_SHOT_EFFECT *ShockwaveEffect() {
    return &shock;
}

bool ShockwavePlay(const float *spot) {
    ShockwaveStop();
    int index = ShotEffectIndex(&shock);
    if (index < 0) {
        std::fprintf(stderr, "big rock: the shockwave is not loaded on this floor\n");
        return false;
    }
    CSHOT_EFFECT *effect = ShotEffectAt(index);
    int           slot = SpawnStill(*effect, spot);
    if (slot < 0) {
        return false;
    }
    burst.effect = effect;
    burst.slot = slot;
    std::copy(spot, spot + 3, burst.spot);
    Hold();
    return true;
}

void ShockwaveStep() {
    if (burst.slot < 0) {
        return;
    }
    if (!Live()) {
        burst = Burst{};
        return;
    }
    Hold();
}

void ShockwaveStop() {
    if (Live()) {
        burst.effect->active[burst.slot] = 0;
    }
    burst = Burst{};
}

bool ShockwavePlaying() {
    return Live();
}

void ShockwaveReset() {
    burst = Burst{};
}

} // namespace expanded::terra_sword
