#include "expanded/weapons/goro/colossus_look.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <optional>
#include <span>
#include <vector>

#include "data/data_read.hpp"
#include "dataread.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"
#include "gfx/gfx.hpp"
#include "render/texture_registry.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

// One body sheet: its texels' palette indices, its palette (0 to 1, and the renderer's alpha) and each texel's region,
// and the texture last written with the share it was written at.
struct Sheet {
    const char                           *name = nullptr;
    int                                   width = 0, height = 0;
    std::vector<unsigned char>            index;
    std::array<std::array<float, 3>, 256> colour{};
    std::array<unsigned char, 256>        alpha{};
    std::vector<ColossusRegion>           region;
    gfx::TextureHandle                    texture = gfx::kNullTexture;
    float                                 share = 0.0f;
    bool                                  refused = false; // a texture at another size: left as it is
};

struct Look {
    bool               read = false;
    std::vector<Sheet> sheets;
};

Look look;

void Hsl(const float *c, float &h, float &s, float &l) {
    float mx = std::max({c[0], c[1], c[2]}), mn = std::min({c[0], c[1], c[2]}), d = mx - mn;
    l = (mx + mn) / 2.0f;
    if (d < 1e-6f) {
        h = s = 0.0f;
        return;
    }
    s = l > 0.5f ? d / (2.0f - mx - mn) : d / (mx + mn);
    h = mx == c[0] ? (c[1] - c[2]) / d + (c[1] < c[2] ? 6.0f : 0.0f) : mx == c[1] ? (c[2] - c[0]) / d + 2.0f
                                                                                  : (c[0] - c[1]) / d + 4.0f;
    h *= 60.0f;
}

void Rgb(float h, float s, float l, float *out) {
    float a = s * std::min(l, 1.0f - l);
    for (int i = 0; i < 3; i++) {
        float k = std::fmod((i == 0 ? 0.0f : i == 1 ? 8.0f
                                                    : 4.0f) +
                                h / 30.0f,
                            12.0f);
        out[i] = l - a * std::max(-1.0f, std::min({k - 3.0f, 9.0f - k, 1.0f}));
    }
}

// The sheets as his pack has them (read once).
void Read() {
    look.read = true;
    std::uintmax_t size = DataFileSize(kColossusLookPack);
    if (size == 0) {
        return;
    }
    std::vector<u_int> pack((size + 2 * 2048) / sizeof(u_int));
    char               path[64];
    std::snprintf(path, sizeof(path), "%s", kColossusLookPack);
    if (LoadFile2(path, pack.data(), nullptr, 0) != 0) {
        char bank_name[32];
        std::snprintf(bank_name, sizeof(bank_name), "%s", kColossusLookBank);
        int   bank_size = 0;
        auto *bank = reinterpret_cast<const unsigned char *>(GetPackFile(pack.data(), bank_name, &bank_size));
        if (bank != nullptr && bank_size > 0) {
            try {
                ImgBank images = ImgBank::Parse({bank, static_cast<std::size_t>(bank_size)});
                bool    swizzled = std::memcmp(images.magic.data(), "IM2", 3) == 0;
                for (const char *name : kColossusLookSheets) {
                    tim8::Picture picture = tim8::Read(images.Block(name), swizzled);
                    Sheet         sheet;
                    sheet.name = name;
                    sheet.width = picture.width;
                    sheet.height = picture.height;
                    sheet.index.assign(picture.pixels.begin(), picture.pixels.end());
                    for (int k = 0; k < 256; k++) {
                        std::size_t j = static_cast<std::size_t>(tim8::Csm1(k)) * 4;
                        for (int c = 0; c < 3; c++) {
                            sheet.colour[k][c] = picture.clut[j + c] / 255.0f;
                        }
                        sheet.alpha[k] = static_cast<unsigned char>(std::min(255, picture.clut[j + 3] * 2));
                    }
                    sheet.region.resize(sheet.index.size());
                    for (int y = 0; y < sheet.height; y++) {
                        for (int x = 0; x < sheet.width; x++) {
                            std::size_t i = static_cast<std::size_t>(y) * sheet.width + x;
                            sheet.region[i] = ColossusRegionOf(name, x, y, sheet.colour[sheet.index[i]].data());
                        }
                    }
                    look.sheets.push_back(std::move(sheet));
                }
            } catch (const std::exception &) {
                look.sheets.clear();
            }
        }
    }
    DataReadForgetPackCapacities(pack.data());
}

