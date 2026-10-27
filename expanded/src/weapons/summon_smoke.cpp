#include "expanded/weapons/summon_smoke.hpp"

#include <algorithm>
#include <cstdio>

#include "bt_shot_effect.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "shot_effect.hpp"

namespace expanded {

namespace {

// e228ex.chr: its one clip playing once where it is put, no flight and no hit.
// clang-format off
BT_SHOT_EFFECT smoke = {"e228ex", 0, 0, {}, {}, -1, 0, 0, 3, kHitsMonsters, {0, -1, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

struct Puff {
    CSHOT_EFFECT *effect = nullptr;
    int           slot = -1;
    float         spot[3] = {};
    const void   *owner = nullptr;
};

Puff puff;

void OnFloor() {
    puff = Puff{};
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

bool Live() {
    return puff.effect != nullptr && puff.slot >= 0 && puff.effect->effect_data == &smoke && puff.effect->active[puff.slot] != 0;
}

void PutOut() {
    if (Live()) {
        puff.effect->active[puff.slot] = 0;
        FadeUnlitFrames(puff.effect->chara[puff.slot].frame, 1.0f); // handed back at full
    }
    puff = Puff{};
}

} // namespace

BT_SHOT_EFFECT *SummonSmokeEffect() {
    return &smoke;
}

bool SummonSmokeStart(const float *spot, float yaw, float scale, const void *owner) {
    PutOut();
    int index = ShotEffectIndex(&smoke);
    if (index < 0) {
        std::fprintf(stderr, "summon smoke: not loaded on this floor\n");
        return false;
    }
    CSHOT_EFFECT *effect = ShotEffectAt(index);
    int           slot = SpawnStill(*effect, spot);
    if (slot < 0) {
        return false;
    }
    puff.effect = effect;
    puff.slot = slot;
    puff.owner = owner;
    std::copy(spot, spot + 3, puff.spot);
    CCharacter &model = effect->chara[slot];
    model.SetScale(scale, scale, scale);
    model.SetRotation(0.0f, yaw, 0.0f);
    model.motion_speed = kSmokeRate;
    FadeUnlitFrames(model.frame, 1.0f); // a slot an earlier smoke faded, back to full
    return true;
}

void SummonSmokeStep() {
    if (puff.slot < 0) {
        return;
    }
    if (!Live()) {
        puff = Puff{};
        return;
    }
    CCharacter &model = puff.effect->chara[puff.slot];
    model.SetPosition(puff.spot[0], puff.spot[1], puff.spot[2]);
    model.motion_speed = kSmokeRate;
    float frame = model.motion_type.state.time;
    if (frame >= kSmokeFadeFrom) {
        FadeUnlitFrames(model.frame, 1.0f - std::clamp((frame - kSmokeFadeFrom) / (kSmokeClipEnd - kSmokeFadeFrom), 0.0f, 1.0f));
    }
}

void SummonSmokeMove(const float *spot, const void *owner) {
    if (puff.owner == owner && puff.slot >= 0) {
        std::copy(spot, spot + 3, puff.spot);
    }
}

void SummonSmokeEnd(const void *owner) {
    if (puff.owner == owner) {
        PutOut();
    }
}

bool SummonSmokePlaying() {
    return Live();
}

float SummonSmokeFrame() {
    return Live() ? puff.effect->chara[puff.slot].motion_type.state.time : -1.0f;
}

} // namespace expanded
