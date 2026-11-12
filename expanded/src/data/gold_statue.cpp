#include "expanded/data/gold_statue.hpp"

#include <algorithm>
#include <cmath>

#include "expanded/data/recolour_pack.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

std::array<int, 3> GoldOf(const std::array<int, 3> &rgb) {
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    double             t = std::clamp((l / 100.0 - kGoldLow) / (kGoldHigh - kGoldLow), 0.0, 1.0); // its lightness: 0 to 100
    const auto        &low = t < 0.5 ? kGoldRamp[0] : kGoldRamp[1];
    const auto        &high = t < 0.5 ? kGoldRamp[1] : kGoldRamp[2];
    double             k = t < 0.5 ? t * 2.0 : (t - 0.5) * 2.0;
    std::array<int, 3> out{};
    for (int c = 0; c < 3; c++) {
        out[c] = static_cast<int>(std::lround(low[c] + (high[c] - low[c]) * k));
    }
    return out;
}

std::vector<DataBuild> GoldStatueBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Midas Touch's gold", kGoldStatueFile, {kGoldStatueSource},
                               [](Files s) { return RecolourPackPicture(s[0], "iwa.img", "d02b10", "iwa.mds", kGoldStatuePicture, GoldOf); }),
    };
}

} // namespace expanded
