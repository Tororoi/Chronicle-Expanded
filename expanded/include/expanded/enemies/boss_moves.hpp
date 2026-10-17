#pragma once

class CMonstorUnit;

namespace expanded {

// Supported bosses moving about a regular floor (expanded/docs/bosses.md): each heads round walls on a path
// (dungeon/navigation.hpp), its body kept clear of them by its radius and a buffer. A boss whose script works in
// its arena's coordinates (Dran) has them measured from where the floor placed it, and each point it heads for or
// looks to pulled to the nearest it can reach from there.

// How a script uses a point it names.
enum class BossPoint {
    Target, // where a shot flies: moved into the world only
    Place,  // where the boss goes or is put: also pulled to where it can reach
    Aim,    // where the boss is making for: also what it steers by
};

// Whether the monster's script works in its arena's coordinates: its setup's places are then where it goes.
bool BossInArenaFrame(int monster);

// Hooks: an opcode naming a point (into the world), _GET_POSITION (the world point into the script's frame),
// _SET_MOVE (the point the boss moves for), and Step just before the monster moves (its heading on a path).
void BossPointIn(CMonstorUnit &unit, int monster, float *point, BossPoint use);
void BossPointOut(int monster, float *point);
void BossMoveGoal(int monster, const float *goal);
void BossSteer(CMonstorUnit &unit, int monster);

// Hook: _CHK_MOVE_INFO (whether the way to a world point is clear): false when the boss's body would not fit.
bool BossLineClear(CMonstorUnit &unit, int monster, const float *to);

} // namespace expanded
