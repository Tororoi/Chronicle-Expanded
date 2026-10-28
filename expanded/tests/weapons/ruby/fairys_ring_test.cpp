#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "clsmes.hpp"
#include "editloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/weapons/ruby/fairys_ring.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"
#include "shop.hpp"
#include "stockitem.hpp"
#include "test_support.hpp"
#include "text/gametext.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::OptionScope;
using expanded::test::TestSave;

constexpr int kFishKinds = 18;
constexpr int kIconSword = -0x2EE;
constexpr int kIconOrb = -0x2ED;

// Norune's catch message (e01talk, English), and as a catch Fairy Fortune paid shows it.
constexpr std::string_view kCatch =
    "{insert1} ({value1}cm)\ncaught!!{page}Fishing Points +{value2}\nTotal Points   {value3}\nRecord {insert1} {value1}cm";
constexpr std::string_view kFortuneCatch = "{insert1} ({value1}cm)\ncaught!!{page}Fishing Points {highlight}+{value2}{/color} "
                                           "{icon-sword}\nTotal Points   {value3}\nRecord {insert1} {value1}cm";

// A fish on the line (AngleFish) 15 long, between its kind's 10 and 20, so the game pays 150 of its 100 to 200 FP.
class FairyFortune : public ::testing::Test {
protected:
    void SetUp() override {
        fish_->size = 15.0f;
        fish_->info.min_size = 10.0f;
        fish_->info.max_size = 20.0f;
        fish_->info.min_fp = 100;
        fish_->info.max_fp = 200;
        AngleFish = fish_.get();
    }

    void TearDown() override { AngleFish = saved_angle_; }

    // The FP a catch of fish_kind pays, as the catch's evaluation reads it (FishingGetAngleFishSize).
    int Paid(int fish_kind) {
        fish_->fish_kind = fish_kind;
        int size = 0;
        int points = 0;
        EXPECT_EQ(FishingGetAngleFishSize(&size, &points), fish_kind);
        EXPECT_EQ(size, 150);
        return points;
    }

    TestSave               save_;
    std::unique_ptr<CFish> fish_ = std::make_unique<CFish>();
    CFish                 *saved_angle_ = AngleFish;
};

} // namespace

TEST_F(FairyFortune, WithoutTheRingEveryFishPaysItsOwn) {
    save_.status().chara_weapons[CHARA_RUBY][0].item_no = ITEM_WEAPON_CRYSTAL_RING;
    for (int kind = 0; kind < kFishKinds; kind++) {
        EXPECT_EQ(Paid(kind), 150) << kind;
    }
}

TEST_F(FairyFortune, OwnedEveryFishButTheGarayansPaysTwice) {
    save_.status().chara_weapons[CHARA_RUBY][2].item_no = ITEM_WEAPON_FAIRY_S_RING;
    for (int kind = 0; kind < kFishKinds; kind++) {
        bool garayan = kind == expanded::kMardanGarayan || kind == expanded::kBaronGarayan;
        EXPECT_EQ(Paid(kind), garayan ? 150 : 150 * expanded::kFairyFortune) << kind;
    }
    EXPECT_EQ(Paid(expanded::kMardanGarayan), 150);
    EXPECT_EQ(Paid(expanded::kBaronGarayan), 150);
}

TEST_F(FairyFortune, TheGarayansAreMardanAndBaron) {
    // GetFishMsgNo's system messages: 45 "Mardan Garayan", 37 "Baron Garayan".
    EXPECT_EQ(GetFishMsgNo(expanded::kMardanGarayan), 45);
    EXPECT_EQ(GetFishMsgNo(expanded::kBaronGarayan), 37);
    EXPECT_EQ(expanded::kGarayans.size(), 2u);
}

TEST_F(FairyFortune, ARingInStorageOrAnotherBagCounts) {
    save_.stock().weapons[20].item_no = ITEM_WEAPON_FAIRY_S_RING;
    EXPECT_EQ(Paid(0), 300);
    save_.stock().weapons[20].item_no = 0;
    save_.status().chara_weapons[CHARA_TOAN][7].item_no = ITEM_WEAPON_FAIRY_S_RING;
    EXPECT_EQ(Paid(0), 300);
}

TEST_F(FairyFortune, OffWithTheWeaponAbilities) {
    OptionScope options;
    save_.status().chara_weapons[CHARA_RUBY][0].item_no = ITEM_WEAPON_FAIRY_S_RING;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_EQ(Paid(0), 150);
}

