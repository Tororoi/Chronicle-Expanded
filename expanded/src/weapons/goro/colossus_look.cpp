#include "expanded/weapons/goro/colossus_look.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <vector>

#include "character.hpp"
#include "data/data_read.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/disc_files.hpp"
#include "expanded/data/goro_shirt.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/tall_hammer_silver.hpp"
#include "expanded/data/wolf_steve.hpp"
#include "expanded/data/xiao_colossus.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"
#include "gfx/gfx.hpp"
#include "render/texture_registry.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// Where a picture is: a pack (the copy the game reads, dcdata's rebuilt one where there is one), its bank and the
// picture's name in it.
struct Picture {
    const char *file;
    const char *bank;
    const char *name;
};

// A sheet the look recolours: whose it is, the texture it is drawn from (its own picture's name), its own picture and
// its copies as he grows and as he shrinks (none for a sheet the size leaves alone that way).
struct Source {
    int         chara;
    Picture     own;
    const char *grow_file, *grow_bank;
    const char *shrink_file, *shrink_bank;
};

const Source kSources[] = {
    {CHARA_GORO, {kGoroDungeonFile, kGoroBank, kGoroShirtSheets[0]},   kGoroDungeonFile,  kGoroShirtBank, nullptr,           nullptr              },
    {CHARA_GORO, {kGoroDungeonFile, kGoroBank, kGoroShirtSheets[1]},   kGoroDungeonFile,  kGoroShirtBank, nullptr,           nullptr              },
    {CHARA_GORO, {kTallHammerFile, kTallHammerBank, kTallHammerSheet}, nullptr,           nullptr,        kTallHammerFile,   kTallHammerSilverBank},
    {CHARA_XIAO, {kXiaoDungeonFile, kXiaoBank, kXiaoSheet},            kXiaoColossusFile, kXiaoGrowBank,  kXiaoColossusFile, kXiaoShrinkBank      },
    {CHARA_XIAO, {kSuperSteveFile, kSuperSteveBank, kSteveSheet},      nullptr,           nullptr,        kXiaoColossusFile, kSteveShrinkBank     },
};

// A source's pictures (RGBA8, the renderer's alpha; a copy empty where there is none), read once its texture is
// loaded, and the texture last written with the shares it was written at.
struct Sheet {
    bool                       read = false, missing = false;
    int                        width = 0, height = 0;
    std::vector<unsigned char> own, grow, shrink;
    gfx::TextureHandle         texture = gfx::kNullTexture;
    float                      grown = 0.0f, shrunk = 0.0f;
    bool                       refused = false; // a texture at another size: left as it is
};

