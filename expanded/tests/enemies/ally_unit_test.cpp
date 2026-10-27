#include <gtest/gtest.h>

#include <cstring>
#include <memory>

#include "dun/gameloop.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monsters.hpp"
#include "monstorunit.hpp"
#include "shot_effect_pack.hpp"
#include "test_support.hpp"
#include "texture.hpp"
#include "textureanime.hpp"

namespace {

using expanded::kAllyMonster;
using expanded::Monster;
using expanded::No;
using expanded::test::TestFloor;

constexpr int kMonsterBlock = 0x2A;

int floor_calls = 0;

void CountFloor() {
    floor_calls++;
}

[[maybe_unused]] const bool kListening = (expanded::AddEnemyFloorListener(CountFloor), true);

int texture_entry = -1, anim_record = -1;

// A kind's load, stood in for: an image entered in the monsters' texture block, a texture animation record and the shot
// pack's fourth slot taken, and the kind set up and stood at position as its monster.
bool FakeLoad(CMonstorUnit &unit, int kind, const float *position) {
    for (int i = 1; i < 196; i++) {
        if (TexManager.textures[i].name[0] == 0) {
            texture_entry = i;
            break;
        }
    }
    CTexture &texture = TexManager.textures[texture_entry];
    std::strcpy(texture.name, "ally_test");
    texture.block = kMonsterBlock;
    texture.tex0 = 0x3FFF; // a key the renderer never handed out
    TexManager.blocks[kMonsterBlock].vram_end += 64;
    for (int i = 0; i < 320; i++) {
        if (MonsterTexAnim[i].kind == -1) {
            anim_record = i;
            break;
        }
    }
    MonsterTexAnim[anim_record].kind = 0;
    NowShotEffect->effect[3].effect_data = BtEntryEffectTbl[19];
    // Its set-up as SetupBaseModel calls it: the floor's listeners are not told.
    expanded::EnemyModelSetUp(unit, 0, kind);
    unit.model[0] = MonstorTable[kind];
    unit.monster[kAllyMonster].state = 2;
    unit.monster[kAllyMonster].hp = 60;
    unit.chara[kAllyMonster][0].SetPosition(position[0], position[1], position[2]);
    return true;
}

class AllyUnit : public ::testing::Test {
protected:
    void SetUp() override {
        saved_pack_ = NowShotEffect;
        NowShotEffect = pack_.get();
        for (CSHOT_EFFECT &effect : pack_->effect) {
            effect.Initialize();
        }
        saved_end_ = TexManager.blocks[kMonsterBlock].vram_end;
        floor_calls = 0;
        expanded::SetAllyLoader(FakeLoad);
    }

    void TearDown() override {
        expanded::AllyDismiss();
        expanded::SetAllyLoader(nullptr);
        if (texture_entry >= 0 && std::strcmp(TexManager.textures[texture_entry].name, "floor_test") == 0) {
            TexManager.textures[texture_entry].Initialize();
        }
        if (anim_record >= 0) {
            MonsterTexAnim[anim_record].Initialize();
        }
        TexManager.blocks[kMonsterBlock].vram_end = saved_end_;
        NowShotEffect = saved_pack_;
    }

    bool Summon() {
        float at[3] = {10.0f, 0.0f, 20.0f};
        return expanded::AllySummon(No(Monster::SUNDAY), at);
    }

    TestFloor                          floor_;
    std::unique_ptr<CSHOT_EFFECT_PACK> pack_ = std::make_unique<CSHOT_EFFECT_PACK>();
    CSHOT_EFFECT_PACK                 *saved_pack_ = nullptr;
    int                                saved_end_ = 0;
};

TEST_F(AllyUnit, StandsBesideTheFloorsUnit) {
    ASSERT_TRUE(Summon());
    CMonstorUnit *ally = expanded::AllyStanding();
    ASSERT_NE(ally, nullptr);
    EXPECT_NE(ally, &floor_.unit());
    EXPECT_EQ(expanded::AllyFloor(), &floor_.unit());
    EXPECT_TRUE(expanded::AllyUnit(*ally));
    EXPECT_FALSE(expanded::AllyUnit(floor_.unit()));
    EXPECT_FALSE(expanded::AllyStepping());
    // No floor kind, and none of the floor's listeners told of it.
    EXPECT_EQ(ally->monster[kAllyMonster].base_model, -1);
    EXPECT_EQ(floor_calls, 0);
    float at[4];
    ally->chara[kAllyMonster][0].GetPosition(at);
    EXPECT_FLOAT_EQ(at[2], 20.0f);
}

TEST_F(AllyUnit, WhatItsLoadAddedGoesBackAsItGoes) {
    ASSERT_TRUE(Summon());
    ASSERT_EQ(std::strcmp(TexManager.textures[texture_entry].name, "ally_test"), 0);
    expanded::AllyDismiss();
    EXPECT_EQ(expanded::AllyStanding(), nullptr);
    EXPECT_EQ(TexManager.textures[texture_entry].name[0], 0);
    EXPECT_EQ(MonsterTexAnim[anim_record].kind, -1);
    EXPECT_EQ(NowShotEffect->effect[3].effect_data, nullptr);
    EXPECT_EQ(TexManager.blocks[kMonsterBlock].vram_end, saved_end_);
}

TEST_F(AllyUnit, WhatTheFloorPutInItsPlaceStays) {
    ASSERT_TRUE(Summon());
    // The floor's reload: its own texture where the ally's was, its own shot in the slot, its block grown.
    CTexture &texture = TexManager.textures[texture_entry];
    texture.Initialize();
    std::strcpy(texture.name, "floor_test");
    texture.block = kMonsterBlock;
    texture.tex0 = 0x3FFE;
    NowShotEffect->effect[3].effect_data = BtEntryEffectTbl[4];
    TexManager.blocks[kMonsterBlock].vram_end += 32;
    int grown = TexManager.blocks[kMonsterBlock].vram_end;
    expanded::AllyDismiss();
    EXPECT_EQ(std::strcmp(texture.name, "floor_test"), 0);
    EXPECT_EQ(NowShotEffect->effect[3].effect_data, BtEntryEffectTbl[4]);
    EXPECT_EQ(TexManager.blocks[kMonsterBlock].vram_end, grown);
}

TEST_F(AllyUnit, ItsShotsCarryItsMark) {
    ASSERT_TRUE(Summon());
    CMonstorUnit *saved = NowMonstorUnit;
    NowMonstorUnit = expanded::AllyStanding();
    NowShotEffect->current_effect = 0;
    NowShotEffect->effect[0].current_slot = 2;
    expanded::AllyShotFired();
    NowMonstorUnit = saved;
    EXPECT_EQ(NowShotEffect->effect[0].user_id_2[2], expanded::kAllyShooter);
}

TEST_F(AllyUnit, ForgettingItLeavesTheTablesAlone) {
    ASSERT_TRUE(Summon());
    expanded::AllyForget();
    EXPECT_EQ(expanded::AllyStanding(), nullptr);
    EXPECT_EQ(std::strcmp(TexManager.textures[texture_entry].name, "ally_test"), 0);
    TexManager.textures[texture_entry].Initialize();
    NowShotEffect->effect[3].Initialize();
}

} // namespace
