#include <gtest/gtest.h>

#include <memory>

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/status/freeze.hpp"
#include "userstatus.hpp"

TEST(BreakFreeze, EndsTheFreezeKeepsOtherAilmentsAndStripsTheHitsFreeze) {
    auto status = std::make_unique<CUserStatus>();
    CUserStatus *saved = UserStatus;
    COLLISION_HIT hit = {};

    UserStatus = status.get();
    status->cur_chara = 0;
    status->ailments[0] = AILMENT_FREEZE | AILMENT_POISON;
    status->ailment_frames[0] = 100;
    BtActStatus.movement_locked = 1;
    hit.flags = 0x100 | 0x100000 | 0x1;

    expanded::BreakFreeze(&hit);

    EXPECT_EQ(status->ailments[0], AILMENT_POISON);
    EXPECT_EQ(status->ailment_frames[0], 0);
    EXPECT_EQ(BtActStatus.movement_locked, 0);
    EXPECT_EQ(hit.flags, 0x1);

    hit.flags = 0x100;
    expanded::BreakFreeze(&hit);
    EXPECT_EQ(hit.flags, 0x100);

    UserStatus = saved;
}
