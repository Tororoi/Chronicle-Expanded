#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "dun/gameloop.hpp"
#include "expanded/data/script_damage.hpp"
#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"
#include "platform/paths.hpp"
#include "platform_fixture.hpp"
#include "shot_effect_pack.hpp"

namespace {

constexpr int kSkeletonSoldier = 1;
constexpr int kWerewolf = 4;
constexpr int kFliFli = 5;
constexpr int kGemronFire = 111;

int g_models = 0;
int g_spawns = 0;

// A data directory whose scripts deal these: Skeleton Soldier, Werewolf, FliFli (a shot without damage,
// its effect's 26) and Gemron (Fire).
void UseScriptDamage() {
    std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("dc_enemy_scaling_" + std::to_string(dc::test::ProcessId()));
    std::filesystem::create_directories((root / expanded::kScriptDamageFile).parent_path());
    std::ofstream(root / expanded::kScriptDamageFile) << "e03a 21 0 0\ne07a 68 0 0\ne08a 37 0 1\ne111a 100 100 0\n";
    PathsSetDataRoot(root);
}

// A floor with nothing on it, off the dungeons so that no feature scales what it sets up.
std::unique_ptr<CMonstorUnit> EmptyFloor() {
    UseScriptDamage();
    selectMapNo = -1;
    auto unit = std::make_unique<CMonstorUnit>();
    for (MONSTOR &monster : unit->monster) {
        monster.state = -1;
    }
    return unit;
}

// SetupBaseModel's part: the kind's record in the model slot.
void SetUpModel(CMonstorUnit &unit, int model_slot, int model_no) {
    unit.model[model_slot] = MonstorTable[model_no];
    expanded::EnemyModelSetUp(unit, model_slot, model_no);
}

// SetupViewMonstor's part: the record's stats on the monster.
void Spawn(CMonstorUnit &unit, int monster, int model_slot) {
    MONSTOR &live = unit.monster[monster];
    const MONSTOR_MODEL &model = unit.model[model_slot];
    live.state = 1;
    live.base_model = model_slot;
    live.max_hp = live.hp = model.max_hp;
    live.defense = model.defense;
    live.hardness = model.hardness;
    live.exp = model.exp;
    expanded::EnemySpawned(unit, monster);
}

} // namespace

TEST(EnemyScaling, RoundsHalfToEvenAndKeepsAtLeastOne) {
    EXPECT_EQ(expanded::ScaleStat(30, 0.75f), 22);
    EXPECT_EQ(expanded::ScaleStat(10, 1.25f), 12);
    EXPECT_EQ(expanded::ScaleStat(3, 0.1f), 1);
    EXPECT_EQ(expanded::ScaleStat(0, 4.0f), 0);
    EXPECT_EQ(expanded::ScaleStat(-1, 4.0f), -1);
}

TEST(EnemyScaling, NativeStatsAreTheRecordAndTheScriptsDamage) {
    UseScriptDamage();
    expanded::EnemyStats werewolf = expanded::NativeStats(kWerewolf);
    EXPECT_EQ(werewolf.hp, 180);
    EXPECT_EQ(werewolf.abs, 12);
    EXPECT_EQ(werewolf.defense, 5);
    EXPECT_EQ(werewolf.hardness, 0);
    EXPECT_EQ(werewolf.melee, 68);
    EXPECT_EQ(werewolf.shot, 0);
    EXPECT_EQ(expanded::NativeStats(kGemronFire).shot, 100);
    EXPECT_EQ(expanded::NativeStats(kFliFli).shot, 26);
    EXPECT_EQ(expanded::NativeStats(167).hp, 0);
}

TEST(EnemyScaling, SpeciesScaleReachesLiveAndLaterMonsters) {
    auto unit = EmptyFloor();
    SetUpModel(*unit, 2, kWerewolf);
    Spawn(*unit, 0, 2);
    unit->monster[0].hp = 90;

    expanded::ScaleSpecies(*unit, 2, {.hp = 2.0f, .abs = 3.0f, .defense = 2.0f, .melee = 1.5f});
    EXPECT_EQ(unit->monster[0].max_hp, 360);
    EXPECT_EQ(unit->monster[0].hp, 180);
    EXPECT_EQ(unit->monster[0].exp, 36);
    EXPECT_EQ(unit->monster[0].defense, 10);
    EXPECT_EQ(unit->monster[0].hardness, 0);
    EXPECT_EQ(expanded::EnemyMeleeDamage(0, 68), 102);

    Spawn(*unit, 5, 2);
    EXPECT_EQ(unit->monster[5].max_hp, 360);
    EXPECT_EQ(unit->monster[5].hp, 360);
    EXPECT_EQ(expanded::MonsterStats(*unit, 5).melee, 102);

    SetUpModel(*unit, 2, kWerewolf);
    EXPECT_EQ(expanded::SpeciesScale(2), expanded::EnemyScale{});
}

