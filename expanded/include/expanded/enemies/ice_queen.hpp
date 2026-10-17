#pragma once

class CMonstorUnit;

namespace expanded {

// The Ice Queen off her floor (expanded/docs/bosses.md): her barrier is the other member of her cast
// (boss_casts.hpp); her other companions are shot effects fired for her script's requests.

// Hooks: the dungeon step after the shot effects (her companions), and her defeat (ends her effects).
void IceQueenStep(CMonstorUnit &unit);
void IceQueenDefeated();

} // namespace expanded
