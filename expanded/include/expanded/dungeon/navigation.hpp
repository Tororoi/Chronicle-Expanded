#pragma once

namespace dc::game {
class CCPoly;
} // namespace dc::game

namespace expanded {

// Paths over a dungeon floor for big monsters (expanded/docs/bosses.md): a grid of 16-unit cells over the floor's
// parts, a cell walkable for a body when there is floor under it near the body's ground and no wall within its
// clearance (its radius and a buffer), worked out as paths reach it and kept for the floor; paths by A* over it.

inline constexpr float kNavCell = 16.0f;

// The floor paths are found over: the dungeon's unless a test gives its own. Whether it is laid out on the 20 x 20
// grid of 160-unit tiles, whether a tile has a part, and its polygons near center (as setCollisionData gathers them).
struct NavigationFloor {
    bool (*gridded)();
    bool (*open)(int col, int row);
    int (*polys)(float *center, float radius, float height_ratio, CCPoly *polys, int limit);
};

// Uses floor (nullptr: the dungeon's), its cells forgotten.
void SetNavigationFloor(const NavigationFloor *floor);

// Where a body of the given clearance, on the ground at height ground, standing at from, should head to reach goal:
// goal itself when it is walkable and in a clear line; else the furthest point it can see on a path to goal, or to
// the walkable spot nearest goal it can reach. reached is that spot (goal when it is reachable). False on a floor
// without the grid (its parts placed freely), leaving both at goal.
bool NavigateTowards(const float *from, const float *goal, float clearance, float ground, float *waypoint, float *reached);

// The walkable spot a body could reach nearest point, searching out from start: point itself when it is walkable
// and reachable. False without the grid.
bool NavigateNearest(const float *start, const float *point, float clearance, float ground, float *spot);

// Whether a body could go straight from from to to: every cell on the line walkable past those it starts in (a body
// pressed nearer a wall than its clearance can still step away). True without the grid.
bool NavigateClear(const float *from, const float *to, float clearance, float ground);

// The walkable cells a body at start can reach (up to limit of them), kept for the floor: the region's number, or -1
// without the grid or walkable ground at start. It starts as the start's cell and grows by NavigateRegionGrow.
int NavigateRegion(const float *start, float clearance, float ground, int limit);

// Takes in more of the region: up to budget of its cells' neighbourhoods.
void NavigateRegionGrow(int region, int budget);

// The region's spot nearest point (point itself when inside it); false for no region.
bool NavigateRegionNearest(int region, const float *point, float *spot);

// Hook: a floor loaded (its cells forgotten).
void NavigationFloorLoaded();

} // namespace expanded
