#include "expanded/data/goro_shirt.hpp"

#include <cstring>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

bool GoroShirtTexel(const std::array<int, 3> &rgb) {
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    return h >= 60 && h < 180 && s > 12;
}

std::array<int, 3> GoroShirtColour(const std::array<int, 3> &rgb) {
    if (!GoroShirtTexel(rgb)) {
        return rgb;
    }
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    return HslToRgb(kGoroTurquoiseHue, s, l);
}

Bytes BuildGoroShirt(std::span<const unsigned char> goro) {
    ChrPack pack = ChrPack::Parse(goro);
    ImgBank bank = ImgBank::Parse(pack.Require(kGoroBank).Payload());
    ImgBank copies = bank;
    bool    swizzled = std::memcmp(bank.magic.data(), "IM2", 3) == 0;
    copies.entries.clear();
    for (const char *sheet : kGoroShirtSheets) {
        copies.entries.emplace_back(sheet, RecolourSheet(bank.Block(sheet), swizzled, GoroShirtColour));
    }
    pack.records.push_back(ChrRecord::Make(kGoroShirtBank, copies.Build()));
    return pack.Rebuild();
}

std::vector<DataBuild> GoroShirtBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Goro's shirt", std::string(kRebuiltDiscFolder) + kGoroDungeonFile, {kGoroDungeonFile},
                               [](Files s) { return BuildGoroShirt(s[0]); }),
    };
}

} // namespace expanded
