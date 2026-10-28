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

// Where a monster at (x, z) goes for another's body: the posed sphere whose edge is nearest (x, z) across the ground, at its
// centre, or kAimDepth inside it toward (x, z) when it is larger than that (a large monster's root lies deeper in its body
// than an attacker, blocked by that body, ever comes), at the monster's root height; its root with no sphere posed. Into
// point (x, y, z, 1).
inline constexpr float kAimDepth = 8.0f;
void                   MonsterAimPoint(const CMonstorUnit &unit, int monster, float x, float z, float *point);

// The same body, its underside: the lowest of its posed spheres' bottoms (its root's height with none); and how near a
// point (x, y, z) is to it, the least distance to a posed sphere's surface (below 0 inside one; the largest float with
// none).
float MonsterBodyUnderside(const CMonstorUnit &unit, int monster);
float MonsterBodyDistance(const CMonstorUnit &unit, int monster, const float *point);

} // namespace expanded
