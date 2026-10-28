#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/osmond/skunk.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "shot_firebar.hpp"
#include "test_support.hpp"

namespace {

constexpr int   kParticles = 24;
constexpr float kGameSpacing = 2.0f;

// Osmond's flame gun held along +z from the origin with weapon_no in hand, as BattleActionPlay_Ozumond_F holds it: Init
// once, then Set and the firebar's step every frame. Its hits are planted in a collision table of the test's own.
class Flame {
public:
    explicit Flame(int weapon_no) : wielding_(weapon_no) {
        floor_.status().cur_chara = CHARA_OSMOND;
        NowColData = hits_.get();
        float aim[4] = {0.0f, 0.0f, 1.0f, 1.0f};
        bar_->Init(origin_, aim, 0, 0);
    }

    ~Flame() { NowColData = saved_hits_; }

    Flame(const Flame &) = delete;
    Flame &operator=(const Flame &) = delete;

    // The burst held for frames; every particle plants its hit once in thirty.
    void Hold(int frames = 30) {
        for (int i = 0; i < frames; i++) {
            float aim[4] = {0.0f, 0.0f, 1.0f, 1.0f};
            bar_->Set(origin_, aim, 10, 0);
            bar_->Step();
        }
    }

    // How far the particle lies from the muzzle.
    float Distance(int particle) const { return std::hypot(bar_->position[particle][0], bar_->position[particle][1], bar_->position[particle][2]); }

    // How many flame hits are planted, and how far the farthest lies.
    int Planted() const {
        int count = 0;
        for (int i = 0; i < 96; i++) {
            count += hits_->active[i] != 0 && hits_->hit[i].owner == CHARA_OSMOND ? 1 : 0;
        }
        return count;
    }

    float HitReach() const {
        float reach = 0.0f;
        for (int i = 0; i < 96; i++) {
            if (hits_->active[i] != 0) {
                reach = std::max(reach, std::hypot(hits_->hit[i].pos[0], hits_->hit[i].pos[1], hits_->hit[i].pos[2]));
            }
        }
        return reach;
    }

    // Whether a monster's hurt sphere of radius, its centre distance along the aim, meets a planted hit, as CheckDmg
    // finds one.
    bool Reaches(float distance, float radius) {
        float centre[4] = {0.0f, 0.0f, distance, 1.0f};
        return hits_->FindMonsterHit(centre, radius) != -1;
    }

private:
    expanded::test::TestFloor       floor_;
    expanded::test::Wielding        wielding_;
    std::unique_ptr<CSHOT_FIREBAR>  bar_ = std::make_unique<CSHOT_FIREBAR>();
    std::unique_ptr<CCollisionData> hits_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_hits_ = NowColData;
    float                           origin_[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

} // namespace

TEST(LongerFlame, TheSkunkSpacesItsParticlesTwiceAsFar) {
    expanded::test::Wielding skunk(ITEM_WEAPON_SKUNK);
    EXPECT_FLOAT_EQ(expanded::FlameSpacing(kGameSpacing), 4.0f);
}

TEST(LongerFlame, TheSkunksFlameReachesTwiceAsFar) {
    Flame flame(ITEM_WEAPON_SKUNK);
    flame.Hold();
    for (int particle = 0; particle < kParticles; particle++) {
        EXPECT_NEAR(flame.Distance(particle), 4.0f * static_cast<float>(particle), 1e-3f) << particle;
    }
    EXPECT_NEAR(flame.Distance(kParticles - 1), 92.0f, 1e-3f);
}

TEST(LongerFlame, TheBlessingGunKeepsTheGamesReach) {
    {
        expanded::test::Wielding gun(ITEM_WEAPON_BLESSING_GUN);
        EXPECT_FLOAT_EQ(expanded::FlameSpacing(kGameSpacing), kGameSpacing);
    }
    Flame flame(ITEM_WEAPON_BLESSING_GUN);
    flame.Hold();
    EXPECT_NEAR(flame.Distance(kParticles - 1), 46.0f, 1e-3f);
}

TEST(LongerFlame, EveryParticleHitsAllTheWayOut) {
    Flame skunk(ITEM_WEAPON_SKUNK);
    skunk.Hold();
    EXPECT_EQ(skunk.Planted(), kParticles);
    EXPECT_NEAR(skunk.HitReach(), 92.0f, 1e-3f);
    // Radius-4 hits 4 apart leave no gap anywhere along the flame.
    for (float distance = 0.0f; distance <= 96.0f; distance += 1.0f) {
        EXPECT_TRUE(skunk.Reaches(distance, 0.0f)) << distance;
    }
    EXPECT_FALSE(skunk.Reaches(97.0f, 0.0f));

    Flame gun(ITEM_WEAPON_BLESSING_GUN);
    gun.Hold();
    EXPECT_EQ(gun.Planted(), kParticles);
    EXPECT_NEAR(gun.HitReach(), 46.0f, 1e-3f);
}

TEST(LongerFlame, AMonsterEightyFiveAwayIsInTheSkunksReachAlone) {
    Flame skunk(ITEM_WEAPON_SKUNK);
    skunk.Hold();
    EXPECT_TRUE(skunk.Reaches(85.0f, 5.0f));

    Flame gun(ITEM_WEAPON_BLESSING_GUN);
    gun.Hold();
    EXPECT_FALSE(gun.Reaches(85.0f, 5.0f));
    EXPECT_TRUE(gun.Reaches(50.0f, 5.0f));
}

TEST(LongerFlame, WeaponAbilitiesOffKeepsTheGamesSpacing) {
    expanded::test::Wielding    skunk(ITEM_WEAPON_SKUNK);
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_FLOAT_EQ(expanded::FlameSpacing(kGameSpacing), kGameSpacing);
}

TEST(LongerFlame, SuperSteveCarryingItsSphereHasNoFlameToLengthen) {
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SKUNK);
    EXPECT_FLOAT_EQ(expanded::FlameSpacing(kGameSpacing), kGameSpacing);
}
