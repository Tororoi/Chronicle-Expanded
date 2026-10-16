#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/randomize.hpp"
#include "expanded/enemies/themes.hpp"
#include "monstorunit.hpp"

namespace {

using expanded::Monster;
using expanded::No;

constexpr int kDivineBeastCave = 0;
constexpr int kDemonShaft = 6;

const expanded::Theme &Named(std::string_view name) {
    for (const expanded::Theme &theme : expanded::Themes()) {
        if (theme.name == name) {
            return theme;
        }
    }
    static const expanded::Theme none;
    return none;
}

bool Contains(const std::vector<Monster> &kinds, Monster kind) {
    return std::find(kinds.begin(), kinds.end(), kind) != kinds.end();
}

int Find(const expanded::Roster &roster, Monster kind) {
    for (int i = 0; i < roster.Size(); i++) {
        if (roster.model_no[i] == No(kind)) {
            return i;
        }
    }
    return -1;
}

} // namespace

TEST(EnemyThemes, TheGroupsAreRegularMonsters) {
    EXPECT_EQ(expanded::Themes().size(), 33u);
    for (const expanded::Theme &theme : expanded::Themes()) {
        EXPECT_FALSE(theme.members.empty()) << theme.name;
        EXPECT_LE(theme.members.size(), 10u) << theme.name;
        for (Monster kind : theme.members) {
            EXPECT_EQ(MonstorTable[No(kind)].kind, MONSTER_KIND_NORMAL) << theme.name << " " << No(kind);
        }
    }
    for (int dungeon = 0; dungeon < 7; dungeon++) {
        for (Monster kind : expanded::DungeonNatives(dungeon)) {
            EXPECT_EQ(MonstorTable[No(kind)].kind, MONSTER_KIND_NORMAL) << dungeon << " " << No(kind);
        }
    }
}

TEST(EnemyThemes, TheTablesKeepTheirIndexes) {
    const std::vector<std::vector<int>> members = {
        {38, 39, 40, 41, 42},
        {146, 128, 120, 138, 159},
        {10, 11, 12, 13, 14, 15, 16},
        {51, 65, 66},
        {77, 26, 61},
        {23, 24, 21, 95, 94},
        {111, 121, 132, 143, 154},
        {111, 118, 26, 51},
        {121, 57, 77, 65},
        {132, 135, 9, 49, 61},
        {143, 27, 25, 144, 50},
        {154, 155, 136, 122, 125},
        {1, 0, 155, 122, 144, 45},
        {63, 19, 20, 98, 64},
        {97, 27, 28, 69},
        {8, 59, 60, 5},
        {3, 4, 113, 55},
        {52, 53, 68},
        {35, 37, 56, 21, 47},
        {46, 17, 133, 112},
        {6, 59},
        {43, 147},
        {9, 99},
        {100, 58, 18, 99},
        {7, 36, 1, 119, 4, 53},
        {22, 2},
        {95, 94, 54},
        {117, 1, 122, 144},
        {111, 119, 115, 112, 116, 117, 118, 120, 113, 114},
        {121, 129, 128, 124, 122, 123, 125, 126, 127},
        {132, 140, 138, 135, 134, 133, 137, 139, 136},
        {143, 147, 144, 150, 148, 151, 149, 146, 145},
        {154, 162, 158, 159, 156, 155, 160, 161, 157},
    };
    const std::vector<std::vector<int>> singles = {
        {},
        {},
        {},
        {},
        {},
        {23, 95, 94},
        {},
        {},
        {},
        {},
        {},
        {},
        {45},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {100},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
    };
    const std::vector<std::vector<int>> natives = {
        {1, 0, 2, 3, 52, 36, 51, 69, 97, 96, 98},
        {8, 9, 5, 6, 7, 4, 18, 100, 99, 10, 11, 12, 13, 14, 15, 16},
        {23, 24, 19, 20, 21, 22, 67, 60, 77},
        {25, 44, 50, 43, 63, 27, 28, 56, 65, 94, 95, 26},
        {35, 17, 49, 58, 64, 62, 48, 28, 68, 54},
        {37, 61, 66, 57, 39, 46, 59, 40, 53, 38, 42, 45, 47, 55, 41},
        {111, 119, 115, 112, 116, 117, 118, 120, 113, 114, 121, 129, 128, 124, 122, 123, 125, 126, 127, 132, 140, 138, 135, 134, 133, 137, 139, 136, 143, 147, 144, 150, 148, 151, 149, 146, 145, 154, 162, 158, 159, 156, 155, 160, 161, 157},
    };
    auto indexes = [](const std::vector<Monster> &kinds) {
        std::vector<int> found;
        for (Monster kind : kinds) {
            found.push_back(No(kind));
        }
        return found;
    };
    ASSERT_EQ(expanded::Themes().size(), members.size());
    for (size_t i = 0; i < members.size(); i++) {
        EXPECT_EQ(indexes(expanded::Themes()[i].members), members[i]) << expanded::Themes()[i].name;
        EXPECT_EQ(indexes(expanded::Themes()[i].single), singles[i]) << expanded::Themes()[i].name;
    }
    for (int dungeon = 0; dungeon < 7; dungeon++) {
        EXPECT_EQ(indexes(expanded::DungeonNatives(dungeon)), natives[dungeon]) << dungeon;
    }
}

