#pragma once

namespace dc::game {
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// 7th Heaven "Divine Guard": with the 7th Heaven in hand, or Super Steve carrying its SynthSphere, the guard blocks
// every monster attack and shot, the guard-breaking ones too (kind 3, which the game lets through a guard and knocks
// the player down with): they come in as a knockback hit (kind 2), so a guard stops them and an unguarded one throws
// the player back rather than down. It also has the Dark Cloud's Guard Crush (dark_cloud.hpp).
inline constexpr int kGuardBreakingHit = 3;
inline constexpr int kKnockbackHit = 2;

// The player's guard (movechara's BtActStatus): the guarding action, its hold (action and motion 9) and the mode the
// hold sets.
inline constexpr int kGuardAction = 6;
inline constexpr int kGuardHold = 9;
inline constexpr int kGuardHeld = 5;

bool DivineGuardActive();

// A monster's hit reaching the player, before the game reacts to its kind.
void DivineGuardHit(COLLISION_HIT &hit);

// A monster's hit has been reacted to, blocked by the player's guard or not. Any hit takes the player out of the
// action they were in, so a blocked one restarts the guard from its raise (motion 8) for a few frames before the hold
// (motion 9) plays again: a stagger on every block. With Divine Guard, a blocked hit leaves the guard held, its hold
// playing on; the push-back still throws the player back.
void DivineGuardBlocked(bool blocked);

} // namespace expanded
