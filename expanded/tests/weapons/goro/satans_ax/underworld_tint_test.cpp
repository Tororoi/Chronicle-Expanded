#include <gtest/gtest.h>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_tint.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

namespace {

// Goro with Satan's Ax, its model the weapon drawn, the tint off.
class UnderworldTint : public ::testing::Test {
protected:
    UnderworldTint() : wielding_(ITEM_WEAPON_SATAN_S_AX) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_GORO;
        saved_weapon_ = NowWeapon;
        NowWeapon = &ax_;
        saved_time_ = CharaMain.motion_type.state.time;
        CharaMain.motion_type.state.time = expanded::kUnderworldSmashLands - 20.0f;
        expanded::UnderworldTintReset();
    }

    void TearDown() override {
        expanded::UnderworldTintReset();
        NowWeapon = saved_weapon_;
        CharaMain.motion_type.state.time = saved_time_;
        BtActStatus = {};
    }

    void Acting(int action) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = action;
    }

    void Steps(int count) {
        for (int i = 0; i < count; i++) {
            expanded::UnderworldTintStep();
        }
    }

    float Lit(const CCharacter &character) {
        float ambient[3] = {10.0f, 20.0f, 30.0f};
        expanded::UnderworldLight(character, ambient);
        EXPECT_FLOAT_EQ(ambient[1] - 20.0f, ambient[0] - 10.0f);
        EXPECT_FLOAT_EQ(ambient[2] - 30.0f, ambient[0] - 10.0f);
        return ambient[0] - 10.0f;
    }

    expanded::test::TestFloor floor_;
    expanded::test::Wielding  wielding_;
    CCharacter                ax_;
    CCharacter               *saved_weapon_ = nullptr;
    float                     saved_time_ = 0.0f;
};

} // namespace

TEST_F(UnderworldTint, NoneWithoutThePull) {
    Acting(0x24);
    Steps(30);
    EXPECT_FLOAT_EQ(Lit(ax_), 0.0f);
}

TEST_F(UnderworldTint, ItFadesInOverAQuarterSecondOfThePull) {
    Acting(13); // the hold's pull too
    Steps(1);
    EXPECT_NEAR(Lit(ax_), expanded::kUnderworldTint / expanded::kUnderworldTintFade, 1e-3f);
    Steps(expanded::kUnderworldTintFade / 2);
    EXPECT_GT(Lit(ax_), 0.0f);
    EXPECT_LT(Lit(ax_), expanded::kUnderworldTint);
    Steps(expanded::kUnderworldTintFade);
    EXPECT_FLOAT_EQ(Lit(ax_), expanded::kUnderworldTint);
    Acting(14);
    Steps(5);
    EXPECT_FLOAT_EQ(Lit(ax_), expanded::kUnderworldTint);
}

TEST_F(UnderworldTint, ItFadesOutAsThePullEnds) {
    Acting(14);
    Steps(expanded::kUnderworldTintFade);
    CharaMain.motion_type.state.time = expanded::kUnderworldSmashLands; // the smash lands
    Steps(expanded::kUnderworldTintFade - 1);
    EXPECT_NEAR(Lit(ax_), expanded::kUnderworldTint / expanded::kUnderworldTintFade, 1e-3f);
    Steps(1);
    EXPECT_FLOAT_EQ(Lit(ax_), 0.0f);
}

TEST_F(UnderworldTint, OnlyTheAx) {
    Acting(14);
    Steps(expanded::kUnderworldTintFade);
    EXPECT_FLOAT_EQ(Lit(CharaMain), 0.0f);
}