TEST(EnemyThemes, DemonShaftsThemesPullHardestAtHome) {
    const expanded::Theme &fire = Named("Demon Shaft: Fire");
    EXPECT_DOUBLE_EQ(expanded::ThemeWeight(fire, kDemonShaft), 4.0);
    EXPECT_DOUBLE_EQ(expanded::ThemeWeight(fire, kDivineBeastCave), 0.1);
    EXPECT_DOUBLE_EQ(expanded::ThemeWeight(fire, 5), 0.1 + 0.9 * 5.0 / 6.0);
    EXPECT_DOUBLE_EQ(expanded::ThemeWeight(Named("Bait"), 2), 3.0);
    EXPECT_DOUBLE_EQ(expanded::ThemeWeight(Named("Gorgon"), 2), 1.0);
}

TEST(EnemyThemes, ARosterFillsTheFloorWithItsGroup) {
    std::mt19937               random(11);
    std::map<std::string, int> seen;
    for (int dungeon : {kDivineBeastCave, 3, kDemonShaft}) {
        for (int roll = 0; roll < 3000; roll++) {
            int                     floor = dungeon == kDemonShaft ? 85 : 5;
            std::string_view        name;
            expanded::Roster        roster = expanded::ThemedRoster(dungeon, floor, random, &name);
            expanded::DungeonMimics mimics = *expanded::MimicsOf(dungeon, floor);
            const expanded::Theme  &theme = Named(name);
            seen[std::string(name) + "@" + std::to_string(dungeon)]++;

            ASSERT_GT(roster.Size(), 0);
            bool repeatable = false;
            for (int i = 0; i < roster.Size(); i++) {
                Monster kind = static_cast<Monster>(roster.model_no[i]);
                repeatable = repeatable || !roster.once[i];
                bool listed = Contains(theme.members, kind) || kind == mimics.mimic || kind == mimics.king ||
                              Contains(expanded::DungeonNatives(dungeon), kind);
                EXPECT_TRUE(listed) << name << " " << No(kind);
            }
            EXPECT_TRUE(repeatable) << name;

            if (name == "Mimics") {
                ASSERT_EQ(roster.Size(), 2);
                EXPECT_FALSE(roster.once[0]);
                EXPECT_FALSE(roster.once[1]);
            } else if (int king = Find(roster, mimics.king); king >= 0) {
                EXPECT_TRUE(roster.once[king]) << name;
            }
            if (name == "Pirates") {
                EXPECT_GE(Find(roster, mimics.mimic), 0);
                for (Monster kind : {Monster::CAPTAIN, Monster::SIL, Monster::GOL}) {
                    EXPECT_TRUE(roster.once[Find(roster, kind)]);
                }
            }
        }
    }
    // Demon Shaft's own bands turn up most there.
    EXPECT_GT(seen["Demon Shaft: Holy@6"], 4 * seen["Demon Shaft: Holy@0"]);
    EXPECT_GT(seen["Mimics@0"], 0);
}
