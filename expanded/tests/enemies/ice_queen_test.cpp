#include <gtest/gtest.h>

#include <cmath>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/ice_queen.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/roster.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;

// Her mailbox's globals (ice_queen.cpp).
constexpr int kPhase = 0;
constexpr int kArrow = 2;
constexpr int kPrison = 3;
constexpr int kMeteor = 4;

// Every loaded effect's KEYs, and a frame for each of its shots; the effects by the pack they load.
std::vector<MOTION_INFO>              g_motions(4, MOTION_INFO{.start = 10, .end = 30, .speed = 0.2f});
std::deque<CFrame>                    g_frames;
std::map<std::string, CSHOT_EFFECT *> g_loaded;

bool FakeLoad(CSHOT_EFFECT &effect, CDataAlloc2<1> &, BT_SHOT_EFFECT *description, int shots, const char *path) {
    effect.Initialize();
    effect.effect_data = description;
    effect.slot_count = shots;
    for (int slot = 0; slot < shots; slot++) {
        effect.chara[slot].motion_type.motion_info = g_motions.data();
        effect.chara[slot].frame = &g_frames.emplace_back();
    }
    g_loaded[path] = &effect;
    return true;
}

// The Ice Queen (monster 0) and her barrier (monster 1) on a staged Divine Beast Cave floor, Ruby in the party, her
// companions' packs faked and the player at the origin.
class IceQueenFight : public ::testing::Test {
protected:
    void SetUp() override {
        saved_pack_ = NowShotEffect;
        for (CSHOT_EFFECT &effect : pack_->effect) {
            effect.Initialize();
        }
        pack_->current_effect = -1;
        NowShotEffect = pack_.get();
        sceVu0CopyVector(saved_player_, CharaMain.pos);
        CharaMain.pos[0] = CharaMain.pos[1] = CharaMain.pos[2] = 0.0f;
        g_loaded.clear();
        expanded::SetShotEffectLoader(FakeLoad);
        expanded::ShotEffectsClear();
        expanded::RosterVisitBegun();
        floor_.status().party_size = CHARA_RUBY + 1;
        floor_.LoadRoster(true, {Monster::ICE_QUEEN});
        floor_.Spawn(0, 0);
        floor_.Spawn(1, 1);
        floor_.unit().chara[0][0].SetPosition(0.0f, 0.0f, 100.0f);
        mailbox_ = expanded::BossCastMailbox(Monster::ICE_QUEEN);
        arrow_ = g_loaded[expanded::kIceArrowFile];
        prison_ = g_loaded[expanded::kIcePrisonFile];
        meteor_ = g_loaded["dun/monstor/i_meteo.chr"];
    }

    void TearDown() override {
        expanded::RosterVisitBegun();
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        sceVu0CopyVector(CharaMain.pos, saved_player_);
        NowShotEffect = saved_pack_;
    }

    void Step() { expanded::IceQueenStep(floor_.unit()); }

    void Freeze(int frames) {
        floor_.status().ailments[0] = frames > 0 ? AILMENT_FREEZE : 0;
        floor_.status().ailment_frames[0] = frames;
    }

    expanded::test::TestFloor          floor_;
    std::unique_ptr<CSHOT_EFFECT_PACK> pack_ = std::make_unique<CSHOT_EFFECT_PACK>();
    CSHOT_EFFECT_PACK                 *saved_pack_ = nullptr;
    float                              saved_player_[4] = {};
    int                               *mailbox_ = nullptr;
    CSHOT_EFFECT                      *arrow_ = nullptr;
    CSHOT_EFFECT                      *prison_ = nullptr;
    CSHOT_EFFECT                      *meteor_ = nullptr;
};

} // namespace

