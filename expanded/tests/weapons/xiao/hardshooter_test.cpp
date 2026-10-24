#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "expanded/weapons/xiao/hardshooter.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::TestFloor;
using expanded::test::Wielding;

// A floor with Xiao holding weapon_no (carrying sphere when given) and a pellet in slot 0 flying along +z.
class Ricochet : public ::testing::Test {
protected:
    explicit Ricochet(int weapon_no = ITEM_WEAPON_HARDSHOOTER, int sphere = 0) : wielding_(weapon_no, sphere) {}

    void SetUp() override {
        floor_.status().cur_chara = CHARA_XIAO;
        *shots_ = CSHOT{};
        shots_->used[0] = 1;
        shots_->pos[0][2] = 0.0f;
        shots_->vector[0][2] = 4.0f;
        shots_->damage[0] = 30;
        shots_->size[0] = 2.5f;
        shots_->life[0] = 50;
    }

    // monster standing with one hurt sphere of radius r at (x, y, z).
    void Stand(int monster, float x, float y, float z, float r = 6.0f) {
        MONSTOR &live = floor_.unit().monster[monster];
        live.state = 2;
        live.revealed = 1;
        live.hp = 10;
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[monster];
        effect.position[0][0] = x;
        effect.position[0][1] = y;
        effect.position[0][2] = z;
        effect.position[0][3] = 1.0f;
        effect.radius[0] = r;
        effect.timer[0] = 1;
    }

    // The pellet in slot 0 landing on monster's sphere, from where it stands.
    int Land(int monster, float at_z) {
        shots_->pos[0][2] = at_z;
        return expanded::RicochetShotEnded(*shots_, 0, floor_.unit().effect[monster].position[0], true);
    }

    float Speed(int slot) const {
        const float *v = shots_->vector[slot];
        return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    }

    TestFloor              floor_;
    Wielding               wielding_;
    std::unique_ptr<CSHOT> shots_ = std::make_unique<CSHOT>();
};

class RicochetWithAnotherWeapon : public Ricochet {
protected:
    RicochetWithAnotherWeapon() : Ricochet(ITEM_WEAPON_BONE_SLINGSHOT) {}
};

class RicochetBySuperSteve : public Ricochet {
protected:
    RicochetBySuperSteve() : Ricochet(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HARDSHOOTER) {}
};

} // namespace

TEST_F(Ricochet, FliesAtTheNextNearestMonsterFromClearOfTheFirst) {
    Stand(0, 0.0f, 10.0f, 50.0f);
    Stand(1, 60.0f, 10.0f, 50.0f);  // 60 from the first
    Stand(2, -90.0f, 10.0f, 50.0f); // farther
    EXPECT_EQ(expanded::RicochetStruck(floor_.unit(), floor_.unit().effect[0].position[0]), 0);

    int slot = Land(0, 44.0f);

    ASSERT_GT(slot, 0);
    EXPECT_TRUE(expanded::Ricochet(slot));
    EXPECT_EQ(shots_->used[slot], 1);
    EXPECT_EQ(shots_->damage[slot], 30);
    EXPECT_FLOAT_EQ(shots_->size[slot], 2.5f);
    EXPECT_EQ(shots_->life[slot], expanded::kRicochetLife);
    EXPECT_NEAR(Speed(slot), 4.0f, 1e-4f);
    EXPECT_GT(shots_->vector[slot][0], 3.0f); // toward monster 1
    // Held off the game's collision until it is clear of the first monster's sphere (radius 6, the pellet's 2, a
    // frame's 4).
    EXPECT_EQ(shots_->halted[slot], 1);
    int frames = 0;
    while (shots_->halted[slot] != 0 && frames < 40) {
        expanded::RicochetStep(*shots_, slot);
        frames++;
    }
    EXPECT_EQ(shots_->halted[slot], 0);
    float dx = shots_->pos[slot][0], dz = shots_->pos[slot][2] - 50.0f, dy = shots_->pos[slot][1] - 10.0f;
    EXPECT_GT(std::sqrt(dx * dx + dy * dy + dz * dz), 12.0f);
    expanded::RicochetStep(*shots_, slot); // the game's from here
    EXPECT_EQ(shots_->halted[slot], 0);
}

