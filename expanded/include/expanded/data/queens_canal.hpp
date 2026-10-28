#pragma once

// Queens' canal at low tide, as dcdata builds it: the Factory's ladder down the canal's south wall with its climb points
// and the point that says the tide is too high, the canal floor's walls and floor to walk on, the wading ripples and
// their ring texture, the canal water's finer grid, and the dock East Harbor's arrival puts a player on. The tide itself
// is the game's (expanded/town/queens_canal.hpp). Includes no game header, so dcdata can build with it.

#include <array>
#include <span>
#include <vector>

#include "expanded/data/data_build.hpp"
#include "expanded/data/fishing_spots.hpp"

namespace expanded {

inline constexpr int kQueensMapNo = 2;

// The canal's water by the time of day: morning's low tide (the floor at 0 walkable, the sign under the east bridge
// fishable), the game's own level afternoon and night, and dusk's high tide.
inline constexpr float kLowTide = 8.0f, kMidTide = 31.0f, kHighTide = 52.0f;
// How deep the fish swim at low tide, from either sign (over the floor; retail's 12 at the higher tides).
inline constexpr float kLowTideFishDepth = 7.0f;

// The labels: the canal floor sign's fishing, and the ladder's message.
inline constexpr int kCanalFloorSignLabel = 401, kLadderMessageLabel = 402;
// The event message the ladder's point shows (expanded/lang: gedit.e03.e03.23).
inline constexpr int kTideTooHighMessage = 23;

// The ladder: its part (the frame its points hang on), its world x and its climb points, from the floor and from the
// walkway.
inline constexpr const char          *kLadderPart = "hasigo";
inline constexpr float                kLadderX = 706.0f;
inline constexpr std::array<float, 3> kLadderBottom = {kLadderX, 0.0f, 40.9f}, kLadderTop = {kLadderX, 70.0f, 54.9f};

// The wading ripple, parked out of sight until it is put under the player, and the two on the ladder's rails.
inline constexpr const char *kWadingRipple = "wripple";
inline constexpr float       kRippleParkY = -3000.0f;
inline constexpr const char *kRailRipples[] = {"wriplL", "wriplR"};
inline constexpr float       kRailRippleX[] = {701.0f, 711.0f}, kRailRippleZ = 48.0f;

// East Harbor (MapNo 19): the event that puts a player arriving from the canal on its dock, where they stand, facing.
inline constexpr int                  kEastHarborMapNo = 19, kDockArrivalEvent = 404;
inline constexpr std::array<float, 3> kDockArrival = {-1311.0f, 7.0f, 875.7f};
inline constexpr float                kDockArrivalYaw = 0.0f;

// What Queens takes beside its fishing (fishing_spots.cpp).
extern const TownAdditions kQueensCanal;

// The Factory's ladder (e05a01's frame hasigo1) turned square to the canal, cut off where its foot ran into the
// Factory's floor and stood on the canal's: a one-frame .mds of world vertices.
Bytes CarveCanalLadder(std::span<const unsigned char> factory_scene);

// A flat square half wide each way, its texture once across it, in the material of Norune's water ripple (e01b22): a
// one-frame .mds.
Bytes RippleDecal(std::span<const unsigned char> norune_scene, float half);

// The ladder's event functions: its two ends (the climb up from the floor, the climb down from the walkway) and the
// script point at its top that says the tide is too high.
Bytes LadderEvents();

// The canal floor's triangles for walking (its floor, walls, end walls and the bridges' feet) and the walls the
// camera stops at, facing into the canal.
std::vector<scene::Triangle> CanalFloorCollision();
std::vector<scene::Triangle> CanalCameraWalls();

// The scene with the canal's west end closed over its floor (the walkway's ledge carried on to its end wall).
Bytes CapCanalWestEnd(std::span<const unsigned char> scene);

// The canal water's grid, 48 by 16 cells, made 64 by 14: square cells over the camera-following surface.
Bytes TuneCanalWater(std::span<const unsigned char> cfg);

// Queens' img.pak with e01b22 and its animation's frames (e01b23) an expanding ring, the bobber's ripple (the bank's
// own effect picture hamon) drawn larger.
Bytes RingRipples(std::span<const unsigned char> pak);

// The canal's own builds: East Harbor's event script with the dock arrival, and Queens' ring ripples.
std::vector<DataBuild> QueensCanalBuilds();

} // namespace expanded
