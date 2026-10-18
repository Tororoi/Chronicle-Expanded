#pragma once

// A monster sheet's look, as the PS2 mod's preview pages make it, and the glow disc built from the Gallery of
// Time's torch glow. Includes no game header, so dcdata can build with it.

#include <array>
#include <map>
#include <set>
#include <span>
#include <string_view>
#include <vector>

#include "expanded/data/chr_pack.hpp"

namespace expanded {

// The body sheet divided into sections by a map (a section per texel); each section has its own sliders (an HSL hue
// turn, then saturation, contrast, brightness, lightness), palette entries it leaves alone and entries set to a
// colour. The colours that gives are merged to 256, the most similar and darkest first, and the sheet re-indexed
// onto them. The maths and rounding are the preview page's, so a look comes out as it was designed.
class SheetLook {
public:
    // A look from its JSON: {"sections": [{"hue", "sat", "bri", "con", "light", "keep", "set"}], "map": {w, h, rle}}.
    static SheetLook Parse(std::string_view json);

    // An IM2 bank's 8-bit picture in the look.
    Bytes ApplyToSheet(std::span<const unsigned char> tim) const;
    // A second sheet: the first section's sliders on every palette entry.
    Bytes ApplyBaseToSheet(std::span<const unsigned char> tim) const;

private:
    struct Section {
        double                            hue = 0, sat = 1, bri = 1, con = 1, light = 0;
        std::set<int>                     keep;
        std::map<int, std::array<int, 3>> set;
    };

    std::array<int, 3> Colour(int section, int entry, const std::vector<std::array<int, 3>> &palette) const;

    std::vector<Section> sections_;
    int                  map_width_ = 0, map_height_ = 0;
    Bytes                map_;
};

// The element glow disc: the torch glow (a 64x64 32-bit TIM2) as an 8-bit picture, its luminance ranks on the 128
// palette entries the GS's CLUT order leaves in place, coloured from core at the centre to outer at its edge, on a
// vanilla 8-bit picture's headers (templ).
Bytes BuildGlowDisc(std::span<const unsigned char> templ, std::span<const unsigned char> torch, std::array<int, 3> core,
                    std::array<int, 3> outer);

// The torch glow's source and the Fire element's ramp.
inline constexpr const char        *kGlowSourcePack = "dun/mpd_pack/d06main_a.mpd";
inline constexpr const char        *kGlowSourceBank = "fire.img";
inline constexpr const char        *kGlowSourcePicture = "lightling";
inline constexpr std::array<int, 3> kFireGlowCore = {128, 118, 52};
inline constexpr std::array<int, 3> kFireGlowOuter = {200, 5, 0};

} // namespace expanded
