#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/bomb_gemron.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/scaling.hpp"
#include "expanded/enemies/species.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

namespace {

using expanded::Monster;
using expanded::No;

bool NoDisc(CSHOT_EFFECT &, CDataAlloc2<1> &, BT_SHOT_EFFECT *, int, const char *) {
    return false;
}

// A floor with a Bomb Gemron in model slot 0, its shots' disc load refused.
class BombGemron : public ::testing::Test {
protected:
    void SetUp() override {
        expanded::SetShotEffectLoader(NoDisc);
        for (MONSTOR &monster : unit_->monster) {
            monster.state = -1;
        }
        unit_->model[0] = *expanded::Species(No(Monster::BOMB_GEMRON));
        expanded::EnemyModelSetUp(*unit_, 0, No(Monster::BOMB_GEMRON));
        motions_.assign(16, MOTION_INFO{.start = 10, .end = 30, .speed = 0.2f});
        motions_[expanded::kBombSelfDestructKey] = {.start = expanded::kBombSelfDestructStart, .end = expanded::kBombSelfDestructEnd, .speed = 0.15f};
        motions_[15] = {.start = -1, .end = -1, .speed = -1.0f};
    }

    void TearDown() override {
        expanded::SetShotEffectLoader(nullptr);
    }

    MONSTOR &Spawn(int monster) {
        MONSTOR &live = unit_->monster[monster];
        live.state = 2;
        live.base_model = 0;
        live.max_hp = live.hp = 1000;
        live.palette_alpha = 128.0f;
        unit_->chara[monster][0].motion_type.motion_info = motions_.data();
        expanded::EnemySpawned(*unit_, monster);
        unit_->alive_count++;
        return live;
    }

    std::vector<MOTION_INFO>      motions_;
    std::unique_ptr<CMonstorUnit> unit_ = std::make_unique<CMonstorUnit>();
};

} // namespace

TEST(BombGemronSpecies, IsHolyGemronsRecordUnderItsOwnModelAndElements) {
    const MONSTOR_MODEL *bomb = expanded::Species(No(Monster::BOMB_GEMRON));
    ASSERT_NE(bomb, nullptr);
    const MONSTOR_MODEL &holy = *expanded::Species(No(Monster::GEMRON_HOLY));
    EXPECT_STREQ(bomb->model_name[0], "e167a");
    EXPECT_STREQ(bomb->script_name, holy.script_name);
    EXPECT_EQ(bomb->max_hp, holy.max_hp);
    EXPECT_EQ(bomb->attachment_weight[0], 150);
    for (int i = 1; i < 5; i++) {
        EXPECT_EQ(bomb->attachment_weight[i], 0) << i;
    }
    EXPECT_EQ(bomb->exp, 40);
    EXPECT_EQ(bomb->steal_item, ITEM_BOMB);
    EXPECT_EQ(bomb->rare_item, ITEM_REPAIR_POWDER);
    EXPECT_EQ(bomb->name_no, 320);
    EXPECT_EQ(bomb->shot_effect[0], -1);
    EXPECT_EQ(expanded::SpeciesStandIn(No(Monster::BOMB_GEMRON)), No(Monster::GEMRON_HOLY));
    EXPECT_EQ(expanded::SpeciesStandIn(No(Monster::WEREWOLF)), No(Monster::WEREWOLF));
    EXPECT_EQ(expanded::Species(expanded::kSpeciesCount), nullptr);
}

TEST(BombGemronSpecies, RecordAndModelFileReachTheLoader) {
    EXPECT_EQ(expanded::SpeciesRecord(No(Monster::BOMB_GEMRON)), expanded::Species(No(Monster::BOMB_GEMRON)));
    EXPECT_EQ(expanded::SpeciesRecord(No(Monster::WEREWOLF)), &MonstorTable[No(Monster::WEREWOLF)]);
    char file[64] = "dun/monstor/e167a.chr";
    expanded::SpeciesPackFile(file, No(Monster::BOMB_GEMRON));
    EXPECT_STREQ(file, expanded::kBombGemronModelFile);
    std::strcpy(file, "dun/monstor/e04a.chr");
    expanded::SpeciesPackFile(file, No(Monster::WEREWOLF));
    EXPECT_STREQ(file, "dun/monstor/e04a.chr");
}

