#pragma once

#include "expanded/enemies/monsters.hpp"

class CMonstorUnit;

namespace expanded {

// A floor's model slots (each a kind set up) and monster slots, and a monster's hurt spheres.
inline constexpr int kModelSlots = 9;
inline constexpr int kFloorMonsters = 16;
inline constexpr int kMonsterSpheres = 16;

// The MonstorTable kind set up in a model slot of the floor, or -1.
int ModelKind(int model_slot);

// Whether a monster slot holds a live monster of a kind.
bool IsKind(const CMonstorUnit &unit, int monster, Monster kind);

// Called as a floor's kinds start being set up (before the first kind's model listeners), once a kind is set up
// in a model slot (after its stats and scales were reset), and once a monster is set up on the floor, before its
// script first runs.
using EnemyFloorListener = void (*)();
using EnemyModelListener = void (*)(CMonstorUnit &unit, int model_slot, int model_no);
using EnemySpawnListener = void (*)(CMonstorUnit &unit, int monster);
void AddEnemyFloorListener(EnemyFloorListener listener);
void AddEnemyModelListener(EnemyModelListener listener);
void AddEnemySpawnListener(EnemySpawnListener listener);

// Hooks: SetupBaseModel set a kind up, SetupViewMonstor set a monster up.
void EnemyModelSetUp(CMonstorUnit &unit, int model_slot, int model_no);
void EnemySpawned(CMonstorUnit &unit, int monster);

} // namespace expanded
