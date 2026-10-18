#include "expanded/enemies/enemy_events.hpp"

#include <vector>

#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

int model_kind[kModelSlots] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};

template <typename Listener>
std::vector<Listener> &Listeners() {
    static std::vector<Listener> listeners;
    return listeners;
}

} // namespace

int ModelKind(int model_slot) {
    return model_slot >= 0 && model_slot < kModelSlots ? model_kind[model_slot] : -1;
}

bool IsKind(const CMonstorUnit &unit, int monster, Monster kind) {
    return monster >= 0 && monster < kFloorMonsters && unit.monster[monster].state != -1 &&
           ModelKind(unit.monster[monster].base_model) == No(kind);
}

void AddEnemyFloorListener(EnemyFloorListener listener) {
    Listeners<EnemyFloorListener>().push_back(listener);
}

void AddEnemyModelListener(EnemyModelListener listener) {
    Listeners<EnemyModelListener>().push_back(listener);
}

void AddEnemySpawnListener(EnemySpawnListener listener) {
    Listeners<EnemySpawnListener>().push_back(listener);
}

void EnemyModelSetUp(CMonstorUnit &unit, int model_slot, int model_no) {
    if (model_slot < 0 || model_slot >= kModelSlots) {
        return;
    }
    if (model_slot == 0) {
        for (EnemyFloorListener listener : Listeners<EnemyFloorListener>()) {
            listener();
        }
    }
    model_kind[model_slot] = model_no;
    ResetSpeciesStats(unit, model_slot);
    for (EnemyModelListener listener : Listeners<EnemyModelListener>()) {
        listener(unit, model_slot, model_no);
    }
}

void EnemySpawned(CMonstorUnit &unit, int monster) {
    if (monster < 0 || monster >= kFloorMonsters) {
        return;
    }
    ResetMonsterScale(unit, monster);
    for (EnemySpawnListener listener : Listeners<EnemySpawnListener>()) {
        listener(unit, monster);
    }
}

} // namespace expanded
