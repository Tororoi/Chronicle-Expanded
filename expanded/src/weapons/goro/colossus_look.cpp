#include "expanded/weapons/goro/colossus_look.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>
#include <optional>
#include <span>
#include <vector>

#include "character.hpp"
#include "data/data_read.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/goro_shirt.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/tall_hammer_silver.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"
#include "gfx/gfx.hpp"
#include "render/texture_registry.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

// One sheet in its own colours and in its copy (RGBA8, the renderer's alpha), whether it fades as he shrinks (the
// hammer's) or as he grows (his body's), and the texture last written with the share it was written at.
struct Sheet {
    const char                *name = nullptr;
    bool                       shrinking = false;
    int                        width = 0, height = 0;
    std::vector<unsigned char> own, copy;
    gfx::TextureHandle         texture = gfx::kNullTexture;
    float                      share = 0.0f;
    bool                       refused = false; // a texture at another size: left as it is
};

struct Look {
    bool               read = false;
    bool               lit = false, dimmed = false;
    std::vector<Sheet> sheets;
};

Look look;

constexpr float kOwnFadeFloor = 0.7f; // CCharacter's own fade[3]

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

// A built pack's sheets and their copies, added to the look (none without the pack).
void ReadPack(const char *file, const char *own_bank, const char *copy_bank, std::span<const char *const> sheets, bool shrinking) {
    std::uintmax_t size = DataFileSize(std::string(kRebuiltDiscFolder) + file);
    if (size == 0) {
        return;
    }
    std::vector<u_int> pack((size + 2 * 2048) / sizeof(u_int));
    char               path[64];
    std::snprintf(path, sizeof(path), "%s", file);
    if (LoadFile2(path, pack.data(), nullptr, 0) != 0) {
        auto bank = [&](const char *name) -> std::optional<ImgBank> {
            char record[32];
            std::snprintf(record, sizeof(record), "%s", name);
            int   bytes = 0;
            auto *data = reinterpret_cast<const unsigned char *>(GetPackFile(pack.data(), record, &bytes));
            if (data == nullptr || bytes <= 0) {
                return std::nullopt;
            }
            return ImgBank::Parse({data, static_cast<std::size_t>(bytes)});
        };
        try {
            std::optional<ImgBank> own = bank(own_bank), copy = bank(copy_bank);
            if (own && copy) {
                bool own_swizzled = std::memcmp(own->magic.data(), "IM2", 3) == 0;
                bool copy_swizzled = std::memcmp(copy->magic.data(), "IM2", 3) == 0;
                for (const char *name : sheets) {
                    tim8::Picture a = tim8::Read(own->Block(name), own_swizzled), b = tim8::Read(copy->Block(name), copy_swizzled);
                    if (a.width == b.width && a.height == b.height) {
                        look.sheets.push_back({name, shrinking, a.width, a.height, Rgba(a), Rgba(b)});
                    }
                }
            }
        } catch (const std::exception &) {
        }
    }
    DataReadForgetPackCapacities(pack.data());
}

// His sheets with the shirt's copies, and the hammer's with its silver one (read once).
void Read() {
    look.read = true;
    ReadPack(kGoroDungeonFile, kGoroBank, kGoroShirtBank, kGoroShirtSheets, false);
    const char *hammer[] = {kTallHammerSheet};
    ReadPack(kTallHammerFile, kTallHammerBank, kTallHammerSilverBank, hammer, true);
}

// The sheet share of the way to its copy, each mip level after the first a box filter of the one before.
void Write(const Sheet &sheet, std::uint32_t levels, float share) {
    std::vector<unsigned char> level(sheet.own.size());
    for (std::size_t i = 0; i < level.size(); i++) {
        level[i] = static_cast<unsigned char>(std::lround(sheet.own[i] + (sheet.copy[i] - sheet.own[i]) * share));
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

// Goro and the hammer in his hand lit share of the way to the full tint: the game's character tint (CCharacter::Draw
// adds ambient_offset to a model's ambient, his cape's cloth drawn inside it). Written while it is lit, and once more as
// it goes out: a member or a weapon it never lit keeps its own.
void Tint(float share) {
    if (share <= 0.0f && !look.lit) {
        return;
    }
    for (CCharacter *model : {&CharaMain, NowWeapon}) {
        if (model == nullptr) {
            continue;
        }
        for (int c = 0; c < 3; c++) {
            model->ambient_offset[c] = kColossusTint[c] * share;
        }
    }
    look.lit = share > 0.0f;
}

// The hammer in his hand dimmed shrink of the way to kColossusHammerDim: the game's own dimming (CCharacter::Draw scales
// a model's ambient and lights by fade[0], its step easing fade[0] toward fade[3] while fade_out is set, as the Terra
// Sword's drop dims a monster). Written while it is dim, and once more as it brightens back.
void Dim(float shrink) {
    if (NowWeapon == nullptr || (shrink <= 0.0f && !look.dimmed)) {
        return;
    }
    if (shrink > 0.0f) {
        float light = 1.0f - (1.0f - kColossusHammerDim) * shrink;
        NowWeapon->fade_out = 1;
        NowWeapon->fade[3] = light;
        NowWeapon->fade[0] = light;
    } else {
        NowWeapon->fade_out = 0;
        NowWeapon->fade[3] = kOwnFadeFloor;
        NowWeapon->fade[0] = 1.0f;
    }
    look.dimmed = shrink > 0.0f;
}

} // namespace

float ColossusLookShare(float size) {
    return std::clamp((size - 1.0f) / (kColossusMost - 1.0f), 0.0f, 1.0f);
}

float ColossusLookShrink(float size) {
    return std::clamp((1.0f - size) / (1.0f - kColossusLeast), 0.0f, 1.0f);
}

void ColossusLookStep(float size) {
    float grown = ColossusLookShare(size), shrunk = ColossusLookShrink(size);
    Tint(grown);
    Dim(shrunk);
    if (!look.read) {
        // Read once his sheets are loaded (none are without the game's data).
        if (TexManager.GetTexture(const_cast<char *>(kGoroShirtSheets[0]), -1) == nullptr) {
            return;
        }
        Read();
    }
    for (Sheet &sheet : look.sheets) {
        float     share = sheet.shrinking ? shrunk : grown;
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
