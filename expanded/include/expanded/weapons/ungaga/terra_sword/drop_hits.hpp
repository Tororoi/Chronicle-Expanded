#pragma once

class CMonstorUnit;
struct COLLISION_HIT;

namespace expanded::terra_sword {

// The drops' hits (terra_sword.hpp): each one hit on a monster's body as a swing's, on its largest body sphere and at
// least kDropHitRadius wide, a share of the weapon's attack (at least 1, halves to even), with none of the weapon's
// abilities and marked as the drops' (BigRockHit), and the throw asked for. Each leaves the game free hit records to
// spare. A watched hit its monster did not take is planted again at the next step, up to kDropHitTries plants.

// The boulder landed at spot (x, floor, z): every living monster whose body comes within kRockBlastRadius, watched and
// thrown away from it. How many were hit.
int RockBlast(CMonstorUnit &unit, const float *spot);
// The boulder resting at spot: every living monster whose root is within kRockRadius of it, no throw.
void RockPin(CMonstorUnit &unit, const float *spot);
// The nut on monster's head: watched, shoved from the player.
bool NutBonk(CMonstorUnit &unit, int monster);

// Whether a hit is one of the drops'.
bool DropHit(const COLLISION_HIT &hit);

// Hooks: a step, before the drop's (watched hits not taken planted again); a monster took a hit.
void DropHitsStep(CMonstorUnit &unit);
void DropHitTaken(int monster, const COLLISION_HIT &hit);

// Nothing watched.
void DropHitsReset();

} // namespace expanded::terra_sword
