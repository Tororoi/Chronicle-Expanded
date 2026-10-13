#include <gtest/gtest.h>

#include "confuse.hpp"
#include "itemdata.hpp"
#include "menu_misc.hpp"
#include "texture_port.hpp"

#include "icon/confuse.hpp"

// Babel's Spear carries Confuse from its weapon data, and no other weapon does.
TEST(Confuse, OnlyBabelsSpearCarriesIt) {
    for (int item = ITEM_WEAPON_START; item < ITEM_WEAPON_START + 120; item++) {
        const WEAPON_DATA *data = GetWeaponData(item);
        if (data == nullptr) {
            continue;
        }
        EXPECT_EQ((data->flags & kWeaponFlagConfuse) != 0, item == ITEM_WEAPON_BABEL_S_SPEAR) << item;
    }
}

// The menus name bit 14 with the system bank's message 0x45 and count it a benefit; the retail bits keep
// their names (bit + 69) and colours.
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

// The icon goes into the empty tile of bit 14, each opaque pixel as the palette's nearest colour, and only
// there: the clear pixels and the rest of the sheet stay.
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
