#include <gtest/gtest.h>
#include <libvu0.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "cloth.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/inferno/cape_flap.hpp"
#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "weaponelement.hpp"

using expanded::Monster;

namespace {

// Goro out with weapon_no (attack 300) at the origin, the dungeon playing; a monster of weight 1 may stand at (x, 0).
class BurningAura : public ::testing::Test {
protected:
    explicit BurningAura(int weapon_no = ITEM_WEAPON_INFERNO) : wielding_(weapon_no) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_GORO;
        wielding_.weapon().attack = 300;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
        }
        saved_time_ = CharaMain.motion_type.state.time;
        CharaMain.SetPosition(0.0f, 0.0f, 0.0f);
        saved_hits_ = NowColData;
        NowColData = hits_.get();
        *hits_ = CCollisionData{};
        expanded::UnderworldReset();
        expanded::BurningAuraReset();
    }

    void TearDown() override {
        expanded::UnderworldReset();
        expanded::BurningAuraReset();
        NowColData = saved_hits_;
        CharaMain.SetPosition(saved_pos_[0], saved_pos_[1], saved_pos_[2]);
        CharaMain.motion_type.state.time = saved_time_;
        BtActStatus = {};
    }

    // action 13 his charge held, 14 his smash (short of the ground), anything else neither.
    void Acting(int action) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = action;
        CharaMain.motion_type.state.time = expanded::kUnderworldSmashLands - 20.0f;
    }

    // A Skeleton Soldier in slot 0 at (x, 0, 0), standing still, of weight 1 (a new floor: placed before anything else).
    void Place(float x) {
        floor_.SetUpModel(0, Monster::SKELETON_SOLDIER);
        floor_.Spawn(0, 0);
        MONSTOR &live = floor_.unit().monster[0];
        live.state = 2;
        live.revealed = -1;
        live.knockback[2] = 1.0f;
        live.movement_speed = 0.0f;
        floor_.unit().chara[0][0].frame = nullptr;
        floor_.unit().chara[0][0].SetPosition(x, 0.0f, 0.0f);
    }

    // Steps of the pull on the monster (kept where it stands), then the aura.
    void Steps(int count) {
        for (int i = 0; i < count; i++) {
            expanded::UnderworldMonsterMove(floor_.unit(), 0);
            floor_.unit().monster[0].movement_speed = 0.0f;
            expanded::BurningAuraStep();
        }
    }

    // The aura's hits planted so far, their damage.
    std::vector<int> Burns() {
        std::vector<int> burns;
        for (int i = 0; i < kHitRecordsHere; i++) {
            const COLLISION_HIT &hit = NowColData->hit[i];
            if (NowColData->active[i] != 0 && (hit.weapon_flags & expanded::kBlastMark) != 0) {
                EXPECT_NE(hit.flags & expanded::kFireElementBit, 0);
                burns.push_back(hit.damage);
            }
        }
        return burns;
    }

    static constexpr int kHitRecordsHere = sizeof(CCollisionData::hit) / sizeof(CCollisionData::hit[0]);

    expanded::test::TestFloor       floor_;
    expanded::test::Wielding        wielding_;
    expanded::test::StepsUnpaused   unpaused_;
    std::unique_ptr<CCollisionData> hits_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_hits_ = nullptr;
    float                           saved_pos_[3] = {};
    float                           saved_time_ = 0.0f;
};

class BurningAuraWithSatansAx : public BurningAura {
protected:
    BurningAuraWithSatansAx() : BurningAura(ITEM_WEAPON_SATAN_S_AX) {}
};

COLLISION_HIT Hit(int flags) {
    COLLISION_HIT hit{};
    hit.flags = flags;
    hit.damage = 50;
    return hit;
}

} // namespace

TEST_F(BurningAura, TheInfernoPullsAsSatansAxDoes) {
    Place(50.0f);
    Acting(14);
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 1.0f);
    expanded::UnderworldMonsterMove(floor_.unit(), 0);
    EXPECT_TRUE(expanded::UnderworldPulling(0));
}

TEST_F(BurningAura, AFireHitPassesHimBy) {
    EXPECT_TRUE(expanded::InfernoFireproof(Hit(expanded::kFireElementBit)));
    EXPECT_FALSE(expanded::InfernoFireproof(Hit(0)));
    EXPECT_FALSE(expanded::InfernoFireproof(Hit(2))); // ice
}

