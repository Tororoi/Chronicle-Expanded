#pragma once

#include "shot_utils.hpp"

class CMonstorUnit;
class CSHOT_EFFECT;

namespace expanded {

// A raised prop made solid: one column standing on the floor, armed with its place and size while the prop stands.
//   - A monster walking into it slides round it: its heading loses the part pointing into the column and keeps the part
//     along it, at the same speed (head-on, it takes the perpendicular), as a wall slides it, so its script, aiming at
//     its target every frame, is carried past. It meets the column at the column's radius plus its own (the radius it
//     keeps from other monsters).
//   - The player walking into it slides along it: his step loses the part pointing into it. He meets it at its radius
//     plus kColumnPlayerReach, his allowance against a monster.
//   - A monster's shot stops at it as at a wall (where the shot is), between kColumnShotBelow under the floor and the
//     column's top; the player's shots pass through.
// Neither is ever stopped moving away from it, or across it.
inline constexpr float kColumnPlayerReach = 6.0f;
inline constexpr float kColumnShotBelow = 2.0f;

// Armed standing at base (x, floor, z), radius wide, up to height top; passable again.
void SolidColumnArm(const float *base, float radius, float top);
void SolidColumnDisarm();
bool SolidColumnArmed();

// Hooks: a monster's step after the other monsters blocked it (CMonstorUnit::Step, after MoveChecMonster); the player's
// step after the monsters blocked it (the player's move, after CMonstorUnit::MoveCheck), velocity his step; a shot
// effect's collision test (CSHOT_EFFECT::Step), result as checkCollision answered it.
void                  SolidColumnMonsterMove(CMonstorUnit &unit, int monster);
void                  SolidColumnPlayerMove(const float *position, float *velocity);
SHOT_COLLISION_RESULT SolidColumnShot(const CSHOT_EFFECT &effect, int slot, SHOT_COLLISION_RESULT result, float *hit_position,
                                      const float *position);

} // namespace expanded
