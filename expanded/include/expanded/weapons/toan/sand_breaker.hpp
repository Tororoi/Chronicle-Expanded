#pragma once

#include "expanded/weapons/kick_back.hpp"

namespace dc::game {
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Sand Breaker "Sand Counter": with the Sand Breaker in hand, X pressed while Toan guards readies a counter for
// kCounterWindow frames (0.1 seconds). A monster's hit meeting the guard in that time is blocked, a guard-breaking one
// too, without the block's push back, and Toan swings at once: the opening combo swing as the Small Sword's Quick Draw
// plays it (its step-in, then its wind-up skipped), its hit dealing kCounterDamage times its damage. The monster whose
// blow it was is thrown back about 5 units; a shot is sent back at its shooter instead, as the Angel Gear's Guardian
// Reflector returns one. With no blow caught in that time Toan lets the guard go and swings as X would out of it,
// wind-up and all: pressed too early, he may be hit mid-swing; pressed late, the blow was an ordinary block.
inline constexpr int      kCounterWindow = 6;
inline constexpr float    kCounterDamage = 2.0f;
inline constexpr KickBack kCounterKick = {1.1f, 0.12f};

bool SandCounterActive();

// Frames the counter has left to catch a hit, 0 when none is readied.
int SandCounterReady();

// Whether the swing under way is a counter.
bool SandCountering();

// Hook: ToanKey_On, X pressed in play.
void SandCounterPress();

// Hook: each dungeon frame.
void SandCounterStep();

// A monster's hit reaching the player, before the game reacts to its kind.
void SandCounterHit(COLLISION_HIT &hit);

// A monster's hit has been reacted to, blocked by the guard or not.
void SandCounterBlocked(bool blocked, const COLLISION_HIT &hit);

// Whether a monster's hit passes the player by: the countered monster's, while the counter plays (its blow still in
// reach the frames after it was caught).
bool SandCounterPassesBy(const COLLISION_HIT &hit);

// Hook: ToanKey_Play: the attack its swings hit with.
int SandCounterAttack(int attack);

} // namespace expanded
