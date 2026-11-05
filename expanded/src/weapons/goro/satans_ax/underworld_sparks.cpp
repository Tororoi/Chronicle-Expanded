#include "expanded/weapons/goro/satans_ax/underworld_sparks.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <random>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "render/draw2d.hpp"
#include "snd.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

struct Spark {
    bool  live = false;
    float position[4] = {};
    float from = 0.0f; // how far from him it started
    int   age = 0;
    int   left = kSparkFadeOut; // its frames left once the pull has ended
    float sway = 0.0f;          // across its path, at most
    float phase = 0.0f;         // its sway's
    float rate = 0.0f;          // its sway's, a step
    float twinkle = 0.0f;       // its twinkle's phase
    float size = 1.0f;          // its own share of kSparkSize
};

std::array<Spark, kSparkCount> sparks;
float                          owed = 0.0f; // the part of a spark the steps so far have started

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(UnderworldSparksReset), true);

float Uniform(float low, float high) {
    return std::uniform_real_distribution<float>(low, high)(ModRandom());
}

void Start() {
    auto free = std::find_if(sparks.begin(), sparks.end(), [](const Spark &spark) { return !spark.live; });
    if (free == sparks.end()) {
        return;
    }
    float angle = Uniform(0.0f, 2.0f * std::numbers::pi_v<float>);
    // Even over the ring's area: as many far out as its wider rim holds.
    float near2 = kSparkNear * kSparkNear, far2 = kSparkFar * kSparkFar;
    float radius = std::sqrt(Uniform(near2 + (far2 - near2) * 0.05f, far2));
    *free = Spark{
        .live = true,
        .position = {CharaMain.pos[0] + radius * std::cos(angle), CharaMain.pos[1] + kSparkLift, CharaMain.pos[2] + radius * std::sin(angle), 1.0f},
        .from = radius,
        .sway = Uniform(0.1f, 0.35f),
        .phase = Uniform(0.0f, 2.0f * std::numbers::pi_v<float>),
        .rate = Uniform(0.05f, 0.12f),
        .twinkle = Uniform(0.0f, 2.0f * std::numbers::pi_v<float>),
        .size = Uniform(0.7f, 1.3f)
    };
}

float Distance(const Spark &spark) {
    return std::hypot(CharaMain.pos[0] - spark.position[0], CharaMain.pos[2] - spark.position[2]);
}

int Alpha(const Spark &spark) {
    float distance = Distance(spark);
    float share = std::min(1.0f, (spark.from - distance) / kSparkFadeIn);
    share = std::min(share, static_cast<float>(kSparkLife - spark.age) / kSparkFadeOut);
    share = std::min(share, (distance - kSparkNear * 0.5f) / (kSparkNear * 0.5f));
    return static_cast<int>(kSparkAlpha * std::clamp(share, 0.0f, 1.0f) * static_cast<float>(spark.left) / kSparkFadeOut);
}

const Spark *Live(int i) {
    for (const Spark &spark : sparks) {
        if (spark.live && i-- == 0) {
            return &spark;
        }
    }
    return nullptr;
}

} // namespace

void UnderworldSparksStep() {
    float strength = UnderworldStrength();
    float rate = strength >= 1.0f ? kSparkSmashPerFrame : strength > 0.0f ? kSparkHoldPerFrame
                                                                          : 0.0f;
    owed = rate > 0.0f ? owed + rate : 0.0f;
    for (; owed >= 1.0f; owed -= 1.0f) {
        Start();
    }
    for (Spark &spark : sparks) {
        if (!spark.live) {
            continue;
        }
        if (strength <= 0.0f) {
            spark.left--;
        }
        float dx = CharaMain.pos[0] - spark.position[0], dz = CharaMain.pos[2] - spark.position[2];
        float distance = std::hypot(dx, dz);
        if (++spark.age >= kSparkLife || spark.left <= 0 || distance < kSparkNear * 0.5f) {
            spark.live = false;
            continue;
        }
        float in = std::min(UnderworldPullAt(distance) * strength + kSparkCreep, distance);
        float across = spark.sway * std::sin(spark.phase + spark.rate * static_cast<float>(spark.age));
        spark.position[0] += (dx * in - dz * across) / distance;
        spark.position[2] += (dz * in + dx * across) / distance;
        spark.position[1] = CharaMain.pos[1] + kSparkLift;
    }
}

void UnderworldSparksDraw() {
    if (std::none_of(sparks.begin(), sparks.end(), [](const Spark &spark) { return spark.live; })) {
        return;
    }
    CTexture *texture = TexManager.GetTexture(const_cast<char *>("basefx00"), -1);
    if (texture == nullptr) {
        return;
    }
    // As a hit's sparks are drawn: depth not written, added to what is behind.
    const draw2d::Services &services = draw2d::Get();
    sceGsZbuf               zbuffer = mgZBuffer;
    zbuffer.bits.zmsk = 1;
    services.set_zbuf(&zbuffer);
    sceGsAlpha alpha = mgAlpha;
    alpha.bits.a = 0;
    alpha.bits.b = 2;
    alpha.bits.c = 0;
    alpha.bits.d = 1;
    services.set_alpha(&alpha);
    for (Spark &spark : sparks) {
        int opacity = spark.live ? Alpha(spark) : 0;
        if (opacity <= 0) {
            continue;
        }
        // Smaller as it nears him, half its size at kSparkNear.
        float shrink = std::clamp(0.5f + 0.5f * (Distance(spark) - kSparkNear) / (kSparkFar * 0.5f - kSparkNear), 0.5f, 1.0f);
        float size = kSparkSize * spark.size * shrink;
        // Square on screen as a hit's sparks are: half as tall in the sprite's units.
        std::array<int, 4> top_left{}, bottom_right{};
        if (MGRotTransPers3DSprite(top_left.data(), bottom_right.data(), spark.position, size, size / 2.0f, 0) != 1) {
            continue;
        }
        std::array<int, 4> top_right = {bottom_right[0], top_left[1], top_left[2], top_left[3]};
        std::array<int, 4> bottom_left = {top_left[0], bottom_right[1], bottom_right[2], bottom_right[3]};
        float              twinkle = 0.75f + 0.25f * std::sin(spark.twinkle + 0.4f * static_cast<float>(spark.age));
        set3DSprite(Vif1Packet, texture, CRect_i_(0, 0, 0x10, 0x10), top_left.data(), top_right.data(), bottom_left.data(), bottom_right.data(),
                    static_cast<unsigned char>(static_cast<float>(opacity) * twinkle));
    }
    services.set_zbuf(nullptr);
    services.set_alpha(nullptr);
}

int UnderworldSparksLive() {
    return static_cast<int>(std::count_if(sparks.begin(), sparks.end(), [](const Spark &spark) { return spark.live; }));
}

bool UnderworldSparkAt(int i, float *position) {
    const Spark *spark = Live(i);
    if (spark == nullptr) {
        return false;
    }
    std::copy(spark->position, spark->position + 3, position);
    return true;
}

int UnderworldSparkAlpha(int i) {
    const Spark *spark = Live(i);
    return spark == nullptr ? -1 : Alpha(*spark);
}

void UnderworldSparksReset() {
    sparks.fill(Spark{});
    owed = 0.0f;
}

} // namespace expanded