struct Look {
    bool  lit = false, dimmed = false;
    Sheet sheets[std::size(kSources)];
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

// A picture from a pack's bank (the pack the game would read for file), or none.
std::optional<tim8::Picture> ReadPicture(const char *file, const char *bank_name, const char *name) {
    std::uintmax_t size = DataFileSize(DiscFile(file)); // what LoadFile2 reads: the rebuilt copy where there is one
    if (size == 0) {
        return std::nullopt;
    }
    std::optional<tim8::Picture> out;
    std::vector<u_int>           pack((size + 2 * 2048) / sizeof(u_int));
    char                         path[96];
    std::snprintf(path, sizeof(path), "%s", file);
    if (LoadFile2(path, pack.data(), nullptr, 0) != 0) {
        char record[32];
        std::snprintf(record, sizeof(record), "%s", bank_name);
        int   bytes = 0;
        auto *data = reinterpret_cast<const unsigned char *>(GetPackFile(pack.data(), record, &bytes));
        if (data != nullptr && bytes > 0) {
            try {
                ImgBank bank = ImgBank::Parse({data, static_cast<std::size_t>(bytes)});
                out = tim8::Read(bank.Block(name), std::memcmp(bank.magic.data(), "IM2", 3) == 0);
            } catch (const std::exception &) {
                out.reset();
            }
        }
    }
    DataReadForgetPackCapacities(pack.data());
    return out;
}

// A source's pictures, its copies the same size as its own.
void Read(const Source &source, Sheet &sheet) {
    sheet.read = true;
    std::optional<tim8::Picture> own = ReadPicture(source.own.file, source.own.bank, source.own.name);
    if (!own) {
        sheet.missing = true;
        return;
    }
    sheet.width = own->width;
    sheet.height = own->height;
    sheet.own = Rgba(*own);
    auto copy = [&](const char *file, const char *bank) -> std::vector<unsigned char> {
        if (file == nullptr) {
            return {};
        }
        std::optional<tim8::Picture> picture = ReadPicture(file, bank, source.own.name);
        return picture && picture->width == sheet.width && picture->height == sheet.height ? Rgba(*picture) : std::vector<unsigned char>{};
    };
    sheet.grow = copy(source.grow_file, source.grow_bank);
    sheet.shrink = copy(source.shrink_file, source.shrink_bank);
}

// The sheet grown of the way to its grow copy and shrunk of the way to its shrink copy (only one is ever more than 0),
// each mip level after the first a box filter of the one before.
void Write(const Sheet &sheet, std::uint32_t levels, float grown, float shrunk) {
    std::vector<unsigned char> level(sheet.own.size());
    for (std::size_t i = 0; i < level.size(); i++) {
        float v = sheet.own[i];
        if (!sheet.grow.empty()) {
            v += (sheet.grow[i] - sheet.own[i]) * grown;
        }
        if (!sheet.shrink.empty()) {
            v += (sheet.shrink[i] - sheet.own[i]) * shrunk;
        }
        level[i] = static_cast<unsigned char>(std::clamp(std::lround(v), 0L, 255L));
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

// The member in play and the weapon in their hand lit share of the way to the full tint: the game's character tint
// (CCharacter::Draw adds ambient_offset to a model's ambient, a cape's cloth drawn inside it). Written while it is lit,
// and once more as it goes out: a member or a weapon it never lit keeps its own.
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

// The weapon in their hand dimmed shrink of the way to kColossusHammerDim: the game's own dimming (CCharacter::Draw
// scales a model's ambient and lights by fade[0], its step easing fade[0] toward fade[3] while fade_out is set, as the
// Terra Sword's drop dims a monster). Written while it is dim, and once more as it brightens back.
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
    for (std::size_t s = 0; s < std::size(kSources); s++) {
        const Source &source = kSources[s];
        Sheet        &sheet = look.sheets[s];
        CTexture     *texture = TexManager.GetTexture(const_cast<char *>(source.own.name), -1);
        if (texture == nullptr) {
            continue; // not loaded (none are without the game's data)
        }
        if (!sheet.read) {
            Read(source, sheet);
        }
        if (sheet.missing) {
            continue;
        }
        dc::render::PortTextureRef ref = dc::render::PortTextureFromCTexture(texture);
        if (!ref.valid || ref.binding.texture == gfx::kNullTexture) {
            continue;
        }
        if (ref.binding.texture != sheet.texture) {
            // A texture as loaded: its own colours, and checked once for its size.
            sheet.texture = ref.binding.texture;
            sheet.grown = sheet.shrunk = 0.0f;
            std::optional<gfx::TextureInfo> info = gfx::GetTextureInfo(sheet.texture);
            sheet.refused = !info || info->format != gfx::TextureFormat::Rgba8 || info->pixel_width != static_cast<std::uint32_t>(sheet.width) ||
                            info->pixel_height != static_cast<std::uint32_t>(sheet.height);
            if (sheet.refused) {
                LogEvent("colossus", std::format("\"look\":\"refused\",\"sheet\":\"{}\"", source.own.name));
            }
        }
        // Only the member in play's own sheets take the size.
        bool  theirs = UserStatus != nullptr && UserStatus->cur_chara == source.chara;
        float g = theirs ? grown : 0.0f, k = theirs ? shrunk : 0.0f;
        if (sheet.refused || (std::fabs(g - sheet.grown) < 1e-4f && std::fabs(k - sheet.shrunk) < 1e-4f)) {
            continue;
        }
        std::optional<gfx::TextureInfo> info = gfx::GetTextureInfo(sheet.texture);
        Write(sheet, info ? info->mip_levels : 1u, g, k);
        sheet.grown = g;
        sheet.shrunk = k;
    }
}

} // namespace expanded