TEST(EnemyScaling, SpeciesStatsReplaceTheNativesUnderTheMultipliers) {
    auto unit = EmptyFloor();
    SetUpModel(*unit, 0, kSkeletonSoldier);
    EXPECT_EQ(expanded::SpeciesStats(0), expanded::NativeStats(kSkeletonSoldier));
    Spawn(*unit, 1, 0);

    expanded::SetSpeciesStats(*unit, 0, {.hp = 503, .abs = 8, .defense = 5, .hardness = 10, .melee = 107});
    expanded::ScaleMonster(*unit, 1, {.hp = 2.0f});
    EXPECT_EQ(unit->monster[1].max_hp, 1006);
    EXPECT_EQ(unit->monster[1].exp, 8);
    EXPECT_EQ(unit->monster[1].defense, 5);
    EXPECT_EQ(unit->monster[1].hardness, 10);
    EXPECT_EQ(expanded::EnemyMeleeDamage(1, 21), 107);
    EXPECT_EQ(expanded::EnemyMeleeDamage(1, 20), 102);
    EXPECT_EQ(expanded::MonsterStats(*unit, 1).melee, 107);

    SetUpModel(*unit, 0, kSkeletonSoldier);
    Spawn(*unit, 1, 0);
    EXPECT_EQ(unit->monster[1].max_hp, 23);
    EXPECT_EQ(expanded::EnemyMeleeDamage(1, 21), 21);
}

TEST(EnemyScaling, MonsterScaleStacksOnItsKindsAndEndsWithIt) {
    auto unit = EmptyFloor();
    SetUpModel(*unit, 0, kWerewolf);
    Spawn(*unit, 3, 0);
    Spawn(*unit, 4, 0);
    expanded::ScaleSpecies(*unit, 0, {.hp = 1.5f});
    expanded::ScaleMonster(*unit, 3, {.hp = 4.0f, .melee = 2.0f});

    EXPECT_EQ(unit->monster[3].max_hp, 1080);
    EXPECT_EQ(unit->monster[4].max_hp, 270);
    EXPECT_EQ(expanded::MonsterScale(*unit, 3).hp, 6.0f);
    EXPECT_EQ(expanded::EnemyMeleeDamage(3, 30), 60);
    EXPECT_EQ(expanded::EnemyMeleeDamage(4, 30), 30);

    Spawn(*unit, 3, 0);
    EXPECT_EQ(unit->monster[3].max_hp, 270);
    EXPECT_EQ(expanded::EnemyMeleeDamage(3, 30), 30);
}

TEST(EnemyScaling, ScalesTheShotJustFired) {
    auto unit = EmptyFloor();
    SetUpModel(*unit, 1, kGemronFire);
    Spawn(*unit, 2, 1);
    expanded::ScaleMonster(*unit, 2, {.shot = 1.25f});

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

TEST(EnemyScaling, ListenersHearSetUpsAndSpawns) {
    expanded::AddEnemyModelListener([](CMonstorUnit &, int model_slot, int model_no) {
        g_models += model_slot * 1000 + model_no;
    });
    expanded::AddEnemySpawnListener([](CMonstorUnit &unit, int monster) {
        g_spawns++;
        expanded::ScaleMonster(unit, monster, {.hp = 2.0f});
    });
    auto unit = EmptyFloor();
    SetUpModel(*unit, 3, kWerewolf);
    EXPECT_EQ(g_models, 3004);
    EXPECT_EQ(expanded::ModelKind(3), kWerewolf);

    Spawn(*unit, 0, 3);
    EXPECT_EQ(g_spawns, 1);
    EXPECT_EQ(unit->monster[0].max_hp, 360);
}

TEST(EnemyScaling, WithoutTheDataFileMonstersDealTheirOwnDamage) {
    PathsSetDataRoot(std::filesystem::temp_directory_path() / "dc_enemy_scaling_no_data");
    expanded::EnemyStats werewolf = expanded::NativeStats(kWerewolf);
    EXPECT_EQ(werewolf.hp, 180);
    EXPECT_EQ(werewolf.melee, 0);
    EXPECT_EQ(expanded::EnemyMeleeDamage(0, 68), 68);
}
