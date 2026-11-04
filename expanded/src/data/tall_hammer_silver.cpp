#include "expanded/data/tall_hammer_silver.hpp"

#include <cstring>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

bool TallHammerGoldTexel(const std::array<int, 3> &rgb) {
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    return h >= 20 && h < 70 && s > 25;
}

std::array<int, 3> TallHammerSilverColour(const std::array<int, 3> &rgb) {
    if (!TallHammerGoldTexel(rgb)) {
        return rgb;
    }
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    return HslToRgb(kTallHammerSilverHue, kTallHammerSilverSaturation, l);
}

Bytes BuildTallHammerSilver(std::span<const unsigned char> hammer) {
    ChrPack pack = ChrPack::Parse(hammer);
    ImgBank bank = ImgBank::Parse(pack.Require(kTallHammerBank).Payload());
    ImgBank copy = bank;
    copy.entries.clear();
    copy.entries.emplace_back(kTallHammerSheet, RecolourSheet(bank.Block(kTallHammerSheet), std::memcmp(bank.magic.data(), "IM2", 3) == 0,
                                                              TallHammerSilverColour));
    pack.records.push_back(ChrRecord::Make(kTallHammerSilverBank, copy.Build()));
    return pack.Rebuild();
}

std::vector<DataBuild> TallHammerSilverBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("the Tall Hammer in silver", std::string(kRebuiltDiscFolder) + kTallHammerFile, {kTallHammerFile},
                               [](Files s) { return BuildTallHammerSilver(s[0]); }),
    };
}

} // namespace expanded
