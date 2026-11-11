#include "expanded/weapons/goro/inferno/cape_wind.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

#include "character.hpp"
#include "cloth.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "frame.hpp"

namespace expanded {

namespace {

float gust = 1.0f;
int   sway_frames = 0;

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(CapeWindReset), true);

bool Cape(const CCloth &cloth) {
    return CharaMain.cloth != nullptr && std::find(CharaMain.cloth, CharaMain.cloth + CHARA_CLOTH_MAX, &cloth) != CharaMain.cloth + CHARA_CLOTH_MAX;
}

} // namespace

void CapeWindStep() {
    if (RisingFlameRisen() <= 0.0f) {
        return;
    }
    gust = std::clamp(gust + 0.2f * (std::uniform_real_distribution<float>(0.0f, 1.0f)(ModRandom()) - 0.5f), 0.0f, 1.0f);
    sway_frames = (sway_frames + 1) % kCapeWindSwayFrames;
}

void CapeWindNow(float *force) {
    // His facing as his moves take it (GoroKey_Play): z turned by his frame's turn about y.
    float turn = CharaMain.rotation.y;
    if (CharaMain.frame != nullptr) {
        float rotation[4];
        CharaMain.frame->GetRotation(rotation);
        turn = rotation[1];
    }
    float ahead[2] = {std::sin(turn), std::cos(turn)};
    float sway = kCapeWindSway * std::sin(2.0f * std::numbers::pi_v<float> * static_cast<float>(sway_frames) / kCapeWindSwayFrames);
    float strength = kCapeWindSpeed * (kCapeWindCalm + (1.0f - kCapeWindCalm) * gust);
    // Back from his facing, across it by the sway (his right: ahead turned a quarter), and up.
    force[0] = strength * (-ahead[0] + sway * ahead[1]);
    force[1] = strength * kCapeWindLift;
    force[2] = strength * (-ahead[1] - sway * ahead[0]);
}

void InfernoCapeWind(const CCloth &cloth, float *force) {
    float risen = RisingFlameRisen();
    if (risen <= 0.0f || !InfernoWielded() || !Cape(cloth)) {
        return;
    }
    float wind[3];
    CapeWindNow(wind);
    for (int c = 0; c < 3; c++) {
        force[c] += wind[c] * risen * cloth.wind_effect;
    }
}

void CapeWindReset() {
    gust = 1.0f;
    sway_frames = 0;
}

} // namespace expanded