TEST_F(BurningAuraWithSatansAx, OnlyTheInfernoIsFireproof) {
    EXPECT_FALSE(expanded::InfernoFireproof(Hit(expanded::kFireElementBit)));
}

TEST_F(BurningAura, EachSecondBurnsAPercentMoreThanTheLast) {
    Place(50.0f);
    Acting(13); // the hold counts too
    Steps(expanded::kBurningAuraFrames - 1);
    EXPECT_TRUE(Burns().empty());
    Steps(1);
    EXPECT_EQ(Burns(), std::vector<int>{3}); // 1% of 300
    *hits_ = CCollisionData{};
    Acting(14);
    Steps(expanded::kBurningAuraFrames);
    EXPECT_EQ(Burns(), std::vector<int>{6}); // 2%
    *hits_ = CCollisionData{};
    Steps(expanded::kBurningAuraFrames);
    EXPECT_EQ(Burns(), std::vector<int>{9}); // 3%: 6% in all over three seconds
    EXPECT_EQ(expanded::BurningAuraSeconds(), 3);
}

TEST_F(BurningAura, ItStartsOverWithTheNextPull) {
    Place(50.0f);
    Acting(14);
    Steps(2 * expanded::kBurningAuraFrames);
    Acting(0);
    Steps(1);
    EXPECT_EQ(expanded::BurningAuraSeconds(), 0);
    *hits_ = CCollisionData{};
    Acting(14);
    Steps(expanded::kBurningAuraFrames);
    EXPECT_EQ(Burns(), std::vector<int>{3});
}

TEST_F(BurningAura, AMonsterThePullDoesNotDrawIsNotBurned) {
    Place(expanded::kUnderworldReach + 20.0f);
    Acting(14);
    Steps(expanded::kBurningAuraFrames);
    EXPECT_FALSE(expanded::UnderworldPulling(0));
    EXPECT_TRUE(Burns().empty());
}

TEST_F(BurningAuraWithSatansAx, SatansAxDoesNotBurn) {
    Place(50.0f);
    Acting(14);
    Steps(expanded::kBurningAuraFrames);
    EXPECT_TRUE(Burns().empty());
    EXPECT_FALSE(expanded::BurningAuraFeared(floor_.unit(), 0));
}

TEST_F(BurningAura, ADrawnMonsterIsAfraidUntilItComesNear) {
    Place(50.0f);
    Acting(14);
    Steps(1);
    EXPECT_TRUE(expanded::BurningAuraFeared(floor_.unit(), 0));
    floor_.unit().chara[0][0].SetPosition(expanded::kBurningAuraBrave - 1.0f, 0.0f, 0.0f);
    EXPECT_FALSE(expanded::BurningAuraFeared(floor_.unit(), 0));
    Acting(0);
    Steps(1);
    floor_.unit().chara[0][0].SetPosition(50.0f, 0.0f, 0.0f);
    EXPECT_FALSE(expanded::BurningAuraFeared(floor_.unit(), 0));
}

TEST_F(BurningAura, TheFogReddensAndThickensOverThreeSecondsAndFadesAfter) {
    Place(50.0f);
    Acting(14);
    Steps(expanded::kBurningAuraFogFrames / 2);
    EXPECT_NEAR(expanded::BurningAuraRed(), 0.5f, 1e-3f);
    float         rate[4] = {1000.0f, 3500.0f, 0.0f, 255.0f};
    unsigned char fog[3] = {0x60, 0xA0, 0xEF};
    expanded::BurningAuraFog(rate, fog);
    EXPECT_EQ(fog[0], (0x60 + 220) / 2);
    EXPECT_FLOAT_EQ(rate[0], 1000.0f * (1.0f + expanded::kBurningAuraFogPull) / 2.0f);
    Steps(expanded::kBurningAuraFogFrames);
    EXPECT_FLOAT_EQ(expanded::BurningAuraRed(), 1.0f);
    float         full_rate[4] = {1000.0f, 3500.0f, 0.0f, 255.0f};
    unsigned char full[3] = {0x60, 0xA0, 0xEF};
    expanded::BurningAuraFog(full_rate, full);
    EXPECT_FLOAT_EQ(full_rate[0], 1000.0f * expanded::kBurningAuraFogPull);
    EXPECT_FLOAT_EQ(full_rate[1], 3500.0f * expanded::kBurningAuraFogPull);
    EXPECT_EQ(full[0], 220);
    EXPECT_EQ(full[1], 40);
    EXPECT_EQ(full[2], 20);
    unsigned char background[3] = {0, 0, 0};
    expanded::BurningAuraBackground(background);
    EXPECT_EQ(background[0], 220);
    EXPECT_EQ(background[1], 40);
    EXPECT_EQ(background[2], 20);
    Acting(0);
    Steps(expanded::kBurningAuraFogFade - 1);
    EXPECT_GT(expanded::BurningAuraRed(), 0.0f);
    Steps(1);
    EXPECT_FLOAT_EQ(expanded::BurningAuraRed(), 0.0f);
}

