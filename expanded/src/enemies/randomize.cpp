#include "expanded/enemies/randomize.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include "expanded/core/options.hpp"

namespace expanded {

namespace {

constexpr int kDungeons = 7;
constexpr int kGaciousEnhanced = 155;

[[maybe_unused]] const bool kRegistered =
    AddToggle({.key = kRandomizeEnemies,
               .label = "Randomize Enemies",
               .help = "\"Randomize Enemies\"\nEach floor's monsters\nare drawn from the\nwhole game on every\ndungeon visit."});

// Regular monsters, by MonstorTable index: every 'e' model but the mimics, Wine Keg and the boss-record Gacious.
const std::vector<int> &Pool() {
    static const std::vector<int> pool = [] {
        std::vector<int> kinds;
        auto             range = [&](int first, int last) {
            for (int kind = first; kind <= last; kind++) {
                kinds.push_back(kind);
            }
        };
        range(0, 28);                      // Master Jacket to Titan
        range(35, 69);                     // Arthur to Rockanoff
        kinds.push_back(77);               // Sam
        range(94, 100);                    // Gol to King Prickly
        range(111, 129);                   // Gemron (Fire) to Corcea (Enhanced)
        range(132, 140);                   // Gemron (Thunder) to Captain (Enhanced)
        range(143, 151);                   // Gemron (Wind) to Space Gyon (Enhanced)
        range(154, 162);                   // Gemron (Holy) to Living Armor (Enhanced)
        kinds.push_back(kGaciousEnhanced); // Gacious's own chance
        return kinds;
    }();
    return pool;
}

constexpr int kDemonShaft = 6;

// Each dungeon's mimic and king mimic, then Demon Shaft's from floor 40, 60 and 80.
constexpr std::array<DungeonMimics, kDungeons + 3> kMimics = {
    {
     {30, 29},   // Divine Beast Cave
        {71, 70},   // Wise Owl Forest
        {73, 72},   // Shipwreck
        {32, 31},   // Sun & Moon Temple
        {34, 33},   // Moon Sea
        {75, 74},   // Gallery of Time
        {130, 131}, // Demon Shaft
        {141, 142},
     {152, 153},
     {163, 164},
     }
};

// Boss and event floors, 0 for a dungeon's first.
const std::vector<int> kEventFloors[kDungeons] = {
    {3, 7, 14},
    {8, 16},
    {8, 17},
    {8, 17},
    {7, 14},
    {24},
    {99},
};

void StageRandomMix(int dungeon, int floor) {
    if (!RandomizeEnemies() || !RandomizedFloor(dungeon, floor)) {
        return;
    }
    StageFloor(floor, {RandomMix(dungeon, floor, RosterRandom()), RandomMix(dungeon, floor, RosterRandom())});
}

[[maybe_unused]] const bool kStager = (AddFloorStager(StageRandomMix), true);

} // namespace

bool RandomizeEnemies() {
    return OptionOn(kRandomizeEnemies);
}

std::span<const int> RandomizerPool() {
    return Pool();
}

DungeonMimics MimicsOf(int dungeon, int floor) {
    if (dungeon == kDemonShaft && floor >= 40) {
        return kMimics[kDemonShaft + std::min(floor / 20 - 1, 3)];
    }
    return dungeon >= 0 && dungeon < kDungeons ? kMimics[dungeon] : DungeonMimics{-1, -1};
}

bool RandomizedFloor(int dungeon, int floor) {
    if (dungeon < 0 || dungeon >= kDungeons) {
        return false;
    }
    const std::vector<int> &events = kEventFloors[dungeon];
    return std::find(events.begin(), events.end(), floor) == events.end();
}

std::mt19937 &RosterRandom() {
    static std::mt19937 random{std::random_device{}()};
    return random;
}

Roster RandomMix(int dungeon, int floor, std::mt19937 &random) {
    Roster                                 roster;
    std::uniform_real_distribution<double> chance(0.0, 1.0);
    DungeonMimics                          mimics = MimicsOf(dungeon, floor);
    if (chance(random) < kMimicChance) {
        roster.Add(mimics.mimic, false);
    }
    if (chance(random) < kKingMimicChance) {
        roster.Add(mimics.king, true);
    }
    const std::vector<int>               &pool = Pool();
    std::uniform_int_distribution<size_t> pick(0, pool.size() - 1);
    while (roster.Size() < kRosterSize) {
        int kind = pool[pick(random)];
        roster.Add(kind, SpawnsOnceNatively(kind));
    }
    return roster;
}

} // namespace expanded
