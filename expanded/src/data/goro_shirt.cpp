#include "expanded/data/goro_shirt.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

namespace {

std::array<int, 3> At(const tim8::Picture &picture, int index) {
    std::size_t j = static_cast<std::size_t>(tim8::Csm1(index)) * 4;
    return {picture.clut[j], picture.clut[j + 1], picture.clut[j + 2]};
}

// The sheet's copy: each texel's colour in the copy, merged to 256 and indexed again.
Bytes ShirtCopy(std::span<const unsigned char> tim, const char *sheet, bool swizzled) {
    tim8::Picture                   picture = tim8::Read(tim, swizzled);
    std::vector<int>                colour(picture.pixels.size());
    std::unordered_map<int, double> weight;
    std::vector<int>                order;
    for (std::size_t i = 0; i < colour.size(); i++) {
        auto c = GoroShirtColour(At(picture, picture.pixels[i]));
        colour[i] = (c[0] << 16) | (c[1] << 8) | c[2];
        if (weight[colour[i]]++ == 0) {
            order.push_back(colour[i]);
        }
    }
    std::unordered_map<int, int> merged = MergeColours(order, weight);
    std::unordered_map<int, int> slot;
    Bytes                        index(colour.size()), clut(256 * 4, 0);
    int                          used = 0;
    for (std::size_t i = 0; i < colour.size(); i++) {
        int v = colour[i];
        if (auto m = merged.find(v); m != merged.end()) {
            v = m->second;
        }
        auto [found, added] = slot.try_emplace(v, used);
        if (added) {
            if (used >= 256) {
                throw std::runtime_error(std::string(sheet) + ": more than 256 colours after merging");
            }
            std::size_t j = static_cast<std::size_t>(tim8::Csm1(used)) * 4;
            clut[j] = static_cast<unsigned char>(v >> 16);
            clut[j + 1] = static_cast<unsigned char>(v >> 8);
            clut[j + 2] = static_cast<unsigned char>(v);
            clut[j + 3] = 0x80;
            used++;
        }
        index[i] = static_cast<unsigned char>(found->second);
    }
    Bytes pixels = swizzled ? tim8::Swizzle(index, picture.width, picture.height) : index;
    return tim8::Build(tim, picture.width, picture.height, pixels, clut);
}

} // namespace

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
        copies.entries.emplace_back(sheet, ShirtCopy(bank.Block(sheet), sheet, swizzled));
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
