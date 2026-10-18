#include "expanded/enemies/roster.hpp"

#include <map>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/species.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kDungeons = 7;
constexpr int kKinds = kSpeciesCount;

// This visit's staged floors, and the roster of the floor side loaded last.
std::map<int, FloorRosters> staged;
int                         visits = 0;
const Roster               *loaded = nullptr;
BT_ENEMY_FLOOR              loaded_layout;

std::vector<FloorStager> &Stagers() {
    static std::vector<FloorStager> stagers;
    return stagers;
}

} // namespace

int Roster::Size() const {
    int size = 0;
    while (size < kRosterSize && model_no[size] != -1) {
        size++;
    }
    return size;
}

bool Roster::Contains(int kind) const {
    for (int i = 0; i < Size(); i++) {
        if (model_no[i] == kind) {
            return true;
        }
    }
    return false;
}

bool Roster::Add(int kind, bool spawn_once) {
    int size = Size();
    if (size >= kRosterSize || Contains(kind)) {
        return false;
    }
    model_no[size] = kind;
    once[size] = spawn_once;
    return true;
}

bool Roster::SetOnce(int kind, bool spawn_once) {
    for (int i = 0; i < Size(); i++) {
        if (model_no[i] == kind) {
            once[i] = spawn_once;
            return true;
        }
    }
    return false;
}

bool Roster::Remove(int kind) {
    int size = Size();
    for (int i = 0; i < size; i++) {
        if (model_no[i] != kind) {
            continue;
        }
        for (int j = i; j < size - 1; j++) {
            model_no[j] = model_no[j + 1];
            once[j] = once[j + 1];
        }
        model_no[size - 1] = -1;
        once[size - 1] = false;
        return true;
    }
    return false;
}

bool SpawnsOnceNatively(int model_no) {
    if (model_no < 0 || model_no >= kKinds) {
        return false;
    }
    if (Species(model_no) == nullptr) {
        return false;
    }
    s16 kind = Species(model_no)->kind;
    return kind != MONSTER_KIND_NORMAL && kind != MONSTER_KIND_MIMIC_SMALL;
}

void StageFloor(int floor, const FloorRosters &rosters) {
    FloorRosters &staging = staged[floor] = rosters;
    for (Roster *roster : {&staging.front, &staging.back}) {
        // An Expanded kind only once dcdata has built it.
        for (int i = roster->Size() - 1; i >= 0; i--) {
            if (!SpeciesBuilt(roster->model_no[i])) {
                roster->Remove(roster->model_no[i]);
            }
        }
        BossCastRoster(*roster);
    }
}

const FloorRosters *StagedFloor(int floor) {
    auto it = staged.find(floor);
    return it != staged.end() ? &it->second : nullptr;
}

void AddFloorStager(FloorStager stager, bool first) {
    Stagers().insert(first ? Stagers().begin() : Stagers().end(), stager);
}

int RosterVisits() {
    return visits;
}

const Roster *LoadedRoster() {
    return loaded;
}

void RosterVisitBegun() {
    visits++;
    staged.clear();
    loaded = nullptr;
}

BT_ENEMY_FLOOR *RosterFloorLayout(BT_ENEMY_FLOOR *layout, int ura) {
    loaded = nullptr;
    int dungeon = selectMapNo;
    int floor = UserStatus != nullptr ? UserStatus->cur_floor : -1;
    if (dungeon < 0 || dungeon >= kDungeons || floor < 0 || floor >= maxFloorTbl__3[dungeon]) {
        return layout;
    }
    for (FloorStager stager : Stagers()) {
        if (StagedFloor(floor) != nullptr) {
            break;
        }
        stager(dungeon, floor);
    }
    const FloorRosters *rosters = StagedFloor(floor);
    if (rosters == nullptr) {
        return layout;
    }
    loaded = ura != 0 ? &rosters->back : &rosters->front;
    loaded_layout = *layout;
    for (int i = 0; i < kRosterSize; i++) {
        loaded_layout.monster[i].monster_no = loaded->model_no[i];
    }
    return &loaded_layout;
}

bool RosterSpawnsOnce(const CMonstorUnit &unit, int model_slot) {
    if (loaded != nullptr && model_slot >= 0 && model_slot < loaded->Size()) {
        return loaded->once[model_slot];
    }
    s16 kind = unit.model[model_slot].kind;
    return kind != MONSTER_KIND_NORMAL && kind != MONSTER_KIND_MIMIC_SMALL;
}

} // namespace expanded
