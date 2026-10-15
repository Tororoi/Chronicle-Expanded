#pragma once

class CMonstorUnit;

namespace expanded {

// A monster's stats: life, ABS (MONSTOR::exp), defense, hardness and its strongest melee hit and shot.
struct EnemyStats {
    int hp = 0;
    int abs = 0;
    int defense = 0;
    int hardness = 0;
    int melee = 0; // 0: no melee hit
    int shot = 0;  // 0: no scripted shot damage

    bool operator==(const EnemyStats &) const = default;
};

// Multipliers on each of a monster's stats; melee and shot apply to every hit and shot it deals.
struct EnemyScale {
    float hp = 1.0f;
    float abs = 1.0f;
    float defense = 1.0f;
    float hardness = 1.0f;
    float melee = 1.0f;
    float shot = 1.0f;

    bool operator==(const EnemyScale &) const = default;
};

// Each stat's multipliers multiplied.
EnemyScale operator*(const EnemyScale &a, const EnemyScale &b);

// value * scale rounded half to even, at least 1; 0 and below stay as they are.
int ScaleStat(int value, float scale);

// A MonstorTable kind's stats: its record and the damage its dun/monstor script deals, as dcdata read it
// into the data directory (kScriptDamageFile); no melee or shot when the data lacks that file.
EnemyStats NativeStats(int model_no);

// The MonstorTable kind set up in a model slot of the floor, or -1.
int ModelKind(int model_slot);

// A kind's stats on the floor (a model slot), before multipliers: its native stats until set. Setting them
// reaches its monsters on the floor now and every later spawn; each hit and shot scales by melee and shot
// over the native strongest. Reset when the slot is set up again.
void       SetSpeciesStats(CMonstorUnit &unit, int model_slot, const EnemyStats &stats);
EnemyStats SpeciesStats(int model_slot);

// Multiplies the stats of a kind on the floor: its monsters now and every later spawn. Reset when the slot
// is set up again.
void ScaleSpecies(CMonstorUnit &unit, int model_slot, const EnemyScale &scale);

// Multiplies one monster's stats, on top of its kind's. Reset when the monster slot spawns again.
void ScaleMonster(CMonstorUnit &unit, int monster, const EnemyScale &scale);

// A kind's multipliers, and a monster's with its kind's.
EnemyScale SpeciesScale(int model_slot);
EnemyScale MonsterScale(const CMonstorUnit &unit, int monster);

// A monster's stats now: its life, ABS, defense and hardness, and its kind's damage with its multipliers.
EnemyStats MonsterStats(const CMonstorUnit &unit, int monster);

// Called once a kind is set up in a model slot (after its stats and scales were reset), and once a monster
// is set up on the floor, before its script first runs.
using EnemyModelListener = void (*)(CMonstorUnit &unit, int model_slot, int model_no);
using EnemySpawnListener = void (*)(CMonstorUnit &unit, int monster);
void AddEnemyModelListener(EnemyModelListener listener);
void AddEnemySpawnListener(EnemySpawnListener listener);

// Hooks: SetupBaseModel set a kind up, SetupViewMonstor set a monster up, CheckDmg's melee hit and a
// monster's shot just fired (scales the shot's damage).
void EnemyModelSetUp(CMonstorUnit &unit, int model_slot, int model_no);
void EnemySpawned(CMonstorUnit &unit, int monster);
int  EnemyMeleeDamage(int monster, int damage);
void EnemyShotFired(int monster);

} // namespace expanded
