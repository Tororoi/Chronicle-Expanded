#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
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
#include "expanded/dungeon/ground_shadow.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/weapons/solid_column.hpp"
#include "expanded/weapons/summon_smoke.hpp"
#include "expanded/weapons/ungaga/cactus.hpp"
#include "expanded/weapons/ungaga/cactus/bloom_prop.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::No;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// ---- Absorb ----

// Ungaga (or Xiao) out with weapon_no (carrying sphere when given), his water at 10 of 50, a Gunny in slot 0 and a Skeleton
// Soldier in slot 1, each with 500 life.
class CactusAbsorb : public ::testing::Test {
protected:
    explicit CactusAbsorb(int chara = CHARA_UNGAGA, int weapon_no = ITEM_WEAPON_CACTUS, int sphere = 0) : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        floor_.status().cur_chara = chara_;
        floor_.status().water_now[chara_] = 10.0f;
        floor_.status().water_max[chara_] = 50.0f;
        floor_.SetUpModel(0, Monster::GUNNY);
        floor_.SetUpModel(1, Monster::SKELETON_SOLDIER);
        floor_.Spawn(0, 0);
        floor_.Spawn(1, 1);
        for (int monster = 0; monster < 2; monster++) {
            floor_.unit().monster[monster].hp = 500;
        }
    }

    // A hit of owner's taking damage off monster, as CheckDmg takes it.
    void Hit(int monster, int damage, int owner) {
        COLLISION_HIT hit = {};
        hit.owner = owner;
        floor_.unit().monster[monster].hp -= damage;
        expanded::MonsterDamaged(floor_.unit(), monster, hit, damage);
    }

    float Water() { return floor_.status().water_now[chara_]; }

    TestFloor floor_;
    Wielding  wielding_;
    int       chara_;
};

class CactusAbsorbBySuperSteve : public CactusAbsorb {
protected:
    CactusAbsorbBySuperSteve() : CactusAbsorb(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_CACTUS) {}
};

// ---- Desert Bloom ----

bool r1_held = false;

bool HeldR1() {
    return r1_held;
}

// The forms' characters, stood in for: a root with the Cactus's grip (a half-turn about y) and a child.
struct Rig {
    CFrameVu1 frames[2];
};

Rig                      cactus_rig, trees_rig, disc_rig;
std::vector<std::string> loaded; // the packs loaded, in order

bool Loaded(const std::string &path) {
    return std::count(loaded.begin(), loaded.end(), path) == 1;
}

bool FakeProp(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *) {
    chara.Initialize();
    loaded.emplace_back(path);
    std::string name = path;
    Rig        &rig = name == expanded::kQueensTreesFile ? trees_rig : name == expanded::kShadowDiscFile ? disc_rig
                                                                                                         : cactus_rig;
    for (CFrameVu1 &frame : rig.frames) {
        frame.Initialize();
    }
    std::strncpy(rig.frames[0].name, "root", sizeof(rig.frames[0].name) - 1);
    if (&rig == &cactus_rig) {
        rig.frames[0].local[0][0] = -1.0f;
        rig.frames[0].local[2][2] = -1.0f;
    }
    rig.frames[1].SetParent(&rig.frames[0]);
    chara.frame = &rig.frames[0];
    if (&rig == &disc_rig) {
        chara.shadow_frame = &rig.frames[1];
    }
    return true;
}

// The smoke, stood in for: one slot with its clip 1-50.
CFrameVu1   smoke_frames[2];
MOTION_INFO smoke_keys[1] = {
    {1, 50, 0.2f, 0}
};

bool FakeSmoke(CSHOT_EFFECT &effect, CDataAlloc2<1> &, BT_SHOT_EFFECT *description, int shots, const char *) {
    effect.Initialize();
    effect.effect_data = description;
    effect.slot_count = shots;
    for (CFrameVu1 &frame : smoke_frames) {
        frame.Initialize();
    }
    smoke_frames[1].SetParent(&smoke_frames[0]);
    smoke_frames[1].attr.use_color = 1;
    for (float &channel : smoke_frames[1].attr.color) {
        channel = 128.0f;
    }
    effect.chara[0].Initialize();
    effect.chara[0].frame = &smoke_frames[0];
    effect.chara[0].motion_type.motion_info = smoke_keys;
    return true;
}

