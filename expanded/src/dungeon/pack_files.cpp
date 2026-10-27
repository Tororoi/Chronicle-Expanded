#include "expanded/dungeon/pack_files.hpp"

#include <strings.h>

#include <cstdio>
#include <cstring>
#include <unordered_map>

#include "dataalloc.hpp"
#include "dataread.hpp"
#include "dataread_port.hpp"
#include "mglib.hpp"
#include "nowload.hpp"
#include "platform/arena.hpp"
#include "texture.hpp"
#include "tim2.hpp"

namespace expanded {

namespace {

std::unordered_map<std::string, std::vector<unsigned char>> &FileCache() {
    static std::unordered_map<std::string, std::vector<unsigned char>> cache;
    return cache;
}

struct TextureNeed {
    int textures = 0;
    int vram = 0;
};

// What the pack's images take of the texture table and of a block's VRAM, as CTextureManager enters them.
TextureNeed NeedOf(u_int *pack) {
    TextureNeed need;
    char       *name;
    int         size;
    for (int index = 0; u_int *file = GetPackFile(pack, index, &name, &size); index++) {
        std::size_t length = std::strlen(name);
        if (length < 4 || strcasecmp(name + length - 4, ".img") != 0) {
            continue;
        }
        auto   *buffer = reinterpret_cast<u_char *>(file);
        u_char *entry = buffer + 16;
        for (u_int i = 0; i < *reinterpret_cast<u_int *>(buffer + 4); i++, entry += 48) {
            auto *head = reinterpret_cast<TM2_head *>(buffer + *reinterpret_cast<int *>(entry + 32));
            int   width = head->image_width;
            int   height = head->image_height;
            int   bytes;
            switch (head->image_type) {
                case TIM2_RGB16:
                    bytes = width * height * 2;
                    break;
                case TIM2_RGB24:
                    bytes = width * height * 3;
                    break;
                case TIM2_RGB32:
                    bytes = width * height * 4;
                    break;
                case TIM2_IDTEX4:
                    bytes = width * height / 2 + 4 * 256;
                    break;
                case TIM2_IDTEX8:
                    bytes = width * height + 4 * 256;
                    break;
                default:
                    continue;
            }
            need.textures++;
            need.vram += ((bytes >> 8) + 31) / 32 * 32;
        }
    }
    return need;
}

int FreeTextures() {
    int free = 0;
    for (int i = 1; i < 196; i++) {
        free += TexManager.textures[i].name[0] == 0;
    }
    return free;
}

} // namespace

const std::vector<unsigned char> *CachedPackFile(const std::string &path) {
    auto found = FileCache().find(path);
    if (found != FileCache().end()) {
        return &found->second;
    }
    int size = 0;
    if (LoadFile2(const_cast<char *>(path.c_str()), read_buffer, &size, 0) == 0) {
        std::fprintf(stderr, "expanded: cannot read %s\n", path.c_str());
        return nullptr;
    }
    wait_now_loading_vsync();
    auto *bytes = reinterpret_cast<unsigned char *>(read_buffer);
    return &FileCache().emplace(path, std::vector<unsigned char>(bytes, bytes + size)).first->second;
}

unsigned int *ReadPackIntoArena(const std::string &path, CDataAlloc2<1> &arena, int copies) {
    const std::vector<unsigned char> *bytes = CachedPackFile(path);
    if (bytes == nullptr) {
        return nullptr;
    }
    std::size_t pack_bytes = (bytes->size() + 2048 + 63) & ~std::size_t{63};
    int         quads = static_cast<int>((pack_bytes / 16) * copies * kArenaHeadroom) + 4096;
    arena.base = ArenaBlock(&arena, static_cast<std::size_t>(quads) * 16);
    arena.limit = quads;
    arena.used = 0;
    auto *pack = reinterpret_cast<u_int *>(arena.Alloc64(static_cast<int>(pack_bytes / 16)));
    std::memcpy(pack, bytes->data(), bytes->size());
    DataReadTrackPack(pack, bytes->size());
    return pack;
}

bool PackTexturesFit(unsigned int *pack, int block, const char *what) {
    TextureNeed need = NeedOf(pack);
    int         free = FreeTextures() - kTextureReserve;
    int         vram_end = TexManager.blocks[block].vram_end;
    int         vram_left = TexManager.vram_max - (vram_end > mgTopVRAM ? vram_end : mgTopVRAM);
    if (need.textures > free || need.vram > vram_left) {
        std::fprintf(stderr, "expanded: no texture room for %s: needs %d textures (%d free) and %d VRAM blocks (%d left)\n", what,
                     need.textures, free, need.vram, vram_left);
        return false;
    }
    return true;
}

} // namespace expanded
