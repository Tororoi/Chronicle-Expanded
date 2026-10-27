#pragma once

namespace dc::game {
class CMonstorUnit;
struct MONSTOR;
} // namespace dc::game

namespace expanded {

// A monster on the floor (near the player or not) with life left.
bool MonsterLiving(const MONSTOR &monster);

// A living monster in play: out of any chest, its appearance done.
bool MonsterInPlay(const MONSTOR &monster);

// A monster checkCollision tests: on the floor and shown (out of its chest, or opened), killed or not.
bool MonsterShown(const MONSTOR &monster);

// The monster in a slot of the floor's unit, or null with no unit or a slot out of range.
const MONSTOR *FloorMonster(int monster);

// The living monster nearest point across the ground (x, z), within reach, or -1.
int NearestMonster(CMonstorUnit &unit, const float *point, float reach);

// The first shown monster with a hurt sphere in use that point is within reach of (the sphere's radius more), or -1;
// sphere, when given, is that sphere.
int MonsterTouching(const CMonstorUnit &unit, const float *point, float reach, int *sphere = nullptr);

// A monster's body as its hurt spheres posed this frame give it (a sphere farther than kUnposedSphere from its root
// across the ground was not posed this frame): how far (x, z) is across the ground from the nearest one's edge (0 inside
// one; with none, from its root less kBodyFallback), and its largest one (centre into centre, its radius returned; its
// root raised kBodyFallback and that wide with none) — where a hit planted on it surely lands.
inline constexpr float kUnposedSphere = 80.0f;
inline constexpr float kBodyFallback = 10.0f;

float MonsterBodyEdge(const CMonstorUnit &unit, int monster, float x, float z);
float MonsterBodyCentre(const CMonstorUnit &unit, int monster, float *centre);

// The same body, its underside: the lowest of its posed spheres' bottoms (its root's height with none); and how near a
// point (x, y, z) is to it, the least distance to a posed sphere's surface (below 0 inside one; the largest float with
// none).
float MonsterBodyUnderside(const CMonstorUnit &unit, int monster);
float MonsterBodyDistance(const CMonstorUnit &unit, int monster, const float *point);

} // namespace expanded
