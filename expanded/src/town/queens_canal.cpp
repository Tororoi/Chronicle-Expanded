#include "expanded/town/queens_canal.hpp"

#include <array>
#include <cmath>
#include <format>
#include <string_view>

#include "character.hpp"
#include "edit.hpp"
#include "editground.hpp"
#include "editloop.hpp"
#include "editloop3.hpp"
#include "editmapscript.hpp"
#include "effectmacro.hpp"
#include "expanded/data/queens_canal.hpp"
#include "expanded/debug/event_log.hpp"
#include "frame.hpp"
#include "gamemode.hpp"
#include "main.hpp"
#include "mglib.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

// The canal's surface (mizu__a01, of the WATER part e03c08) stands at 30 with its frame at rest.
constexpr const char *kWaterPart = "e03c08";
constexpr float       kSurfaceY = 30.0f;
// The rippling water body sits a little over the surface, so the two never fight over the same depth.
constexpr float kBodyLift = 1.0f;
// The ripples sit a hair over the surface.
constexpr float kRippleLift = 0.2f;
// The fade's alpha (of 128) from which the screen counts as black, and the walking frames a change of level waits for
// one before it shows anyway (the time's change is an event, its black at its end).
constexpr float kBlack = 100.0f, kFullBlack = 128.0f;
constexpr int   kUnhiddenChange = 180;
// Down in the canal: below the water's afternoon level, near its walls at z -50 and 50 (the banks and bridges stand
// above it).
constexpr float kCanalZ = 60.0f, kCanalWall = 50.0f;
// Frames a player out of the canal still counts as caught, and frames the rising tide's black is waited for.
constexpr int kCaughtFrames = 60, kEvictionFrames = 900;
// Standing still for the wading ripple: moving less than this a frame, for this many frames.
constexpr float kStill = 0.17f;
constexpr int   kStillFrames = 9;
// The ladder's ends and its message point: those within this of its x.
constexpr float kLadderReach = 12.0f;

// The waterfalls' mist: each fall's emitters spread from its centre across a span, leant toward the canal's middle and
// held low over the water; spray's own lean north (-0.5 z) taken off.
struct Fall {
    float x, z, span_x, span_z;
    int   emitters;
    float lean_x, lean_z, rise;
};

constexpr float kLean = 0.35f, kSprayNorth = 0.5f;
constexpr Fall  kFalls[] = {
    {198.0f,  -27.0f, 8.0f, 0.0f,  1,  0.0f,   kLean,  -0.7f},
    {198.0f,  27.0f,  8.0f, 0.0f,  1,  0.0f,   -kLean, -0.7f},
    {601.0f,  -27.0f, 8.0f, 0.0f,  1,  0.0f,   kLean,  -0.7f},
    {601.0f,  27.0f,  8.0f, 0.0f,  1,  0.0f,   -kLean, -0.7f},
    {1100.0f, -27.0f, 8.0f, 0.0f,  1,  0.0f,   kLean,  -0.7f},
    {1100.0f, 27.0f,  8.0f, 0.0f,  1,  0.0f,   -kLean, -0.7f},
    // The east fall, along its whole width, leant west down the canal.
    {1262.0f, 2.0f,   0.0f, 50.0f, 10, -kLean, 0.0f,   -0.5f},
};
constexpr std::array<float, 4> kMistSize = {5.0f, 2.0f, 5.0f, 1.0f};
constexpr int                  kMistPeriod = 15;

struct Canal {
    float shown = NAN;
    float target = NAN;
    int   waited = 0;
    int   body = -1; // the canal's water surface
    int   caught = 0;
    int   eviction = 0;
    float last[3] = {};
    int   still = 0;
    float lean[3] = {};
};

Canal canal;

bool InQueens() {
    return MapNo == kQueensMapNo && pEditGround != nullptr && EditMapInfo != nullptr;
}

float FadeAlpha() {
    float colour[4];
    EdGetFadeColor(colour);
    return colour[3];
}

// A fixed part by the name its mapinfo places, or null.
CMapParts *FixedPart(std::string_view name) {
    for (int i = 0; i < 64; i++) {
        std::string_view placed = EditMapInfo->map_objects[i].name[0];
        if (placed.empty()) {
            break;
        }
        if (placed.ends_with(name) && pEditGround->fixed_parts[i].handle >= 0) {
            return &pEditGround->fixed_parts[i];
        }
    }
    return nullptr;
}

bool Low() {
    return CanalTideShown() <= kLowTide;
}

bool InCanal(const float *position) {
    return position[1] <= kMidTide && std::abs(position[2]) <= kCanalZ;
}

void ShowLevel() {
    if (CMapParts *water = FixedPart(kWaterPart); water != nullptr && water->frame[0] != nullptr) {
        char    name[] = "mizu__a01";
        CFrame *surface = water->frame[0]->SearchFrame(name);
        float   rise = canal.shown - kSurfaceY;
        if (surface != nullptr && surface->local[3][1] != rise) {
            surface->local[3][1] = rise;
            surface->world_valid = 0;
        }
    }
    if (canal.body < 0) {
        for (int i = 0; i < 4; i++) {
            const CGroundWater &surface = pEditGround->water_surfaces[i];
            if (surface.draw != 0 && surface.parts_no < 0 && surface.offset[1] > 20.0f && surface.offset[1] < 80.0f) {
                canal.body = i;
                break;
            }
        }
    }
    if (canal.body >= 0) {
        pEditGround->water_surfaces[canal.body].offset[1] = canal.shown + kBodyLift;
    }
}

