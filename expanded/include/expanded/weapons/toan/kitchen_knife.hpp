#pragma once

namespace dc::game {
struct COLLISION_HIT;
} // namespace dc::game

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

// Super Steve carrying the Kitchen Knife's SynthSphere fires Aquamarines (kitchen_knife_shot.hpp), and each hit throws
// the monster back half as far as a Baselard pellet's (kick_back.hpp).
bool KitchenKnifePellets();

// Whether a hit is a Super Steve pellet with the sphere.
bool KitchenKnifeHit(const COLLISION_HIT &record);

} // namespace expanded
