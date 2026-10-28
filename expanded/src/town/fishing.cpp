#include "expanded/town/fishing.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <vector>

#include "collision.hpp"
#include "data/data_read.hpp"
#include "expanded/data/fishing_spots.hpp"
#include "expanded/weapons/ungaga/scorpion.hpp"
#include "expanded/weapons/xiao/flamingo.hpp"
#include "main.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

// fishing.cpp's distp: the length of each of the line's segments.
constexpr float kRetailLineSegment = 1.6666666f;

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

float BaitNoticeRadius(int item_no, float radius) {
    return VenomLureRadius(item_no, BaitFlamboyanceRadius(radius));
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
    return fishing != nullptr ? kRetailLineSegment * fishing->line : kRetailLineSegment;
}

float SpotFishDepth(float depth) {
    const FishingSpot *spot = FishingSpotOfMap(MapNo);
    return spot != nullptr ? spot->fish_depth : depth;
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
