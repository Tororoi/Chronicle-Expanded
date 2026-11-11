#include "expanded/weapons/goro/satans_ax/underworld_sparks.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <random>
#include <vector>

#include "character.hpp"
#include "charaheight.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "framevu1.hpp"
#include "mdt.hpp"
#include "mglib.hpp"
#include "monstorunit.hpp"
#include "rect.hpp"
#include "render/draw2d.hpp"
#include "snd.hpp"
#include "texture.hpp"
#include "userstatus.hpp"
#include "visualvu1.hpp"

namespace expanded {

namespace {

struct Spark {
    bool  live = false;
    float position[4] = {};
    float from = 0.0f;         // how far from him it started, across the ground
    bool  ground = true;       // on the ground, or risen from a monster's body
    float rise = 0.0f;         // the height it would meet what draws it at, above the ground (kept within its)
    float speed = kSparkCreep; // toward him, a step
    int   age = 0;
    int   left = kSparkFadeOut; // its frames left once the pull has ended
    float twinkle = 0.0f;       // its twinkle's phase
    float size = 1.0f;          // its own share of kSparkSize
    bool  lowered = false;      // the Inferno's: heading for the height on him its own maps to (InfernoSparkHeight)
};

std::array<Spark, kSparkCount> sparks;
UnderworldCentre               centre;                         // what they are drawn to
float                          owed = 0.0f;                    // the part of a spark the steps so far have started
float                          body_owed[kFloorMonsters] = {}; // and each monster's body has
bool                           risen = false;                  // the last ground spark started off the ground

// A monster's meshes this frame, and its frames' bounds (for a model whose meshes keep no vertices).
struct Mesh {
    CFrame              *frame;
    const sceVu0FVECTOR *vertices;
    int                  count;
};

struct Bound {
    CFrame *frame;
    float   volume;
};

std::vector<Mesh>  meshes;
std::vector<Bound> bounds;

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(UnderworldSparksReset), true);

float Uniform(float low, float high) {
    return std::uniform_real_distribution<float>(low, high)(ModRandom());
}

void Collect(CFrame *frame) {
    for (; frame != nullptr; frame = frame->brother) {
        CVisualVu1 *visual = static_cast<CFrameVu1 *>(frame)->GetVisual();
        u_int      *data = visual != nullptr ? visual->GetMDTDataAddress() : nullptr;
        if (data != nullptr) {
            auto *mdt = reinterpret_cast<MDT_HEADER *>(data);
            if (mdt->vertex_num > 0) {
                meshes.push_back({frame, reinterpret_cast<const sceVu0FVECTOR *>(reinterpret_cast<u_char *>(mdt) + mdt->vertex_ofs), static_cast<int>(mdt->vertex_num)});
            }
        }
        float volume = (frame->max[0] - frame->min[0]) * (frame->max[1] - frame->min[1]) * (frame->max[2] - frame->min[2]);
        if (volume > 0.0f) {
            bounds.push_back({frame, volume});
        }
        Collect(frame->child);
    }
}

// Somewhere on a monster's model as it stands this frame, any part of it alike, where its frames' world matrices put
// it: one of its meshes' vertices (each as likely), else a point inside one of its frames' bounds (each as likely by
// its room; the game keeps no monster's vertices). False with neither.
bool FramePoint(CMonstorUnit &unit, int monster, float *point) {
    meshes.clear();
    bounds.clear();
    Collect(unit.chara[monster][0].frame);
    float local[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (!meshes.empty()) {
        int total = 0;
        for (const Mesh &mesh : meshes) {
            total += mesh.count;
        }
        int pick = std::uniform_int_distribution<int>(0, total - 1)(ModRandom());
        for (const Mesh &mesh : meshes) {
            if (pick < mesh.count) {
                std::copy(mesh.vertices[pick], mesh.vertices[pick] + 3, local);
                mesh.frame->GetWorldPosition(point, local);
                return true;
            }
            pick -= mesh.count;
        }
    }
    if (!bounds.empty()) {
        float total = 0.0f;
        for (const Bound &bound : bounds) {
            total += bound.volume;
        }
        float pick = Uniform(0.0f, total);
        for (const Bound &bound : bounds) {
            if (pick <= bound.volume || &bound == &bounds.back()) {
                for (int c = 0; c < 3; c++) {
                    local[c] = Uniform(bound.frame->min[c], bound.frame->max[c]);
                }
                bound.frame->GetWorldPosition(point, local);
                return true;
            }
            pick -= bound.volume;
        }
    }
    return false;
}

// Somewhere inside one of a monster's posed hurt spheres, false with none.
bool SpherePoint(const CMonstorUnit &unit, int monster, float *point) {
    const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
    float                       root[4];
    const_cast<CMonstorUnit &>(unit).chara[monster][0].GetPosition(root);
    std::array<int, kMonsterSpheres> posed{};
    int                              count = 0;
    for (int i = 0; i < kMonsterSpheres; i++) {
        if (body.timer[i] != 0 && std::fabs(body.position[i][0] - root[0]) <= kUnposedSphere && std::fabs(body.position[i][2] - root[2]) <= kUnposedSphere) {
            posed[count++] = i;
        }
    }
    if (count == 0) {
        return false;
    }
    int i = posed[ModRoll(count)];
    // Even through the sphere: a direction, and a radius as deep as its shells' room.
    float y = Uniform(-1.0f, 1.0f), angle = Uniform(0.0f, 2.0f * std::numbers::pi_v<float>);
    float across = std::sqrt(1.0f - y * y), radius = body.radius[i] * std::cbrt(Uniform(0.0f, 1.0f));
    point[0] = body.position[i][0] + radius * across * std::cos(angle);
    point[1] = body.position[i][1] + radius * y;
    point[2] = body.position[i][2] + radius * across * std::sin(angle);
    return true;
}

// Somewhere on a monster: a point on its model, else in its hurt spheres. Its frames' world matrices place the model
// off where the monster stands (by the floor's offset), so a model's point is taken relative to its root frame and set
// on the monster's position.
bool BodyPoint(CMonstorUnit &unit, int monster, float *point) {
    if (!FramePoint(unit, monster, point)) {
        return SpherePoint(unit, monster, point);
    }
    CFrame *root = unit.chara[monster][0].frame;
    float   origin[4] = {0.0f, 0.0f, 0.0f, 1.0f}, local[4] = {0.0f, 0.0f, 0.0f, 1.0f}, at[4];
    root->GetWorldPosition(origin, local);
    unit.chara[monster][0].GetPosition(at);
    for (int c = 0; c < 3; c++) {
        point[c] += at[c] - origin[c];
    }
    return true;
}

Spark *Free() {
    auto free = std::find_if(sparks.begin(), sparks.end(), [](const Spark &spark) { return !spark.live; });
    return free == sparks.end() ? nullptr : &*free;
}

Spark Fresh() {
    return Spark{.live = true,
                 .twinkle = Uniform(0.0f, 2.0f * std::numbers::pi_v<float>),
                 .size = Uniform(0.7f, 1.3f)};
}

void StartOnBody(CMonstorUnit &unit, int monster) {
    Spark *free = Free();
    float  point[4];
    if (free == nullptr || !BodyPoint(unit, monster, point)) {
        return;
    }
    *free = Fresh();
    std::copy(point, point + 3, free->position);
    free->position[3] = 1.0f;
    free->from = std::hypot(centre.x - point[0], centre.z - point[2]);
    free->ground = false;
    free->rise = point[1] - centre.ground;
    free->lowered = InfernoWielded();
}

void Start() {
    Spark *free = Free();
    if (free == nullptr) {
        return;
    }
    float angle = Uniform(0.0f, 2.0f * std::numbers::pi_v<float>);
    // Even over the ring's area: as many far out as its wider rim holds.
    float near2 = kSparkNear * kSparkNear, far2 = kSparkFar * kSparkFar;
    float radius = std::sqrt(Uniform(near2 + (far2 - near2) * 0.05f, far2));
    *free = Fresh();
    free->rise = kSparkLift;
    // The Inferno's: every other one starts at a height within his, and comes in at it.
    risen = InfernoWielded() && !risen;
    if (risen) {
        free->rise = Uniform(kSparkLift, std::max(kSparkLift, kInfernoSparkTop * (centre.high - centre.ground)));
        free->lowered = true;
    }
    free->position[0] = centre.x + radius * std::cos(angle);
    free->position[1] = centre.ground + free->rise;
    free->position[2] = centre.z + radius * std::sin(angle);
    free->position[3] = 1.0f;
    free->from = radius;
}

float Distance(const Spark &spark) {
    return std::hypot(centre.x - spark.position[0], centre.z - spark.position[2]);
}

int Alpha(const Spark &spark) {
    // Its fades in and out each take no more than a quarter of its way in, so one starting near him (off a monster at
    // his side) still shows at full between them.
    float distance = Distance(spark), stop = kSparkNear * 0.5f;
    float quarter = std::max(0.25f * (spark.from - stop), 0.01f);
    float share = std::min(1.0f, (spark.from - distance) / std::min(kSparkFadeIn, quarter));
    share = std::min(share, static_cast<float>(kSparkLife - spark.age) / kSparkFadeOut);
    share = std::min(share, (distance - stop) / std::min(stop, quarter));
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

float InfernoSparkHeight(float rise, float top) {
    if (rise <= top) {
        return std::max(0.0f, rise * kInfernoSparkBelow);
    }
    float over = (rise - top) / ((kInfernoSparkTop - 1.0f) * top);
    return top * std::min(1.0f, kInfernoSparkBelow + (1.0f - kInfernoSparkBelow) * over);
}

void UnderworldSparksStep() {
    float            strength = UnderworldStrength();
    UnderworldCentre now;
    if (UnderworldCentreNow(now)) {
        centre = now; // kept as the pull ends, for the sparks fading on their way
    }
    float rate = strength >= 1.0f ? kSparkSmashPerFrame : strength > 0.0f ? kSparkHoldPerFrame
                                                                          : 0.0f;
    owed = rate > 0.0f ? owed + rate : 0.0f;
    for (; owed >= 1.0f; owed -= 1.0f) {
        Start();
    }
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        bool drawn = NowMonstorUnit != nullptr && strength > 0.0f && UnderworldPulling(monster) && MonsterInPlay(NowMonstorUnit->monster[monster]);
        body_owed[monster] = drawn ? body_owed[monster] + kBodySparkPerFrame * strength : 0.0f;
        for (; body_owed[monster] >= 1.0f; body_owed[monster] -= 1.0f) {
            StartOnBody(*NowMonstorUnit, monster);
        }
    }
    for (Spark &spark : sparks) {
        if (!spark.live) {
            continue;
        }
        if (strength <= 0.0f) {
            spark.left--;
        }
        float dx = centre.x - spark.position[0], dz = centre.z - spark.position[2];
        float distance = std::hypot(dx, dz);
        if (++spark.age >= kSparkLife || spark.left <= 0 || distance < kSparkNear * 0.5f) {
            spark.live = false;
            continue;
        }
        float in = std::min(spark.speed, distance);
        spark.speed += (spark.ground ? kSparkAccel : kBodySparkAccel) * strength;
        spark.position[0] += dx * in / distance;
        spark.position[2] += dz * in / distance;
        // Rising or falling to the height it heads for as it comes in, to meet it there: its own within his (a ground
        // spark's on the ground), the pellet's; the Inferno's, its own mapped onto him.
        float height = spark.lowered ? centre.ground + InfernoSparkHeight(spark.rise, centre.high - centre.ground)
                                     : std::clamp(centre.ground + spark.rise, centre.low, centre.high);
        spark.position[1] += (height - spark.position[1]) * in / distance;
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
    const spRGBA tint = InfernoWielded() ? spRGBA{.r = kInfernoSparkTint[0], .g = kInfernoSparkTint[1], .b = kInfernoSparkTint[2], .a = 0}
                                         : spRGBA{.r = 0x80, .g = 0x80, .b = 0x80, .a = 0};
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
        spRGBA             colour = tint;
        colour.a = static_cast<unsigned char>(static_cast<float>(opacity) * twinkle);
        set3DSprite(Vif1Packet, texture, CRect_i_(0, 0, 0x10, 0x10), top_left.data(), top_right.data(), bottom_left.data(), bottom_right.data(), &colour);
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
    risen = false;
    std::fill(std::begin(body_owed), std::end(body_owed), 0.0f);
}

} // namespace expanded