TEST_F(BurningAura, TheLightReddensWithTheFog) {
    Place(50.0f);
    Acting(14);
    Steps(expanded::kBurningAuraFogFrames);
    float         ambient[4] = {64.0f, 64.0f, 64.0f, 128.0f};
    sceVu0FMATRIX colours = {
        {120.0f, 120.0f, 120.0f, 128.0f},
        {},
        {},
        {}
    };
    expanded::BurningAuraLight(ambient, colours);
    EXPECT_FLOAT_EQ(ambient[0], 64.0f * expanded::kBurningAuraLight[0]);
    EXPECT_FLOAT_EQ(ambient[1], 64.0f * expanded::kBurningAuraLight[1]);
    EXPECT_FLOAT_EQ(ambient[2], 64.0f * expanded::kBurningAuraLight[2]);
    EXPECT_FLOAT_EQ(ambient[3], 128.0f);
    EXPECT_FLOAT_EQ(colours[0][0], 120.0f * expanded::kBurningAuraLight[0]);
    EXPECT_FLOAT_EQ(colours[0][2], 120.0f * expanded::kBurningAuraLight[2]);
    EXPECT_FLOAT_EQ(colours[0][3], 128.0f);
}

TEST_F(BurningAura, TheShimmerComesUpFromHalfwayToFull) {
    Place(50.0f);
    Acting(14);
    Steps(expanded::kBurningAuraFogFrames / 2);
    EXPECT_FLOAT_EQ(expanded::BurningAuraHaze(), 0.0f);
    Steps(expanded::kBurningAuraFogFrames / 4);
    EXPECT_NEAR(expanded::BurningAuraHaze(), 0.5f, 1e-3f);
    Steps(expanded::kBurningAuraFogFrames / 4);
    EXPECT_FLOAT_EQ(expanded::BurningAuraHaze(), 1.0f);
    Acting(0);
    Steps(expanded::kBurningAuraFogFade);
    EXPECT_FLOAT_EQ(expanded::BurningAuraHaze(), 0.0f);
}

TEST_F(BurningAura, AFlameRisesOverHimAsThePullBeginsAndSinksAfter) {
    Place(50.0f);
    EXPECT_FLOAT_EQ(expanded::RisingFlameRisen(), 0.0f);
    Acting(13);
    Steps(expanded::kRisingFlameRise / 2);
    EXPECT_FLOAT_EQ(expanded::RisingFlameRisen(), 0.5f);
    Steps(expanded::kRisingFlameRise);
    EXPECT_FLOAT_EQ(expanded::RisingFlameRisen(), 1.0f);
    Acting(0);
    Steps(expanded::kRisingFlameRise);
    EXPECT_FLOAT_EQ(expanded::RisingFlameRisen(), 0.0f);
}

TEST_F(BurningAura, TheFlameGrowsAsHisSmashBeginsAndKeepsItsSizeUntilItHasSunk) {
    Place(50.0f);
    Acting(13);
    Steps(expanded::kRisingFlameRise);
    EXPECT_FLOAT_EQ(expanded::RisingFlameScale(), expanded::kRisingFlameScale);
    Acting(14);
    Steps(expanded::kRisingFlameGrow);
    EXPECT_FLOAT_EQ(expanded::RisingFlameScale(), expanded::kRisingFlameSmashScale);
    Acting(0);
    Steps(expanded::kRisingFlameRise - 1);
    EXPECT_FLOAT_EQ(expanded::RisingFlameScale(), expanded::kRisingFlameSmashScale);
    Steps(1);
    EXPECT_FLOAT_EQ(expanded::RisingFlameScale(), expanded::kRisingFlameScale);
}

