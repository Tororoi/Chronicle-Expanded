#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <string>

#include "expanded/enemies/randomize.hpp"
#include "expanded/enemies/themes.hpp"
#include "monstorunit.hpp"

namespace {

constexpr int kDivineBeastCave = 0;
constexpr int kDemonShaft = 6;
constexpr int kCaptain = 23;
constexpr int kSil = 95;
constexpr int kGol = 94;

const expanded::Theme &Named(std::string_view name) {
    for (const expanded::Theme &theme : expanded::Themes()) {
        if (theme.name == name) {
            return theme;
        }
    }
    static const expanded::Theme none;
    return none;
}

bool Contains(const std::vector<int> &kinds, int kind) {
    return std::find(kinds.begin(), kinds.end(), kind) != kinds.end();
}

int Find(const expanded::Roster &roster, int kind) {
    for (int i = 0; i < roster.Size(); i++) {
        if (roster.model_no[i] == kind) {
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
        for (int kind : theme.members) {
            EXPECT_EQ(MonstorTable[kind].kind, MONSTER_KIND_NORMAL) << theme.name << " " << kind;
        }
    }
    for (int dungeon = 0; dungeon < 7; dungeon++) {
        for (int kind : expanded::DungeonNatives(dungeon)) {
            EXPECT_EQ(MonstorTable[kind].kind, MONSTER_KIND_NORMAL) << dungeon << " " << kind;
        }
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
            expanded::DungeonMimics mimics = expanded::MimicsOf(dungeon, floor);
            const expanded::Theme  &theme = Named(name);
            seen[std::string(name) + "@" + std::to_string(dungeon)]++;

            ASSERT_GT(roster.Size(), 0);
            bool repeatable = false;
            for (int i = 0; i < roster.Size(); i++) {
                int kind = roster.model_no[i];
                repeatable = repeatable || !roster.once[i];
                bool listed = Contains(theme.members, kind) || kind == mimics.mimic || kind == mimics.king ||
                              Contains(expanded::DungeonNatives(dungeon), kind);
                EXPECT_TRUE(listed) << name << " " << kind;
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
                for (int kind : {kCaptain, kSil, kGol}) {
                    EXPECT_TRUE(roster.once[Find(roster, kind)]);
                }
            }
        }
    }
    // Demon Shaft's own bands turn up most there.
    EXPECT_GT(seen["Demon Shaft: Holy@6"], 4 * seen["Demon Shaft: Holy@0"]);
    EXPECT_GT(seen["Mimics@0"], 0);
}
