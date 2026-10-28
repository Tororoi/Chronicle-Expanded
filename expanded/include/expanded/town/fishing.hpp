#pragma once

#include <span>

#include "expanded/data/town_scene.hpp"

namespace dc::game {
class CCPoly;
} // namespace dc::game

namespace expanded {

struct FloorDropZone;

// The game's fishing hooks, each calling the features that act there in turn.

// The distance from which fish notice the bait on the hook (FishingStepFish): radius, the game's for bait item_no, as
// the weapons' passives change it.
float BaitNoticeRadius(int item_no, float radius);

// FishingLoadFish at Expanded's spots (expanded/data/fishing_spots.hpp): the kind of a fish from the slot's roll, kind
// elsewhere; the fish count, count elsewhere; and the fishing line's segment length (distp), retail's elsewhere.
int   SpotFish(int spot, int roll, int kind);
int   SpotFishCount(int spot, int count);
float SpotLineSegment(int spot);

// How deep under the water FishingInitFish puts the fish of the town's spot; depth elsewhere.
float SpotFishDepth(float depth);

// The town's fishing spot's collision applied to the count polys _LOAD_FISHING_DATA gathered: its dropped polys taken
// out, then its triangles added, as far as capacity; the new count. DropPolys takes out the polys in the zones (the
// rest kept in order), AddCollision adds triangles, each a poly like the first gathered.
int AddSpotCollision(dc::game::CCPoly *polys, int count, int capacity);
int DropPolys(dc::game::CCPoly *polys, int count, std::span<const FloorDropZone> zones);
int AddCollision(dc::game::CCPoly *polys, int count, int capacity, std::span<const scene::Triangle> triangles);

// The fishing sign's picture, entered with the fixed textures of a town with a spot.
void EnterFishingSign();

} // namespace expanded
