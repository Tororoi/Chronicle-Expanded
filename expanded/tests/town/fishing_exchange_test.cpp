#include <gtest/gtest.h>

#include <algorithm>
#include <array>

#include "dngstatusdata.hpp"
#include "itemdata.hpp"
#include "menu_inventory.hpp"
#include "savedata.hpp"
#include "shop.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::TestSave;

// The exchange's first weapon, Toan's; one follows for each member who has joined, then the Mardan Eins.
constexpr int kFirstWeapon = 26;
constexpr int kXiaosPrize = kFirstWeapon + CHARA_XIAO;

// The prizes as Expanded offers them: retail's, with the Flamingo at 1000 FP in place of the Matador at 1400.
constexpr std::array<FISH_EXCHANGE_ITEM, 35> kPrizes = {
    {{ITEM_ANTIDOTE_DRINK, 5},
     {ITEM_HOLY_WATER, 5},
     {ITEM_SOAP, 5},
     {ITEM_MIGHTY_HEALING, 15},
     {ITEM_ATTACH_DINOSLAYER, 50},
     {ITEM_ATTACH_UNDEAD_BUSTER, 50},
     {ITEM_ATTACH_SEA_KILLER, 50},
     {ITEM_ATTACH_STONE_BREAKER, 50},
     {ITEM_ATTACH_PLANT_BUSTER, 50},
     {ITEM_ATTACH_BEAST_BUSTER, 50},
     {ITEM_ATTACH_SKY_HUNTER, 50},
     {ITEM_ATTACH_METALBREAKER, 50},
     {ITEM_ATTACH_MIMIC_BREAKER, 50},
     {ITEM_ATTACH_MAGE_SLAYER, 50},
     {ITEM_ATTACH_GARNET, 100},
     {ITEM_ATTACH_AMETHYST, 100},
     {ITEM_ATTACH_AQUAMARINE, 100},
     {ITEM_ATTACH_DIAMOND, 100},
     {ITEM_ATTACH_EMERALD, 100},
     {ITEM_ATTACH_PEARL, 100},
     {ITEM_ATTACH_RUBY, 100},
     {ITEM_ATTACH_PERIDOT, 100},
     {ITEM_ATTACH_SAPPHIRE, 100},
     {ITEM_ATTACH_OPAL, 100},
     {ITEM_ATTACH_TOPAZ, 100},
     {ITEM_ATTACH_TURQUOISE, 100},
     {ITEM_WEAPON_TSUKIKAGE, 1100},
     {ITEM_WEAPON_FLAMINGO, 1000},
     {ITEM_WEAPON_MAGICAL_HAMMER, 1800},
     {ITEM_WEAPON_FAIRY_S_RING, 1300},
     {ITEM_WEAPON_SCORPION, 1400},
     {ITEM_WEAPON_BLESSING_GUN, 1500},
     {ITEM_WEAPON_MARDAN_EINS, 2500},
     {ITEM_BOMB_NUTS, 2},
     {ITEM_POISONOUS_APPLE, 2}}
};

} // namespace

TEST(FishingExchange, XiaosPrizeIsTheFlamingoFor1000FP) {
    EXPECT_EQ(exitemlst[kXiaosPrize].item_no, ITEM_WEAPON_FLAMINGO);
    EXPECT_EQ(exitemlst[kXiaosPrize].price, 1000);
    EXPECT_EQ(WhoIsWeaponEquip(ITEM_WEAPON_FLAMINGO), CHARA_XIAO);
}

TEST(FishingExchange, TheMatadorIsGoneAndTheRestAsTheyWere) {
    EXPECT_TRUE(std::ranges::none_of(exitemlst, [](const FISH_EXCHANGE_ITEM &prize) { return prize.item_no == ITEM_WEAPON_MATADOR; }));
    for (std::size_t i = 0; i < kPrizes.size(); i++) {
        EXPECT_EQ(exitemlst[i].item_no, kPrizes[i].item_no) << i;
        EXPECT_EQ(exitemlst[i].price, kPrizes[i].price) << i;
    }
}

TEST(FishingExchange, EachMembersWeaponIsOfferedOnceTheyJoin) {
    // FishingExchangeKey lists the prizes up to kFirstWeapon - 1 + party_size, so Xiao's from when she joins.
    for (int chara = CHARA_TOAN; chara <= CHARA_OSMOND; chara++) {
        EXPECT_EQ(WhoIsWeaponEquip(exitemlst[kFirstWeapon + chara].item_no), chara) << chara;
    }
}

TEST(FishingExchange, TheFlamingoGoesToXiaosBag) {
    TestSave save;
    // As the exchange hands a prize over.
    SaveData->GetDngStatus()->GetItem(exitemlst[kXiaosPrize].item_no, 0);
    const WEAPON_HAVE *weapons = save.status().chara_weapons[CHARA_XIAO];
    EXPECT_TRUE(std::any_of(weapons, weapons + 10, [](const WEAPON_HAVE &weapon) { return weapon.item_no == ITEM_WEAPON_FLAMINGO; }));
    for (int chara : {CHARA_TOAN, CHARA_OSMOND}) {
        const WEAPON_HAVE *others = save.status().chara_weapons[chara];
        EXPECT_TRUE(std::none_of(others, others + 10, [](const WEAPON_HAVE &weapon) { return weapon.item_no == ITEM_WEAPON_FLAMINGO; })) << chara;
    }
}
