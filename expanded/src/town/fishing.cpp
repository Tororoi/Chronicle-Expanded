#include "expanded/town/fishing.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <map>
#include <vector>

#include "camerafollow.hpp"
#include "collision.hpp"
#include "data/data_read.hpp"
#include "ebattle.hpp"
#include "edit.hpp"
#include "editloop.hpp"
#include "expanded/data/fishing_spots.hpp"
#include "expanded/data/queens_canal.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fish_steering.hpp"
#include "expanded/town/queens_canal.hpp"
#include "expanded/weapons/toan/mardan.hpp"
#include "expanded/weapons/ungaga/scorpion.hpp"
#include "expanded/weapons/xiao/flamingo.hpp"
#include "fishing.hpp"
#include "main.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

// fishing.cpp's distp: the length of each of the line's segments.
constexpr float kRetailLineSegment = 1.6666666f;

// The line's point the float rides (FishLineGetUki), the segments under it to the hook, and how far under them the bait
// rests: 3 under the hook, and the float's own 0.47 under the water.
constexpr int   kFloatPoint = 18, kHangSegments = 5;
constexpr float kBaitUnderHang = 3.47f, kShortestHang = 0.2f;

// A cast's pay-out: it starts once the rod tip swings kWindUp back from its place ahead of the angler at rest, then
// kFling forward; the line grows kPayOut a frame, the float's and hook's flight across the ground carried kCarry times
// a frame as it does; and a float back within kReeledIn of the rod draws the line in again.
constexpr float kWindUp = -5.0f, kFling = 2.0f, kPayOut = 0.25f, kCarry = 1.16f, kReeledIn = 6.0f;

struct Cast {
    float rest = 0.0f; // the rod tip's distance ahead of the angler at rest
    bool  wound_up = false;
    bool  paying_out = false;
    bool  stopped = false; // a wall or leg stopped the cast
};

Cast cast;

// A canal's walls stop a cast thrown with the float hanging low in it, as the last frame not casting or waiting saw.
bool canal_walls = false;

// A float is in a leg kLegInset inside its sides; a floor kOverhead over the float is over it.
constexpr float kLegInset = 1.5f, kOverhead = 2.0f;

// |ukiv[0]|² under this: a settled float (~0.22 a frame).
constexpr float kSettled = 0.0498046875f;

// The fishing camera's eye stays kInsideWalls inside a canal's walls.
constexpr float kInsideWalls = 2.0f;

// How far point lies ahead of the angler at at, turned yaw.
float Ahead(const float *point, const float *at, float yaw) {
    return (point[0] - at[0]) * std::sin(yaw) + (point[2] - at[2]) * std::cos(yaw);
}

