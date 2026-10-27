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
#include "test_support.hpp"

namespace {

// Harder Enemy AI on for a test.
struct Harder {
    Harder() { EXPECT_TRUE(options.Set("harder_enemy_ai", true)); }

    expanded::test::OptionScope options;
};

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

// Rules stay once added, so the one test's keeps kills only while it runs.
bool g_always_down = false;

bool AlwaysDown(const CMonstorUnit &, int) {
    return g_always_down;
}

struct AlwaysDownScope {
    AlwaysDownScope() { g_always_down = true; }

    ~AlwaysDownScope() { g_always_down = false; }
};

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
        EXPECT_EQ(expanded::ReviveCheckDmg(*floor.unit, MonsterDamageResult::Killed), MonsterDamageResult::Killed);
        EXPECT_FALSE(expanded::ReviveStep(*floor.unit, 0));
    }
    EXPECT_EQ(expanded::ReviveRoll(100, 7), 7);
}

TEST(Revive, StaysDownRuleKeepsEveryKill) {
    Harder harder;
    ASSERT_TRUE(expanded::AddStaysDownRule(AlwaysDown));
    EXPECT_FALSE(expanded::AddStaysDownRule(AlwaysDown));
    AlwaysDownScope always_down;

    Floor floor("e03a");
    for (int i = 0; i < 50; i++) {
        EXPECT_EQ(expanded::ReviveCheckDmg(*floor.unit, MonsterDamageResult::Killed), MonsterDamageResult::Killed);
    }

    // A death script's own roll is made to fail, once.
    Floor mummy("e50a");
    EXPECT_EQ(expanded::ReviveCheckDmg(*mummy.unit, MonsterDamageResult::Killed), MonsterDamageResult::Killed);
    EXPECT_EQ(expanded::ReviveRoll(24, 5), 5);
    EXPECT_EQ(expanded::ReviveRoll(100, 3), 99);
    EXPECT_EQ(expanded::ReviveRoll(100, 3), 3);
}

TEST(Revive, ScriptRollsAtTheNativeChance) {
    Harder harder;
    Floor  mummy("e50a");
    int    revived = 0;
    for (int i = 0; i < 2000; i++) {
        EXPECT_EQ(expanded::ReviveCheckDmg(*mummy.unit, MonsterDamageResult::Killed), MonsterDamageResult::Killed);
        int roll = expanded::ReviveRoll(100, 50);
        EXPECT_TRUE(roll == 0 || roll == 99);
        revived += roll == 0;
    }
    EXPECT_NEAR(revived / 2000.0, expanded::kNativeReviveChance / 100.0, 0.05);
}

TEST(Revive, CollapsesLiesStillAndGetsUp) {
    Harder        harder;
    Floor         floor("e14a");
    CMonstorUnit &unit = *floor.unit;

    // About 30% of kills revive, the revival's commands run on the unit and the current one put back.
    auto other = std::make_unique<CMonstorUnit>();
    NowMonstorUnit = other.get();
    int kills = 0;
    while (expanded::ReviveCheckDmg(unit, MonsterDamageResult::Killed) == MonsterDamageResult::Killed) {
        ASSERT_LT(++kills, 200);
    }
    EXPECT_EQ(NowMonstorUnit, other.get());
    NowMonstorUnit = &unit;
    EXPECT_EQ(unit.script_state[0], 1);
    EXPECT_EQ(floor.Chara().motion_no, 9);
    EXPECT_EQ(unit.monster[0].invincible_timer, expanded::kReviveInvincibleSteps);

    // The collapse plays to its end.
    floor.Chara().motion_type.state.time = 175.0f;
    NowMonstorUnit = other.get();
    EXPECT_TRUE(expanded::ReviveStep(unit, 0));
    EXPECT_EQ(NowMonstorUnit, other.get());
    NowMonstorUnit = &unit;
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
    Harder harder;
    Floor  floor("e03a");
    int    kills = 0;
    while (expanded::ReviveCheckDmg(*floor.unit, MonsterDamageResult::Killed) == MonsterDamageResult::Killed) {
        ASSERT_LT(++kills, 200);
    }
    EXPECT_EQ(expanded::ReviveCheckDmg(*floor.unit, MonsterDamageResult::Hit), MonsterDamageResult::Hit);
    EXPECT_FALSE(expanded::ReviveStep(*floor.unit, 0));
}
