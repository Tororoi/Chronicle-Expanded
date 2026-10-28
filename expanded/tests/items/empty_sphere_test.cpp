#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "battlemenu.hpp"
#include "clsmes.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/items/empty_sphere.hpp"
#include "expanded/weapons/osmond/star_breaker.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/seven_branch_sword.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "menu_dungeon.hpp"
#include "menu_inventory.hpp"
#include "menu_misc.hpp"
#include "test_support.hpp"
#include "text/gametext.hpp"
#include "weaponlevelup.hpp"

namespace {

using expanded::test::OptionScope;
using expanded::test::TestFloor;
using expanded::test::Wielding;

ATTACH_LIST Sphere(int weapon_no) {
    ATTACH_LIST sphere = {};
    sphere.item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    sphere.sphere_weapon_no = static_cast<s16>(weapon_no);
    return sphere;
}

// A Baselard at level, its stats and abilities a made one's, its first slot holding attach.
WEAPON_HAVE Baselard(int level, const ATTACH_LIST &attach = {}) {
    WEAPON_HAVE weapon = {};
    WepDataListToHaveCopy(ITEM_WEAPON_BASELARD, &weapon);
    weapon.level = static_cast<s16>(level);
    weapon.attach[0] = attach;
    return weapon;
}

bool Breaks(WEAPON_HAVE weapon) {
    return WeaponStatusBreakEnable(&weapon) != 0;
}

// An allmenu bank holding the disc's Status Break confirmation.
std::vector<s16> Bank() {
    GameTextFile file;
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

// The weapon menu with Toan's first weapon selected.
struct Menu {
    TestFloor       floor;
    CDngStatusData *saved = BtlMenuStatusPt;

    explicit Menu(const WEAPON_HAVE &weapon) {
        BtlMenuStatusPt = static_cast<CDngStatusData *>(&floor.status());
        BtlMenuStatusPt->chara_weapons[CHARA_TOAN][0] = weapon;
        WepMenu.chara = CHARA_TOAN;
        WepMenu.weapon_slot = 0;
    }

    ~Menu() { BtlMenuStatusPt = saved; }
};

} // namespace

TEST(EmptySphere, IsASphereOfNoWeapon) {
    EXPECT_TRUE(expanded::EmptySphere(Sphere(0)));
    EXPECT_FALSE(expanded::EmptySphere(Sphere(ITEM_WEAPON_STAR_BREAKER)));
    ATTACH_LIST fire = {};
    fire.item_no = ITEM_ATTACH_FIRE;
    EXPECT_FALSE(expanded::EmptySphere(fire));
}

// In the first slot it lets a weapon below +5 Status Break; in another slot, or a weapon's sphere there, does not.
TEST(EmptySphere, LetsAWeaponBelowFiveBreak) {
    EXPECT_FALSE(Breaks(Baselard(2)));
    EXPECT_TRUE(Breaks(Baselard(2, Sphere(0))));
    EXPECT_TRUE(Breaks(Baselard(0, Sphere(0))));
    EXPECT_FALSE(Breaks(Baselard(2, Sphere(ITEM_WEAPON_DAGGER))));
    WEAPON_HAVE second = Baselard(2);
    second.attach[1] = Sphere(0);
    EXPECT_FALSE(Breaks(second));
    EXPECT_TRUE(Breaks(Baselard(5)));
    EXPECT_TRUE(Breaks(Baselard(7, Sphere(0))));
}

TEST(EmptySphere, NotWithTheWeaponAbilitiesOff) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_FALSE(Breaks(Baselard(2, Sphere(0))));
    EXPECT_TRUE(Breaks(Baselard(5)));
    EXPECT_FALSE(expanded::EmptySphereWarns(Sphere(0)));
}

// It changes neither the weapon's level nor its stats, elements or anti-monster values, and adds nothing to a break.
TEST(EmptySphere, ChangesNothingOfTheWeapon) {
    WEAPON_HAVE bare = Baselard(2);
    WEAPON_HAVE carrying = Baselard(2, Sphere(0));
    EXPECT_TRUE(Breaks(carrying));
    EXPECT_EQ(carrying.level, 2);
    WEAPON_HAVE bare_total = {}, carrying_total = {};
    WeaponAllValueSet(&bare, &bare_total, 0);
    WeaponAllValueSet(&carrying, &carrying_total, 0);
    EXPECT_EQ(carrying_total.attack, bare_total.attack);
    EXPECT_EQ(carrying_total.endurance, bare_total.endurance);
    EXPECT_EQ(carrying_total.speed, bare_total.speed);
    EXPECT_EQ(carrying_total.magic, bare_total.magic);
    EXPECT_EQ(std::memcmp(carrying_total.elem, bare_total.elem, sizeof(bare_total.elem)), 0);
    EXPECT_EQ(std::memcmp(carrying_total.vs_monster, bare_total.vs_monster, sizeof(bare_total.vs_monster)), 0);

    // The break folds the weapon's attachments into its sphere (AttachMentValuePlus): the empty one adds nothing.
    ATTACH_LIST total = Sphere(ITEM_WEAPON_BASELARD);
    total.status[0] = 7;
    total.elem[1] = 3;
    total.vs_monster[2] = 4;
    ATTACH_LIST before = total;
    ATTACH_LIST empty = Sphere(0);
    AttachMentValuePlus(&total, &empty, 1.0f);
    EXPECT_EQ(std::memcmp(&total, &before, sizeof(total)), 0);
}