float Distance(const float *a, const float *b) {
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void LogCast(std::string_view step, std::string_view fields = {}) {
    if (EventLogOn()) {
        LogEvent("cast", std::format("\"step\":\"{}\"{}{}", step, fields.empty() ? "" : ",", fields));
    }
}

// The pay-out: distp from kCastStart times retail's until the rod swings back and forward, then out to the spot's line,
// the float and hook carried farther as it grows; back to the start once reeled in or the float is back at the rod.
void PayOut(const FishingSpot &spot, const float *angler, float yaw) {
    if (spot.line <= 1.0f) {
        return;
    }
    const float start = kRetailLineSegment * kCastStart, full = kRetailLineSegment * spot.line;
    const bool  casting = chara_fishing >= ED_FISHING_CASTING;
    if (!casting) {
        cast.rest = Ahead(point[0], angler, yaw);
        cast.wound_up = false;
    }
    float swing = Ahead(point[0], angler, yaw) - cast.rest;
    if (casting && swing < kWindUp) {
        cast.wound_up = true;
    }
    float length = distp;
    if (!casting || length > full) {
        length = casting ? full : start;
    } else if (length > start + 0.01f && Distance(ukip[0], point[0]) < kReeledIn) {
        length = start;
    } else if (length < full && ((cast.wound_up && swing > kFling) || length > start + 0.01f)) {
        length = std::min(length + kPayOut, full);
        for (sceVu0FVECTOR &velocity : ukiv) {
            velocity[0] *= kCarry;
            velocity[2] *= kCarry;
        }
        for (sceVu0FVECTOR &velocity : hookv) {
            velocity[0] *= kCarry;
            velocity[2] *= kCarry;
        }
    }
    distp = length;
    bool paying_out = length > start + 0.01f && length < full;
    if (paying_out != cast.paying_out) {
        cast.paying_out = paying_out;
        LogCast(paying_out ? "pay_out" : "paid_out", std::format("\"line\":{:.2f},\"ahead\":{:.1f}", length / kRetailLineSegment, Ahead(ukip[0], angler, yaw)));
    }
}

// The cast's first stop, at a wall or a leg.
void Stopped(std::string_view at) {
    if (!cast.stopped) {
        cast.stopped = true;
        LogCast("stopped", std::format("\"at\":\"{}\"", at));
    }
}

bool InLeg(const std::array<float, 6> &leg, const float *at, float inset) {
    return at[0] >= leg[0] + inset && at[0] <= leg[1] - inset && at[2] >= leg[2] + inset && at[2] <= leg[3] - inset && at[1] >= leg[4] && at[1] <= leg[5];
}

// A flying point at at (old: FishLineStep's copy of it) stopped at the canal's walls, while they stand, and its legs:
// out of a leg through the nearest face, or, put in it by the walls, the face it was beyond.
void Stop(const FishingCanal &canal, float *at, float *old) {
    const float came_z = at[2];
    if (canal_walls) {
        float z = std::clamp(at[2], -canal.wall, canal.wall);
        if (z != at[2]) {
            at[2] = old[2] = z;
            Stopped("wall");
        }
    }
    for (const std::array<float, 6> &leg : canal.legs) {
        if (!InLeg(leg, at, 0.0f)) {
            continue;
        }
        float to_za = at[2] - leg[2], to_zb = leg[3] - at[2], to_xa = at[0] - leg[0], to_xb = leg[1] - at[0];
        if (came_z < leg[2] || came_z > leg[3]) {
            at[2] = old[2] = came_z < leg[2] ? leg[2] : leg[3];
        } else if (std::min(to_za, to_zb) < std::min(to_xa, to_xb)) {
            at[2] = old[2] = to_za < to_zb ? leg[2] : leg[3];
        } else {
            at[0] = old[0] = to_xa < to_xb ? leg[0] : leg[1];
        }
        Stopped("leg");
        break;
    }
}

// The canal's walls arm as a cast starts, the float hanging low inside them; then the cast's flying points stop.
void StopAtCanal(const FishingCanal &canal) {
    if (chara_fishing == ED_FISHING_CASTING) {
        for (int i = kFloatPoint; i < 24; i++) {
            Stop(canal, point[i], old_p[i]);
        }
        for (int i = 0; i < 4; i++) {
            Stop(canal, ukip[i], ukiop[i]);
        }
        for (int i = 0; i < 3; i++) {
            Stop(canal, hookp[i], hookop[i]);
        }
    } else if (chara_fishing != ED_FISHING_WAIT_BITE) {
        canal_walls = std::abs(point[kFloatPoint][2]) < canal.reach && point[kFloatPoint][1] < canal.low;
        cast.stopped = false;
    }
}

// The spot _LOAD_FISHING_DATA last loaded, and the water it set; null for the game's own.
const FishingSpot *fished = nullptr;
float              fished_water = 0.0f;

// The spot fished: the one last loaded, while in its town.
const FishingSpot *SpotFished() {
    return fished != nullptr && fished->map_no == MapNo ? fished : nullptr;
}

const FishingCanal *TownCanal() {
    const FishingSpot *spot = SpotFished();
    return spot != nullptr ? spot->canal : nullptr;
}

// A spot's collision file, read once.
struct SpotCollision {
    std::vector<scene::Triangle> triangles;
    std::vector<FloorDropZone>   zones;
};

const SpotCollision &ReadSpotCollision(const FishingSpot &spot) {
    static std::map<const FishingSpot *, SpotCollision> read;
    auto                                                found = read.find(&spot);
    if (found == read.end()) {
        std::vector<std::uint8_t> file;
        SpotCollision             collision;
        if (DataReadReadFile(FishingCollisionPath(spot), file)) {
            try {
                collision = {ReadFishingCollision(file), ReadFloorDropZones(file)};
            } catch (const std::exception &) {
                collision = {};
            }
        }
        found = read.emplace(&spot, std::move(collision)).first;
    }
    return found->second;
}

} // namespace

int FishKind(int kind, int rare_interval) {
    return MardanFishKind(kind, rare_interval);
}

float FishSize(float size, const FishInfo &info) {
    return AriseFishSize(size, info);
}