TEST_F(IceQueenFight, HerArrowFliesAndAFrozenPlayerIsImprisonedUntilTheFreezeEnds) {
    ASSERT_NE(mailbox_, nullptr);
    ASSERT_NE(arrow_, nullptr);
    ASSERT_NE(prison_, nullptr);

    // Fired 15 ahead of her at chest height, flying at once.
    mailbox_[kArrow] = 1;
    Step();
    EXPECT_EQ(mailbox_[kArrow], 0);
    EXPECT_EQ(mailbox_[kPhase], 1);
    ASSERT_NE(arrow_->active[0], 0);
    EXPECT_EQ(arrow_->phase[0], 1);
    float from[4];
    arrow_->chara[0].GetPosition(from);
    EXPECT_FLOAT_EQ(from[1], 10.1f);
    EXPECT_FLOAT_EQ(from[2], 85.0f);

    // Steered at the player's chest at 1.2 a step.
    Step();
    const float *velocity = arrow_->velocity[0];
    EXPECT_NEAR(std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]), 1.2f, 1e-5f);

    // Spent on a frozen player, its hit reaches him a step later and the prison forms at his feet.
    Freeze(30);
    arrow_->active[0] = 0;
    Step();
    EXPECT_EQ(mailbox_[kPrison], 0);
    EXPECT_EQ(prison_->active[0], 0);
    Step();
    EXPECT_EQ(mailbox_[kPrison], 4);
    ASSERT_NE(prison_->active[0], 0);
    EXPECT_EQ(prison_->phase[0], 0);
    EXPECT_EQ(prison_->user_id_2[0], 0);

    // Formed, it holds on its last frame while his ailment runs.
    prison_->phase[0] = 1;
    Step();
    EXPECT_EQ(mailbox_[kPrison], 3);
    EXPECT_EQ(mailbox_[kPhase], 3);
    EXPECT_FLOAT_EQ(prison_->chara[0].motion_type.state.time, 30.0f - 0.01f);
    EXPECT_EQ(prison_->chara[0].motion_speed, 0.0f);
    Step();
    EXPECT_EQ(mailbox_[kPrison], 3);
    EXPECT_EQ(prison_->phase[0], 1);

    // The freeze over, it shatters; gone, she may imprison again.
    Freeze(0);
    Step();
    EXPECT_EQ(mailbox_[kPrison], 0);
    EXPECT_EQ(prison_->phase[0], 2);
    prison_->active[0] = 0;
    Step();
    EXPECT_EQ(mailbox_[kPrison], 0);
}

TEST_F(IceQueenFight, ASpentArrowOnAnUnfrozenPlayerLetsHerAttackAgain) {
    mailbox_[kArrow] = 1;
    Step();
    arrow_->active[0] = 0;
    Step();
    EXPECT_EQ(mailbox_[kPhase], 1);
    Step();
    EXPECT_EQ(mailbox_[kPhase], 0);
    EXPECT_EQ(mailbox_[kPrison], 0);
    EXPECT_EQ(prison_->active[0], 0);
}

TEST_F(IceQueenFight, HerMeteorFallsOnThePlayerAndHerScriptCarriesOn) {
    ASSERT_NE(meteor_, nullptr);
    mailbox_[kMeteor] = 1;
    Step();
    EXPECT_EQ(mailbox_[kMeteor], 2);
    ASSERT_NE(meteor_->active[0], 0);
    float above[4];
    meteor_->chara[0].GetPosition(above);
    EXPECT_FLOAT_EQ(above[1], 50.0f);

    meteor_->phase[0] = 1;
    Step();
    EXPECT_FLOAT_EQ(meteor_->velocity[0][1], -1.6f);
    EXPECT_EQ(mailbox_[kMeteor], 2);

    meteor_->phase[0] = 2;
    Step();
    EXPECT_EQ(mailbox_[kMeteor], 0);
    EXPECT_EQ(mailbox_[kPhase], 4);
}

TEST_F(IceQueenFight, HerDefeatEndsHerCompanionsWhichStayLoaded) {
    mailbox_[kArrow] = 1;
    Step();
    ASSERT_NE(arrow_->active[0], 0);
    expanded::IceQueenDefeated();
    EXPECT_EQ(arrow_->active[0], 0);

    mailbox_[kArrow] = 1;
    Step();
    EXPECT_NE(arrow_->active[0], 0);
    EXPECT_EQ(mailbox_[kArrow], 0);
}
