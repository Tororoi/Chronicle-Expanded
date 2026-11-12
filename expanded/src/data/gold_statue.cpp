#include "expanded/data/gold_statue.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/recolour_pack.hpp"

namespace expanded {

namespace {

// Where on kGoldRamp (0 to 1) a point of the ball shines, x right and y up across it, both -1 to 1.
double Shine(double x, double y) {
    double r2 = x * x + y * y;
    if (r2 >= 1.0) {
        return 0.0; // off the ball: never sampled but at its rim
    }
    double nz = std::sqrt(1.0 - r2);
    // The view (0, 0, -1) reflected about the normal: up is the sky, down the floor.
    double ry = 2.0 * nz * y;
    double sky = std::clamp((ry - kGoldHorizon) / (1.0 - kGoldHorizon), 0.0, 1.0);
    double floor = std::clamp((-ry - kGoldHorizon) / (1.0 - kGoldHorizon), 0.0, 1.0);
    double horizon = std::exp(-std::pow(ry / kGoldHorizonWidth, 2.0)); // the dark band between them
    double shine = kGoldSky * std::sqrt(sky) + kGoldFloor * floor + kGoldBase * (1.0 - horizon);
    // A key light up and to the left.
    double hx = x - kGoldHighlight[0], hy = y - kGoldHighlight[1];
    shine += std::exp(-(hx * hx + hy * hy) / (kGoldHighlightSize * kGoldHighlightSize));
    // Darker toward the rim, as metal seen edge-on picks up the room.
    return std::clamp(shine * (0.55 + 0.45 * nz), 0.0, 1.0);
}

std::array<int, 3> RampAt(double t) {
    double             at = t * (kGoldRamp.size() - 1);
    std::size_t        i = std::min(kGoldRamp.size() - 2, static_cast<std::size_t>(at));
    double             k = at - static_cast<double>(i);
    std::array<int, 3> out{};
    for (int c = 0; c < 3; c++) {
        out[c] = static_cast<int>(std::lround(kGoldRamp[i][c] + (kGoldRamp[i + 1][c] - kGoldRamp[i][c]) * k));
    }
    return out;
}

} // namespace

Bytes GoldBall(std::span<const unsigned char> templ, bool swizzled) {
    constexpr int n = kGoldBallSize;
    Bytes         pixels(static_cast<std::size_t>(n * n));
    for (int row = 0; row < n; row++) {
        for (int col = 0; col < n; col++) {
            double x = (col + 0.5) / n * 2.0 - 1.0, y = 1.0 - (row + 0.5) / n * 2.0; // the top row faces up
            pixels[static_cast<std::size_t>(row * n + col)] = static_cast<unsigned char>(std::lround(Shine(x, y) * 255.0));
        }
    }
    Bytes clut(256 * 4);
    for (int i = 0; i < 256; i++) {
        std::array<int, 3> rgb = RampAt(i / 255.0);
        int                at = tim8::Csm1(i) * 4;
        for (int c = 0; c < 3; c++) {
            clut[static_cast<std::size_t>(at + c)] = static_cast<unsigned char>(rgb[c]);
        }
        clut[static_cast<std::size_t>(at + 3)] = 0x80;
    }
    return tim8::Build(templ, n, n, swizzled ? tim8::Swizzle(pixels, n, n) : pixels, clut);
}

Bytes BuildGoldStatue(std::span<const unsigned char> rock) {
    // The rock's picture renamed (its meshes' materials with it), then drawn over as the ball.
    ChrPack pack = ChrPack::Parse(RecolourPackPicture(rock, "iwa.img", "d02b10", "iwa.mds", kGoldStatuePicture,
                                                      [](const std::array<int, 3> &rgb) { return rgb; }));
    ImgBank bank = ImgBank::Parse(pack.Require("iwa.img").Payload());
    bool    swizzled = std::memcmp(bank.magic.data(), "IM2", 3) == 0;
    auto    picture = std::find_if(bank.entries.begin(), bank.entries.end(), [](const auto &e) { return e.first == kGoldStatuePicture; });

    picture->second = GoldBall(picture->second, swizzled);
    pack.Require("iwa.img").ReplacePayload(bank.Build());
    return pack.Rebuild();
}

std::vector<DataBuild> GoldStatueBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Midas Touch's gold", kGoldStatueFile, {kGoldStatueSource}, [](Files s) { return BuildGoldStatue(s[0]); }),
    };
}

} // namespace expanded
