#pragma once

#include <span>

#include "expanded/data/town_scene.hpp"

namespace dc::game {
class CCPoly;
class CCameraFollow;
} // namespace dc::game

namespace expanded {

// The game's fishing hooks, each calling the features that act there in turn.

// The distance from which fish notice the bait on the hook (FishingStepFish): radius, the game's for bait item_no, as
// the weapons' passives change it.
float BaitNoticeRadius(int item_no, float radius);

// FishingLoadFish at Expanded's spots (expanded/data/fishing_spots.hpp): the kind of a fish from the slot's roll, kind
// elsewhere; the fish count, count elsewhere; and the length of the line's segments from the rod to the float (distp)
// as a cast starts, retail's elsewhere.
int   SpotFish(int spot, int roll, int kind);
int   SpotFishCount(int spot, int count);
float SpotLineSegment(int spot);

// How deep under the water FishingInitFish puts the fish of the town's spot; depth elsewhere.
float SpotFishDepth(float depth);

// The length FishLineInit and FishLineStep hold the line's segment to (1 to 23, from the rod): distp to the float
// (point 18), and under it, at the town's spot, the length that hangs the bait (3 under the hook) at the fish's depth.
float LineSegment(int segment);

// After EdMoveChara's FishLineStep, the angler at angler turned yaw: at the town's spot, a cast stopped at its canal's
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

// The fishing camera's height at the town's spot; height elsewhere.
float FishingCameraHeight(float height);

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
