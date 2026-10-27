#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "clsmes.hpp"
#include "data_fixture.hpp"
#include "expanded/core/menu_text.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "mainselect.hpp"
#include "sysmes.hpp"
#include "text/gametext.hpp"

namespace fs = std::filesystem;

namespace {

constexpr int kStatusBreakHint = 118;

int DescriptionOf(int item_no) {
    return GetCommonItemInfo(item_no)->msg + 500;
}

// A menu bank holding the Dagger's and the Baselard's descriptions and the Status Break hint.
std::vector<s16> Bank() {
    GameTextFile file;
    EXPECT_EQ(file.Set(DescriptionOf(ITEM_WEAPON_DAGGER), "\"Dagger{value}\"\nFairly easy to use.\ndefault weapon."), 0);
    EXPECT_EQ(file.Set(DescriptionOf(ITEM_WEAPON_BASELARD), "\"Baselard{value}\"\nA big single-edged\nsword."), 0);
    EXPECT_EQ(file.Set(kStatusBreakHint, "{alert}Seal 60% weapon\nability to sphere."), 0);
    const s16 *data = file.Data();
    // The last message by offset ends the file.
    size_t end = 2 + static_cast<size_t>(data[0]) * 2;
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

bool g_seven = false;

std::optional<std::string> SevenHint() {
    if (!g_seven) {
        return std::nullopt;
    }
    return "{alert}Seal 77% weapon\nability to sphere.";
}

std::optional<std::string> DaggerLines(int item_no) {
    return "Item " + std::to_string(item_no) + ".\nA test override.";
}

std::optional<std::string> SphereLine() {
    return "{icon-orb}\"{insert1}\" SynthSphere acquired.";
}

struct Reset {
    Reset() { expanded::MenuTextReset(); }

    ~Reset() { expanded::MenuTextReset(); }
};

} // namespace

TEST(ExpandedMenuText, OnlyTheMenuBanksTakeExpandedText) {
    Reset reset;
    std::vector<s16> bank = Bank();
    const int        dagger = DescriptionOf(ITEM_WEAPON_DAGGER);
    expanded::SetWeaponDescription(ITEM_WEAPON_DAGGER, DaggerLines);

    // A bank that was not taken as allmenu.mes or itemshop.bin keeps its text.
    EXPECT_EQ(Text(bank, dagger), "\"Dagger{value}\"\nFairly easy to use.\ndefault weapon.");
    expanded::MenuTextLoaded("qchr.mes", bank.data());
    EXPECT_EQ(Text(bank, dagger), "\"Dagger{value}\"\nFairly easy to use.\ndefault weapon.");

    expanded::MenuTextLoaded("ALLMENU.MES", bank.data());
    EXPECT_EQ(Text(bank, dagger), "\"Dagger{value}\"\nItem 258.\nA test override.");
    EXPECT_EQ(Text(bank, DescriptionOf(ITEM_WEAPON_BASELARD)), "\"Baselard{value}\"\nA big single-edged\nsword.");

    // Another file at the same address is not the bank.
    expanded::MenuTextLoaded("shopman.bin", bank.data());
    EXPECT_EQ(Text(bank, dagger), "\"Dagger{value}\"\nFairly easy to use.\ndefault weapon.");

    expanded::MenuTextLoaded("itemshop.bin", bank.data());
    EXPECT_EQ(Text(bank, dagger), "\"Dagger{value}\"\nItem 258.\nA test override.");
    expanded::SetWeaponDescription(ITEM_WEAPON_DAGGER, nullptr);
    EXPECT_EQ(Text(bank, dagger), "\"Dagger{value}\"\nFairly easy to use.\ndefault weapon.");
}

TEST(ExpandedMenuText, AMessageFollowsItsProvider) {
    Reset reset;
    std::vector<s16> bank = Bank();
    expanded::MenuTextLoaded("allmenu.mes", bank.data());
    expanded::SetMenuMessage(kStatusBreakHint, SevenHint);

    g_seven = false;
    EXPECT_EQ(Text(bank, kStatusBreakHint), "{alert}Seal 60% weapon\nability to sphere.");
    g_seven = true;
    EXPECT_EQ(Text(bank, kStatusBreakHint), "{alert}Seal 77% weapon\nability to sphere.");

    // The shop's bank has other messages under these numbers.
    expanded::MenuTextLoaded("itemshop.bin", bank.data());
    EXPECT_EQ(Text(bank, kStatusBreakHint), "{alert}Seal 60% weapon\nability to sphere.");
    g_seven = false;
}

TEST(ExpandedMenuText, ASystemMessageIsTheItemLinesBanksOnly) {
    Reset            reset;
    std::vector<s16> bank = Bank();
    expanded::SetSystemMessage(1000, SphereLine);
    EXPECT_EQ(Text(bank, 1000), "<none>");
    short *own = SystemMessageWindow().buff;
    SystemMessageWindow().buff = bank.data();
    EXPECT_EQ(Text(bank, 1000), "{icon-orb}\"{insert1}\" SynthSphere acquired.");
    EXPECT_EQ(Text(bank, kStatusBreakHint), "{alert}Seal 60% weapon\nability to sphere.");
    SystemMessageWindow().buff = own;
}

TEST(ExpandedMenuText, LanguageFilesGiveADescription) {
    Reset reset;
    fs::path dir = datafix::TempDir("menu_text");
    std::ofstream(dir / "en_gb.json") << R"({"expanded.weapon.259.description": "From the\nlanguage file."})";
    LocalizeReset();
    LocalizeSetDirectories({dir});
    LanguageCode = 2;

    std::vector<s16> bank = Bank();
    expanded::MenuTextLoaded("allmenu.mes", bank.data());
    EXPECT_EQ(expanded::WeaponDescriptionKey(ITEM_WEAPON_BASELARD), "expanded.weapon.259.description");
    EXPECT_EQ(Text(bank, DescriptionOf(ITEM_WEAPON_BASELARD)), "\"Baselard{value}\"\nFrom the\nlanguage file.");
    EXPECT_EQ(Text(bank, DescriptionOf(ITEM_WEAPON_DAGGER)), "\"Dagger{value}\"\nFairly easy to use.\ndefault weapon.");

    LocalizeReset();
    LocalizeSetDirectories({});
    std::error_code error;
    fs::remove_all(dir, error);
}
