#include <gtest/gtest.h>

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/crystal_gemron.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/enemies/monsters.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;

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

// A Crystal Gemron (monster 0) set up with its arrow and prison packs faked, the player at the origin.
class CrystalGemronPrison : public ::testing::Test {
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
        floor_.SetUpModel(0, Monster::CRYSTAL_GEMRON);
        floor_.Spawn(0, 0);
        arrows_ = g_loaded[expanded::kIceArrowFile];
        prison_ = g_loaded[expanded::kIcePrisonFile];
        for (int index = expanded::kRetailShotEffects; index < expanded::ShotEffectCount(); index++) {
            if (expanded::ShotEffectAt(index) == arrows_) {
                arrow_index_ = index;
            }
        }
    }

    void TearDown() override {
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        sceVu0CopyVector(CharaMain.pos, saved_player_);
        NowShotEffect = saved_pack_;
    }

    void Step() { expanded::CrystalGemronStep(floor_.unit()); }

    void Freeze(int frames) {
        floor_.status().ailments[0] = frames > 0 ? AILMENT_FREEZE : 0;
        floor_.status().ailment_frames[0] = frames;
    }

    // An arrow fired at the player's chest from z, flying a step, then spent there.
    void SpendArrowFrom(float z) {
        float from[4] = {0.0f, 10.1f, z, 1.0f};
        float chest[4] = {0.0f, 10.1f, 0.0f, 1.0f};
        NowShotEffect->Set(arrow_index_, from, chest);
        int slot = arrows_->current_slot;
        ASSERT_GE(slot, 0);
        Step();
        arrows_->active[slot] = 0;
        Step();
    }

    expanded::test::TestFloor          floor_;
    std::unique_ptr<CSHOT_EFFECT_PACK> pack_ = std::make_unique<CSHOT_EFFECT_PACK>();
    CSHOT_EFFECT_PACK                 *saved_pack_ = nullptr;
    float                              saved_player_[4] = {};
    CSHOT_EFFECT                      *arrows_ = nullptr;
    CSHOT_EFFECT                      *prison_ = nullptr;
    int                                arrow_index_ = -1;
};

} // namespace

TEST_F(CrystalGemronPrison, AnArrowSpentNearAFrozenPlayerEncasesHimUntilTheFreezeEnds) {
    ASSERT_GE(arrow_index_, 0);
    ASSERT_NE(prison_, nullptr);
    Freeze(30);
    SpendArrowFrom(15.0f);
    EXPECT_EQ(prison_->active[0], 0);

    // Its hit takes a step more to freeze him; then the prison forms at his feet, the Gemron's.
    Step();
    ASSERT_NE(prison_->active[0], 0);
    EXPECT_EQ(prison_->phase[0], 0);
    EXPECT_EQ(prison_->user_id_2[0], 0);

    // Formed, it holds while his ailment runs, another spent arrow or not.
    prison_->phase[0] = 1;
    SpendArrowFrom(15.0f);
    Step();
    EXPECT_EQ(prison_->phase[0], 1);
    EXPECT_FLOAT_EQ(prison_->chara[0].motion_type.state.time, 30.0f - 0.01f);

    // The freeze over, it shatters.
    Freeze(0);
    Step();
    EXPECT_EQ(prison_->phase[0], 2);
    Step();
    EXPECT_EQ(prison_->phase[0], 2);

    // Gone, another may form.
    prison_->active[0] = 0;
    Step();
    Freeze(30);
    SpendArrowFrom(15.0f);
    Step();
    EXPECT_NE(prison_->active[0], 0);
}

TEST_F(CrystalGemronPrison, AFarArrowOrAnUnfrozenPlayerEncasesNoOne) {
    Freeze(30);
    SpendArrowFrom(25.0f);
    Step();
    Step();
    EXPECT_EQ(prison_->active[0], 0);

    Freeze(0);
    SpendArrowFrom(15.0f);
    Step();
    Step();
    EXPECT_EQ(prison_->active[0], 0);
}
