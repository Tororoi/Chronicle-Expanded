#pragma once

namespace expanded {

// Kitchen Knife "Spring's Blessing": stepping into a healing spring with the knife out blesses it for
// kBlessingSteps: the blade grows to kBlessingBlade times its length (the hit with it) and its attack is doubled.
// Standing in the spring keeps the blessing fresh, and each step into it mends the knife's WHP to its maximum.
inline constexpr int   kBlessingSteps = 60 * 60;
inline constexpr float kBlessingBlade = 3.0f;
inline constexpr int   kBlessingAttack = 2;

// Steps the blessing has left, 0 when there is none.
int SpringsBlessingLeft();

// Hook: HealingWater, every step before it heals: counts the blessing down and keeps the blade's size.
void SpringsBlessingStep();

// Hook: HealingWater, a step in which the player stands in a spring.
void SpringsBlessingSoak();

// Hook: ToanKey_Play: the attack its swings hit with.
int SpringsBlessingAttack(int attack);

} // namespace expanded