TEST_F(FairyFortune, ADoubledCatchShowsItsPointsRedWithTheRing) {
    save_.status().chara_weapons[CHARA_RUBY][2].item_no = ITEM_WEAPON_FAIRY_S_RING;
    Paid(0);
    EXPECT_EQ(expanded::FairyFortuneCatchText(kCatch), kFortuneCatch);
    // Points with no sign before them are red alone; a text without the points is left as it is.
    EXPECT_EQ(expanded::FairyFortuneCatchText("Punti {value2}!"), "Punti {highlight}{value2}{/color} {icon-sword}!");
    EXPECT_EQ(expanded::FairyFortuneCatchText("caught!!"), std::nullopt);
}

TEST_F(FairyFortune, AnyOtherCatchShowsTheGamesOwnMessage) {
    Paid(0);
    EXPECT_EQ(expanded::FairyFortuneCatchText(kCatch), std::nullopt);

    save_.status().chara_weapons[CHARA_RUBY][2].item_no = ITEM_WEAPON_FAIRY_S_RING;
    Paid(expanded::kMardanGarayan);
    EXPECT_EQ(expanded::FairyFortuneCatchText(kCatch), std::nullopt);
    Paid(0);
    Paid(expanded::kBaronGarayan);
    EXPECT_EQ(expanded::FairyFortuneCatchText(kCatch), std::nullopt);

    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    Paid(0);
    EXPECT_EQ(expanded::FairyFortuneCatchText(kCatch), std::nullopt);
}

TEST_F(FairyFortune, TheTalkWindowShowsTheMarkedCatch) {
    expanded::MenuTextReset(); // keeps Fairy Fortune's own provider
    GameTextFile file;
    ASSERT_EQ(file.Set(expanded::kCatchMessage, kCatch), 0);
    ASSERT_EQ(file.Set(281, "Dran lives in the deepest part of\nthe {highlight}Divine Beast Cave{/color}."), 0);
    short *own = EditMes1.buff;
    EditMes1.buff = file.Data();
    auto text = [](ClsMes &mes, int id) { return GameTextDecode(mes.GetTextLineDataTop(id)); };

    save_.status().chara_weapons[CHARA_RUBY][2].item_no = ITEM_WEAPON_FAIRY_S_RING;
    Paid(0);
    EXPECT_EQ(text(EditMes1, expanded::kCatchMessage), kFortuneCatch);
    EXPECT_EQ(text(EditMes1, 281), "Dran lives in the deepest part of\nthe {highlight}Divine Beast Cave{/color}.");
    Paid(expanded::kBaronGarayan);
    EXPECT_EQ(text(EditMes1, expanded::kCatchMessage), kCatch);

    // Another bank with the same message keeps its own.
    GameTextFile other_file;
    ASSERT_EQ(other_file.Set(expanded::kCatchMessage, kCatch), 0);
    auto other = std::make_unique<ClsMes>();
    other->SetBuff(other_file.Data());
    Paid(0);
    EXPECT_EQ(text(*other, expanded::kCatchMessage), kCatch);

    EditMes1.buff = own;
}

TEST_F(FairyFortune, TheRingTakesTheCatchMessagesWeaponIcon) {
    const s32 made = EditMes1.mes_made;
    EditMes1.mes_made = expanded::kCatchMessage;
    save_.status().chara_weapons[CHARA_RUBY][2].item_no = ITEM_WEAPON_FAIRY_S_RING;
    Paid(0);
    EXPECT_EQ(expanded::FairyFortunePicture(EditMes1, kIconSword), ITEM_WEAPON_FAIRY_S_RING);
    EXPECT_EQ(expanded::FairyFortunePicture(EditMes1, kIconOrb), 0);

    // Only the talk window's catch message.
    auto other = std::make_unique<ClsMes>();
    other->mes_made = expanded::kCatchMessage;
    EXPECT_EQ(expanded::FairyFortunePicture(*other, kIconSword), 0);
    EditMes1.mes_made = 281;
    EXPECT_EQ(expanded::FairyFortunePicture(EditMes1, kIconSword), 0);

    EditMes1.mes_made = expanded::kCatchMessage;
    Paid(expanded::kMardanGarayan);
    EXPECT_EQ(expanded::FairyFortunePicture(EditMes1, kIconSword), 0);
    EditMes1.mes_made = made;
}
