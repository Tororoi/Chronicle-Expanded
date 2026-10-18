#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "dun/gameloop.hpp"
#include "expanded/data/script_damage.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"
#include "platform/paths.hpp"
#include "shot_effect_pack.hpp"
#include "test_support.hpp"

namespace {

constexpr int kSkeletonSoldier = 1;
constexpr int kWerewolf = 4;
constexpr int kFliFli = 5;
constexpr int kGemronFire = 111;

// A floor in a data directory whose scripts deal these: Skeleton Soldier, Werewolf, FliFli (a shot without damage,
// its effect's 26) and Gemron (Fire).
class EnemyScaling : public ::testing::Test {
protected:
    EnemyScaling() {
        std::filesystem::create_directories((data_.path() / expanded::kScriptDamageFile).parent_path());
        std::ofstream(data_.path() / expanded::kScriptDamageFile) << "e03a 21 0 0\ne07a 68 0 0\ne08a 37 0 1\ne111a 100 100 0\n";
    }

    CMonstorUnit &Unit() { return floor_.unit(); }

    expanded::test::ScratchDir    data_{"enemy_scaling"};
    expanded::test::DataRootScope root_{data_.path()};
    expanded::test::TestFloor     floor_;
};

// Listeners stay once added, so the one test's acts only while it runs.
bool g_listening = false;
int  g_models = 0;
int  g_spawns = 0;

} // namespace

TEST_F(EnemyScaling, RoundsHalfToEvenAndKeepsAtLeastOne) {
    EXPECT_EQ(expanded::ScaleStat(30, 0.75f), 22);
    EXPECT_EQ(expanded::ScaleStat(10, 1.25f), 12);
    EXPECT_EQ(expanded::ScaleStat(3, 0.1f), 1);
    EXPECT_EQ(expanded::ScaleStat(0, 4.0f), 0);
    EXPECT_EQ(expanded::ScaleStat(-1, 4.0f), -1);
}

TEST_F(EnemyScaling, NativeStatsAreTheRecordAndTheScriptsDamage) {
    expanded::EnemyStats werewolf = expanded::NativeStats(kWerewolf);
    EXPECT_EQ(werewolf.hp, 180);
    EXPECT_EQ(werewolf.abs, 12);
    EXPECT_EQ(werewolf.defense, 5);
    EXPECT_EQ(werewolf.hardness, 0);
    EXPECT_EQ(werewolf.melee, 68);
    EXPECT_EQ(werewolf.shot, 0);
    EXPECT_EQ(expanded::NativeStats(kGemronFire).shot, 100);
    EXPECT_EQ(expanded::NativeStats(kFliFli).shot, 26);
    EXPECT_EQ(expanded::NativeStats(expanded::kSpeciesCount).hp, 0);
}

TEST_F(EnemyScaling, SpeciesScaleReachesLiveAndLaterMonsters) {
    floor_.SetUpModel(2, kWerewolf);
    floor_.Spawn(0, 2);
    Unit().monster[0].hp = 90;

    expanded::ScaleSpecies(Unit(), 2, {.hp = 2.0f, .abs = 3.0f, .defense = 2.0f, .melee = 1.5f});
    EXPECT_EQ(Unit().monster[0].max_hp, 360);
    EXPECT_EQ(Unit().monster[0].hp, 180);
    EXPECT_EQ(Unit().monster[0].exp, 36);
    EXPECT_EQ(Unit().monster[0].defense, 10);
    EXPECT_EQ(Unit().monster[0].hardness, 0);
    EXPECT_EQ(expanded::EnemyMeleeDamage(0, 68), 102);

    floor_.Spawn(5, 2);
    EXPECT_EQ(Unit().monster[5].max_hp, 360);
    EXPECT_EQ(Unit().monster[5].hp, 360);
    EXPECT_EQ(expanded::MonsterStats(Unit(), 5).melee, 102);

    floor_.SetUpModel(2, kWerewolf);
    EXPECT_EQ(expanded::SpeciesScale(2), expanded::EnemyScale{});
}