TEST_F(Ricochet, StaysOffTheGamesCollisionWhileStillWithinTheFirst) {
    Stand(0, 0.0f, 10.0f, 50.0f);
    Stand(1, 12.0f, 10.0f, 50.0f); // overlapping it

    int slot = Land(0, 44.0f);
    ASSERT_GT(slot, 0);
    while (shots_->halted[slot] != 0) {
        expanded::RicochetStep(*shots_, slot);
    }
    // Released past the first monster's reach (6 + 2), within the second's.
    float dx = shots_->pos[slot][0], dz = shots_->pos[slot][2] - 50.0f;
    EXPECT_GT(std::sqrt(dx * dx + dz * dz), 8.0f);
    float ex = shots_->pos[slot][0] - 12.0f;
    EXPECT_LE(std::sqrt(ex * ex + dz * dz), 8.0f);
}

TEST_F(Ricochet, AimsAtTheLargestSphereTakingHitsNow) {
    Stand(0, 0.0f, 10.0f, 50.0f);
    Stand(1, 60.0f, 2.0f, 50.0f, 3.0f); // a small foot sphere
    MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[1];
    // A larger body sphere, and a larger still one out of its motion window.
    effect.position[1][0] = 60.0f;
    effect.position[1][1] = 14.0f;
    effect.position[1][2] = 50.0f;
    effect.radius[1] = 8.0f;
    effect.timer[1] = 1;
    effect.position[2][0] = 60.0f;
    effect.position[2][1] = 30.0f;
    effect.position[2][2] = 50.0f;
    effect.radius[2] = 12.0f;
    effect.timer[2] = 1;
    effect.motion_start[2] = 40.0f;
    effect.motion_end[2] = 50.0f;
    // A nearer monster with only a sphere out of its window is passed over.
    Stand(2, -30.0f, 10.0f, 50.0f);
    floor_.unit().effect[2].motion_start[0] = 40.0f;
    floor_.unit().effect[2].motion_end[0] = 50.0f;

    int slot = Land(0, 44.0f);

    ASSERT_GT(slot, 0);
    const float *from = shots_->pos[slot];
    const float *v = shots_->vector[slot];
    float        t = (60.0f - from[0]) / v[0];
    EXPECT_GT(t, 0.0f);
    EXPECT_NEAR(from[1] + v[1] * t, 14.0f, 0.01f); // at the body sphere's height
    EXPECT_NEAR(from[2] + v[2] * t, 50.0f, 0.01f);
}

TEST_F(Ricochet, GoesALevelRandomWayWithNoneNear) {
    Stand(0, 0.0f, 10.0f, 50.0f);
    Stand(1, 200.0f, 10.0f, 50.0f); // past the range

    int slot = Land(0, 44.0f);

    ASSERT_GT(slot, 0);
    EXPECT_FLOAT_EQ(shots_->vector[slot][1], 0.0f);
    EXPECT_NEAR(Speed(slot), 4.0f, 1e-4f);
}

TEST_F(Ricochet, NeitherARicochetNorAWallHitNorAMissBounces) {
    Stand(0, 0.0f, 10.0f, 50.0f);
    Stand(1, 60.0f, 10.0f, 50.0f);
    int slot = Land(0, 44.0f);
    ASSERT_GT(slot, 0);

    // The ricochet landing on monster 1 sends none.
    shots_->pos[slot][0] = 55.0f;
    EXPECT_EQ(expanded::RicochetShotEnded(*shots_, slot, floor_.unit().effect[1].position[0], true), -1);
    EXPECT_FALSE(expanded::Ricochet(slot));

    float wall[4] = {0.0f, 10.0f, 40.0f, 1.0f};
    EXPECT_EQ(expanded::RicochetShotEnded(*shots_, 0, wall, false), -1);
    EXPECT_EQ(expanded::RicochetShotEnded(*shots_, 0, wall, true), -1); // no sphere there
}

TEST_F(Ricochet, AFreshPelletInTheSlotIsNoRicochet) {
    Stand(0, 0.0f, 10.0f, 50.0f);
    int slot = Land(0, 44.0f);
    ASSERT_GT(slot, 0);
    expanded::RicochetShotFired(slot);
    EXPECT_FALSE(expanded::Ricochet(slot));
}

TEST_F(RicochetWithAnotherWeapon, SendsNone) {
    Stand(0, 0.0f, 10.0f, 50.0f);
    EXPECT_EQ(Land(0, 44.0f), -1);
}

TEST_F(RicochetBySuperSteve, SendsOneWithTheSphere) {
    Stand(0, 0.0f, 10.0f, 50.0f);
    EXPECT_GT(Land(0, 44.0f), 0);
}
