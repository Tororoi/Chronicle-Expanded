#pragma once

#include "expanded/enemies/scaling.hpp"

class CMonstorUnit;

namespace expanded {

// Each species on a new floor rolls once, with 1 - (1 - rate)^n for its n eligible monsters.
inline constexpr float kMinibossFirstRate = 0.12f;  // the rate until the floor has a miniboss
inline constexpr float kMinibossLaterRate = 0.012f; // the rate once it has one

// A miniboss's multipliers on its stats; the money it drops is multiplied by kMinibossMoney.
inline constexpr EnemyScale kMinibossScale = {.hp = 3.0f, .abs = 3.0f, .defense = 1.5f, .hardness = 1.5f, .melee = 1.5f, .shot = 1.5f};
inline constexpr int        kMinibossMoney = 4;

// Its model and body spheres grow by kMinibossSize, its attack spheres by kMinibossReach (the bigger model lifts
// them off short characters) and its lock-on cursor by kMinibossCursor.
inline constexpr float kMinibossSize = 1.5f;
inline constexpr float kMinibossReach = 1.6f;
inline constexpr float kMinibossCursor = 1.5f;

// Its script sees the player this many times nearer, so it gives chase and attacks from farther away; the
// long-reach kinds (plants, Opar, King Prickly, Rockanoff, Pirate's Chariot) by kMinibossRangeWide.
inline constexpr float kMinibossRange = 1.3f;
inline constexpr float kMinibossRangeWide = 1.5f;

// Its walk plays this many times slower, so its longer strides keep pace with the ground.
inline constexpr float kMinibossWalkSlowdown = 1.3f;

// Whether the floor's monster is a miniboss.
bool IsMiniboss(int monster);

// Makes the floor's monster a miniboss, carrying a drop rolled from its dungeon's tables.
void MakeMiniboss(CMonstorUnit &unit, int monster);

// Hooks: a new floor's monsters are about to be placed (BtArrengeMonstor), the model slot ArrangementPos picked
// for its placement n (the first placements on a return take the living minibosses' kinds), the floor or its
// back floor was populated (end of ArrangementPos), a monster was defeated (Step), the distance a monster's
// script measures (_GET_DISTANCE; argument_count 1 for the player's), and a drawn monster about to step its
// motions (DrawMonstor).
void  MinibossFloorEntered();
int   MinibossPlacedModel(const CMonstorUnit &unit, int placement, int model_slot);
void  MinibossesArranged(CMonstorUnit &unit);
void  MinibossDefeated(CMonstorUnit &unit, int monster);
float MinibossScriptDistance(int monster, int argument_count, float distance);
void  MinibossMotion(CMonstorUnit &unit, int monster);

// The new floor's draws: whether a species passes its chance, and a pick from 0 to count - 1 (the next species
// to roll, then its miniboss). Null restores the random ones (for tests).
using MinibossChance = bool (*)(float chance);
using MinibossPick = int (*)(int count);
void SetMinibossRoll(MinibossChance chance, MinibossPick pick);

} // namespace expanded
