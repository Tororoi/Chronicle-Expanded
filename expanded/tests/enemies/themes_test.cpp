#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/core/options.hpp"
#include "expanded/core/random.hpp"
#include "expanded/core/save.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/randomize.hpp"
#include "expanded/enemies/themes.hpp"
#include "monstorunit.hpp"
#include "platform/config.hpp"
#include "platform/paths.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

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
    std::set<std::string> names;
    for (const expanded::Theme &theme : expanded::Themes()) {
        EXPECT_TRUE(names.insert(std::string(theme.name)).second) << theme.name;
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
        {43, 147, 167},
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
            expanded::ThemeOutcome  outcome;
            expanded::Roster        roster = expanded::ThemedRoster(dungeon, floor, random, &outcome);
            std::string_view        name = outcome.name;
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

TEST(EnemyThemes, ACappedFloorsFillersKeepToTheirCaps) {
    expanded::SeedModRandom(1);
    using Fill = expanded::ThemeOutcome::Fill;
    std::map<Fill, int> sources;
    int                 mimics_only_one = 0, mimics_only_roomy = 0;
    for (int dungeon : {kDivineBeastCave, 3, kDemonShaft}) {
        int                     floor = dungeon == kDemonShaft ? 85 : 5;
        expanded::DungeonMimics mimics = *expanded::MimicsOf(dungeon, floor);
        for (int roll = 0; roll < 20000; roll++) {
            expanded::ThemeOutcome outcome;
            expanded::Roster       roster = expanded::ThemedRoster(dungeon, floor, expanded::ModRandom(), &outcome);
            if (outcome.fill == Fill::None) {
                EXPECT_EQ(outcome.fillers, 0);
                continue;
            }
            sources[outcome.fill]++;
            const expanded::Theme &theme = Named(outcome.name);
            int                    members = roster.Size() - outcome.fillers;
            ASSERT_GE(outcome.fillers, 1) << outcome.name;
            ASSERT_GE(members, 1) << outcome.name;
            // The mimic first and repeatable, the members once each, then the other fillers, repeatable but the king.
            EXPECT_EQ(roster.model_no[0], No(mimics.mimic));
            EXPECT_FALSE(roster.once[0]);
            for (int i = 1; i <= members; i++) {
                EXPECT_TRUE(Contains(theme.members, static_cast<Monster>(roster.model_no[i]))) << outcome.name;
                EXPECT_TRUE(roster.once[i]) << outcome.name;
            }
            for (int i = members + 1; i < roster.Size(); i++) {
                EXPECT_EQ(roster.once[i], roster.model_no[i] == No(mimics.king)) << outcome.name;
            }
            if (outcome.fill == Fill::Mimics) {
                EXPECT_LE(outcome.fillers, 2);
                if (members <= expanded::kRosterSize - 2) {
                    mimics_only_roomy++;
                    mimics_only_one += outcome.fillers == 1;
                }
            } else {
                for (int i = members + 2; i < roster.Size(); i++) { // past the mimic and the king
                    EXPECT_TRUE(Contains(expanded::DungeonNatives(dungeon), static_cast<Monster>(roster.model_no[i])));
                }
            }
        }
    }
    // Half the backfills are the mimics alone.
    int filled = sources[Fill::Mimics] + sources[Fill::MimicsAndNatives];
    ASSERT_GT(filled, 2000);
    EXPECT_NEAR(sources[Fill::Mimics] / static_cast<double>(filled), expanded::kThemeMimicFillChance, 0.03);
    // The mimics alone are one or two kinds, by the first two weights rescaled.
    double one = expanded::kKindCountWeights[0] / (expanded::kKindCountWeights[0] + expanded::kKindCountWeights[1]);
    EXPECT_NEAR(mimics_only_one / static_cast<double>(mimics_only_roomy), one, 0.03);
}

namespace {

// Sets Randomizer Bosses in the current settings, a party of everyone and a save that has beaten no one.
class BossThemes : public ::testing::Test {
protected:
    void SetUp() override {
        expanded::SeedModRandom(1);
        saved_status_ = UserStatus;
        status_->party_size = 6;
        UserStatus = status_.get();
        expanded::SaveClear();
    }

    void TearDown() override {
        Set(expanded::RandomizerBosses::Off);
        expanded::SaveClear();
        UserStatus = saved_status_;
    }

    void Set(expanded::RandomizerBosses setting) { ASSERT_TRUE(options_.Choose(expanded::kRandomizerBosses, static_cast<int>(setting))); }

    static const expanded::Theme &Boss(Monster boss) {
        for (const expanded::Theme &theme : expanded::BossThemes()) {
            if (theme.boss == boss) {
                return theme;
            }
        }
        static const expanded::Theme none;
        return none;
    }

    expanded::test::OptionScope  options_;
    std::unique_ptr<CUserStatus> status_ = std::make_unique<CUserStatus>();
    CUserStatus                 *saved_status_ = nullptr;
};

} // namespace

TEST_F(BossThemes, EachIsItsBossAndSixMore) {
    ASSERT_EQ(expanded::BossThemes().size(), 6u);
    for (const expanded::Theme &theme : expanded::BossThemes()) {
        ASSERT_TRUE(theme.boss.has_value()) << theme.name;
        EXPECT_EQ(theme.members.size(), 7u) << theme.name;
        EXPECT_TRUE(theme.whole_set) << theme.name;
        EXPECT_TRUE(theme.with_mimics) << theme.name;
        EXPECT_TRUE(Contains(theme.members, *theme.boss)) << theme.name;
        EXPECT_TRUE(Contains(theme.single, *theme.boss)) << theme.name;
    }
}

TEST_F(BossThemes, TheSettingSaysWhichAreDrawn) {
    const expanded::Theme &dran = Boss(Monster::DRAN);
    EXPECT_EQ(expanded::RandomizerBossesSetting(), expanded::RandomizerBosses::Off);
    EXPECT_FALSE(expanded::BossThemeAvailable(dran));
    EXPECT_TRUE(expanded::BossThemeAvailable(expanded::Themes().front()));

    Set(expanded::RandomizerBosses::Defeated);
    EXPECT_FALSE(expanded::BossThemeAvailable(dran));
    expanded::SaveValue<std::vector<int>>("bosses.defeated").Set({No(Monster::DRAN)});
    EXPECT_TRUE(expanded::BossThemeAvailable(dran));
    EXPECT_FALSE(expanded::BossThemeAvailable(Boss(Monster::MINOTAUR_JOE)));

    Set(expanded::RandomizerBosses::Any);
    EXPECT_TRUE(expanded::BossThemeAvailable(Boss(Monster::MINOTAUR_JOE)));
    // The Ice Queen comes only with the party member who can hurt her.
    EXPECT_TRUE(expanded::BossThemeAvailable(Boss(Monster::ICE_QUEEN)));
    status_->party_size = 2;
    EXPECT_FALSE(expanded::BossThemeAvailable(Boss(Monster::ICE_QUEEN)));
}

TEST_F(BossThemes, ABossFloorHoldsTheBossItsCompanyAndTheMimics) {
    Set(expanded::RandomizerBosses::Any);
    std::mt19937               random(5);
    std::map<std::string, int> seen;
    for (int roll = 0; roll < 4000; roll++) {
        expanded::ThemeOutcome  outcome;
        expanded::Roster        roster = expanded::ThemedRoster(3, 5, random, &outcome);
        expanded::DungeonMimics mimics = *expanded::MimicsOf(3, 5);
        for (const expanded::Theme &theme : expanded::BossThemes()) {
            if (theme.name != outcome.name) {
                continue;
            }
            seen[std::string(theme.name)]++;
            ASSERT_EQ(roster.Size(), expanded::kRosterSize) << theme.name;
            for (Monster kind : theme.members) {
                int at = Find(roster, kind);
                ASSERT_GE(at, 0) << theme.name << " " << No(kind);
                EXPECT_EQ(roster.once[at], Contains(theme.single, kind)) << theme.name << " " << No(kind);
            }
            EXPECT_GE(Find(roster, mimics.mimic), 0) << theme.name;
            EXPECT_GE(Find(roster, mimics.king), 0) << theme.name;
        }
    }
    EXPECT_EQ(seen.size(), 6u);
}
