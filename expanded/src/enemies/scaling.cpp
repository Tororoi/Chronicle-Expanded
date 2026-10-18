#include "expanded/enemies/scaling.hpp"

#include <algorithm>
#include <cmath>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"

namespace expanded {

namespace {

// Each model slot's kind's stats on the floor.
EnemyStats species_stats[kModelSlots];

// Each model slot's multipliers, each monster's own, and each monster's with its kind's (melee and shot
// also carrying its kind's damage over the native).
EnemyScale species_scale[kModelSlots];
EnemyScale monster_scale[kFloorMonsters];
EnemyScale effective[kFloorMonsters];

bool ValidModel(int model_slot) {
    return model_slot >= 0 && model_slot < kModelSlots;
}

bool ValidMonster(int monster) {
    return monster >= 0 && monster < kFloorMonsters;
}

float Ratio(int value, int native) {
    return native > 0 && value != native ? value / static_cast<float>(native) : 1.0f;
}

// Life, ABS, defense and hardness from the kind's stats with the monster's multipliers; life keeps its share.
void Apply(CMonstorUnit &unit, int monster) {
    MONSTOR &live = unit.monster[monster];
    if (!ValidModel(live.base_model)) {
        return;
    }
    const EnemyStats &stats = species_stats[live.base_model];
    EnemyStats        native = NativeStats(ModelKind(live.base_model));
    EnemyScale        scale = species_scale[live.base_model] * monster_scale[monster];
    effective[monster] = scale;
    effective[monster].melee *= Ratio(stats.melee, native.melee);
    effective[monster].shot *= Ratio(stats.shot, native.shot);

    int max_hp = ScaleStat(stats.hp, scale.hp);
    if (live.max_hp > 0 && live.hp > 0 && max_hp != live.max_hp) {
        live.hp = std::max(1, static_cast<int>(std::nearbyint(live.hp * (max_hp / static_cast<double>(live.max_hp)))));
    }
    live.max_hp = max_hp;
    live.exp = ScaleStat(stats.abs, scale.abs);
    live.defense = static_cast<s16>(std::min(ScaleStat(stats.defense, scale.defense), 0x7FFF));
    live.hardness = static_cast<s16>(std::min(ScaleStat(stats.hardness, scale.hardness), 0x7FFF));
}

// Applies a model slot's stats and multipliers to its monsters on the floor.
void ApplySpecies(CMonstorUnit &unit, int model_slot) {
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (unit.monster[monster].state != -1 && unit.monster[monster].base_model == model_slot) {
            Apply(unit, monster);
        }
    }
}

} // namespace

EnemyScale operator*(const EnemyScale &a, const EnemyScale &b) {
    return {a.hp * b.hp, a.abs * b.abs, a.defense * b.defense, a.hardness * b.hardness, a.melee * b.melee, a.shot * b.shot};
}

int ScaleStat(int value, float scale) {
    if (value <= 0 || scale == 1.0f) {
        return value;
    }
    float scaled = value * scale;
    return std::max(1, static_cast<int>(std::nearbyint(static_cast<double>(scaled))));
}

void SetSpeciesStats(CMonstorUnit &unit, int model_slot, const EnemyStats &stats) {
    if (!ValidModel(model_slot)) {
        return;
    }
    species_stats[model_slot] = stats;
    ApplySpecies(unit, model_slot);
}

EnemyStats SpeciesStats(int model_slot) {
    return ValidModel(model_slot) ? species_stats[model_slot] : EnemyStats{};
}

void ScaleSpecies(CMonstorUnit &unit, int model_slot, const EnemyScale &scale) {
    if (!ValidModel(model_slot)) {
        return;
    }
    species_scale[model_slot] = species_scale[model_slot] * scale;
    ApplySpecies(unit, model_slot);
}

void ScaleMonster(CMonstorUnit &unit, int monster, const EnemyScale &scale) {
    if (!ValidMonster(monster)) {
        return;
    }
    monster_scale[monster] = monster_scale[monster] * scale;
    Apply(unit, monster);
}

EnemyScale SpeciesScale(int model_slot) {
    return ValidModel(model_slot) ? species_scale[model_slot] : EnemyScale{};
}

EnemyScale MonsterScale(const CMonstorUnit &unit, int monster) {
    if (!ValidMonster(monster)) {
        return {};
    }
    return SpeciesScale(unit.monster[monster].base_model) * monster_scale[monster];
}

EnemyStats MonsterStats(const CMonstorUnit &unit, int monster) {
    if (!ValidMonster(monster)) {
        return {};
    }
    const MONSTOR &live = unit.monster[monster];
    EnemyScale     scale = MonsterScale(unit, monster);
    EnemyStats     stats = SpeciesStats(live.base_model);
    return {live.max_hp, live.exp, live.defense, live.hardness, ScaleStat(stats.melee, scale.melee), ScaleStat(stats.shot, scale.shot)};
}

void ResetSpeciesStats(const CMonstorUnit &unit, int model_slot) {
    if (!ValidModel(model_slot)) {
        return;
    }
    const MONSTOR_MODEL &model = unit.model[model_slot];
    EnemyStats           native = NativeStats(ModelKind(model_slot));
    species_stats[model_slot] = {model.max_hp, model.exp, model.defense, model.hardness, native.melee, native.shot};
    species_scale[model_slot] = {};
}

void ResetMonsterScale(CMonstorUnit &unit, int monster) {
    if (!ValidMonster(monster)) {
        return;
    }
    monster_scale[monster] = {};
    Apply(unit, monster);
}

int EnemyMeleeDamage(int monster, int damage) {
    return ValidMonster(monster) ? ScaleStat(damage, effective[monster].melee) : damage;
}

void EnemyShotFired(int monster) {
    if (!ValidMonster(monster) || effective[monster].shot == 1.0f || NowShotEffect->current_effect == -1) {
        return;
    }
    CSHOT_EFFECT *effect = ShotEffectAt(NowShotEffect->current_effect);
    if (effect->current_slot != -1) {
        effect->damage[effect->current_slot] = ScaleStat(effect->damage[effect->current_slot], effective[monster].shot);
    }
}

} // namespace expanded
