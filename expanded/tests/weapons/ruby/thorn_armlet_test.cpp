#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/prop_models.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/ruby/thorn_armlet.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::TestFloor;
using expanded::test::Wielding;

constexpr int kBite = 50;
constexpr int kMonster = 3;

// The armlet's character, stood in for: an identity root and a child.
struct Rig {
    CFrameVu1 frames[2];
};

Rig                      rig;
std::vector<std::string> loaded;

bool FakeArmlet(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *) {
    chara.Initialize();
    loaded.emplace_back(path);
    for (CFrameVu1 &frame : rig.frames) {
        frame.Initialize();
    }
    std::strncpy(rig.frames[0].name, "13", sizeof(rig.frames[0].name) - 1);
    rig.frames[1].SetParent(&rig.frames[0]);
    chara.frame = &rig.frames[0];
    return true;
}

// Ruby out with weapon_no (carrying sphere when given), at the origin; a monster in slot kMonster at (0, 0, 10).
class ThornGuard : public ::testing::Test {
protected:
    explicit ThornGuard(int chara = CHARA_RUBY, int weapon_no = ITEM_WEAPON_THORN_ARMLET, int sphere = 0)
        : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        BtActStatus = {};
        BtActStatus.action_gauge = expanded::kFullGauge;
        floor_.status().cur_chara = chara_;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        saved_data_ = NowColData;
        NowColData = data_.get();
        loaded.clear();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakeArmlet);
        expanded::ThornArmletReset();
        Stand();
    }

    void TearDown() override {
        expanded::ThornArmletReset();
        expanded::SetFloorCharacterLoader(nullptr);
        expanded::FloorCharactersClear();
        NowColData = saved_data_;
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        BtActStatus = {};
    }

    void Guard(bool on) { BtActStatus.action_on = on ? expanded::kGuardAction : 0; }

    // motionDrive's refill of amount, through the hook.
    static void Refill(float amount) {
        for (float left = amount; left > 0.0f; left -= 1.0f) {
            BtActStatus.action_gauge = std::min(expanded::kFullGauge, BtActStatus.action_gauge + expanded::ThornGuardGaugeGain(std::min(1.0f, left)));
        }
    }

    void Steps(int frames) {
        for (int i = 0; i < frames; i++) {
            expanded::ThornArmletStep();
        }
    }

    void Stand() {
        MONSTOR &live = floor_.unit().monster[kMonster];
        live.state = 2;
        live.revealed = -1;
        live.hp = 100;
        live.max_hp = 100;
        floor_.unit().chara[kMonster][0].pos[2] = 10.0f;
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[kMonster];
        effect.position[0][2] = 10.0f;
        effect.position[0][1] = 5.0f;
        effect.position[0][3] = 1.0f;
        effect.radius[0] = 3.0f;
        effect.timer[0] = 1;
    }

    // The monster's bite, as its attack plants it (owner its slot * 5 + 200, no monster_no); a shot names its monster.
    static COLLISION_HIT Bite(bool shot = false) {
        COLLISION_HIT hit = {};
        hit.owner = kMonster * 5 + 200;
        hit.monster_no = shot ? kMonster : -1;
        hit.attack_no = 0;
        hit.damage = kBite;
        hit.kind = 3;
        return hit;
    }

    std::vector<COLLISION_HIT *> Planted() {
        std::vector<COLLISION_HIT *> hits;
        for (int i = 0; i < 96; i++) {
            if (NowColData->active[i] != 0) {
                hits.push_back(&NowColData->hit[i]);
            }
        }
        return hits;
    }

    TestFloor                       floor_;
    Wielding                        wielding_;
    int                             chara_;
    std::unique_ptr<CCollisionData> data_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_data_ = nullptr;
    float                           saved_pos_[3] = {};
};

class ThornGuardBySuperSteve : public ThornGuard {
protected:
    ThornGuardBySuperSteve() : ThornGuard(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_THORN_ARMLET) {}
};

