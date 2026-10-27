#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "battlemenu.hpp"
#include "clsmes.hpp"
#include "dngstatusdata.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/weapons/toan/seven_branch_sword.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"
#include "text/gametext.hpp"

namespace {

WEAPON_HAVE Weapon(int item_no, int level) {
    WEAPON_HAVE weapon = {};
    weapon.item_no = static_cast<s16>(item_no);
    weapon.level = static_cast<s16>(level);
    return weapon;
}

// An allmenu bank holding the disc's Status Break hint and confirmation.
std::vector<s16> Bank() {
    GameTextFile file;
    EXPECT_EQ(file.Set(expanded::kStatusBreakHintMessage, "\"Status break\"\nChange weapons to\n\"SynthSphere\"."), 0);
    EXPECT_EQ(file.Set(expanded::kStatusBreakConfirmMessage, "{alert}Seal 60% weapon\nability to sphere."), 0);
    const s16 *data = file.Data();
    size_t     end = 2 + static_cast<size_t>(data[0]) * 2;
    for (int i = 0; i < data[0]; i++) {
        size_t at = 1 + static_cast<size_t>(data[0]) + static_cast<size_t>(data[3 + i * 2]);
        while (data[at] != MES_CODE_END) {
            at++;
        }
        end = std::max(end, at + 1);
    }
    return std::vector<s16>(data, data + end);
}

std::string Text(std::vector<s16> &bank, int id) {
    ClsMes mes;
    mes.SetBuff(bank.data());
    const s16 *top = mes.GetTextLineDataTop(id);
    return top == nullptr ? "<none>" : GameTextDecode(top);
}

// The weapon menu with Toan's first weapon, item_no, selected.
struct Menu {
    expanded::test::TestFloor floor;
    CDngStatusData           *saved = BtlMenuStatusPt;

    explicit Menu(int item_no) {
        BtlMenuStatusPt = static_cast<CDngStatusData *>(&floor.status());
        BtlMenuStatusPt->chara_weapons[CHARA_TOAN][0] = Weapon(item_no, 7);
        WepMenu.chara = CHARA_TOAN;
        WepMenu.weapon_slot = 0;
    }

    ~Menu() { BtlMenuStatusPt = saved; }
};

} // namespace

TEST(SevenBranchSword, RefusesAStatusBreakBelowLevelSeven) {
    EXPECT_TRUE(expanded::SevenfoldRefuses(Weapon(ITEM_WEAPON_7BRANCH_SWORD, 5)));
    EXPECT_TRUE(expanded::SevenfoldRefuses(Weapon(ITEM_WEAPON_7BRANCH_SWORD, 6)));
    EXPECT_FALSE(expanded::SevenfoldRefuses(Weapon(ITEM_WEAPON_7BRANCH_SWORD, 7)));
    EXPECT_FALSE(expanded::SevenfoldRefuses(Weapon(ITEM_WEAPON_DAGGER, 5)));
}

TEST(SevenBranchSword, ItsSphereKeepsSeventySevenPercent) {
    EXPECT_FLOAT_EQ(expanded::StatusBreakShare(Weapon(ITEM_WEAPON_7BRANCH_SWORD, 7)), 0.77f);
    EXPECT_FLOAT_EQ(expanded::StatusBreakShare(Weapon(ITEM_WEAPON_DAGGER, 7)), 0.6f);
}

TEST(SevenBranchSword, TheMenuExplainsTheRule) {
    std::vector<s16> bank = Bank();
    expanded::MenuTextLoaded("allmenu.mes", bank.data());
    {
        Menu menu(ITEM_WEAPON_7BRANCH_SWORD);
        EXPECT_NE(Text(bank, expanded::kStatusBreakHintMessage).find("77%"), std::string::npos);
        EXPECT_NE(Text(bank, expanded::kStatusBreakConfirmMessage).find("Seal 77%"), std::string::npos);
        EXPECT_NE(Text(bank, expanded::kSevenfoldRefusedMessage).find("below +7"), std::string::npos);
    }
    Menu menu(ITEM_WEAPON_DAGGER);
    EXPECT_EQ(Text(bank, expanded::kStatusBreakHintMessage), "\"Status break\"\nChange weapons to\n\"SynthSphere\".");
    EXPECT_EQ(Text(bank, expanded::kStatusBreakConfirmMessage), "{alert}Seal 60% weapon\nability to sphere.");
}