// Ungaga out with weapon_no (attack 120; carrying sphere when given), standing at the origin facing +z on a floor of no
// polygons (so the cactus stands at his height), guarding; the forms' characters and the smoke stood in for.
class DesertBloom : public ::testing::Test {
protected:
    explicit DesertBloom(int chara = CHARA_UNGAGA, int weapon_no = ITEM_WEAPON_CACTUS, int sphere = 0) : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = chara_;
        wielding_.weapon().attack = 120;
        saved_event_ = BtEventMode;
        BtEventMode = 0;
        saved_map_ = NowDngMap;
        NowDngMap = nullptr;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        CharaMain.GetRotation(saved_rot_);
        CharaMain.SetRotation(0.0f, 0.0f, 0.0f);
        saved_data_ = NowColData;
        NowColData = data_.get();
        loaded.clear();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakeProp);
        expanded::SetShotEffectLoader(FakeSmoke);
        expanded::ShotEffectsClear();
        expanded::EnterShotEffect(expanded::SummonSmokeEffect(), expanded::kSmokeShots, expanded::kSummonSmokeFile);
        expanded::SetDesertBloomGuardButton(HeldR1);
        expanded::GroundShadowReset();
        expanded::DesertBloomReset();
        r1_held = false;
    }

    void TearDown() override {
        expanded::DesertBloomReset();
        expanded::SetDesertBloomGuardButton(nullptr);
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        expanded::SetFloorCharacterLoader(nullptr);
        expanded::FloorCharactersClear();
        NowColData = saved_data_;
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
        NowDngMap = saved_map_;
        BtEventMode = saved_event_;
        BtActStatus = {};
        r1_held = false;
    }

    void Guard(bool on) {
        r1_held = on;
        BtActStatus.guard_mode = on ? 5 : 0;
    }

    void Steps(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::DesertBloomStep();
        }
    }

    // Held long enough to raise one, then let go.
    void Raise() {
        Guard(true);
        Steps(expanded::kBloomGuardFrames + 1);
        Guard(false);
    }

    // Raised and steps more frames, ending at the frame given from its raising.
    void RaiseTo(int age) {
        Raise();
        Steps(age);
    }

    // monster standing at (x, 0, z), in play, with a body sphere of radius 3 at height 5.
    void Stand(int monster, float x, float z) {
        MONSTOR &live = floor_.unit().monster[monster];
        live.state = 2;
        live.revealed = -1;
        live.hp = 100;
        live.max_hp = 100;
        live.collision_radius = 4.0f;
        CCharacter &body = floor_.unit().chara[monster][0];
        body.pos[0] = x;
        body.pos[2] = z;
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[monster];
        effect.position[0][0] = x;
        effect.position[0][1] = 5.0f;
        effect.position[0][2] = z;
        effect.position[0][3] = 1.0f;
        effect.radius[0] = 3.0f;
        effect.timer[0] = 1;
    }

    // The planted hits, in order.
    std::vector<COLLISION_HIT *> Planted() {
        std::vector<COLLISION_HIT *> hits;
        for (int i = 0; i < 96; i++) {
            if (NowColData->active[i] != 0) {
                hits.push_back(&NowColData->hit[i]);
            }
        }
        return hits;
    }

    void ClearHits() {
        for (int &active : NowColData->active) {
            active = 0;
        }
    }

    float RootHeight() {
        float at[4];
        expanded::cactus::PropCharacter()->GetPosition(at);
        return at[1];
    }

    CSHOT_EFFECT &Smoke() { return *expanded::ShotEffectAt(expanded::ShotEffectIndex(expanded::SummonSmokeEffect())); }

    TestFloor                       floor_;
    Wielding                        wielding_;
    int                             chara_;
    std::unique_ptr<CCollisionData> data_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_data_ = nullptr;
    CDungeonMap                    *saved_map_ = nullptr;
    float                           saved_pos_[3] = {};
    float                           saved_rot_[4] = {};
    s32                             saved_event_ = 0;
};

class DesertBloomBySuperSteve : public DesertBloom {
protected:
    DesertBloomBySuperSteve() : DesertBloom(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_CACTUS) {}
};

class DesertBloomWithTheHalberd : public DesertBloom {
protected:
    DesertBloomWithTheHalberd() : DesertBloom(CHARA_UNGAGA, ITEM_WEAPON_HALBERT) {}
};

constexpr float kSpawnScale = 3.0f * expanded::kBloomSpawnShare;

} // namespace