// The sheet's texels at a share of the look, as RGBA8, each mip level after the first a box filter of the one before.
void Write(Sheet &sheet, gfx::TextureHandle texture, std::uint32_t levels, float share) {
    std::array<std::array<std::array<unsigned char, 4>, 256>, 3> lut{};
    for (int r = 0; r < 3; r++) {
        for (int k = 0; k < 256; k++) {
            float out[3];
            ColossusLookColour(static_cast<ColossusRegion>(r), share, sheet.colour[k].data(), out);
            for (int c = 0; c < 3; c++) {
                lut[r][k][c] = static_cast<unsigned char>(std::lround(std::clamp(out[c], 0.0f, 1.0f) * 255.0f));
            }
            lut[r][k][3] = sheet.alpha[k];
        }
    }
    std::vector<unsigned char> level(sheet.index.size() * 4);
    for (std::size_t i = 0; i < sheet.index.size(); i++) {
        std::memcpy(&level[i * 4], lut[static_cast<int>(sheet.region[i])][sheet.index[i]].data(), 4);
    }
    int w = sheet.width, h = sheet.height;
    for (std::uint32_t mip = 0; mip < std::max(levels, 1u); mip++) {
        gfx::UpdateTexture(texture, mip, 0, 0, w, h, level.data());
        if (w == 1 && h == 1) {
            break;
        }
        int                        nw = std::max(1, w / 2), nh = std::max(1, h / 2);
        std::vector<unsigned char> next(static_cast<std::size_t>(nw) * nh * 4);
        for (int y = 0; y < nh; y++) {
            for (int x = 0; x < nw; x++) {
                for (int c = 0; c < 4; c++) {
                    int sum = 0;
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            int sx = std::min(w - 1, x * 2 + dx), sy = std::min(h - 1, y * 2 + dy);
                            sum += level[(static_cast<std::size_t>(sy) * w + sx) * 4 + c];
                        }
                    }
                    next[(static_cast<std::size_t>(y) * nw + x) * 4 + c] = static_cast<unsigned char>((sum + 2) / 4);
                }
            }
        }
        level = std::move(next);
        w = nw;
        h = nh;
    }
}

} // namespace

ColossusRegion ColossusRegionOf(const char *sheet, int x, int y, const float *rgb) {
    if (std::strcmp(sheet, kColossusWrapsSheet) == 0 && x >= kColossusWraps[0] && y >= kColossusWraps[1] && x < kColossusWraps[2] &&
        y < kColossusWraps[3]) {
        return ColossusRegion::Kept;
    }
    float h, s, l;
    Hsl(rgb, h, s, l);
    if (h >= 60.0f && h < 180.0f && s > 0.12f) {
        return ColossusRegion::Turquoise;
    }
    if (s < 0.1f || l > 0.85f || l < 0.02f) { // bone, buckles, highlights, the blackest lines
        return ColossusRegion::Kept;
    }
    if (h < 8.0f || h >= 330.0f) { // the red strip, the pink
        return ColossusRegion::Kept;
    }
    if (h < 30.0f && l >= 0.5f && s >= 0.35f) { // flesh
        return ColossusRegion::Kept;
    }
    if (h >= 25.0f && h < 60.0f && l >= 0.62f && s < 0.35f) { // teeth and claws
        return ColossusRegion::Kept;
    }
    return h < 70.0f ? ColossusRegion::Gold : ColossusRegion::Kept;
}

void ColossusLookColour(ColossusRegion region, float share, const float *rgb, float *out) {
    std::copy(rgb, rgb + 3, out);
    if (region == ColossusRegion::Kept || share <= 0.0f) {
        return;
    }
    float h, s, l;
    Hsl(rgb, h, s, l);
    float to = region == ColossusRegion::Gold ? kColossusGoldHue : kColossusTurquoiseHue;
    float turn = std::fmod(to - h + 540.0f, 360.0f) - 180.0f; // the shorter way round
    Rgb(std::fmod(h + turn * share + 360.0f, 360.0f), s, l, out);
    if (region == ColossusRegion::Gold) {
        for (int c = 0; c < 3; c++) {
            out[c] *= 1.0f + kColossusTint[c] * kColossusTintStrength * share;
        }
    }
}

float ColossusLookShare(float size) {
    return std::clamp((size - 1.0f) / (kColossusMost - 1.0f), 0.0f, 1.0f);
}

void ColossusLookStep(float size) {
    if (!look.read) {
        // Read once his sheets are loaded (none are without the game's data).
        if (TexManager.GetTexture(const_cast<char *>(kColossusLookSheets[0]), -1) == nullptr) {
            return;
        }
        Read();
    }
    float share = ColossusLookShare(size);
    for (Sheet &sheet : look.sheets) {
        CTexture *texture = TexManager.GetTexture(const_cast<char *>(sheet.name), -1);
        if (texture == nullptr) {
            continue;
        }
        dc::render::PortTextureRef ref = dc::render::PortTextureFromCTexture(texture);
        if (!ref.valid || ref.binding.texture == gfx::kNullTexture) {
            continue;
        }
        if (ref.binding.texture != sheet.texture) {
            // A texture as loaded: its own colours, and checked once for its size.
            sheet.texture = ref.binding.texture;
            sheet.share = 0.0f;
            std::optional<gfx::TextureInfo> info = gfx::GetTextureInfo(sheet.texture);
            sheet.refused = !info || info->format != gfx::TextureFormat::Rgba8 || info->pixel_width != static_cast<std::uint32_t>(sheet.width) ||
                            info->pixel_height != static_cast<std::uint32_t>(sheet.height);
            if (sheet.refused) {
                LogEvent("colossus", std::format("\"look\":\"refused\",\"sheet\":\"{}\"", sheet.name));
            }
        }
        if (sheet.refused || std::fabs(share - sheet.share) < 1e-4f) {
            continue;
        }
        std::optional<gfx::TextureInfo> info = gfx::GetTextureInfo(sheet.texture);
        Write(sheet, sheet.texture, info ? info->mip_levels : 1u, share);
        sheet.share = share;
    }
}

} // namespace expanded
