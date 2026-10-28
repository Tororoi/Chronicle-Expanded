#include "expanded/town/fishing.hpp"

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
const std::vector<scene::Triangle> &SpotTriangles(const FishingSpot &spot) {
    static std::map<const FishingSpot *, std::vector<scene::Triangle>> read;
    auto                                                               found = read.find(&spot);
    if (found == read.end()) {
        std::vector<std::uint8_t>    file;
        std::vector<scene::Triangle> triangles;
        if (DataReadReadFile(FishingCollisionPath(spot), file)) {
            try {
                triangles = ReadFishingCollision(file);
            } catch (const std::exception &) {
                triangles.clear();
            }
        }
        found = read.emplace(&spot, std::move(triangles)).first;
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
        return std::rand() % 5 == 0 ? kBaronGarayan : kMardanGarayan;
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
    return spot != nullptr ? AddCollision(polys, count, capacity, SpotTriangles(*spot)) : count;
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
