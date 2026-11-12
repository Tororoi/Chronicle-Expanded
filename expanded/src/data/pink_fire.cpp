#include "expanded/data/pink_fire.hpp"

#include <cmath>

#include "expanded/data/recolour_pack.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

namespace {

std::array<int, 3> Turned(const std::array<int, 3> &rgb) {
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    return HslToRgb(std::fmod(h + kPinkFireHueTurn + 360.0, 360.0), s, l);
}

} // namespace

Bytes BuildPinkFire(std::span<const unsigned char> source, const char *bank, const char *picture, const char *mds, const char *renamed) {
    return RecolourPackPicture(source, bank, picture, mds, renamed, Turned);
}

std::vector<DataBuild> PinkFireBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Satan's Ax's fireball", kPinkFireballFile, {kPinkFireballSource},
                               [](Files s) { return BuildPinkFire(s[0], "f_boll_3.img", "f_booll_2", "f_boll_3.mds", kPinkFireballPicture); }),
        DataBuild::FromSources("Satan's Ax's blast", kPinkBlastFile, {kPinkBlastSource},
                               [](Files s) { return BuildPinkFire(s[0], "zibaku_f.img", "zibaku_f", "zibaku_f2.mds", kPinkBlastPicture); }),
    };
}

} // namespace expanded
