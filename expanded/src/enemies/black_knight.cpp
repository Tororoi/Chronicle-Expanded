#include "expanded/enemies/black_knight.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "frame.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "snd.hpp"

namespace expanded {

namespace {

// The light: e508_ex's KEY 1 (21-80), seen and heard only.
// clang-format off
BT_SHOT_EFFECT light_shot = {"e508_ex", 1, 0, {}, {}, -1, 0, 0, 2, 1, {1, -1, -1, -1}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
// clang-format on
constexpr const char *kLightFile = "gedit/s78/chara/e508_ex.chr";

constexpr float kHit = 36.0f;
constexpr float kFadeFrom = 60.0f;
constexpr float kFadeTo = 68.0f;
constexpr float kSceneSpeed = 0.25f;   // event 508's, up to the hit
constexpr float kHerculesSpeed = 0.2f; // Hercules' Wrath's, from the hit

// The light's sound, stopped where the rest of it leads into the scene's white-out.
constexpr int kLightSound = 2588;
constexpr int kLightSoundSteps = 112;

struct LitFrame {
    CFrame *frame;
    float   color[4];
};

struct Light {
    int  effect = -1;
    int  slot = -1;
    int  steps = 0;
    bool lit = false;
    // The colour-lit frames of the shot's model (all of e508_ex's), and their colours, faded together.
    std::vector<LitFrame> frames;
};

Light light;

CSHOT_EFFECT *Effect() {
    return light.effect >= 0 ? ShotEffectAt(light.effect) : nullptr;
}

void CollectFrames(CFrame *frame) {
    for (; frame != nullptr; frame = frame->brother) {
        if (frame->attr.use_color) {
            LitFrame &lit = light.frames.emplace_back();
            lit.frame = frame;
            std::copy(std::begin(frame->attr.color), std::end(frame->attr.color), std::begin(lit.color));
        }
        CollectFrames(frame->child);
    }
}

void Fade(float k) {
    for (LitFrame &lit : light.frames) {
        for (int i = 0; i < 3; i++) {
            lit.frame->attr.color[i] = lit.color[i] * k;
        }
    }
}

void End(CSHOT_EFFECT &effect) {
    if (light.steps < kLightSoundSteps) {
        SndSeStop(kLightSound, 0);
    }
    Fade(1.0f);
    light.frames.clear();
    effect.active[light.slot] = 0;
    light.slot = -1;
}

void OnModel(CMonstorUnit &, int model_slot, int model_no) {
    if (model_slot == 0) {
        light = {};
    }
    if (model_no == No(Monster::BLACK_KNIGHT) && LoadedRoster() != nullptr && BossSupported(model_no)) {
        light.effect = EnterShotEffect(&light_shot, 1, kLightFile);
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyModelListener(OnModel), true);

} // namespace

bool BlackKnightLight(CMonstorUnit &unit, int monster) {
    CSHOT_EFFECT *effect = Effect();
    if (effect == nullptr) {
        return false;
    }
    if (light.slot >= 0) {
        End(*effect);
    }
    float feet[4];
    unit.chara[monster][0].GetPosition(feet);
    feet[3] = 1.0f;
    NowShotEffect->Set(light.effect, feet, feet);
    if (effect->current_slot < 0) {
        return false;
    }
    light.slot = effect->current_slot;
    light.steps = 0;
    light.lit = false;
    effect->velocity[light.slot][0] = effect->velocity[light.slot][1] = effect->velocity[light.slot][2] = 0.0f;
    CollectFrames(effect->chara[light.slot].frame);
    SndSePlay(kLightSound, -1, 0);
    return true;
}

bool BlackKnightLit() {
    return light.lit;
}

void BlackKnightStep() {
    CSHOT_EFFECT *effect = Effect();
    if (effect == nullptr || light.slot < 0) {
        return;
    }
    if (++light.steps == kLightSoundSteps) {
        SndSeStop(kLightSound, 0);
    }
    if (effect->active[light.slot] == 0) {
        light.frames.clear();
        light.slot = -1;
        return;
    }
    CCharacter &model = effect->chara[light.slot];
    float       frame = model.motion_type.state.time;
    light.lit = light.lit || frame >= kHit;
    model.SetMotionSpeed(frame >= kHit ? kHerculesSpeed : kSceneSpeed);
    if (frame >= kFadeTo) {
        End(*effect);
        return;
    }
    if (frame >= kFadeFrom) {
        Fade(1.0f - (frame - kFadeFrom) / (kFadeTo - kFadeFrom));
    }
}

} // namespace expanded
