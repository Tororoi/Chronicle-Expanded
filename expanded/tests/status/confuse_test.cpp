#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>

#include "expanded/status/confuse.hpp"
#include "expanded/status/confuse_icon.hpp"
#include "itemdata.hpp"
#include "localize.hpp"
#include "menu_misc.hpp"
#include "texture_port.hpp"

using namespace expanded;

// Babel's Spear is built with Confuse, and no other weapon is.
TEST(Confuse, OnlyBabelsSpearCarriesIt) {
    for (int item = ITEM_WEAPON_START; item < ITEM_WEAPON_START + 120; item++) {
        const WEAPON_DATA *data = GetWeaponData(item);
        if (data == nullptr) {
            continue;
        }
        EXPECT_EQ((WeaponAbilities(item, data->flags, 99) & kAbilityConfuse) != 0, item == ITEM_WEAPON_BABEL_S_SPEAR) << item;
    }
}

// The menus name bit 14 with message 0x45 and count it a benefit; the retail bits keep theirs.
TEST(Confuse, MenusNameItAndCountItGood) {
    EXPECT_EQ(ConfuseOptionMessage(kConfuseOptionBit), 0x45);
    EXPECT_EQ(ConfuseOptionMessage(1), 0x46);
    EXPECT_EQ(ConfuseOptionMessage(13), 0x52);
    const int retail[14] = {0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1};
    for (int bit = 0; bit < 14; bit++) {
        EXPECT_EQ(IsWeaponOptionGoodOrBad(bit), retail[bit]) << bit;
    }
    EXPECT_EQ(IsWeaponOptionGoodOrBad(kConfuseOptionBit), 1);
}

namespace {

// A charaface-sized index sheet of colour 0 (clear), with an opaque grey ramp in the palette.
PortDecodedTexture Sheet() {
    PortDecodedTexture sheet;
    sheet.width = 256;
    sheet.height = 320;
    sheet.format = gfx::TextureFormat::Index8;
    sheet.levels.emplace_back(256 * 320, 0);
    for (unsigned i = 1; i < 256; i++) {
        sheet.palette[i] = 0xFF000000u | i << 16 | i << 8 | i;
    }
    return sheet;
}

} // namespace

// The icon fills only bit 14's empty tile, each opaque pixel as the nearest palette colour.
TEST(Confuse, IconFillsItsEmptyTile) {
    PortDecodedTexture sheet = Sheet();
    ConfuseIconTexture("charaface", sheet);
    constexpr int kX = 0xD8;
    constexpr int kY = 0xC8;
    int           drawn = 0;
    for (int y = 0; y < 320; y++) {
        for (int x = 0; x < 256; x++) {
            uint8_t index = sheet.levels[0][y * 256 + x];
            bool    inside = x >= kX && x < kX + kConfuseIconSize && y >= kY && y < kY + kConfuseIconSize;
            if (!inside) {
                ASSERT_EQ(index, 0) << x << "," << y;
                continue;
            }
            const uint8_t *rgba = &kConfuseIconRgba[((y - kY) * kConfuseIconSize + (x - kX)) * 4];
            if (rgba[3] < 128) {
                EXPECT_EQ(index, 0);
                continue;
            }
            drawn++;
            EXPECT_NE(index, 0);
        }
    }
    EXPECT_GT(drawn, 0);
}

// A sheet whose tile is not empty, or another texture, is left alone.
TEST(Confuse, IconLeavesOtherSheetsAlone) {
    PortDecodedTexture used = Sheet();
    used.levels[0][(0xC8 + 5) * 256 + 0xD8 + 5] = 7;
    std::vector<uint8_t> before = used.levels[0];
    ConfuseIconTexture("charaface", used);
    EXPECT_EQ(used.levels[0], before);

    PortDecodedTexture other = Sheet();
    ConfuseIconTexture("charaface2", other);
    EXPECT_EQ(other.levels[0], std::vector<uint8_t>(256 * 320, 0));
}

// Every language file of expanded/lang names Confuse in both system banks.
TEST(Confuse, EveryLanguageNamesIt) {
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
        EXPECT_FALSE(table.files["meswin.system"][kConfuseNameMessage].empty()) << name;
        EXPECT_FALSE(table.files["meswin.system14"][kConfuseNameMessage].empty()) << name;
    }
}