// ---- Absorb ----

TEST_F(CactusAbsorb, HitsRestoreThirstByATenthOfTheDamage) {
    Hit(0, 100, CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(Water(), 20.0f);
    Hit(0, 37, CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(Water(), 23.7f);
}

TEST_F(CactusAbsorb, OnlyTheLifeTakenCounts) {
    floor_.unit().monster[0].hp = 30;
    Hit(0, 100, CHARA_UNGAGA); // a kill: 30 taken
    EXPECT_FLOAT_EQ(Water(), 13.0f);
    Hit(0, 0, CHARA_UNGAGA); // an immune hit
    EXPECT_FLOAT_EQ(Water(), 13.0f);
}

TEST_F(CactusAbsorb, UpToTheGaugesMost) {
    Hit(0, 450, CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(Water(), 50.0f);
    floor_.status().water_now[CHARA_UNGAGA] = 50.0f;
    Hit(0, 10, CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(Water(), 50.0f);
}

TEST_F(CactusAbsorb, DryKindsGiveNothing) {
    Hit(1, 100, CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(Water(), 10.0f);
    for (Monster dry : {Monster::MASTER_JACKET, Monster::STATUE, Monster::GOLEM, Monster::STEEL_GIANT_ENHANCED, Monster::GOL_ENHANCED,
                        Monster::HORN_HEAD, Monster::SILVER_GEAR, Monster::LIVING_ARMOR_ENHANCED, Monster::STATUE_DOG_ENHANCED}) {
        EXPECT_TRUE(expanded::CactusDryKind(No(dry)));
    }
    for (Monster wet : {Monster::GUNNY, Monster::WEREWOLF, Monster::HALLOWEEN, Monster::CAVE_BAT}) {
        EXPECT_EQ(expanded::CactusDryKind(No(wet)), false) << No(wet);
    }
}

TEST_F(CactusAbsorb, OnlyTheWieldersHits) {
    Hit(0, 100, -1);         // an item's
    Hit(0, 100, CHARA_XIAO); // not the wielder's
    EXPECT_FLOAT_EQ(Water(), 10.0f);
    Wielding halberd(ITEM_WEAPON_HALBERT);
    Hit(0, 100, CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(Water(), 10.0f);
}

TEST_F(CactusAbsorbBySuperSteve, PelletHitsWaterXiao) {
    Hit(0, 100, CHARA_XIAO);
    EXPECT_FLOAT_EQ(Water(), 20.0f);
    Hit(1, 100, CHARA_XIAO);
    EXPECT_FLOAT_EQ(Water(), 20.0f);
    Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HALBERT);
    Hit(0, 100, CHARA_XIAO);
    EXPECT_FLOAT_EQ(Water(), 20.0f);
}

// ---- Desert Bloom ----

TEST_F(DesertBloom, HoldingTheGuardAQuarterSecondRaisesTheCactusAhead) {
    Guard(true);
    Steps(expanded::kBloomGuardFrames); // the first frame in the pose starts the clock
    EXPECT_FALSE(expanded::DesertBloomUp());
    Steps(1);
    ASSERT_TRUE(expanded::DesertBloomUp());
    EXPECT_TRUE(Loaded("commenu/weapon/c10w13.chr"));
    float spot[3];
    expanded::DesertBloomSpot(spot);
    EXPECT_NEAR(spot[0], 0.0f, 1e-4f);
    EXPECT_FLOAT_EQ(spot[1], 0.0f); // no floor under it: his
    EXPECT_NEAR(spot[2], expanded::kBloomAhead, 1e-4f);
    // Point-up: the grip turned a quarter about x, so the blade's axis (the model's +z) stands straight up.
    const CFrame *root = expanded::cactus::PropCharacter()->frame;
    EXPECT_FLOAT_EQ(root->local[2][0], 0.0f);
    EXPECT_FLOAT_EQ(root->local[2][1], 1.0f);
    EXPECT_FLOAT_EQ(root->local[2][2], 0.0f);
    EXPECT_FLOAT_EQ(root->local[1][2], 1.0f);
    // It rises in the smoke, at its rate.
    CSHOT_EFFECT &smoke = Smoke();
    ASSERT_NE(smoke.active[0], 0);
    EXPECT_FLOAT_EQ(smoke.chara[0].motion_speed, expanded::kSmokeRate);
    EXPECT_FLOAT_EQ(smoke.chara[0].scale[0], expanded::kBloomSmokeScale);
    // Held on, no other rises.
    Steps(60);
    EXPECT_EQ(expanded::DesertBloomAge(), 60);
}

TEST_F(DesertBloom, AGuardBrokenOffStartsAgain) {
    Guard(true);
    Steps(10);
    Guard(false);
    Steps(1);
    Guard(true);
    Steps(expanded::kBloomGuardFrames);
    EXPECT_FALSE(expanded::DesertBloomUp());
    Steps(1);
    EXPECT_TRUE(expanded::DesertBloomUp());
}

TEST_F(DesertBloom, ItEmergesGrowsAndSettlesOnTheSmokesFrames) {
    Raise();
    EXPECT_FLOAT_EQ(expanded::DesertBloomScale(), kSpawnScale);
    float buried = -11.7f * kSpawnScale - expanded::kBloomBuried;
    EXPECT_NEAR(RootHeight(), buried, 1e-4f);
    // Out at its small size by frame 9: an ease from below, its top over the floor.
    Steps(9);
    EXPECT_FLOAT_EQ(expanded::DesertBloomScale(), kSpawnScale);
    EXPECT_GT(RootHeight(), -11.7f * kSpawnScale + 6.8f * kSpawnScale - 0.1f);
    // Growing evenly to its peak by frame 16, then settling to its size by 19.
    Steps(1);
    EXPECT_NEAR(expanded::DesertBloomScale(), kSpawnScale + (3.4f - kSpawnScale) * (10.0f - 55.0f / 6.0f) / (16.0f - 55.0f / 6.0f), 1e-4f);
    Steps(6);
    EXPECT_NEAR(expanded::DesertBloomScale(), 3.4f, 1e-5f);
    EXPECT_FALSE(expanded::SolidColumnArmed());
    Steps(1);
    EXPECT_NEAR(expanded::DesertBloomScale(), 3.4f - 0.4f * 3.0f / 7.0f, 1e-4f);
    Steps(2);
    EXPECT_FLOAT_EQ(expanded::DesertBloomScale(), 3.0f);
    EXPECT_NEAR(RootHeight(), (6.8f - 11.7f) * 3.0f, 1e-4f);
    EXPECT_TRUE(expanded::SolidColumnArmed());
    EXPECT_TRUE(expanded::GroundShadowShown());
}

TEST_F(DesertBloom, ItStandsTenSecondsFadesAndGoes) {
    RaiseTo(expanded::kBloomStandFrames - 1);
    EXPECT_FLOAT_EQ(expanded::DesertBloomAlpha(), 1.0f);
    EXPECT_TRUE(expanded::GroundShadowShown());
    Steps(16);
    EXPECT_NEAR(expanded::DesertBloomAlpha(), 0.5f, 1e-5f);
    EXPECT_FALSE(expanded::GroundShadowShown());
    EXPECT_TRUE(expanded::SolidColumnArmed());
    Steps(14);
    EXPECT_TRUE(expanded::DesertBloomUp());
    Steps(1);
    EXPECT_FALSE(expanded::DesertBloomUp());
    EXPECT_FALSE(expanded::SolidColumnArmed());
}

TEST_F(DesertBloom, ItPricksTheMonstersAtItsColumn) {
    Stand(0, 0.0f, 17.0f);  // its body 4 from the cactus's axis
    Stand(1, 40.0f, 10.0f); // far
    Stand(2, 0.0f, 21.5f);  // its body 8.5 from it: out of reach
    RaiseTo(18);
    EXPECT_TRUE(Planted().empty());
    Steps(1); // risen
    auto hits = Planted();
    ASSERT_EQ(hits.size(), 1u);
    COLLISION_HIT &hit = *hits[0];
    EXPECT_EQ(hit.damage, 20);
    EXPECT_EQ(hit.owner, CHARA_UNGAGA);
    EXPECT_EQ(hit.target_mask, 2);
    EXPECT_FLOAT_EQ(hit.radius, expanded::kBloomHitRadius);
    EXPECT_FLOAT_EQ(hit.pos[2], 17.0f);
    EXPECT_EQ(hit.knockback_mode, 2);
    EXPECT_FLOAT_EQ(hit.knockback_speed, expanded::kBloomKick.speed);
    EXPECT_FLOAT_EQ(hit.knockback_origin[2], expanded::kBloomAhead);
    EXPECT_TRUE(expanded::DesertBloomPrick(hit));
    EXPECT_FALSE(expanded::HitWearsWeapon(hit));
    // Every quarter second.
    ClearHits();
    Steps(expanded::kBloomHitFrames - 1);
    EXPECT_TRUE(Planted().empty());
    Steps(1);
    EXPECT_EQ(Planted().size(), 1u);
}

TEST_F(DesertBloom, ItsColumnStopsMonstersThePlayerAndShots) {
    RaiseTo(19);
    ASSERT_TRUE(expanded::SolidColumnArmed());
    // A monster walking straight at it slides round it; one walking away goes on.
    Stand(0, 0.0f, 21.0f);
    MONSTOR &live = floor_.unit().monster[0];
    live.movement[0] = 0.0f, live.movement[2] = -1.0f, live.movement_speed = 1.5f;
    expanded::MonsterMoveChecked(floor_.unit(), 0);
    EXPECT_NEAR(std::fabs(live.movement[0]), 1.0f, 1e-5f);
    EXPECT_NEAR(live.movement[2], 0.0f, 1e-5f);
    live.movement[0] = 0.0f, live.movement[2] = 1.0f;
    expanded::MonsterMoveChecked(floor_.unit(), 0);
    EXPECT_FLOAT_EQ(live.movement[2], 1.0f);
    // The player walking at it slides along it.
    float far_at[3] = {3.0f, 0.0f, -10.0f}, far_step[3] = {0.0f, 0.0f, 1.0f};
    expanded::PlayerMoveChecked(far_at, far_step);
    EXPECT_FLOAT_EQ(far_step[2], 1.0f);
    float near_at[3] = {3.0f, 0.0f, -1.5f}, step[3] = {0.0f, 0.0f, 4.5f};
    expanded::PlayerMoveChecked(near_at, step);
    EXPECT_LT(step[2], 4.5f);
    EXPECT_NEAR(step[0] * (0.0f - near_at[0]) + step[2] * (expanded::kBloomAhead - near_at[2]), 0.0f, 1e-4f); // nothing into it
    // A monster's shot stops at it; the player's go through.
    BT_SHOT_EFFECT monsters_shot = {
        "f_boll_3", 0, 0, {0.0f, 1.5f, 0.0f, 0.0f},
           {0.0f, 4.0f, 0.0f, 0.0f},
           100, 10, 0, 3, 1, {-1, 0, -1, -1},
           0, {},
           0.0f, 0, {-1, -1, -1, -1}
    };
    BT_SHOT_EFFECT players_shot = monsters_shot;
    players_shot.target = 2;
    auto shots = std::make_unique<CSHOT_EFFECT>();
    shots->Initialize();
    shots->effect_data = &monsters_shot;
    shots->phase[0] = 1;
    shots->velocity[0][2] = 1.5f;
    float position[4] = {0.0f, 5.0f, 2.0f, 1.0f}, hit_position[4] = {};
    EXPECT_EQ(expanded::SolidColumnShot(*shots, 0, SHOT_COLLISION_NONE, hit_position, position), SHOT_COLLISION_MAP);
    EXPECT_FLOAT_EQ(hit_position[2], 2.0f);
    float over[4] = {0.0f, 25.0f, 2.0f, 1.0f};
    EXPECT_EQ(expanded::SolidColumnShot(*shots, 0, SHOT_COLLISION_NONE, hit_position, over), SHOT_COLLISION_NONE);
    shots->effect_data = &players_shot;
    EXPECT_EQ(expanded::SolidColumnShot(*shots, 0, SHOT_COLLISION_NONE, hit_position, position), SHOT_COLLISION_NONE);
}

TEST_F(DesertBloom, HoldingAgainMovesIt) {
    RaiseTo(100);
    CharaMain.pos[0] = 50.0f;
    CharaMain.SetRotation(0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f);
    Raise();
    ASSERT_TRUE(expanded::DesertBloomUp());
    EXPECT_EQ(expanded::DesertBloomAge(), 0);
    float spot[3];
    expanded::DesertBloomSpot(spot);
    EXPECT_NEAR(spot[0], 60.0f, 1e-4f);
    EXPECT_NEAR(spot[2], 0.0f, 1e-4f);
    EXPECT_FALSE(expanded::SolidColumnArmed()); // the new one rises first
    EXPECT_TRUE(Loaded("commenu/weapon/c10w13.chr"));
}

TEST_F(DesertBloom, ItGoesWithTheWeaponOrTheWielder) {
    RaiseTo(40);
    floor_.status().cur_chara = CHARA_XIAO;
    Steps(1);
    EXPECT_FALSE(expanded::DesertBloomUp());
    EXPECT_FALSE(expanded::SolidColumnArmed());
    floor_.status().cur_chara = CHARA_UNGAGA;
    RaiseTo(40);
    Wielding halberd(ITEM_WEAPON_HALBERT);
    Steps(1);
    EXPECT_FALSE(expanded::DesertBloomUp());
}

TEST_F(DesertBloomWithTheHalberd, NothingRises) {
    Raise();
    EXPECT_FALSE(expanded::DesertBloomUp());
    EXPECT_TRUE(loaded.empty());
}

TEST_F(DesertBloomBySuperSteve, QueensTreesRiseAsAWallThatHurtsNothing) {
    Stand(0, 0.0f, 17.0f);
    RaiseTo(19);
    ASSERT_TRUE(expanded::DesertBloomUp());
    EXPECT_TRUE(Loaded(expanded::kQueensTreesFile));
    EXPECT_FLOAT_EQ(expanded::DesertBloomScale(), 0.7f);
    // Turned a quarter from her facing, centred on the spot by their grass square's centre, standing on the floor.
    float at[4], rotation[4];
    expanded::cactus::PropCharacter()->GetPosition(at);
    expanded::cactus::PropCharacter()->GetRotation(rotation);
    EXPECT_NEAR(rotation[1], std::numbers::pi_v<float> / 2.0f, 1e-5f);
    EXPECT_NEAR(at[0], 0.7f, 1e-4f);
    EXPECT_NEAR(at[1], 0.0f, 1e-4f);
    EXPECT_NEAR(at[2], 10.0f - 6.0f * 0.7f, 1e-4f);
    // Not point-up.
    EXPECT_FLOAT_EQ(expanded::cactus::PropCharacter()->frame->local[1][1], 1.0f);
    EXPECT_TRUE(expanded::SolidColumnArmed());
    EXPECT_FALSE(expanded::GroundShadowShown());
    Steps(60);
    EXPECT_TRUE(Planted().empty());
}

TEST_F(DesertBloom, TheSmokeIsHeldAndFadesFromItsFrame32) {
    Raise();
    CSHOT_EFFECT &smoke = Smoke();
    CCharacter   &puff = smoke.chara[0];
    EXPECT_FLOAT_EQ(smoke_frames[1].attr.color[0], 128.0f);
    puff.SetPosition(5.0f, 5.0f, 5.0f);
    puff.motion_type.state.time = 41.0f; // half way from 32 to 50
    Steps(1);
    float at[4];
    puff.GetPosition(at);
    EXPECT_NEAR(at[2], expanded::kBloomAhead, 1e-4f);
    EXPECT_NEAR(smoke_frames[1].attr.color[0], 64.0f, 1e-3f);
    // Moving it puts the old one out; the new one starts at full.
    Raise();
    EXPECT_FLOAT_EQ(smoke_frames[1].attr.color[0], 128.0f);
    EXPECT_NE(smoke.active[0], 0);
}

// The smoke, the column and the shadow are one at a time and shared: the Cactus puts out only its own.
TEST_F(DesertBloom, ItPutsOutOnlyItsOwnSmokeColumnAndShadow) {
    const int   other = 0;
    const float spot[3] = {50.0f, 0.0f, 50.0f};
    ASSERT_TRUE(expanded::SummonSmokeStart(spot, 0.0f, 1.0f, &other));
    expanded::SolidColumnArm(spot, 5.0f, 10.0f, &other);
    expanded::GroundShadowShow(spot[0], spot[1], spot[2], 5.0f, &other);
    expanded::DesertBloomReset();
    EXPECT_TRUE(expanded::SummonSmokePlaying());
    EXPECT_TRUE(expanded::SolidColumnArmed());
    EXPECT_TRUE(expanded::GroundShadowShown());
    expanded::SummonSmokeEnd(&other);
    expanded::SolidColumnDisarm(&other);
    expanded::GroundShadowHide(&other);
    EXPECT_FALSE(expanded::SummonSmokePlaying());
    EXPECT_FALSE(expanded::SolidColumnArmed());
    EXPECT_FALSE(expanded::GroundShadowShown());
}