void PlaceRipples(const float *player) {
    for (std::size_t i = 0; i < std::size(kRailRipples); i++) {
        if (CMapParts *rail = FixedPart(kRailRipples[i])) {
            rail->SetPosition(kRailRippleX[i], canal.shown + kRippleLift, kRailRippleZ);
        }
    }
    float moved = std::abs(player[0] - canal.last[0]) + std::abs(player[1] - canal.last[1]) + std::abs(player[2] - canal.last[2]);
    for (int k = 0; k < 3; k++) {
        canal.last[k] = player[k];
    }
    canal.still = moved < kStill ? std::min(canal.still + 1, kStillFrames) : 0;
    if (CMapParts *ripple = FixedPart(kWadingRipple)) {
        bool wading = Low() && player[1] <= canal.shown && std::abs(player[2]) <= kCanalWall && canal.still >= kStillFrames;
        if (wading) {
            ripple->SetPosition(player[0], canal.shown + kRippleLift, player[2]);
        } else {
            ripple->SetPosition(0.0f, kRippleParkY, 0.0f);
        }
    }
}

// The rising tide: a player caught in the canal since it was last low is taken to the dock, on the black of the time's
// change (the event it runs in jumps there as it ends).
void Evict(float target, const float *player) {
    if (Low() && InCanal(player)) {
        canal.caught = kCaughtFrames;
    } else if (canal.caught > 0) {
        canal.caught--;
    }
    if (!std::isnan(canal.target) && canal.target <= kLowTide && target > kLowTide && canal.caught > 0 && GameMode != ED_MODE_FISHING) {
        canal.eviction = kEvictionFrames;
    }
    if (canal.eviction > 0) {
        canal.eviction--;
        if (FadeAlpha() >= kFullBlack && GameMode == ED_MODE_EVENT) {
            MapJump(kEastHarborMapNo, kDockArrivalEvent);
            EdEventInfo.return_code = ED_EVENT_RETURN_MAP_JUMP;
            canal.eviction = 0;
            LogEvent("tide", "\"step\":\"evict\"");
        }
    }
}

} // namespace

float CanalTideLevel(float time) {
    switch (EdGetTime(time)) {
        case ED_TIME_MORNING:
            return kLowTide;
        case ED_TIME_EVENING:
            return kHighTide;
        default:
            return kMidTide;
    }
}

float CanalTideShown() {
    return std::isnan(canal.shown) ? CanalTideLevel(NowTime) : canal.shown;
}

void QueensCanalStep() {
    if (!InQueens()) {
        canal = Canal{};
        return;
    }
    float target = CanalTideLevel(NowTime);
    if (GameMode == ED_MODE_WALK) {
        canal.waited++;
    }
    if (std::isnan(canal.shown) || target == canal.shown || FadeAlpha() >= kBlack || canal.waited > kUnhiddenChange) {
        if (canal.shown != target) {
            LogEvent("tide", std::format("\"step\":\"level\",\"level\":{}", target));
        }
        canal.shown = target;
        canal.waited = 0;
    }
    float player[4] = {};
    if (Chara != nullptr) {
        Chara->GetPosition(player);
    }
    Evict(target, player);
    canal.target = target;
    ShowLevel();
    PlaceRipples(player);
}

bool CanalEventPointOn(const ED_EVENT_POINT &point) {
    if (MapNo != kQueensMapNo || std::abs(point.position[0] - kLadderX) > kLadderReach) {
        return true;
    }
    switch (point.event_type) {
        case ED_EVENT_POINT_LADDER_TOP:
            return Low();
        case ED_EVENT_POINT_EVENT:
            return point.side != kLadderMessageLabel || !Low();
        default:
            return true;
    }
}

void DrawWadingPlayer() {
    if (!InQueens() || !Low() || Chara == nullptr || EdDrawOffMap != 0 || EdDebugCharaDrawOff != 0) {
        return;
    }
    switch (GameMode) {
        case ED_MODE_WALK:
        case ED_MODE_FISHING:
        case ED_MODE_TALK:
        case ED_MODE_TIME_CHANGE:
            break;
        case ED_MODE_EVENT:
            if (EdEventInfo.player_draw == 0) {
                return;
            }
            break;
        default:
            return;
    }
    float position[4];
    Chara->GetPosition(position);
    if (!InCanal(position)) {
        return;
    }
    TexManager.ReloadTexture(Vif1Packet, 8);
    Chara->Draw();
}

void CanalMist(CEffectGroup *group) {
    if (!InQueens() || !Low()) {
        return;
    }
    int phase = 0;
    for (const Fall &fall : kFalls) {
        for (int k = 0; k < fall.emitters; k++, phase++) {
            float across = fall.emitters == 1 ? 0.0f : static_cast<float>(k) / static_cast<float>(fall.emitters - 1) * 2.0f - 1.0f;
            float position[4] = {fall.x + across * fall.span_x, canal.shown, fall.z + across * fall.span_z, 1.0f};
            float size[4] = {kMistSize[0], kMistSize[1], kMistSize[2], kMistSize[3]};
            canal.lean[0] = fall.lean_x;
            canal.lean[1] = fall.rise;
            canal.lean[2] = fall.lean_z + kSprayNorth;
            EffectWaterSpray(group, position, size, kMistPeriod, phase);
        }
    }
    canal.lean[0] = canal.lean[1] = canal.lean[2] = 0.0f;
}

void SprayLean(float *velocity) {
    for (int k = 0; k < 3; k++) {
        velocity[k] += canal.lean[k];
    }
}

} // namespace expanded