class ThornGuardWithAnotherRing : public ThornGuard {
protected:
    ThornGuardWithAnotherRing() : ThornGuard(CHARA_RUBY, ITEM_WEAPON_CRYSTAL_RING) {}
};

} // namespace

TEST_F(ThornGuard, TheArmletStandsAroundHerWhileSheGuards) {
    Steps(3);
    EXPECT_FALSE(expanded::ThornArmletUp());
    EXPECT_TRUE(loaded.empty());

    Guard(true);
    Steps(expanded::kThornFadeFrames);
    ASSERT_EQ(loaded, std::vector<std::string>{"commenu/weapon/c05w13.chr"});
    EXPECT_FLOAT_EQ(expanded::ThornArmletAlpha(), 1.0f);
    // Its ring's axis, the model's x, stood upright, pointing down.
    CFrame &root = *expanded::ThornArmletCharacter()->frame;
    EXPECT_FLOAT_EQ(root.local[0][1], -1.0f);
    EXPECT_FLOAT_EQ(root.local[1][0], 1.0f);
    // Turning a full turn every kThornTurnFrames.
    float before = expanded::ThornArmletTurn();
    Steps(expanded::kThornTurnFrames / 4);
    EXPECT_NEAR(expanded::ThornArmletTurn() - before, -std::numbers::pi_v<float> / 2.0f, 1e-3f);

    Guard(false);
    Steps(expanded::kThornFadeFrames);
    EXPECT_FALSE(expanded::ThornArmletUp());

    // Raised again, the same copy, stood up once.
    Guard(true);
    Steps(1);
    EXPECT_EQ(loaded.size(), 1u);
    EXPECT_FLOAT_EQ(root.local[0][1], -1.0f);
}

// The glow grows over kThornGlowFrames as the thorns rise, and shrinks over as many once they go.
TEST_F(ThornGuard, TheGlowGrowsAndShrinksOverAQuarterSecond) {
    Guard(true);
    Steps(expanded::kThornGlowFrames - 1);
    EXPECT_LT(expanded::ThornArmletGlow(), 1.0f);
    EXPECT_FLOAT_EQ(expanded::ThornArmletAlpha(), 1.0f); // the thorns are solid sooner
    Steps(1);
    EXPECT_FLOAT_EQ(expanded::ThornArmletGlow(), 1.0f);
    Guard(false);
    Steps(expanded::kThornGlowFrames / 2);
    EXPECT_GT(expanded::ThornArmletGlow(), 0.0f);
    EXPECT_LT(expanded::ThornArmletGlow(), 1.0f);
    Steps(expanded::kThornGlowFrames);
    EXPECT_FLOAT_EQ(expanded::ThornArmletGlow(), 0.0f);
}

TEST_F(ThornGuard, AMeleeHitTurnsOnTheMonster) {
    Guard(true);
    Steps(1);
    COLLISION_HIT bite = Bite();
    EXPECT_TRUE(expanded::PlayerHitPassesBy(bite)); // she is untouched
    std::vector<COLLISION_HIT *> hits = Planted();
    ASSERT_EQ(hits.size(), 1u);
    const COLLISION_HIT &thorns = *hits[0];
    EXPECT_EQ(thorns.owner, CHARA_RUBY);
    EXPECT_EQ(thorns.damage, kBite);
    EXPECT_FLOAT_EQ(thorns.pos[2], 10.0f);
    EXPECT_GE(thorns.radius, expanded::kThornHitRadius);
    EXPECT_TRUE(expanded::ThornHit(thorns));
    EXPECT_FALSE(expanded::HitWearsWeapon(thorns));
    EXPECT_FLOAT_EQ(thorns.knockback_speed, expanded::kThornKick.speed);
    EXPECT_FLOAT_EQ(thorns.knockback_decay, expanded::kThornKick.decay);
    EXPECT_EQ(thorns.knockback_mode, 2);
}

