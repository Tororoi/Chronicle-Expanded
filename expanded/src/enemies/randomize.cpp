#include "expanded/enemies/randomize.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include "expanded/core/options.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/enemies/themes.hpp"

namespace expanded {

namespace {

constexpr int kDungeons = 7;

[[maybe_unused]] const bool kRegistered =
    AddToggle({.key = kRandomizeEnemies,
               .label = "Randomize Enemies",
               .help = "\"Randomize Enemies\"\nEach floor's monsters\nare drawn from the\nwhole game on every\ndungeon visit."});

// Regular monsters: every 'e' model but the mimics, Wine Keg and the boss-record Gacious, then Expanded's.
const std::vector<Monster> &Pool() {
    static const std::vector<Monster> pool = [] {
        std::vector<Monster> kinds;
        auto                 range = [&](Monster first, Monster last) {
            for (int kind = No(first); kind <= No(last); kind++) {
                kinds.push_back(static_cast<Monster>(kind));
            }
        };
        range(Monster::MASTER_JACKET, Monster::TITAN);
        range(Monster::ARTHUR, Monster::ROCKANOFF);
        kinds.push_back(Monster::SAM);
        range(Monster::GOL, Monster::KING_PRICKLY);
        range(Monster::GEMRON_FIRE, Monster::CORCEA_ENHANCED);
        range(Monster::GEMRON_THUNDER, Monster::CAPTAIN_ENHANCED);
        range(Monster::GEMRON_WIND, Monster::SPACE_GYON_ENHANCED);
        range(Monster::GEMRON_HOLY, Monster::LIVING_ARMOR_ENHANCED);
        kinds.push_back(Monster::GACIOUS_ENHANCED); // Gacious's own chance
        kinds.push_back(Monster::BOMB_GEMRON);      // Expanded's (expanded/enemies/species.hpp)
        return kinds;
    }();
    return pool;
}

constexpr int kDemonShaft = 6;

// Each dungeon's mimic and king mimic, then Demon Shaft's from floor 40, 60 and 80.
constexpr std::array<DungeonMimics, kDungeons + 3> kMimics = {
    {
     {Monster::MIMIC_DBC, Monster::KING_MIMIC_DBC}, // Divine Beast Cave
        {Monster::MIMIC_WOF, Monster::KING_MIMIC_WOF}, // Wise Owl Forest
        {Monster::MIMIC_SW, Monster::KING_MIMIC_SW},   // Shipwreck
        {Monster::MIMIC_SMT, Monster::KING_MIMIC_SMT}, // Sun & Moon Temple
        {Monster::MIMIC_MS, Monster::KING_MIMIC_MS},   // Moon Sea
        {Monster::MIMIC_GOT, Monster::KING_MIMIC_GOT}, // Gallery of Time
        {Monster::MIMIC_DS, Monster::KING_MIMIC_DS},   // Demon Shaft
        {Monster::MIMIC_DS_ENHANCED, Monster::KING_MIMIC_DS_ENHANCED},
     {Monster::MIMIC_DS_ENHANCED_TWICE, Monster::KING_MIMIC_DS_ENHANCED_TWICE},
     {Monster::MIMIC_DS_ENHANCED_THRICE, Monster::KING_MIMIC_DS_ENHANCED_THRICE},
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
    // A themed floor shares one roster between its sides.
    if (std::uniform_real_distribution<double>(0.0, 1.0)(RosterRandom()) < kThemeChance) {
        Roster themed = ThemedRoster(dungeon, floor, RosterRandom());
        StageFloor(floor, {themed, themed});
        return;
    }
    StageFloor(floor, {RandomMix(dungeon, floor, RosterRandom()), RandomMix(dungeon, floor, RosterRandom())});
}

[[maybe_unused]] const bool kStager = (AddFloorStager(StageRandomMix), true);

} // namespace

bool RandomizeEnemies() {
    return OptionOn(kRandomizeEnemies);
}

std::span<const Monster> RandomizerPool() {
    return Pool();
}

std::optional<DungeonMimics> MimicsOf(int dungeon, int floor) {
    if (dungeon == kDemonShaft && floor >= 40) {
        return kMimics[kDemonShaft + std::min(floor / 20 - 1, 3)];
    }
    if (dungeon < 0 || dungeon >= kDungeons) {
        return std::nullopt;
    }
    return kMimics[dungeon];
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
    std::optional<DungeonMimics>           mimics = MimicsOf(dungeon, floor);
    if (chance(random) < kMimicChance && mimics) {
        roster.Add(mimics->mimic, false);
    }
    if (chance(random) < kKingMimicChance && mimics) {
        roster.Add(mimics->king, true);
    }
    const std::vector<Monster>           &pool = Pool();
    std::uniform_int_distribution<size_t> pick(0, pool.size() - 1);
    int                                   size = roster.Size() + RollKindCount(kRosterSize - roster.Size(), random);
    while (roster.Size() < size) {
        Monster kind = pool[pick(random)];
        if (SpeciesBuilt(No(kind))) {
            roster.Add(kind, SpawnsOnceNatively(kind));
        }
    }
    return roster;
}

int RollKindCount(int most, std::mt19937 &random) {
    most = std::min(most, kRosterSize);
    if (most < 1) {
        return 0;
    }
    std::discrete_distribution<int> count(kKindCountWeights.begin(), kKindCountWeights.begin() + most);
    return count(random) + 1;
}

} // namespace expanded