TEST_F(EnemyScaling, SpeciesStatsReplaceTheNativesUnderTheMultipliers) {
    floor_.SetUpModel(0, kSkeletonSoldier);
    EXPECT_EQ(expanded::SpeciesStats(0), expanded::NativeStats(kSkeletonSoldier));
    floor_.Spawn(1, 0);

    expanded::SetSpeciesStats(Unit(), 0, {.hp = 503, .abs = 8, .defense = 5, .hardness = 10, .melee = 107});
    expanded::ScaleMonster(Unit(), 1, {.hp = 2.0f});
    EXPECT_EQ(Unit().monster[1].max_hp, 1006);
    EXPECT_EQ(Unit().monster[1].exp, 8);
    EXPECT_EQ(Unit().monster[1].defense, 5);
    EXPECT_EQ(Unit().monster[1].hardness, 10);
    EXPECT_EQ(expanded::EnemyMeleeDamage(1, 21), 107);
    EXPECT_EQ(expanded::EnemyMeleeDamage(1, 20), 102);
    EXPECT_EQ(expanded::MonsterStats(Unit(), 1).melee, 107);

    floor_.SetUpModel(0, kSkeletonSoldier);
    floor_.Spawn(1, 0);
    EXPECT_EQ(Unit().monster[1].max_hp, 23);
    EXPECT_EQ(expanded::EnemyMeleeDamage(1, 21), 21);
}

TEST_F(EnemyScaling, MonsterScaleStacksOnItsKindsAndEndsWithIt) {
    floor_.SetUpModel(0, kWerewolf);
    floor_.Spawn(3, 0);
    floor_.Spawn(4, 0);
    expanded::ScaleSpecies(Unit(), 0, {.hp = 1.5f});
    expanded::ScaleMonster(Unit(), 3, {.hp = 4.0f, .melee = 2.0f});

    EXPECT_EQ(Unit().monster[3].max_hp, 1080);
    EXPECT_EQ(Unit().monster[4].max_hp, 270);
    EXPECT_EQ(expanded::MonsterScale(Unit(), 3).hp, 6.0f);
    EXPECT_EQ(expanded::EnemyMeleeDamage(3, 30), 60);
    EXPECT_EQ(expanded::EnemyMeleeDamage(4, 30), 30);

    floor_.Spawn(3, 0);
    EXPECT_EQ(Unit().monster[3].max_hp, 270);
    EXPECT_EQ(expanded::EnemyMeleeDamage(3, 30), 30);
}

TEST_F(EnemyScaling, ScalesTheShotJustFired) {
    floor_.SetUpModel(1, kGemronFire);
    floor_.Spawn(2, 1);
    expanded::ScaleMonster(Unit(), 2, {.shot = 1.25f});

    auto               pack = std::make_unique<CSHOT_EFFECT_PACK>();
    CSHOT_EFFECT_PACK *saved = NowShotEffect;
    NowShotEffect = pack.get();
    pack->current_effect = 0;
    pack->effect[0].current_slot = 3;
    pack->effect[0].damage[3] = 100;

    expanded::EnemyShotFired(2);
    EXPECT_EQ(pack->effect[0].damage[3], 125);

    pack->effect[0].current_slot = -1;
    expanded::EnemyShotFired(2);
    EXPECT_EQ(pack->effect[0].damage[3], 125);
    NowShotEffect = saved;
}

TEST_F(EnemyScaling, ListenersHearSetUpsAndSpawns) {
    expanded::AddEnemyModelListener([](CMonstorUnit &, int model_slot, int model_no) {
        if (g_listening) {
            g_models += model_slot * 1000 + model_no;
        }
    });
    expanded::AddEnemySpawnListener([](CMonstorUnit &unit, int monster) {
        if (g_listening) {
            g_spawns++;
            expanded::ScaleMonster(unit, monster, {.hp = 2.0f});
        }
    });
    g_listening = true;
    floor_.SetUpModel(3, kWerewolf);
    EXPECT_EQ(g_models, 3004);
    EXPECT_EQ(expanded::ModelKind(3), kWerewolf);

    floor_.Spawn(0, 3);
    EXPECT_EQ(g_spawns, 1);
    EXPECT_EQ(Unit().monster[0].max_hp, 360);
    g_listening = false;
}

TEST_F(EnemyScaling, WithoutTheDataFileMonstersDealTheirOwnDamage) {
    expanded::test::ScratchDir    empty("enemy_scaling_no_data");
    expanded::test::DataRootScope root(empty.path());
    expanded::EnemyStats werewolf = expanded::NativeStats(kWerewolf);
    EXPECT_EQ(werewolf.hp, 180);
    EXPECT_EQ(werewolf.melee, 0);
    floor_.SetUpModel(0, kWerewolf);
    floor_.Spawn(0, 0);
    EXPECT_EQ(expanded::EnemyMeleeDamage(0, 68), 68);
}
