#include "expanded/weapons/toan/mardan.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <initializer_list>

#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/weapons/ownership.hpp"
#include "expanded/weapons/ruby/fairys_ring.hpp"
#include "fish.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

// size scaled from 1 time at low to growth times at high.
float Grow(float size, float low, float high, float growth) {
    if (growth <= 1.0f || high <= low) {
        return size;
    }
    float t = (size - low) / (high - low);
    return size * (1.0f + (growth - 1.0f) * t);
}

// size lifted toward high by strength * range * t^exponent * (1 - t); low and high keep theirs.
float Smooth(float size, float low, float high, float strength, float exponent) {
    if (strength <= 0.0f || high <= low || size <= low) {
        return size;
    }
    float range = high - low;
    float t = std::min(1.0f, (size - low) / range);
    return size + strength * range * std::pow(t, exponent) * (1.0f - t);
}

} // namespace

int MardanSword() {
    for (int sword : {ITEM_WEAPON_ARISE_MARDAN, ITEM_WEAPON_MARDAN_TWEI, ITEM_WEAPON_MARDAN_EINS}) {
        if (WeaponOwned(sword)) {
            return sword;
        }
    }
    return 0;
}

int MardanFishKind(int kind, int rare_interval) {
    if (kind < 0 || std::ranges::contains(kGarayans, kind) || rare_interval <= 0) {
        return kind;
    }
    int sword = MardanSword();
    if ((sword != ITEM_WEAPON_MARDAN_TWEI && sword != ITEM_WEAPON_ARISE_MARDAN) || ModRoll(rare_interval) != 0) {
        return kind;
    }
    int garayan = ModRoll(kBaronOdds) == 0 ? kBaronGarayan : kMardanGarayan;
    LogEvent("mardan_garayan", std::format("\"kind\":{},\"garayan\":{}", kind, garayan));
    return garayan;
}

float AriseSize(float size, float usual_size, float max_size) {
    const float smallest = 0.5f * usual_size; // CFish::SetScale's floor
    const float most = kAriseGrowth * max_size;
    float       grown = Grow(std::min(size, max_size), smallest, max_size, kAriseGrowth);
    float       usual = Grow(usual_size, smallest, max_size, kAriseGrowth);
    return std::min(Smooth(grown, usual, most, kAriseSmoothing, kAriseSmoothingExponent), most);
}

float AriseFishSize(float size, const FishInfo &info) {
    if (MardanSword() != ITEM_WEAPON_ARISE_MARDAN) {
        return size;
    }
    float grown = AriseSize(size, info.min_size, info.max_size);
    LogEvent("arise_size", std::format("\"from\":{:.3f},\"to\":{:.3f}", size, grown));
    return grown;
}

int MardanSteerFrames(int sword, int fish_kind, float affinity) {
    bool steered = fish_kind == kUmadakara || std::ranges::contains(kGarayans, fish_kind);
    if (sword < ITEM_WEAPON_MARDAN_EINS || sword > ITEM_WEAPON_ARISE_MARDAN || !steered || affinity <= 0.0f) {
        return 0;
    }
    return static_cast<int>(std::ceil(static_cast<float>(kMardanSteerFrames[sword - ITEM_WEAPON_MARDAN_EINS]) / affinity));
}

} // namespace expanded
