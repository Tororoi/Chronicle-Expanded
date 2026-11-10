#include "expanded/dungeon/torch_fire.hpp"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/pack_files.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "fireomni.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "runeffect.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

struct alignas(64) Line {
    unsigned char bytes[64];
};

std::vector<Line> image; // the renamed copy, kept while its pictures are in
bool              failed = false;

void OnFloor() {
    failed = false;
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

std::string Named(const char *name) {
    return std::string(kTorchFirePrefix) + name;
}

} // namespace

CTexture *TorchFireTexture(const char *name) {
    std::string named = Named(name);
    return TexManager.GetTexture(named.data(), -1);
}

bool TorchFireReady() {
    if (TorchFireTexture("lightling") != nullptr) {
        return true;
    }
    if (failed) {
        return false;
    }
    failed = true;
    const std::vector<unsigned char> *file = CachedPackFile(kTorchFireImage);
    if (file == nullptr || file->size() < sizeof(IMG_head)) {
        return false;
    }
    image.assign((file->size() + sizeof(Line) - 1) / sizeof(Line), Line{});
    auto *bytes = reinterpret_cast<unsigned char *>(image.data());
    std::memcpy(bytes, file->data(), file->size());
    auto *head = reinterpret_cast<IMG_head *>(bytes);
    auto *entries = reinterpret_cast<IMG_entry *>(head + 1);
    for (u_int i = 0; i < head->pictures; i++) {
        std::string named = Named(entries[i].name);
        if (named.size() >= sizeof(entries[i].name)) {
            return false;
        }
        std::strncpy(entries[i].name, named.c_str(), sizeof(entries[i].name));
    }
    if (!ImageTexturesFit(reinterpret_cast<unsigned int *>(bytes), kFloorCharacterBlock, kTorchFireImage)) {
        return false;
    }
    LOADTEXTURE_INFO2 entries_in[] = {
        {reinterpret_cast<char *>(bytes), kFloorCharacterBlock, 0},
        {nullptr,                         0,                    0},
    };
    TexManager.LoadTextureBlockEX(kFloorCharacterBlock, entries_in);
    failed = TorchFireTexture("lightling") == nullptr;
    return !failed;
}

void TorchFireCreate(const CFireOmni &fire) {
    CTexture *sheet = TorchFireTexture("d01e02");
    CTexture *scroll = TorchFireTexture("d01e03");
    CTexture *blender = TexManager.GetTexture(const_cast<char *>("blender"), -1);
    if (sheet == nullptr || scroll == nullptr || blender == nullptr) {
        return;
    }
    int       cell = static_cast<int>(fire.cell_phase);
    CRect_i_  frame = sheet->bpp == 1 ? CRect_i_((cell % 4) * 64, (cell / 4) * 64, 64, 64) : CRect_i_(0, cell * 64, 64, 64);
    sceGsTex0 blend = *reinterpret_cast<sceGsTex0 *>(&blender->tex0);
    blendTextuer(Vif1Packet, blend.TBP0, blend.TBW, blend.PSM, scroll, CRect_i_(0, 0, 128, 128),
                 CRect_i_(0, static_cast<int>(fire.flame_phase), 128, 64), sheet, CRect_i_(0, 0, 128, 128), frame);
}

} // namespace expanded
