#include "expanded/enemies/boss_stats.hpp"

#include <iterator>

#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/normalize.hpp"

namespace expanded {

namespace {

// Each region's stats: Divine Beast Cave, Wise Owl Forest, Shipwreck, Sun and Moon Temple, Moon Sea, Gallery of
// Time, then Demon Shaft's floors 1-20 to 81-100. {} keeps the boss's own (its own dungeon).
struct BossLine {
    Monster    kind;
    EnemyStats stats[kEnemyRegions];
};

// clang-format off
const BossLine kLines[] = {
    {Monster::DRAN, {
        {},
        {600, 20, 10, 20, 62, 50},
        {1000, 30, 10, 20, 74, 69},
        {1600, 40, 10, 20, 91, 80},
        {2000, 50, 10, 20, 110, 90},
        {2000, 60, 10, 20, 125, 110},
        {8500, 70, 10, 20, 131, 118},
        {13500, 80, 10, 20, 137, 126},
        {21500, 90, 10, 20, 143, 134},
        {27500, 100, 10, 20, 149, 142},
        {50000, 110, 10, 20, 155, 150},
    }},
    {Monster::MASTER_UTAN, {
        {300, 10, 12, 0, 45, 25},
        {},
        {1200, 30, 12, 0, 74, 50},
        {1800, 40, 12, 0, 91, 69},
        {2200, 50, 12, 0, 110, 75},
        {2200, 60, 12, 0, 125, 100},
        {9500, 70, 12, 0, 131, 107},
        {15000, 80, 12, 0, 137, 114},
        {24000, 90, 12, 0, 143, 121},
        {30000, 100, 12, 0, 149, 128},
        {55000, 110, 12, 0, 155, 135},
    }},
    {Monster::ICE_QUEEN, {
        {200, 10, 10, 0, 0, 0},
        {400, 20, 10, 0, 0, 0},
        {},
        {1200, 40, 10, 0, 0, 0},
        {1600, 50, 10, 0, 0, 0},
        {1600, 60, 10, 0, 0, 0},
        {7000, 70, 10, 0, 0, 0},
        {11000, 80, 10, 0, 0, 0},
        {17500, 90, 10, 0, 0, 0},
        {22000, 100, 10, 0, 0, 0},
        {40000, 110, 10, 0, 0, 0},
    }},
    {Monster::ICE_BARRIER, {
        {20, 17, 5, 0, 0, 0},
        {50, 17, 5, 0, 0, 0},
        {},
        {150, 17, 5, 0, 0, 0},
        {200, 17, 5, 0, 0, 0},
        {300, 17, 5, 0, 0, 0},
        {850, 17, 5, 0, 0, 0},
        {1350, 17, 5, 0, 0, 0},
        {2000, 17, 5, 0, 0, 0},
        {2500, 17, 5, 0, 0, 0},
        {5000, 17, 5, 0, 0, 0},
    }},
    {Monster::KINGS_CURSE_COFFIN, {
        {400, 10, 10, 40, 0, 0},
        {800, 20, 10, 40, 0, 0},
        {1600, 30, 10, 40, 0, 0},
        {},
        {2500, 50, 10, 40, 0, 0},
        {2500, 60, 10, 40, 0, 0},
        {10500, 70, 10, 40, 0, 0},
        {17000, 80, 10, 40, 0, 0},
        {27000, 90, 10, 40, 0, 0},
        {34000, 100, 10, 40, 0, 0},
        {62500, 110, 10, 40, 0, 0},
    }},
    {Monster::KINGS_CURSE, {
        {200, 10, 10, 0, 45, 0},
        {400, 20, 10, 0, 62, 0},
        {800, 30, 10, 0, 74, 0},
        {},
        {1250, 50, 10, 0, 118, 0},
        {1250, 60, 10, 0, 134, 0},
        {5500, 70, 10, 0, 140, 0},
        {8500, 80, 10, 0, 146, 0},
        {13500, 90, 10, 0, 152, 0},
        {17000, 100, 10, 0, 158, 0},
        {31250, 110, 10, 0, 164, 0},
    }},
    {Monster::MINOTAUR_JOE, {
        {250, 10, 12, 40, 50, 0},
        {600, 20, 12, 40, 70, 0},
        {1000, 30, 12, 40, 86, 0},
        {1600, 40, 12, 40, 105, 0},
        {},
        {2000, 60, 12, 40, 140, 0},
        {8500, 70, 12, 40, 146, 0},
        {13500, 80, 12, 40, 152, 0},
        {21500, 90, 12, 40, 158, 0},
        {27500, 100, 12, 40, 164, 0},
        {50000, 110, 12, 40, 170, 0},
    }},
    {Monster::BLACK_KNIGHT, {
        {200, 10, 8, 100, 50, 35},
        {500, 20, 8, 100, 70, 50},
        {800, 30, 8, 100, 86, 69},
        {1300, 40, 8, 100, 105, 80},
        {1600, 50, 8, 100, 125, 90},
        {1600, 60, 8, 100, 140, 110},
        {7000, 70, 8, 100, 146, 118},
        {11000, 80, 8, 100, 152, 126},
        {17500, 90, 8, 100, 158, 134},
        {22000, 100, 8, 100, 164, 142},
        {40000, 110, 8, 100, 170, 150},
    }},
    {Monster::BLACK_KNIGHT_MOUNT, {
        {250, 10, 8, 100, 50, 55},
        {600, 20, 8, 100, 70, 70},
        {1000, 30, 8, 100, 86, 89},
        {1600, 40, 8, 100, 105, 100},
        {2000, 50, 8, 100, 125, 110},
        {2000, 60, 8, 100, 140, 130},
        {8500, 70, 8, 100, 146, 138},
        {13500, 80, 8, 100, 152, 146},
        {21500, 90, 8, 100, 158, 154},
        {27500, 100, 8, 100, 164, 162},
        {50000, 110, 8, 100, 170, 170},
    }},
};

// The Ice Queen's pieces' damage by region, the arrow's then the aura's; {} keeps theirs.
const int kPieces[kEnemyRegions][2] = {
    {35, 45}, {50, 62}, {}, {80, 91}, {100, 110}, {120, 125},
    {126, 131}, {132, 137}, {138, 143}, {144, 149}, {150, 155},
};
// clang-format on

} // namespace

std::optional<EnemyStats> BossRegionStats(int model_no, int region) {
    if (region < 0 || region >= kEnemyRegions) {
        return std::nullopt;
    }
    for (const BossLine &line : kLines) {
        if (No(line.kind) == model_no) {
            const EnemyStats &stats = line.stats[region];
            if (stats.hp == 0) {
                return std::nullopt;
            }
            return stats;
        }
    }
    return std::nullopt;
}

int IceQueenPieceDamage(int native_damage, int region) {
    if (region < 0 || region >= kEnemyRegions || kPieces[region][0] == 0) {
        return native_damage;
    }
    if (native_damage == kIceQueenArrowDamage) {
        return kPieces[region][0];
    }
    if (native_damage == kIceQueenAuraDamage) {
        return kPieces[region][1];
    }
    return native_damage;
}

} // namespace expanded
