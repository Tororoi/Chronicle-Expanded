#pragma once

#include <span>

#include "expanded/data/town_scene.hpp"

namespace dc::game {
class CCPoly;
struct FishInfo;
} // namespace dc::game

namespace expanded {

// The game's fishing hooks, each calling the features that act there in turn.

// FishingLoadFish's spots, and the fish kinds (fish_info's index) Expanded names.
inline constexpr int kMatatakiWaterfall = 2;
inline constexpr int kEastHarbor = 3;
inline constexpr int kGobbler = 1;
inline constexpr int kGummy = 6;
inline constexpr int kNiler = 7;
inline constexpr int kUmadakara = 9;
inline constexpr int kPiccoly = 11;

// The kind of fish FishingLoadFish loads in a spot's slot: kind, the game's roll (-1 for none), as the weapons change it;
// rare_interval is the slot's one-in-n chance of a Garayan for the time of day.
int FishKind(int kind, int rare_interval);

// A fish's size as CFish::SetScale makes it: size, the game's roll for a kind of info, as the weapons change it.
float FishSize(float size, const FishInfo &info);

// FishingLoadFish having loaded a spot's fish (spot, the script's fish set), and FishingStepFish about to step them.
void FishSpotLoaded(int spot);
void FishStep();

// The distance from which fish notice the bait on the hook (FishingStepFish): radius, the game's for bait item_no, as
// the weapons' passives change it.
float BaitNoticeRadius(int item_no, float radius);

// The kind of a fish FishingLoadFish puts in spot, the game's drawn kind: the Matataki waterfall's Gummy is a Niler and
// East Harbor's Piccoly a Gobbler.
int PoolFish(int spot, int kind);

// FishingLoadFish at Expanded's spots (expanded/data/fishing_spots.hpp): the kind of a fish from the slot's roll, kind
// elsewhere; the fish count, count elsewhere; and the fishing line's segment length (distp), retail's elsewhere.
int   SpotFish(int spot, int roll, int kind);
int   SpotFishCount(int spot, int count);
float SpotLineSegment(int spot);

// How deep under the water FishingInitFish puts the fish of the town's spot; depth elsewhere.
float SpotFishDepth(float depth);

// The town's fishing spot's collision added after the count polys _LOAD_FISHING_DATA gathered, as far as capacity: the
// new count. AddCollision adds triangles so, each a poly like the first gathered.
int AddSpotCollision(dc::game::CCPoly *polys, int count, int capacity);
int AddCollision(dc::game::CCPoly *polys, int count, int capacity, std::span<const scene::Triangle> triangles);

// The fishing sign's picture, entered with the fixed textures of a town with a spot.
void EnterFishingSign();

} // namespace expanded
