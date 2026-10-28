#pragma once

// The fishing spots Expanded adds to towns that have none, and what dcdata builds for them: the town's scene with a
// fishing sign for each spot carrying its trigger, its mapinfo placing them, its event script with the fishing
// scripts, its messages with Norune's fishing menus and catch text, the signs' picture, the town's extra fishing
// collision, and what else the town takes (Queens' canal). Includes no game header, so dcdata can build with it.

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "expanded/data/data_build.hpp"
#include "expanded/data/stb.hpp"
#include "expanded/data/town_scene.hpp"

namespace expanded {

inline constexpr int   kFishingSignLabel = 400;
inline constexpr float kFishingSignRadius = 10.0f;

// A canal fished from its banks or its floor. A cast thrown with the float hanging low in it (under low, within reach
// of its middle) stops at its walls, z = ±wall; every cast stops at its bridges' legs; a waiting float is reeled in
// past ±drag, inside a leg, or under the bridge whose deck (over deck) the angler stands on; and the float does not
// lift onto a floor over it (a deck, a pipe).
struct FishingCanal {
    float wall, reach, low;
    float drag;
    float deck;
    // The legs: x, z and y from and to. legs[0] and legs[2] give the two bridges' spans of x.
    std::array<std::array<float, 6>, 4> legs;
};

// What a town with a spot takes beside its fishing (Queens' canal): parts added to its scene once the signs are in,
// given the disc files sources names (read in that order), and placed in its mapinfo; and scripts added to its event
// script.
struct TownAdditions {
    std::vector<std::string_view> sources;
    Bytes (*scene)(std::span<const unsigned char> scene, const std::vector<Bytes> &sources);
    Bytes (*cfg)(std::span<const unsigned char> cfg);
    void (*scripts)(stb::Appender &appender);
};

// A spot: a sign to fish from. A town may have several, each under a label of its own; its first spot's collision and
// additions are the town's.
struct FishingSpot {
    std::string_view code;   // the town's gedit folder
    int              map_no; // MapNo
    int              area;   // FishingLoadFish's spot, 5 and up
    // The sign: its part (its collision frame the part's name and "_a"), the GROUND block it is placed after, its
    // position and its yaw in degrees, whether it is solid, and the atari model its mapinfo block names.
    std::string_view   part = "kanban";
    std::string_view   anchor;
    std::array<int, 3> sign;
    int                sign_yaw;
    bool               sign_collision;
    std::string_view   atari;
    // The trigger, in the sign's frame, and the label its script answers to.
    std::array<float, 3> trigger;
    int                  label = kFishingSignLabel;
    // _LOAD_FISHING_DATA's rectangle (the casts' bounds and the collision gathered), water and ground.
    float x1, z1, x2, z2;
    float water, ground;
    // Where the player stands to fish, and facing; and the line from the rod to the float, retail's times line: over 1,
    // it starts short (kCastStart) and pays out as a cast flies (a bank high over its water needs it to reach).
    std::array<float, 3> stance;
    float                facing;
    float                line;
    // How deep under the water the fish swim (retail's 12); the line under the float hangs the bait there.
    float fish_depth;
    // The fish: each drawn evenly from fish (a kind listed twice twice as likely), but one in rare_one_in a Garayan
    // (none at 0), as the game draws its own spots'.
    std::vector<int> fish;
    int              rare_one_in;
    // The fishing collision added to what the spot's rectangle gathers.
    std::vector<scene::Triangle> (*collision)(std::span<const unsigned char> scene, std::span<const unsigned char> cfg);
    // The town's scene and mapinfo, the signs in them, as the spot changes them (the scene with the disc's mapinfo at
    // hand); null for as they are.
    Bytes (*edit_scene)(std::span<const unsigned char> scene, std::span<const unsigned char> cfg);
    Bytes (*edit_cfg)(std::span<const unsigned char> cfg);
    // The fishing camera's height (retail's 40); and, not 0, the highest its eye goes as it watches the float, no less
    // than kLowestFishingCamera over it (under walls it would pass over).
    float camera_height = 40.0f;
    float camera_top = 0.0f;
    // The canal the casts are thrown in; null for none.
    const FishingCanal  *canal = nullptr;
    const TownAdditions *additions = nullptr;
};

// A paying-out line's length from the rod to the float as a cast starts, retail's times this; and the lowest a camera
// kept under a spot's camera_top goes.
inline constexpr float kCastStart = 0.9f;
inline constexpr float kLowestFishingCamera = 12.0f;

// Fish kinds: fish_info's index (8 is none).
namespace fish {
inline constexpr int kBobo = 0, kGobbler = 1, kNonky = 2, kKaiji = 3, kBakuBaku = 4, kMardanGarayan = 5, kGummy = 6, kNiler = 7;
inline constexpr int kUmadakara = 9, kTarton = 10, kPiccoly = 11, kBon = 12, kHamahama = 13, kNegie = 14, kDen = 15, kHeela = 16;
inline constexpr int kBaronGarayan = 17;
} // namespace fish

// The fish at a spot of Expanded's.
inline constexpr int kSpotFishCount = 6;

// The spots, the first of a town (MapNo) and the one of a FishingLoadFish spot; null for none. A town's spots.
std::span<const FishingSpot>     FishingSpots();
const FishingSpot               *FishingSpotOfMap(int map_no);
const FishingSpot               *FishingSpotOfArea(int area);
std::vector<const FishingSpot *> TownFishingSpots(int map_no);

// The built files: the sign's picture bank, and a spot's fishing collision (FishingCollisionFile's format).
inline constexpr const char *kFishingSignPictures = "expanded/fishing/sign.img";
std::string                  FishingCollisionPath(const FishingSpot &spot);

// A fishing collision file: 'DCFC', version 1, MapNo, the triangle count, then 9 floats a triangle.
Bytes                        FishingCollisionFile(int map_no, const std::vector<scene::Triangle> &triangles);
std::vector<scene::Triangle> ReadFishingCollision(std::span<const unsigned char> file);

std::vector<DataBuild> FishingSpotBuilds();

} // namespace expanded