void FishSpotLoaded(int spot) {
    FishSteeringLoaded(spot);
}

void FishStep() {
    FishSteeringStep();
}

float BaitNoticeRadius(int item_no, float radius) {
    return VenomLureRadius(item_no, BaitFlamboyanceRadius(radius));
}

int PoolFish(int spot, int kind) {
    if (spot == kMatatakiWaterfall && kind == fish::kGummy) {
        return fish::kNiler;
    }
    if (spot == kEastHarbor && kind == fish::kPiccoly) {
        return fish::kGobbler;
    }
    return kind;
}

int SpotFish(int spot, int roll, int kind) {
    const FishingSpot *fishing = FishingSpotOfArea(spot);
    if (fishing == nullptr || fishing->fish.empty()) {
        return kind;
    }
    if (fishing->rare_one_in > 0 && roll % fishing->rare_one_in == 0) {
        return std::rand() % 5 == 0 ? fish::kBaronGarayan : fish::kMardanGarayan;
    }
    return fishing->fish[static_cast<std::size_t>(roll) % fishing->fish.size()];
}

int SpotFishCount(int spot, int count) {
    return FishingSpotOfArea(spot) != nullptr ? kSpotFishCount : count;
}

float SpotLineSegment(int spot) {
    const FishingSpot *fishing = FishingSpotOfArea(spot);
    if (fishing == nullptr) {
        return kRetailLineSegment;
    }
    return kRetailLineSegment * (fishing->line > 1.0f ? kCastStart : fishing->line);
}

float FishingSpotLoad(int area, float water) {
    fished = FishingSpotOfArea(area);
    fished_water = fished != nullptr && fished->map_no == kQueensMapNo ? CanalTideLevel(NowTime) : water;
    return fished_water;
}

float SpotFishDepth(float depth) {
    const FishingSpot *spot = SpotFished();
    if (spot == nullptr) {
        return depth;
    }
    return spot->map_no == kQueensMapNo && fished_water <= kLowTide ? kLowTideFishDepth : spot->fish_depth;
}

float LineSegment(int segment) {
    const FishingSpot *spot = SpotFished();
    if (spot == nullptr || segment <= kFloatPoint) {
        return distp;
    }
    return std::max((SpotFishDepth(spot->fish_depth) - kBaitUnderHang) / kHangSegments, kShortestHang);
}

void FishLineStepped(const float *angler, float yaw) {
    const FishingSpot *spot = SpotFished();
    if (spot == nullptr) {
        return;
    }
    if (spot->canal != nullptr) {
        StopAtCanal(*spot->canal);
    }
    PayOut(*spot, angler, yaw);
}

bool FloatOutOfCanal(const float *uki) {
    const FishingCanal *canal = TownCanal();
    if (canal == nullptr || chara_fishing != ED_FISHING_WAIT_BITE) {
        return false;
    }
    bool out = std::abs(uki[2]) > canal->drag;
    for (const std::array<float, 6> &leg : canal->legs) {
        out = out || InLeg(leg, uki, kLegInset);
    }
    // The line through a bridge: the rod over its deck, the float under it.
    for (int bridge : {0, 2}) {
        const std::array<float, 6> &span = canal->legs[static_cast<std::size_t>(bridge)];
        if (out || !(point[0][1] > canal->deck) || point[0][0] < span[0] || point[0][0] > span[1]) {
            continue;
        }
        out = uki[0] >= span[0] && uki[0] < span[1] && uki[1] < canal->deck;
        break;
    }
    if (out) {
        LogCast("out_of_canal");
    }
    return out;
}

bool FloatSettled() {
    const float *velocity = ukiv[0];
    return velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2] < kSettled;
}

bool FloorOverFloat(float floor) {
    return TownCanal() != nullptr && floor > point[kFloatPoint][1] + kOverhead;
}

float FishingCameraHeight(float height) {
    const FishingSpot *spot = SpotFished();
    if (spot == nullptr) {
        return height;
    }
    if (spot->camera_top > 0.0f) {
        return std::max(kLowestFishingCamera, std::min(spot->camera_height, spot->camera_top - WaterLevel));
    }
    return spot->camera_height;
}

bool FishingCameraInCanal() {
    const int state = chara_fishing;
    return EdMoveCharaInfo.fishing != 0 && TownCanal() != nullptr &&
           (state == ED_FISHING_WAIT_BITE || state == ED_FISHING_BITE_CHECK || state == ED_FISHING_HOOKED || state == ED_FISHING_BATTLE || state == ED_FISHING_LANDED);
}

