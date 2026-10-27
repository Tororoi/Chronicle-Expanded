#pragma once

namespace dc::game {
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// A hit of the player's on monsters at at, as a swing's (CCollisionData::Set with damage, life, radius and the element
// bits attribute), the current member's, carrying the abilities of the weapon in hand: its record, or null with every
// record in use.
COLLISION_HIT *PlantWeaponHit(float *at, int damage, int life, float radius, int attribute = 0);

} // namespace expanded
