#include <gtest/gtest.h>

#include <cstring>
#include <memory>

#include "dun/gameloop.hpp"
#include "expanded/enemies/revive.hpp"
#include "expanded/weapons/toan/bone_rapier.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "test_support.hpp"

using expanded::test::Wielding;

namespace {

// The window the bone door's script asks for: the Bone Key alone.
const int kBoneKeyWindow[] = {ITEM_BONE_KEY, -1};

// A floor of one monster of script at slot 0.
struct Floor {
    std::unique_ptr<CMonstorUnit> unit = std::make_unique<CMonstorUnit>();
    CMonstorUnit                 *saved = NowMonstorUnit;

    explicit Floor(const char *script) {
        std::strcpy(unit->model[0].script_name, script);
        unit->monster[0].base_model = 0;
        unit->monster[0].max_hp = 100;
        unit->current_monster = 0;
        NowMonstorUnit = unit.get();
    }

    ~Floor() { NowMonstorUnit = saved; }
};

} // namespace

TEST(BoneRapier, SkeletonKeyAnswersTheBoneDoor) {
    for (int weapon : {ITEM_WEAPON_BONE_RAPIER, ITEM_WEAPON_BONE_SLINGSHOT}) {
        Wielding     wielding(weapon);
        RS_STACKDATA result = {};
        BtEventInfo.item_select_result = 1;
        EXPECT_TRUE(expanded::SkeletonKeyAnswers(kBoneKeyWindow, &result));
        EXPECT_EQ(result.i, ITEM_BONE_KEY);
        EXPECT_EQ(BtEventInfo.item_select_result, 0);
    }
}

TEST(BoneRapier, OtherWindowsAndWeaponsShowTheWindow) {
    RS_STACKDATA result = {};
    {
        Wielding wielding(ITEM_WEAPON_DAGGER);
        EXPECT_FALSE(expanded::SkeletonKeyAnswers(kBoneKeyWindow, &result));
    }

    Wielding  wielding(ITEM_WEAPON_BONE_RAPIER);
    const int other_key[] = {ITEM_BONE_KEY + 1, -1};
    const int with_more[] = {ITEM_BONE_KEY, ITEM_BONE_KEY + 1, -1};
    const int any_item[] = {-1};
    EXPECT_FALSE(expanded::SkeletonKeyAnswers(other_key, &result));
    EXPECT_FALSE(expanded::SkeletonKeyAnswers(with_more, &result));
    EXPECT_FALSE(expanded::SkeletonKeyAnswers(any_item, &result));
    EXPECT_FALSE(expanded::SkeletonKeyAnswers(kBoneKeyWindow, nullptr));
    EXPECT_EQ(result.i, 0);
}

TEST(BoneRapier, SuperSteveSpheres) {
    for (int sphere : {ITEM_WEAPON_BONE_RAPIER, ITEM_WEAPON_BONE_SLINGSHOT}) {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);
        EXPECT_TRUE(expanded::SkeletonKeyActive());
        EXPECT_TRUE(expanded::GravediggerActive());
    }

    Wielding cross_hinder(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_CROSS_HINDER);
    EXPECT_FALSE(expanded::SkeletonKeyActive());
    EXPECT_TRUE(expanded::GravediggerActive());

    Wielding dagger(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER);
    EXPECT_FALSE(expanded::SkeletonKeyActive());
    EXPECT_FALSE(expanded::GravediggerActive());
}

TEST(BoneRapier, GravediggerKeepsTheUndeadDown) {
    Floor mummy("e50a");
    {
        Wielding wielding(ITEM_WEAPON_BONE_RAPIER);
        EXPECT_TRUE(expanded::StaysDown(*mummy.unit, 0));
        EXPECT_EQ(expanded::ReviveCheckDmg(*mummy.unit, MonsterDamageResult::Killed), MonsterDamageResult::Killed);
        EXPECT_EQ(expanded::ReviveRoll(100, 3), 99);
    }

    // Without it the script's own roll stands.
    Wielding dagger(ITEM_WEAPON_DAGGER);
    EXPECT_FALSE(expanded::StaysDown(*mummy.unit, 0));
    EXPECT_EQ(expanded::ReviveCheckDmg(*mummy.unit, MonsterDamageResult::Killed), MonsterDamageResult::Killed);
    EXPECT_EQ(expanded::ReviveRoll(100, 3), 3);
}

TEST(BoneRapier, GravediggerKeepsHarderAiGetUpsDown) {
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set("harder_enemy_ai", true));
    Floor floor("e03a");
    {
        Wielding wielding(ITEM_WEAPON_BONE_RAPIER);
        EXPECT_TRUE(expanded::StaysDown(*floor.unit, 0));
        for (int i = 0; i < 50; i++) {
            EXPECT_EQ(expanded::ReviveCheckDmg(*floor.unit, MonsterDamageResult::Killed), MonsterDamageResult::Killed);
        }
    }

    // Without it some get up.
    Wielding dagger(ITEM_WEAPON_DAGGER);
    EXPECT_FALSE(expanded::StaysDown(*floor.unit, 0));
}
