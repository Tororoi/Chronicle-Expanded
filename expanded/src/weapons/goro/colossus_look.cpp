#include "expanded/weapons/goro/colossus_look.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>
#include <optional>
#include <vector>

#include "character.hpp"
#include "cloth.hpp"
#include "data/data_read.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frame_tint.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/data/goro_gold.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"
#include "gfx/gfx.hpp"
#include "render/texture_registry.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

// One body sheet in its own colours and in its gold copy (RGBA8, the renderer's alpha), and the texture last written
// with the share it was written at.
struct Sheet {
    const char                *name = nullptr;
    int                        width = 0, height = 0;
    std::vector<unsigned char> own, gold;
    gfx::TextureHandle         texture = gfx::kNullTexture;
    float                      share = 0.0f;
    bool                       refused = false; // a texture at another size: left as it is
};

struct Look {
    bool               read = false;
    std::vector<Sheet> sheets;
};

Look look;

std::vector<unsigned char> Rgba(const tim8::Picture &picture) {
    std::vector<unsigned char> out(picture.pixels.size() * 4);
    for (std::size_t i = 0; i < picture.pixels.size(); i++) {
        std::size_t j = static_cast<std::size_t>(tim8::Csm1(picture.pixels[i])) * 4;
        out[i * 4] = picture.clut[j];
        out[i * 4 + 1] = picture.clut[j + 1];
        out[i * 4 + 2] = picture.clut[j + 2];
        out[i * 4 + 3] = static_cast<unsigned char>(std::min(255, picture.clut[j + 3] * 2));
    }
    return out;
}

// The sheets and their copies as his built pack has them (read once: none without it).
void Read() {
    look.read = true;
    std::uintmax_t size = DataFileSize(std::string(kRebuiltDiscFolder) + kGoroDungeonFile);
    if (size == 0) {
        return;
    }
    std::vector<u_int> pack((size + 2 * 2048) / sizeof(u_int));
    char               path[64];
    std::snprintf(path, sizeof(path), "%s", kGoroDungeonFile);
    if (LoadFile2(path, pack.data(), nullptr, 0) != 0) {
        auto bank = [&](const char *name) -> std::optional<ImgBank> {
            char file[32];
            std::snprintf(file, sizeof(file), "%s", name);
            int   bytes = 0;
            auto *data = reinterpret_cast<const unsigned char *>(GetPackFile(pack.data(), file, &bytes));
            if (data == nullptr || bytes <= 0) {
                return std::nullopt;
            }
            return ImgBank::Parse({data, static_cast<std::size_t>(bytes)});
        };
        try {
            std::optional<ImgBank> own = bank(kGoroBank), gold = bank(kGoroGoldBank);
            if (own && gold) {
                bool own_swizzled = std::memcmp(own->magic.data(), "IM2", 3) == 0;
                bool gold_swizzled = std::memcmp(gold->magic.data(), "IM2", 3) == 0;
                for (const char *name : kGoroGoldSheets) {
                    tim8::Picture a = tim8::Read(own->Block(name), own_swizzled), b = tim8::Read(gold->Block(name), gold_swizzled);
                    if (a.width == b.width && a.height == b.height) {
                        look.sheets.push_back({name, a.width, a.height, Rgba(a), Rgba(b)});
                    }
                }
            }
        } catch (const std::exception &) {
            look.sheets.clear();
        }
    }
    DataReadForgetPackCapacities(pack.data());
}

// The sheet share of the way to its copy, each mip level after the first a box filter of the one before.
void Write(const Sheet &sheet, std::uint32_t levels, float share) {
    std::vector<unsigned char> level(sheet.own.size());
    for (std::size_t i = 0; i < level.size(); i++) {
        level[i] = static_cast<unsigned char>(std::lround(sheet.own[i] + (sheet.gold[i] - sheet.own[i]) * share));
    }
    int w = sheet.width, h = sheet.height;
    for (std::uint32_t mip = 0; mip < std::max(levels, 1u); mip++) {
        gfx::UpdateTexture(sheet.texture, mip, 0, 0, w, h, level.data());
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
                            sum += level[(static_cast<std::size_t>(std::min(h - 1, y * 2 + dy)) * w + std::min(w - 1, x * 2 + dx)) * 4 + c];
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

// The gold parts lit share of the way to the full tint: the gold frame, the cape's frame and the cloth drawing it.
void Tint(float share) {
    float add[3];
    for (int c = 0; c < 3; c++) {
        add[c] = kColossusTint[c] * share;
    }
    for (const char *name : {kGoroGoldBody, kGoroCape}) {
        if (CFrame *frame = FrameNamed(CharaMain.frame, name); frame != nullptr) {
            SetFrameTint(frame, add[0], add[1], add[2]);
        }
    }
    if (CharaMain.cloth != nullptr) {
        for (int i = 0; i < CHARA_CLOTH_MAX; i++) {
            if (CharaMain.cloth[i] != nullptr) {
                SetVisualTint(CharaMain.cloth[i], add[0], add[1], add[2]);
            }
        }
    }
}

} // namespace

float ColossusLookShare(float size) {
    return std::clamp((size - 1.0f) / (kColossusMost - 1.0f), 0.0f, 1.0f);
}

void ColossusLookStep(float size) {
    float share = ColossusLookShare(size);
    Tint(share);
    if (!look.read) {
        // Read once his sheets are loaded (none are without the game's data).
        if (TexManager.GetTexture(const_cast<char *>(kGoroGoldSheets[0]), -1) == nullptr) {
            return;
        }
        Read();
    }
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
        Write(sheet, info ? info->mip_levels : 1u, share);
        sheet.share = share;
    }
}

} // namespace expanded
