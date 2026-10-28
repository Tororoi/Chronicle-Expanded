#include "expanded/weapons/ungaga/babels_spear/beam.hpp"

#include <cstdio>

#include "bt_shot_effect.hpp"
#include "expanded/data/prop_models.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "shot_effect.hpp"

namespace expanded::babels_spear {

namespace {

// The beam's pack, its vanish the first phase's motion, no flight and no hit.
// clang-format off
BT_SHOT_EFFECT beam = {"babel_beam", 0, 0, {}, {}, -1, 0, 0, 3, kHitsMonsters, {kBeamVanish, -1, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

struct Shot {
    CSHOT_EFFECT *effect = nullptr;
    int           slot = -1;
    int           key = -1;
    float         scale = 1.0f;
};

Shot shot;

constexpr float kClipStart[3] = {kBeamRiseStart, kBeamLoopStart, kBeamVanishStart}; // by KEY

void OnFloor() {
    shot = Shot{};
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

bool Live() {
    return shot.effect != nullptr && shot.slot >= 0 && shot.effect->effect_data == &beam && shot.effect->active[shot.slot] != 0;
}

// A KEY from its first frame, on the beam's rate, stopping on its last.
void Play(int key) {
    CCharacter &model = shot.effect->chara[shot.slot];
    model.SetMotion(key, 6, kBeamRate);
    model.motion_type.state.time = kClipStart[key];
    shot.key = key;
}

} // namespace

BT_SHOT_EFFECT *BeamEffect() {
    return &beam;
}

bool BeamStart(const float *spot, float scale) {
    BeamStop();
    int index = ShotEffectIndex(&beam);
    if (index < 0) {
        std::fprintf(stderr, "babel's spear: the beam is not loaded on this floor\n");
        return false;
    }
    CSHOT_EFFECT *effect = ShotEffectAt(index);
    int           slot = SpawnStill(*effect, spot);
    if (slot < 0) {
        return false;
    }
    shot.effect = effect;
    shot.slot = slot;
    shot.scale = scale;
    Play(kBeamRise);
    BeamStep(spot, false);
    return true;
}

void BeamStep(const float *spot, bool vanish) {
    if (shot.slot < 0 || shot.effect == nullptr || shot.effect->effect_data != &beam) {
        return;
    }
    bool        alive = Live();
    CCharacter &model = shot.effect->chara[shot.slot];
    float       frame = model.motion_type.state.time;
    bool        risen = shot.key == kBeamRise && frame >= kBeamRiseEnd - kBeamClipLead;
    int         key = kBeamLoop;
    if (vanish) {
        key = kBeamVanish;
    } else if (shot.key == kBeamRise && !risen) {
        key = kBeamRise;
    }
    if (!alive && key == kBeamVanish) {
        shot = Shot{}; // its vanish ran out: the game ended it
        return;
    }
    bool restart = key != shot.key || (key == kBeamLoop && (frame >= kBeamLoopEnd - kBeamClipLead || frame < kBeamLoopStart - 1.0f));
    if (!alive) {
        // Ended before its vanish: taken up again.
        shot.effect->phase[shot.slot] = 0;
        shot.effect->active[shot.slot] = 1;
        restart = true;
    }
    if (restart) {
        Play(key);
    }
    model.motion_speed = kBeamRate;
    model.SetPosition(spot[0], spot[1], spot[2]);
    model.SetScale(shot.scale, shot.scale, shot.scale);
}

void BeamStop() {
    if (Live()) {
        shot.effect->active[shot.slot] = 0;
    }
    shot = Shot{};
}

bool BeamPlaying() {
    return Live();
}

int BeamKey() {
    return Live() ? shot.key : -1;
}

float BeamFrame() {
    return Live() ? shot.effect->chara[shot.slot].motion_type.state.time : -1.0f;
}

} // namespace expanded::babels_spear
