#pragma once

// The fishing spots Expanded adds to towns that have none, and what dcdata builds for them: the town's scene with the
// fishing sign carrying the spot's trigger, its mapinfo placing the sign, its event script with the fishing scripts,
// its messages with Norune's fishing menus and catch text, the sign's picture, and the spot's extra fishing collision.
// Includes no game header, so dcdata can build with it.

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "expanded/data/data_build.hpp"
#include "expanded/data/town_scene.hpp"

namespace expanded {

struct FishingSpot {
    std::string_view code;   // the town's gedit folder
    int              map_no; // MapNo
    int              area;   // FishingLoadFish's spot, 5 and up
    // The sign: the GROUND block it is placed after, its position and its yaw in degrees, whether it is solid, and
    // the atari model its mapinfo block names.
    std::string_view   anchor;
    std::array<int, 3> sign;
    int                sign_yaw;
    bool               sign_collision;
    std::string_view   atari;
    // The trigger, in the sign's frame.
    std::array<float, 3> trigger;
    // _LOAD_FISHING_DATA's rectangle (the casts' bounds and the collision gathered), water and ground.
    float x1, z1, x2, z2;
    float water, ground;
    // Where the player stands to fish, and facing; and the fishing line's length there, retail's times line (a bank
    // high over its water needs a longer one to reach it).
    std::array<float, 3> stance;
    float                facing;
    float                line;
    // How deep under the water the fish swim (retail's 12), the line's hook at rest a little above them.
    float fish_depth;
    // The fish: each drawn evenly from fish (a kind listed twice twice as likely), but one in rare_one_in a Garayan
    // (none at 0), as the game draws its own spots'.
    std::vector<int> fish;
    int              rare_one_in;
    // The fishing collision added to what the spot's rectangle gathers.
    std::vector<scene::Triangle> (*collision)(std::span<const unsigned char> scene, std::span<const unsigned char> cfg);
    // The town's scene and mapinfo, the sign in them, as the spot changes them (the scene with the disc's mapinfo at
    // hand); null for as they are.
    Bytes (*edit_scene)(std::span<const unsigned char> scene, std::span<const unsigned char> cfg);
    Bytes (*edit_cfg)(std::span<const unsigned char> cfg);

    // Polys gathered for fishing that the float must not rest on, dropped: any whose lowest corner stands at or over
    // min_y with its middle within radius of a part the mapinfo places whose name starts with parts (Brownboo's ladder
    // tops, over its pond); none when parts is empty.
    struct FloorDrop {
        std::string_view parts;
        float            radius = 0.0f;
        float            min_y = 0.0f;
    } floor_drop{};
};

// Where a spot drops high polys from what it gathers: FishingSpot::floor_drop round one of its parts.
struct FloorDropZone {
    float x, z, radius, min_y;
};

inline constexpr int   kFishingSignLabel = 400;
inline constexpr float kFishingSignRadius = 10.0f;

// Fish kinds: fish_info's index (8 is none).
namespace fish {
inline constexpr int kBobo = 0, kGobbler = 1, kNonky = 2, kKaiji = 3, kBakuBaku = 4, kMardanGarayan = 5, kGummy = 6, kNiler = 7;
inline constexpr int kUmadakara = 9, kTarton = 10, kPiccoly = 11, kBon = 12, kHamahama = 13, kNegie = 14, kDen = 15, kHeela = 16;
inline constexpr int kBaronGarayan = 17;
} // namespace fish

// The fish at a spot of Expanded's.
inline constexpr int kSpotFishCount = 6;

// The spots, and the one of a town (MapNo) or of a FishingLoadFish spot; null for none.
std::span<const FishingSpot> FishingSpots();
const FishingSpot           *FishingSpotOfMap(int map_no);
const FishingSpot           *FishingSpotOfArea(int area);

// The built files: the sign's picture bank, and a spot's fishing collision (FishingCollisionFile's format).
inline constexpr const char *kFishingSignPictures = "expanded/fishing/sign.img";
std::string                  FishingCollisionPath(const FishingSpot &spot);

// A fishing collision file: 'DCFC', version 1, MapNo, the triangle count, then 9 floats a triangle; version 2 when the
// spot drops polys, the zones' count and 4 floats a zone (x, z, radius, min_y) after them.
Bytes                        FishingCollisionFile(int map_no, const std::vector<scene::Triangle> &triangles, const std::vector<FloorDropZone> &zones = {});
std::vector<scene::Triangle> ReadFishingCollision(std::span<const unsigned char> file);
std::vector<FloorDropZone>   ReadFloorDropZones(std::span<const unsigned char> file);

std::vector<DataBuild> FishingSpotBuilds();

} // namespace expanded