TEST_F(ThornGuard, OnceASwing) {
    Guard(true);
    COLLISION_HIT bite = Bite();
    for (int frame = 0; frame < 10; frame++) { // the attack's window, planting its hit every frame
        Steps(1);
        EXPECT_TRUE(expanded::ThornGuardHit(bite)) << frame;
    }
    EXPECT_EQ(Planted().size(), 1u);

    Steps(expanded::kThornRearmFrames + 1); // its next swing
    EXPECT_TRUE(expanded::ThornGuardHit(bite));
    EXPECT_EQ(Planted().size(), 2u);
}

// The raise counts: a blow as she raises the guard (its action on, the hold's guard_mode not yet set) is turned.
TEST_F(ThornGuard, RaisingTheGuardCounts) {
    BtActStatus.action_on = expanded::kGuardAction;
    BtActStatus.guard_mode = 0;
    Steps(1);
    EXPECT_TRUE(expanded::ThornGuardHit(Bite()));
    EXPECT_EQ(Planted().size(), 1u);
    EXPECT_GT(expanded::ThornArmletAlpha(), 0.0f);
}

// The gauge is its health: a third off for each hit it turns, and the third breaks it.
TEST_F(ThornGuard, ThreeHitsBreakIt) {
    Guard(true);
    Steps(expanded::kThornFadeFrames);
    for (int hit = 1; hit <= expanded::kThornHits; hit++) {
        EXPECT_TRUE(expanded::ThornGuardHit(Bite())) << hit;
        EXPECT_NEAR(BtActStatus.action_gauge, expanded::kFullGauge * (expanded::kThornHits - hit) / expanded::kThornHits, 0.01f) << hit;
        EXPECT_EQ(Planted().size(), static_cast<size_t>(hit));
        Steps(expanded::kThornRearmFrames + 1);
    }
    EXPECT_FALSE(expanded::ThornArmletStanding());
    EXPECT_TRUE(expanded::ThornArmletCooling());
    // It clears away, and her guard is the game's again.
    Steps(expanded::kThornBreakFrames);
    EXPECT_FALSE(expanded::ThornArmletUp());
    EXPECT_FALSE(expanded::ThornGuardHit(Bite()));
    EXPECT_EQ(Planted().size(), static_cast<size_t>(expanded::kThornHits));
}

// The swing that breaks it passes her by to its end.
TEST_F(ThornGuard, TheBreakingSwingStillPassesHerBy) {
    Guard(true);
    Steps(1);
    for (int hit = 1; hit < expanded::kThornHits; hit++) {
        expanded::ThornGuardHit(Bite());
        Steps(expanded::kThornRearmFrames + 1);
    }
    EXPECT_TRUE(expanded::ThornGuardHit(Bite())); // breaks it
    Steps(1);
    EXPECT_TRUE(expanded::ThornGuardHit(Bite())); // the same swing's next frame
}

// While it stands the gauge holds: no refill, and a hit on her (which fills it) is put back.
TEST_F(ThornGuard, TheGaugeHoldsWhileItStands) {
    Guard(true);
    Steps(1);
    expanded::ThornGuardHit(Bite());
    EXPECT_FLOAT_EQ(expanded::ThornGuardGaugeGain(3.0f), 0.0f);
    BtActStatus.action_gauge = expanded::kFullGauge;
    Steps(1);
    EXPECT_NEAR(BtActStatus.action_gauge, expanded::kFullGauge * 2 / 3, 0.01f);
}

