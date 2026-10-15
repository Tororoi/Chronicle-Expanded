#pragma once

struct BT_ENEMY_FLOOR;
class CMonstorUnit;

namespace expanded {

// Kinds one side of a floor lays out, as BT_ENEMY_FLOOR's entries.
inline constexpr int kRosterSize = 9;

// One side of a floor's monsters: MonstorTable kinds, -1 past the last, and whether each spawns at most once.
struct Roster {
    int  model_no[kRosterSize] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};
    bool once[kRosterSize] = {};

    int  Size() const;
    bool Contains(int model_no) const;
    // Appends a kind; false when full or already there.
    bool Add(int model_no, bool once);
    // Sets an entry's spawn-once flag; false for a kind not in the roster.
    bool SetOnce(int model_no, bool once);

    bool operator==(const Roster &) const = default;
};

// A floor's front and back rosters.
struct FloorRosters {
    Roster front;
    Roster back;
};

// Whether a kind spawns once per floor by its own record: anything but an ordinary monster or a small mimic.
bool SpawnsOnceNatively(int model_no);

// Stages a floor of the dungeon being visited; it keeps the rosters until the visit ends.
void StageFloor(int floor, const FloorRosters &rosters);

// The floor's staged rosters this visit, or null.
const FloorRosters *StagedFloor(int floor);

// Asked, in the order added, before a floor's monsters first load in a visit, until one stages it.
using FloorStager = void (*)(int dungeon, int floor);
void AddFloorStager(FloorStager stager);

// The roster the floor side loaded last uses, or null for its own layout.
const Roster *LoadedRoster();

// Hooks: GameInit began a dungeon visit; BtLoadMonstor reads a floor side's layout (returns the one to load);
// ArrangementPos asks whether a loaded kind may spawn only once.
void            RosterVisitBegun();
BT_ENEMY_FLOOR *RosterFloorLayout(BT_ENEMY_FLOOR *layout, int ura);
bool            RosterSpawnsOnce(const CMonstorUnit &unit, int model_slot);

} // namespace expanded
