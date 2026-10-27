#pragma once

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// The Ice Queen off her floor (expanded/docs/bosses.md): her barrier is the other member of her cast
// (boss_casts.hpp); her other companions are shot effects fired for her script's requests.

// Her chest's height over her feet: where she fires her arrow from and her barrier follows her.
inline constexpr float kIceQueenChest = 10.1f;

// The dungeon step after the shot effects: her companions.
void IceQueenStep(CMonstorUnit &unit);

// Her defeat: ends her companions' shots.
void IceQueenDefeated();

} // namespace expanded
