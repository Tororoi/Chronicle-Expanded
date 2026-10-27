#pragma once

class CMonstorUnit;
struct MONSTOR;

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

} // namespace expanded
