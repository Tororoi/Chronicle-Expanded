#pragma once

namespace dc::game {
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Whether a hit is one of Ungaga's charge shots, whose landed hits don't wear his weapon.
bool ChargeShotHit(int attacker, const COLLISION_HIT *record);

} // namespace expanded