// Broken, it refills only once she lets go, and stands again on a full gauge.
TEST_F(ThornGuard, BrokenItWaitsForTheGuardToDropAndTheGaugeToFill) {
    Guard(true);
    Steps(1);
    for (int hit = 0; hit < expanded::kThornHits; hit++) {
        expanded::ThornGuardHit(Bite());
        Steps(expanded::kThornRearmFrames + 1);
    }
    Steps(expanded::kThornBreakFrames);
    EXPECT_FLOAT_EQ(expanded::ThornGuardGaugeGain(3.0f), 0.0f); // guard still held
    // A hit on her fills the gauge: it does not count.
    BtActStatus.action_gauge = expanded::kFullGauge;
    Steps(1);
    EXPECT_FLOAT_EQ(BtActStatus.action_gauge, 0.0f);
    EXPECT_FALSE(expanded::ThornArmletUp());
    EXPECT_FALSE(expanded::ThornGuardHit(Bite()));
    // Let go, it refills at her rate; raised again before it is full, no armlet.
    Guard(false);
    Steps(1);
    Refill(60.0f);
    Guard(true);
    Steps(expanded::kThornFadeFrames);
    EXPECT_FALSE(expanded::ThornArmletUp());
    EXPECT_TRUE(expanded::ThornArmletCooling());
    Guard(false);
    Refill(expanded::kFullGauge - 60.0f);
    EXPECT_FALSE(expanded::ThornArmletCooling());
    Guard(true);
    Steps(expanded::kThornFadeFrames);
    EXPECT_TRUE(expanded::ThornArmletStanding());
    EXPECT_EQ(expanded::ThornArmletHits(), expanded::kThornHits);
}

// Lowered after a hit, the gauge refills from what is left; raised before it is full, there is no armlet.
TEST_F(ThornGuard, OnlyOnAFullGauge) {
    Guard(true);
    Steps(1);
    expanded::ThornGuardHit(Bite());
    Guard(false);
    Steps(expanded::kThornFadeFrames);
    EXPECT_FALSE(expanded::ThornArmletUp());
    EXPECT_FLOAT_EQ(expanded::ThornGuardGaugeGain(3.0f), 3.0f);
    Guard(true);
    Steps(expanded::kThornFadeFrames);
    EXPECT_FALSE(expanded::ThornArmletUp());
    Steps(expanded::kThornRearmFrames + 1);
    EXPECT_FALSE(expanded::ThornGuardHit(Bite()));
}

TEST_F(ThornGuard, ShotsAndUnguardedHitsAreLeftToTheGame) {
    Guard(true);
    Steps(1);
    EXPECT_FALSE(expanded::ThornGuardHit(Bite(true)));
    Guard(false);
    EXPECT_FALSE(expanded::ThornGuardHit(Bite()));
    EXPECT_TRUE(Planted().empty());
}

TEST_F(ThornGuard, ADeadMonsterTurnsNothing) {
    floor_.unit().monster[kMonster].hp = 0;
    floor_.unit().monster[kMonster].state = -1;
    Guard(true);
    Steps(1);
    EXPECT_FALSE(expanded::ThornGuardHit(Bite()));
}

TEST_F(ThornGuardWithAnotherRing, NoThorns) {
    Guard(true);
    Steps(expanded::kThornFadeFrames);
    EXPECT_FALSE(expanded::ThornArmletUp());
    EXPECT_FALSE(expanded::ThornGuardHit(Bite()));
    EXPECT_TRUE(loaded.empty());
}

TEST_F(ThornGuardBySuperSteve, XiaoCarriesThemToo) {
    Guard(true);
    Steps(expanded::kThornFadeFrames);
    EXPECT_TRUE(expanded::ThornArmletUp());
    // Her thorns are the Pricklys, not the armlet.
    EXPECT_EQ(loaded, std::vector<std::string>{expanded::kPricklyFile});
    EXPECT_TRUE(expanded::ThornGuardHit(Bite()));
    ASSERT_EQ(Planted().size(), 1u);
    EXPECT_EQ(Planted()[0]->owner, CHARA_XIAO);
}

TEST_F(ThornGuard, NothingWithTheWeaponAbilitiesOff) {
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    Guard(true);
    Steps(expanded::kThornFadeFrames);
    EXPECT_FALSE(expanded::ThornArmletUp());
    EXPECT_FALSE(expanded::ThornGuardHit(Bite()));
}