TEST_F(BurningAura, TheFlamePlaysFasterAsItGrows) {
    Place(50.0f);
    Acting(13);
    Steps(expanded::kRisingFlameRise);
    EXPECT_FLOAT_EQ(expanded::RisingFlameMotionSpeed(), expanded::kRisingFlameMotionSpeed);
    Acting(14);
    Steps(expanded::kRisingFlameGrow / 2);
    EXPECT_FLOAT_EQ(expanded::RisingFlameMotionSpeed(), 0.5f * (expanded::kRisingFlameMotionSpeed + expanded::kRisingFlameSmashMotionSpeed));
    Steps(expanded::kRisingFlameGrow);
    EXPECT_FLOAT_EQ(expanded::RisingFlameMotionSpeed(), expanded::kRisingFlameSmashMotionSpeed);
}

TEST_F(BurningAura, GoroGlowsAsTheFlameRisesAndMoreAsItGrows) {
    auto glow = [](const CCharacter &character) {
        std::array<float, 3> ambient{};
        expanded::RisingFlameLight(character, ambient.data());
        return ambient;
    };
    Place(50.0f);
    EXPECT_EQ(glow(CharaMain), (std::array<float, 3>{}));
    Acting(13);
    Steps(expanded::kRisingFlameRise / 2);
    for (int c = 0; c < 3; c++) {
        EXPECT_FLOAT_EQ(glow(CharaMain)[c], 0.5f * expanded::kRisingFlameGlow[c]);
    }
    Steps(expanded::kRisingFlameRise);
    for (int c = 0; c < 3; c++) {
        EXPECT_FLOAT_EQ(glow(CharaMain)[c], expanded::kRisingFlameGlow[c]);
    }
    Acting(14);
    Steps(expanded::kRisingFlameGrow);
    for (int c = 0; c < 3; c++) {
        EXPECT_FLOAT_EQ(glow(CharaMain)[c], expanded::kRisingFlameSmashGlow[c]);
    }
    CCharacter other;
    EXPECT_EQ(glow(other), (std::array<float, 3>{}));
    Acting(0);
    Steps(expanded::kRisingFlameRise / 2);
    for (int c = 0; c < 3; c++) {
        EXPECT_FLOAT_EQ(glow(CharaMain)[c], 0.5f * expanded::kRisingFlameSmashGlow[c]);
    }
}

TEST(CapeFlap, TheCollarStaysDownAndTheHemFlies) {
    EXPECT_FLOAT_EQ(expanded::CapeFlapSwing(0, 8), 0.0f);
    EXPECT_NEAR(expanded::CapeFlapSwing(7, 8), expanded::kCapeFlapLift, expanded::kCapeFlapSwell + 1e-4f);
    EXPECT_LT(expanded::CapeFlapSwing(2, 8), expanded::CapeFlapSwing(7, 8));
}

TEST_F(BurningAura, HisCapeSwingsUpWithoutStretchingAndIsItsOwnOnceTheFlameHasSunk) {
    // A 3 by 8 cape hanging straight down from a collar along x.
    auto cape = std::make_unique<CCloth>();
    cape->num_i = 3;
    cape->num_j = 8;
    cape->frame = nullptr;
    for (int a = 0; a < 3; a++) {
        for (int b = 0; b < 8; b++) {
            cape->home[a][b][0] = static_cast<float>(a) * 2.0f;
            cape->home[a][b][1] = -static_cast<float>(b) * 1.5f;
            cape->home[a][b][2] = 0.0f;
        }
    }
    auto     own = std::make_unique<CCloth>(*cape);
    CCloth  *worn[CHARA_CLOTH_MAX] = {cape.get()};
    CCloth **saved = CharaMain.cloth;
    CharaMain.cloth = worn;
    Place(50.0f);
    Acting(13);
    for (int i = 0; i < expanded::kRisingFlameRise; i++) {
        Steps(1);
        expanded::CapeFlapStep();
    }
    float hem_was = own->home[1][7][1], hem_now = cape->home[1][7][1];
    EXPECT_FLOAT_EQ(cape->gravity[1], 0.0f); // held to the shape
    EXPECT_FLOAT_EQ(cape->stiffness[0], std::max(own->stiffness[0], expanded::kCapeFlapStiffness));
    EXPECT_GT(hem_now, hem_was + 1.0f); // up off the drop
    for (int a = 0; a < 3; a++) {
        EXPECT_FLOAT_EQ(cape->home[a][0][1], own->home[a][0][1]); // the collar stays
        for (int b = 1; b < 8; b++) {
            float dx = cape->home[a][b][0] - cape->home[a][b - 1][0], dy = cape->home[a][b][1] - cape->home[a][b - 1][1],
                  dz = cape->home[a][b][2] - cape->home[a][b - 1][2];
            EXPECT_NEAR(std::sqrt(dx * dx + dy * dy + dz * dz), 1.5f, 1e-4f);
            EXPECT_NEAR(cape->home[a][b][0], static_cast<float>(a) * 2.0f, 1e-4f); // each column in its own plane
        }
    }
    Acting(0);
    for (int i = 0; i < expanded::kRisingFlameRise; i++) {
        Steps(1);
        expanded::CapeFlapStep();
    }
    for (int a = 0; a < 3; a++) {
        for (int b = 0; b < 8; b++) {
            for (int k = 0; k < 3; k++) {
                EXPECT_FLOAT_EQ(cape->home[a][b][k], own->home[a][b][k]);
            }
        }
    }
    EXPECT_FLOAT_EQ(cape->gravity[1], own->gravity[1]);
    EXPECT_FLOAT_EQ(cape->stiffness[0], own->stiffness[0]);
    CharaMain.cloth = saved;
}

