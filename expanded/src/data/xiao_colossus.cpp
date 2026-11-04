#include "expanded/data/xiao_colossus.hpp"

#include <cstring>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/goro_shirt.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"
#include "expanded/data/tall_hammer_silver.hpp"

namespace expanded {

namespace {

bool In(const int (&tile)[4], int x, int y) {
    return x >= tile[0] && y >= tile[1] && x < tile[2] && y < tile[3];
}

bool Swizzled(const ImgBank &bank) {
    return std::memcmp(bank.magic.data(), "IM2", 3) == 0;
}

std::array<int, 3> At(const tim8::Picture &picture, int x, int y) {
    std::size_t j = static_cast<std::size_t>(tim8::Csm1(picture.pixels[static_cast<std::size_t>(y) * picture.width + x])) * 4;
    return {picture.clut[j], picture.clut[j + 1], picture.clut[j + 2]};
}

} // namespace

std::array<int, 3> XiaoGrowColour(int x, int y, const std::array<int, 3> &rgb) {
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    if ((In(kXiaoTurquoise[0], x, y) || In(kXiaoTurquoise[1], x, y)) && (h < 25 || h >= 330) && s > 25) {
        return HslToRgb(kGoroTurquoiseHue, s, l);
    }
    return rgb;
}

std::array<int, 3> XiaoShrinkColour(int x, int y, const std::array<int, 3> &rgb) {
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    if ((In(kXiaoBell[0], x, y) || In(kXiaoBell[1], x, y)) && h >= 20 && h < 70 && s > 30) {
        return HslToRgb(kTallHammerSilverHue, kTallHammerSilverSaturation, l);
    }
    return rgb;
}

bool SuperSteveHair(int x, int y, const std::array<int, 3> &rgb) {
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    return In(kSuperSteveHairTile, x, y) && h >= 40 && h < 80 && s > 25 && l < 85;
}

Bytes BuildXiaoColossus(std::span<const unsigned char> xiao, std::span<const unsigned char> super_steve, std::span<const unsigned char> steve) {
    ChrPack       xiao_pack = ChrPack::Parse(xiao), super_pack = ChrPack::Parse(super_steve), steve_pack = ChrPack::Parse(steve);
    ImgBank       xiao_bank = ImgBank::Parse(xiao_pack.Require(kXiaoBank).Payload());
    ImgBank       super_bank = ImgBank::Parse(super_pack.Require(kSuperSteveBank).Payload());
    ImgBank       steve_bank = ImgBank::Parse(steve_pack.Require(kSteveOwnBank).Payload());
    const Bytes  &sheet = xiao_bank.Block(kXiaoSheet);
    tim8::Picture own_steve = tim8::Read(steve_bank.Block(kSteveOwnSheet), Swizzled(steve_bank));

    ImgBank grow = xiao_bank, shrink = xiao_bank, steve_shrink = super_bank; // each in its own bank's pixel order
    grow.entries.clear();
    shrink.entries.clear();
    steve_shrink.entries.clear();
    grow.entries.emplace_back(kXiaoSheet, RecolourSheetAt(sheet, Swizzled(xiao_bank), XiaoGrowColour));
    shrink.entries.emplace_back(kXiaoSheet, RecolourSheetAt(sheet, Swizzled(xiao_bank), XiaoShrinkColour));
    steve_shrink.entries.emplace_back(kSteveSheet, RecolourSheetAt(super_bank.Block(kSteveSheet), Swizzled(super_bank), [&](int x, int y, const std::array<int, 3> &rgb) {
                                          return SuperSteveHair(x, y, rgb) && x < own_steve.width && y < own_steve.height ? At(own_steve, x, y) : rgb;
                                      }));
    return ChrPack::Build({ChrRecord::Make(kXiaoGrowBank, grow.Build()), ChrRecord::Make(kXiaoShrinkBank, shrink.Build()),
                           ChrRecord::Make(kSteveShrinkBank, steve_shrink.Build())});
}

std::vector<DataBuild> XiaoColossusBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Xiao's Colossus look", kXiaoColossusFile, {kXiaoDungeonFile, "commenu/weapon/c04w13.chr", kSteveFile},
                               [](Files s) { return BuildXiaoColossus(s[0], s[1], s[2]); }),
    };
}

} // namespace expanded
