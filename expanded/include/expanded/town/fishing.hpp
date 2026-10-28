#pragma once

#include <span>

#include "expanded/data/town_scene.hpp"

namespace dc::game {
class CCPoly;
class CCameraFollow;
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
// elsewhere; the fish count, count elsewhere; and the length of the line's segments from the rod to the float (distp)
// as a cast starts, retail's elsewhere.
int   SpotFish(int spot, int roll, int kind);
int   SpotFishCount(int spot, int count);
float SpotLineSegment(int spot);

// _LOAD_FISHING_DATA at spot area: the water it sets (a Queens spot's the canal's tide level, water elsewhere). The spot
// is the one fished until the next load.
float FishingSpotLoad(int area, float water);

// How deep under the water FishingInitFish puts the fish of the spot fished (in Queens, at low tide, the canal floor's);
// depth elsewhere.
float SpotFishDepth(float depth);

// The length FishLineInit and FishLineStep hold the line's segment to (1 to 23, from the rod): distp to the float
// (point 18), and under it, at the spot fished, the length that hangs the bait (3 under the hook) at the fish's depth.
float LineSegment(int segment);

// After EdMoveChara's FishLineStep, the angler at angler turned yaw: at the spot fished, a cast stopped at its canal's
// walls and legs as it flies, and the line paid out from the rod to the float once the rod swings back and forward,
// the float and hook carried farther as it goes; drawn in again as the float comes back to the rod.
void FishLineStepped(const float *angler, float yaw);

// FishingCheckUkiHook, its casting bounds passed: whether the float at uki is reeled in as out of the canal (waiting,
// past its walls, in a leg, or under the bridge the angler stands on); and whether it has settled (over the water,
// only a settled float is reeled in: a long cast may still be flying).
bool FloatOutOfCanal(const float *uki);
bool FloatSettled();

// FishLineStep's float probe, down from 40 over the float: whether the floor it found is over the float, at a canal (a
// deck, a pipe), which the float does not lift onto.
bool FloorOverFloat(float floor);

// The fishing camera at the spot fished: its height, height elsewhere; whether it watches the float in a canal, the
// game's camera walls left out (the canal's banks have none, and the game would back the eye out along it); and there
// its eye kept inside the canal's walls, turned along them.
float FishingCameraHeight(float height);
bool  FishingCameraInCanal();
void  KeepFishingCameraInCanal(dc::game::CCameraFollow &camera);

// After EdMoveChara's fishing step (state chara_fishing): the camera's distance and the angle it turns to as they were
// before a catch's close-up, put back once it ends.
void FishingCatchCamera(dc::game::CCameraFollow &camera, int state);

// The town's fishing spot's collision added after the count polys _LOAD_FISHING_DATA gathered, as far as capacity: the
// new count. AddCollision adds triangles so, each a poly like the first gathered.
int AddSpotCollision(dc::game::CCPoly *polys, int count, int capacity);
int AddCollision(dc::game::CCPoly *polys, int count, int capacity, std::span<const scene::Triangle> triangles);

// The fishing sign's picture, entered with the fixed textures of a town with a spot.
void EnterFishingSign();

} // namespace expanded
