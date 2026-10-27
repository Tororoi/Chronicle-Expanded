#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>

#include "bt_shot_effect.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/status/confuse.hpp"
#include "hitvalue.hpp"
#include "itembombeffect.hpp"
#include "monstorunit.hpp"
#include "shot_effect_pack.hpp"
#include "test_support.hpp"

namespace dc::game {
extern CHitValue HitValue[32];
} // namespace dc::game

namespace {

using expanded::Monster;

constexpr int kAttacker = 0;
constexpr int kVictim = 1;

// Two Master Jackets in play, the first confused, and the tables a hit between them lands in.
class ConfusedHit : public ::testing::Test {
protected:
    void SetUp() override {
        saved_hits_ = NowColData;
        saved_values_ = NowHitValue;
        saved_pack_ = NowShotEffect;
        saved_bombs_ = NowBombEffect;
        NowColData = hits_.get();
        NowHitValue = HitValue;
        NowShotEffect = pack_.get();
        NowBombEffect = bombs_;
        for (CSHOT_EFFECT &effect : pack_->effect) {
            effect.Initialize();
        }
        for (CItemBombEffect &bomb : bombs_) {
            bomb.Initialize();
        }
        floor_.SetUpModel(0, Monster::MASTER_JACKET);
        for (int i = 0; i < 12; i++) {
            motions_[i] = {i * 20, i * 20 + 10, 0.5f, 0};
        }
        for (int monster : {kAttacker, kVictim}) {
            floor_.Spawn(monster, 0);
            MONSTOR &live = unit().monster[monster];
            live.state = 2;
            live.revealed = -1;
            unit().chara[monster][0].motion_type.motion_info = motions_;
        }
        unit().monster[kVictim].hp = 1000;
        unit().monster[kVictim].defense = 0;
        unit().monster[kVictim].item_damage_rate = 100;
        unit().effect[kVictim].position[0][0] = 50.0f;
        unit().effect[kVictim].radius[0] = 5.0f;
        Confuse(kAttacker);
    }

    void TearDown() override {
        expanded::BlindEnd();
        expanded::ConfuseFloorReset(false);
        NowColData = saved_hits_;
        NowHitValue = saved_values_;
        NowShotEffect = saved_pack_;
        NowBombEffect = saved_bombs_;
    }

    CMonstorUnit &unit() { return floor_.unit(); }

    // monster confused: rolled until its 5% comes up, the game's numbers seeded so it is the same each run.
    void Confuse(int monster) {
        expanded::ConfuseStep(unit(), monster);
        unit().effect[monster].hit_attributes |= expanded::kAbilityConfuse;
        std::srand(1);
        for (int i = 0; i < 500; i++) {
            expanded::ConfuseRoll(unit(), monster);
        }
    }

    // The victim in its guard window.
    void Guarding() {
        unit().guard[kVictim].active[0] = 1;
        unit().guard[kVictim].motion_start[0] = 0;
        unit().guard[kVictim].motion_end[0] = 100;
        unit().chara[kVictim][0].motion_type.state.time = 10.0f;
    }

    // The attacker's swing (or its shot, monster_no) reaching the victim's sphere.
    int Swing(int damage, int monster_no = -1) {
        int            index = 0;
        COLLISION_HIT &hit = NowColData->hit[index];
        hit = {};
        hit.pos[0] = 50.0f;
        hit.radius = 5.0f;
        hit.damage = damage;
        hit.owner = 200 + 5 * kAttacker;
        hit.monster_no = monster_no;
        NowColData->active[index] = 1;
        return index;
    }

    std::optional<MonsterDamageResult> Hit() {
        unit().current_monster = kVictim;
        return expanded::ConfuseMonsterHit(unit(), 0);
    }

    expanded::test::TestFloor          floor_;
    std::unique_ptr<CCollisionData>    hits_ = std::make_unique<CCollisionData>();
    std::unique_ptr<CSHOT_EFFECT_PACK> pack_ = std::make_unique<CSHOT_EFFECT_PACK>();
    CItemBombEffect                    bombs_[3];
    MOTION_INFO                        motions_[12] = {};
    CCollisionData                    *saved_hits_ = nullptr;
    CHitValue                         *saved_values_ = nullptr;
    CSHOT_EFFECT_PACK                 *saved_pack_ = nullptr;
    CItemBombEffect                   *saved_bombs_ = nullptr;
};

int OwnersBlast(int) {
    return 77;
}

} // namespace

TEST_F(ConfusedHit, AGuardBlocksIt) {
    Guarding();
    Swing(30);
    EXPECT_EQ(Hit(), MonsterDamageResult::None);
    EXPECT_EQ(unit().monster[kVictim].hp, 1000);
}

// Blinded, the victim's guard blocks nothing, a confused monster's swing as much as the player's.
TEST_F(ConfusedHit, ABlindedMonstersGuardDoesNot) {
    Guarding();
    expanded::Blind(unit(), 100);
    ASSERT_TRUE(expanded::Blinded(kVictim));
    Swing(30);
    EXPECT_EQ(Hit(), MonsterDamageResult::Hit);
    EXPECT_EQ(unit().monster[kVictim].hp, 970);
}

// A confused monster's shot that bursts on another bursts as it would on the player: its owner's blast damage.
TEST_F(ConfusedHit, AShotBurstsWithItsOwnersBlast) {
    BT_SHOT_EFFECT description = {};
    description.motion[2] = -1;
    description.end_effect = SHOT_END_EFFECT_BOMB;
    description.bomb_damage = 10;
    description.bomb_scale = 1.0f;
    expanded::SetBlastDamage(&description, OwnersBlast);
    CSHOT_EFFECT &effect = NowShotEffect->effect[0];
    effect.effect_data = &description;
    effect.slot_count = 1;
    effect.active[0] = 1;
    effect.phase[0] = 1;
    effect.user_id_2[0] = kAttacker;
    effect.chara[0].SetPosition(50.0f, 0.0f, 0.0f);
    int shot = Swing(30, kAttacker);
    expanded::ConfuseLanded(unit(), shot, kVictim);
    expanded::SetBlastDamage(&description, nullptr);
    EXPECT_EQ(effect.active[0], 0);
    EXPECT_EQ(NowColData->hit[NowColData->now_hit].damage, 77);
}