TEST_F(BombGemron, SelfDestructsWhenNearDeathWithThePlayerClose) {
    MONSTOR &gemron = Spawn(3);
    gemron.player_distance = 10.0f;
    EXPECT_FALSE(expanded::BombGemronStep(*unit_, 3)); // full of life

    gemron.hp = 240;
    gemron.player_distance = 30.0f;
    EXPECT_FALSE(expanded::BombGemronStep(*unit_, 3)); // too far

    gemron.player_distance = 10.0f;
    gemron.movement_speed = 1.0f;
    EXPECT_TRUE(expanded::BombGemronStep(*unit_, 3));
    EXPECT_EQ(gemron.requested_motion, expanded::kBombSelfDestructKey);
    EXPECT_EQ(gemron.movement_speed, 0.0f);

    // The fuse burns to its final pose, then it blows up and fades out with no kill.
    CCharacter &body = unit_->chara[3][0];
    body.motion_type.state.time = 210.0f;
    EXPECT_TRUE(expanded::BombGemronStep(*unit_, 3));
    EXPECT_EQ(gemron.palette_alpha_step, 0.0f);
    body.motion_type.state.time = expanded::kBombSelfDestructEnd - 1;
    EXPECT_TRUE(expanded::BombGemronStep(*unit_, 3));
    EXPECT_GT(gemron.palette_alpha_step, 0.0f);
    int alive = unit_->alive_count;
    gemron.palette_alpha = 0.0f;
    EXPECT_TRUE(expanded::BombGemronStep(*unit_, 3));
    EXPECT_EQ(gemron.state, -1);
    EXPECT_EQ(unit_->alive_count, alive - 1);
}

TEST_F(BombGemron, AHitTakesTheFuseOverUntilItIsLitAgain) {
    MONSTOR &gemron = Spawn(2);
    gemron.hp = 100;
    gemron.player_distance = 10.0f;
    EXPECT_TRUE(expanded::BombGemronStep(*unit_, 2));
    // Label 110's reaction plays another motion: the script has it back for a while.
    unit_->chara[2][0].motion_no = 5;
    EXPECT_FALSE(expanded::BombGemronStep(*unit_, 2));
    for (int step = 0; step < 59; step++) {
        EXPECT_FALSE(expanded::BombGemronStep(*unit_, 2)) << step;
    }
    unit_->chara[2][0].motion_no = 0;
    EXPECT_FALSE(expanded::BombGemronStep(*unit_, 2));
    EXPECT_TRUE(expanded::BombGemronStep(*unit_, 2));
}

TEST_F(BombGemron, OthersAreLeftAlone) {
    MONSTOR &other = unit_->monster[5];
    other.state = 2;
    other.base_model = 1;
    other.hp = 1;
    other.max_hp = 100;
    other.player_distance = 1.0f;
    expanded::EnemySpawned(*unit_, 5);
    EXPECT_FALSE(expanded::BombGemronStep(*unit_, 5));
    float target[4] = {0.0f, 7.4f, 0.0f, 1.0f};
    expanded::BombGemronAim(*unit_, 5, target);
    EXPECT_EQ(target[1], 7.4f);
}

TEST_F(BombGemron, AimsItsThrowAtTheContactHeight) {
    Spawn(1);
    float target[4] = {0.0f, 7.4f, 0.0f, 1.0f};
    expanded::BombGemronAim(*unit_, 1, target);
    EXPECT_FLOAT_EQ(target[1], 14.0f);
}

TEST(CrystalGemronSpecies, IsHolyGemronsRecordWithItsOwnModelScriptAndElements) {
    const MONSTOR_MODEL *crystal = expanded::Species(No(Monster::CRYSTAL_GEMRON));
    ASSERT_NE(crystal, nullptr);
    const MONSTOR_MODEL &holy = *expanded::Species(No(Monster::GEMRON_HOLY));
    EXPECT_STREQ(crystal->model_name[0], "e168a");
    EXPECT_STREQ(crystal->script_name, holy.script_name); // its damage; the built script loads in its place
    EXPECT_EQ(crystal->attachment_weight[1], 150);
    for (int i : {0, 2, 3, 4}) {
        EXPECT_EQ(crystal->attachment_weight[i], 0) << i;
    }
    EXPECT_EQ(crystal->exp, 80);
    EXPECT_EQ(crystal->steal_item, ITEM_POWERUP_POWDER);
    EXPECT_EQ(crystal->rare_item, ITEM_CRYSTAL_EYEBALL);
    EXPECT_EQ(crystal->name_no, 321);
    char script[64] = "dun/monstor/e115a.stb";
    expanded::SpeciesScriptFile(script, No(Monster::CRYSTAL_GEMRON));
    EXPECT_STREQ(script, expanded::kCrystalGemronScriptFile);
    std::strcpy(script, "dun/monstor/e115a.stb");
    expanded::SpeciesScriptFile(script, No(Monster::BOMB_GEMRON));
    EXPECT_STREQ(script, "dun/monstor/e115a.stb");
}