TEST_F(BurningAura, ItsChargeHitsWithFireItsSwingWithTheSelectedElement) {
    EXPECT_EQ(expanded::InfernoAttackElement(1, 2, WEAPON_ELEMENT_THUNDER), WEAPON_ELEMENT_FIRE);
    EXPECT_EQ(expanded::InfernoAttackElement(0, 2, WEAPON_ELEMENT_THUNDER), WEAPON_ELEMENT_THUNDER);
}

TEST_F(BurningAuraWithSatansAx, SatansAxsChargeHitsWithTheSelectedElement) {
    EXPECT_EQ(expanded::InfernoAttackElement(1, 2, WEAPON_ELEMENT_THUNDER), WEAPON_ELEMENT_THUNDER);
}

TEST_F(BurningAuraWithSatansAx, SatansAxRaisesTheFlameTooItsPull) {
    Place(50.0f);
    EXPECT_TRUE(expanded::SatansAxFlame());
    EXPECT_FALSE(expanded::InfernoBurning());
    Acting(13);
    Steps(expanded::kRisingFlameRise);
    EXPECT_FLOAT_EQ(expanded::RisingFlameRisen(), 1.0f);
    Acting(14);
    Steps(expanded::kRisingFlameGrow);
    EXPECT_FLOAT_EQ(expanded::RisingFlameScale(), expanded::kRisingFlameSmashScale);
    EXPECT_TRUE(Burns().empty()); // its flame alone: no Burning Aura
}

TEST_F(BurningAura, TheInfernoReddensToItsTintOverThreeSecondsAndFadesAfter) {
    CCharacter  inferno;
    CCharacter *saved = NowWeapon;
    NowWeapon = &inferno;
    auto lit = [&](const CCharacter &character) {
        float ambient[3] = {10.0f, 20.0f, 30.0f};
        expanded::BurningAuraWeaponLight(character, ambient);
        return std::array<float, 3>{ambient[0] - 10.0f, ambient[1] - 20.0f, ambient[2] - 30.0f};
    };
    Place(50.0f);
    EXPECT_FLOAT_EQ(lit(inferno)[0], 0.0f);
    Acting(13);
    Steps(expanded::kBurningAuraFogFrames / 2);
    EXPECT_NEAR(lit(inferno)[0], expanded::kBurningAuraTint[0] / 2.0f, 1e-3f);
    Steps(expanded::kBurningAuraFogFrames);
    std::array<float, 3> full = lit(inferno);
    for (int c = 0; c < 3; c++) {
        EXPECT_FLOAT_EQ(full[c], expanded::kBurningAuraTint[c]);
    }
    EXPECT_FLOAT_EQ(lit(CharaMain)[0], 0.0f); // only the weapon
    Acting(0);
    Steps(expanded::kBurningAuraFogFade);
    EXPECT_FLOAT_EQ(lit(inferno)[0], 0.0f);
    NowWeapon = saved;
}
