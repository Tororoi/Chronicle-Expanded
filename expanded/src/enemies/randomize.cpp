#include "dungeon.h"

#include <algorithm>
#include <array>
#include <vector>

#include "expanded/core/options.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/normalize.hpp"
#include "expanded/enemies/randomize.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/enemies/themes.hpp"

namespace expanded {

namespace {

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
        kinds.push_back(Monster::GACIOUS_ENHANCED);        // Gacious's own chance
        range(Monster::BOMB_GEMRON, Monster::ATLA_GEMRON); // Expanded's (expanded/enemies/species.hpp)
        return kinds;
    }();
    return pool;
}

// Boss and event floors, 0 for a dungeon's first.
const std::vector<int> kEventFloors[DUNGEON_COUNT] = {
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
    if (ModChance(kThemeChance)) {
        Roster themed = ThemedRoster(dungeon, floor, ModRandom());
        StageFloor(floor, {themed, themed});
        return;
    }
    StageFloor(floor, {RandomMix(dungeon, floor, ModRandom()), RandomMix(dungeon, floor, ModRandom())});
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
    if (dungeon < 0 || dungeon >= DUNGEON_COUNT) {
        return std::nullopt;
    }
    // Demon Shaft's own pair until its third band, then the next one each band.
    int line = dungeon == DUNGEON_DEMON_SHAFT ? DUNGEON_DEMON_SHAFT + std::max(DemonShaftBand(floor) - 1, 0) : dungeon;
    return DungeonMimics{kMimicLine[line], kKingMimicLine[line]};
}

bool RandomizedFloor(int dungeon, int floor) {
    if (dungeon < 0 || dungeon >= DUNGEON_COUNT) {
        return false;
    }
    const std::vector<int> &events = kEventFloors[dungeon];
    return std::find(events.begin(), events.end(), floor) == events.end();
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
