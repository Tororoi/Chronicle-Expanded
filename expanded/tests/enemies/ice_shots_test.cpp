#include <gtest/gtest.h>

#include <cstring>

#include "bt_shot_effect.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "test_support.hpp"

TEST(IceShots, ThePrisonHoldsWithoutAHit) {
    BT_SHOT_EFFECT prison = expanded::IcePrisonShot();
    EXPECT_STREQ(prison.model_name, "info");
    EXPECT_NE(prison.stationary, 0);
    EXPECT_EQ(prison.damage, 0);
    EXPECT_EQ(prison.motion[1], 0); // formed
    EXPECT_EQ(prison.motion[2], 1); // shattering
    // Each user enters its own copy.
    BT_SHOT_EFFECT again = expanded::IcePrisonShot();
    EXPECT_EQ(std::memcmp(&prison, &again, sizeof(prison)), 0);
}

TEST(IceShots, TheFreezeIsTheCurrentMembersAndAnyAilmentKeepsThePrison) {
    expanded::test::TestFloor floor;
    CUserStatus              &status = floor.status();
    status.cur_chara = 2;
    EXPECT_FALSE(expanded::PlayerFrozen());
    EXPECT_FALSE(expanded::PlayerAfflicted());

    status.ailments[1] = AILMENT_FREEZE;
    EXPECT_FALSE(expanded::PlayerFrozen());
    status.ailments[2] = AILMENT_FREEZE;
    EXPECT_TRUE(expanded::PlayerFrozen());

    status.ailment_frames[2] = 30;
    EXPECT_TRUE(expanded::PlayerAfflicted());
    status.ailment_frames[2] = 0;
    EXPECT_FALSE(expanded::PlayerAfflicted());
}

TEST(IceShots, ReachIsMeasuredToThePlayersChest) {
    float saved[4];
    sceVu0CopyVector(saved, CharaMain.pos);
    CharaMain.pos[0] = 100.0f;
    CharaMain.pos[1] = 5.0f;
    CharaMain.pos[2] = -40.0f;
    const float chest[3] = {100.0f, 15.1f, -40.0f};
    const float beside[3] = {100.0f, 15.1f, -20.0f};
    const float feet[3] = {100.0f, 5.0f, -40.0f};
    EXPECT_TRUE(expanded::NearPlayerChest(chest, 0.0f));
    EXPECT_TRUE(expanded::NearPlayerChest(beside, 20.0f));
    EXPECT_FALSE(expanded::NearPlayerChest(beside, 19.9f));
    EXPECT_FALSE(expanded::NearPlayerChest(feet, 10.0f));
    EXPECT_TRUE(expanded::NearPlayerChest(feet, 10.2f));
    sceVu0CopyVector(CharaMain.pos, saved);
}