void KeepFishingCameraInCanal(CCameraFollow &camera) {
    const FishingCanal *canal = TownCanal();
    if (canal == nullptr || camera.distance <= 0.0f) {
        return;
    }
    const float eye = camera.follow[2] + camera.distance * std::cos(camera.angle);
    const float wall = canal->wall - kInsideWalls;
    if (std::abs(eye) <= wall) {
        return;
    }
    // Slid along the wall: the turn, on the same side of the float along the canal, that puts the eye on it.
    const float across = std::clamp((std::copysign(wall, eye) - camera.follow[2]) / camera.distance, -1.0f, 1.0f);
    const float along = std::copysign(std::sqrt(1.0f - across * across), std::sin(camera.angle));
    camera.SetAngleSoon(std::atan2(along, across));
}

void FishingCatchCamera(CCameraFollow &camera, int state) {
    static bool  catching = false, kept = false;
    static float distance = 0.0f, angle = 0.0f;
    if (state == ED_FISHING_LANDED || state == ED_FISHING_RESULT || state == ED_FISHING_FINISH) {
        catching = true;
    } else if (catching) {
        catching = false;
        if (kept) {
            camera.SetDistance(distance);
            camera.SetAngle(angle);
        }
    } else {
        distance = camera.GetDistance();
        angle = camera.next_angle;
        kept = true;
    }
}

int AddSpotCollision(CCPoly *polys, int count, int capacity) {
    const FishingSpot *spot = FishingSpotOfMap(MapNo);
    if (spot == nullptr) {
        return count;
    }
    const SpotCollision &collision = ReadSpotCollision(*spot);
    return AddCollision(polys, DropPolys(polys, count, collision.zones), capacity, collision.triangles);
}

int DropPolys(CCPoly *polys, int count, std::span<const FloorDropZone> zones) {
    auto dropped = [&](const CCPoly &poly) {
        float low = std::min({poly.vertex[0][1], poly.vertex[1][1], poly.vertex[2][1]});
        float x = (poly.vertex[0][0] + poly.vertex[1][0] + poly.vertex[2][0]) / 3.0f;
        float z = (poly.vertex[0][2] + poly.vertex[1][2] + poly.vertex[2][2]) / 3.0f;
        for (const FloorDropZone &zone : zones) {
            float dx = x - zone.x, dz = z - zone.z;
            if (low >= zone.min_y && dx * dx + dz * dz < zone.radius * zone.radius) {
                return true;
            }
        }
        return false;
    };
    int kept = 0;
    for (int i = 0; i < count; i++) {
        if (!dropped(polys[i])) {
            polys[kept++] = polys[i];
        }
    }
    return kept;
}

int AddCollision(CCPoly *polys, int count, int capacity, std::span<const scene::Triangle> triangles) {
    // The surface kind and flags of a gathered poly.
    const CCPolyInfo info = count > 0 ? polys[0].info : CCPolyInfo{};
    for (const scene::Triangle &triangle : triangles) {
        if (count >= capacity) {
            break;
        }
        CCPoly &poly = polys[count];
        poly = CCPoly{};
        for (std::size_t v = 0; v < 3; v++) {
            for (std::size_t k = 0; k < 3; k++) {
                poly.vertex[v][k] = static_cast<float>(triangle[v][k]);
            }
            poly.vertex[v][3] = 1.0f;
        }
        float u[3], w[3];
        for (int k = 0; k < 3; k++) {
            u[k] = poly.vertex[1][k] - poly.vertex[0][k];
            w[k] = poly.vertex[2][k] - poly.vertex[0][k];
        }
        float normal[3] = {u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0]};
        float length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        if (length < 1e-6f) {
            continue;
        }
        for (int k = 0; k < 3; k++) {
            poly.normal[k] = normal[k] / length;
        }
        poly.info = info;
        count++;
    }
    return count;
}

void EnterFishingSign() {
    if (FishingSpotOfMap(MapNo) == nullptr) {
        return;
    }
    // Kept: the fixed textures point into it.
    static std::vector<std::uint8_t> bank;
    if (bank.empty() && !DataReadReadFile(kFishingSignPictures, bank)) {
        return;
    }
    TexManager.EnterIMGFile(bank.data(), -1, 0, 0);
}

} // namespace expanded
