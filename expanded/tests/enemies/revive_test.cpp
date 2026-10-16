#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

#include "dun/gameloop.hpp"
#include "expanded/core/options.hpp"
#include "expanded/enemies/revive.hpp"
#include "monstorunit.hpp"
#include "platform/config.hpp"
#include "platform/paths.hpp"
#include "platform_fixture.hpp"

namespace {

// Turns Harder Enemy AI on in the current settings.
void SetHarder() {
    std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("dc_expanded_revive_" + std::to_string(dc::test::ProcessId()));
    std::filesystem::create_directories(root);
    PathsSetSaveRoot(root);
    Config config = ConfigGet();
    ASSERT_TRUE(expanded::SetOption(config.expanded_options, "harder_enemy_ai", true));
    ASSERT_TRUE(ConfigChange(config));
}

// A floor of one monster of script, at slot 0, with the motions a revival plays.
struct Floor {
    std::unique_ptr<CMonstorUnit> unit = std::make_unique<CMonstorUnit>();
    MOTION_INFO                   motions[12] = {};

    explicit Floor(const char *script) {
        std::strcpy(unit->model[0].script_name, script);
        unit->monster[0].base_model = 0;
        unit->monster[0].max_hp = 100;
        unit->current_monster = 0;
        motions[0] = {0, 40, 0.5f, 0};
        motions[9] = {170, 190, 0.5f, 0};
        motions[10] = {200, 220, 0.5f, 0};
        motions[11] = {230, 250, 0.5f, 0};
        unit->chara[0][0].motion_type.motion_info = motions;
        NowMonstorUnit = unit.get();
    }

    CCharacter &Chara() { return unit->chara[0][0]; }
};

bool AlwaysDown(const CMonstorUnit &, int) {
    return true;
}

} // namespace

TEST(Revive, SpeciesAndTheirCollapse) {
    ASSERT_NE(expanded::ReviveSpeciesOf("e03a"), nullptr);
    EXPECT_EQ(expanded::ReviveSpeciesOf("e03a")->revival, expanded::Revival::kGetUp);
    EXPECT_EQ(expanded::ReviveSpeciesOf("e03a")->collapse_motion, 11);
    EXPECT_EQ(expanded::ReviveSpeciesOf("e14a")->collapse_motion, 9);
    EXPECT_EQ(expanded::ReviveSpeciesOf("e50a")->revival, expanded::Revival::kScript);
    EXPECT_EQ(expanded::ReviveSpeciesOf("e118a")->revival, expanded::Revival::kScript);
    EXPECT_EQ(expanded::ReviveSpeciesOf("e05a"), nullptr);
}

TEST(Revive, OffKillsAsRetail) {
    Floor floor("e03a");
    EXPECT_FALSE(expanded::HarderEnemyAiOn());
    for (int i = 0; i < 50; i++) {
        EXPECT_EQ(expanded::ReviveCheckDmg(*floor.unit, MONSTER_DMG_KILLED), MONSTER_DMG_KILLED);
        EXPECT_FALSE(expanded::ReviveStep(*floor.unit, 0));
    }
    EXPECT_EQ(expanded::ReviveRoll(100, 7), 7);
}

TEST(Revive, StaysDownRuleKeepsEveryKill) {
    SetHarder();
    ASSERT_TRUE(expanded::AddStaysDownRule(AlwaysDown));
    EXPECT_FALSE(expanded::AddStaysDownRule(AlwaysDown));

    Floor floor("e03a");
    for (int i = 0; i < 50; i++) {
        EXPECT_EQ(expanded::ReviveCheckDmg(*floor.unit, MONSTER_DMG_KILLED), MONSTER_DMG_KILLED);
    }

    // A death script's own roll is made to fail, once.
    Floor mummy("e50a");
    EXPECT_EQ(expanded::ReviveCheckDmg(*mummy.unit, MONSTER_DMG_KILLED), MONSTER_DMG_KILLED);
    EXPECT_EQ(expanded::ReviveRoll(24, 5), 5);
    EXPECT_EQ(expanded::ReviveRoll(100, 3), 99);
    EXPECT_EQ(expanded::ReviveRoll(100, 3), 3);
}

TEST(Revive, ScriptRollsAtTheNativeChance) {
    SetHarder();
    Floor mummy("e50a");
    int   revived = 0;
    for (int i = 0; i < 2000; i++) {
        EXPECT_EQ(expanded::ReviveCheckDmg(*mummy.unit, MONSTER_DMG_KILLED), MONSTER_DMG_KILLED);
        int roll = expanded::ReviveRoll(100, 50);
        EXPECT_TRUE(roll == 0 || roll == 99);
        revived += roll == 0;
    }
    EXPECT_NEAR(revived / 2000.0, expanded::kNativeReviveChance / 100.0, 0.05);
}

TEST(Revive, CollapsesLiesStillAndGetsUp) {
    SetHarder();
    Floor         floor("e14a");
    CMonstorUnit &unit = *floor.unit;

    // About 30% of kills revive.
    int kills = 0;
    while (expanded::ReviveCheckDmg(unit, MONSTER_DMG_KILLED) == MONSTER_DMG_KILLED) {
        ASSERT_LT(++kills, 200);
    }
    EXPECT_EQ(unit.script_state[0], 1);
    EXPECT_EQ(floor.Chara().motion_no, 9);
    EXPECT_EQ(unit.monster[0].invincible_timer, expanded::kReviveInvincibleSteps);

    // The collapse plays to its end.
    floor.Chara().motion_type.state.time = 175.0f;
    EXPECT_TRUE(expanded::ReviveStep(unit, 0));
    EXPECT_TRUE(expanded::ReviveStep(unit, 0));
    floor.Chara().motion_type.state.time = 186.0f;
    EXPECT_TRUE(expanded::ReviveStep(unit, 0));

    // It lies still, then gets up with its life.
    int lying = 0;
    while (floor.Chara().motion_no == 9) {
        EXPECT_TRUE(expanded::ReviveStep(unit, 0));
        ASSERT_LE(++lying, expanded::kReviveLieSteps + expanded::kReviveLieSpread);
    }
    EXPECT_GE(lying, expanded::kReviveLieSteps);
    EXPECT_EQ(floor.Chara().motion_no, expanded::kGetUpMotion);
    EXPECT_EQ(unit.monster[0].hp, 40);

    floor.Chara().motion_type.state.time = 216.0f;
    EXPECT_TRUE(expanded::ReviveStep(unit, 0));
    EXPECT_TRUE(expanded::ReviveStep(unit, 0));
    EXPECT_EQ(floor.Chara().motion_no, expanded::kIdleMotion);
    EXPECT_EQ(unit.script_state[0], 0);
    EXPECT_FALSE(expanded::ReviveStep(unit, 0));
}

TEST(Revive, AHitHandsItBackToItsScript) {
    SetHarder();
    Floor floor("e03a");
    while (expanded::ReviveCheckDmg(*floor.unit, MONSTER_DMG_KILLED) == MONSTER_DMG_KILLED) {
    }
    EXPECT_EQ(expanded::ReviveCheckDmg(*floor.unit, MONSTER_DMG_HIT), MONSTER_DMG_HIT);
    EXPECT_FALSE(expanded::ReviveStep(*floor.unit, 0));
}
