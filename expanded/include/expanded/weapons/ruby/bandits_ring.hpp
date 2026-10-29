#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Bandit's Ring "Double Take": a monster robbed once, by any steal, can be robbed once more by a hit of the ring's:
// Ruby's with the ring in her hand, or Xiao's with Super Steve carrying its SynthSphere (the sphere carries the ring's
// Steal ability). The second steal takes the monster's own item again, its kind's, on the game's roll and bag check.
// A monster is robbed kMostSteals times at most; one set up again starts unrobbed.
inline constexpr int kMostSteals = 2;

// The times a floor monster has been robbed.
int MonsterSteals(int monster);

// Hooks: a hit reached monster (CheckDmg, before its steal): a monster robbed once has its kind's item for a hit of the
// ring's and none for any other; a monster's item stolen.
void DoubleTakeHitTaken(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit);
void DoubleTakeStolen(const CMonstorUnit &unit, int monster, int item);

} // namespace expanded
