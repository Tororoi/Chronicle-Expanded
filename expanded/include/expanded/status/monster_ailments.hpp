#pragma once

struct MONSTOR;

namespace expanded {

// The ailment bits of a hit on a monster (COLLISION_HIT::flags, a shot description's hit_flags) as CheckDmg reads them.
inline constexpr int kMonsterFreeze = 0x100, kMonsterPoison = 0x200, kMonsterGoo = 0x800;

// Gives monster the ailments in given as CheckDmg gives those its hit carries: none to a monster immune to them
// (status_chance 0); poison unless it is frozen or angry; freeze, which ends the others (or, frozen already, lifts it);
// goo unless another is on.
void GiveMonsterAilments(MONSTOR &live, int monster, int given);

} // namespace expanded