// The menu warns as it refuses an empty sphere for a slot that takes no sphere; a weapon's sphere, refused there too by
// the game, it refuses as the game does.
TEST(EmptySphere, TheMenuSaysItTakesTheFirstSlot) {
    EXPECT_TRUE(expanded::EmptySphereWarns(Sphere(0)));
    EXPECT_FALSE(expanded::EmptySphereWarns(Sphere(ITEM_WEAPON_DAGGER)));
    std::vector<s16> bank = Bank();
    expanded::MenuTextLoaded("allmenu.mes", bank.data());
    EXPECT_NE(Text(bank, expanded::kEmptySphereSlotMessage).find("first slot"), std::string::npos);
}

// Breaking by it, the confirmation says the sphere is spent; at +5, or the 7 Branch Sword's own refusal, it is as before.
TEST(EmptySphere, TheConfirmationSaysItIsSpent) {
    std::vector<s16> bank = Bank();
    expanded::MenuTextLoaded("allmenu.mes", bank.data());
    {
        Menu menu(Baselard(2, Sphere(0)));
        EXPECT_NE(Text(bank, expanded::kStatusBreakConfirmMessage).find("empty sphere"), std::string::npos);
    }
    {
        Menu menu(Baselard(6, Sphere(0)));
        EXPECT_EQ(Text(bank, expanded::kStatusBreakConfirmMessage), "{alert}Seal 60% weapon\nability to sphere.");
    }
    {
        WEAPON_HAVE seven = Baselard(3, Sphere(0));
        seven.item_no = ITEM_WEAPON_7BRANCH_SWORD;
        Menu menu(seven);
        EXPECT_NE(Text(bank, expanded::kStatusBreakConfirmMessage).find("Seal 77%"), std::string::npos);
    }
}

// Super Steve carrying an empty sphere takes on no weapon's effect.
TEST(EmptySphere, SuperSteveInheritsNothing) {
    TestFloor floor;
    floor.status().cur_chara = CHARA_XIAO;
    Wielding wielding(ITEM_WEAPON_SUPER_STEVE, 0);
    wielding.weapon().attach[0] = Sphere(0);
    EXPECT_EQ(expanded::SphereWeapon(wielding.weapon()), 0);
    EXPECT_EQ(expanded::SuperSteveSphere(), 0);
    EXPECT_FALSE(expanded::WeaponActive(ITEM_WEAPON_STAR_BREAKER));
    EXPECT_DOUBLE_EQ(expanded::ShootingStarChance(CHARA_XIAO), 0.0);
}

// Every language file of expanded/lang names, in both system banks, the SynthSphere (the item-got line of a shooting
// star's sphere) and an empty sphere's weapon (its description's "capability of" line).
TEST(EmptySphere, EveryLanguageNamesIt) {
    std::filesystem::path lang = std::filesystem::path(__FILE__).parent_path();
    while (!std::filesystem::exists(lang / "lang") && lang.has_parent_path() && lang != lang.parent_path()) {
        lang = lang.parent_path();
    }
    lang /= "lang";
    for (const char *name : {"en_us", "en_gb", "fr_fr", "de_de", "it_it", "es_es"}) {
        std::ifstream stream(lang / (std::string(name) + ".json"));
        ASSERT_TRUE(stream) << name;
        std::string   text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        LocalizeTable table;
        std::string   why;
        ASSERT_TRUE(LocalizeParse(text, table, why)) << name << ": " << why;
        for (const char *bank : {"meswin.system", "meswin.system14"}) {
            EXPECT_FALSE(table.files[bank][ITEM_ATTACH_SYNTHESIS_SPHERE + 100].empty()) << name << " " << bank;
            EXPECT_FALSE(table.files[bank][100].empty()) << name << " " << bank;
        }
    }
}
