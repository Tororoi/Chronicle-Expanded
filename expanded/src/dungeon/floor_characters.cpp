#include "expanded/dungeon/floor_characters.hpp"

#include <cstdio>
#include <cstring>
#include <deque>
#include <string>

#include "character.hpp"
#include "dataalloc.hpp"
#include "dataread_port.hpp"
#include "expanded/dungeon/pack_files.hpp"
#include "platform/arena.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

// A character with the arena its model, motions, skins and images live in.
struct Slot {
    CCharacter     chara;
    CDataAlloc2<1> arena{-1};
    std::string    path;
};

struct Store {
    std::deque<Slot> slots; // only grows, so no character moves
    int              live = 0;
};

Store &State() {
    static Store store;
    return store;
}

bool PackLoad(CCharacter &chara, CDataAlloc2<1> &arena, const char *path, const char *cfg) {
    const std::vector<unsigned char> *bytes = CachedPackFile(path);
    if (bytes == nullptr) {
        return false;
    }
    std::size_t pack_bytes = (bytes->size() + 2048 + 63) & ~std::size_t{63};
    // The pack, then the model, motions, skins and images it copies out.
    int quads = static_cast<int>((pack_bytes / 16) * 4 * kArenaHeadroom) + 4096;
    arena.base = ArenaBlock(&arena, static_cast<std::size_t>(quads) * 16);
    arena.limit = quads;
    arena.used = 0;
    auto *pack = reinterpret_cast<u_int *>(arena.Alloc64(static_cast<int>(pack_bytes / 16)));
    std::memcpy(pack, bytes->data(), bytes->size());
    DataReadTrackPack(pack, bytes->size());
    if (!PackTexturesFit(pack, kFloorCharacterBlock, path)) {
        return false;
    }
    chara.Initialize();
    chara.LoadPackData2(pack, const_cast<char *>(cfg), &arena, kFloorCharacterBlock, &arena, 0);
    return chara.frame != nullptr;
}

FloorCharacterLoader &Loader() {
    static FloorCharacterLoader loader = PackLoad;
    return loader;
}

CCharacter *Entered(const char *path) {
    Store &store = State();
    for (int i = 0; i < store.live; i++) {
        if (store.slots[i].path == path) {
            return &store.slots[i].chara;
        }
    }
    return nullptr;
}

} // namespace

CCharacter *EnterFloorCharacter(const char *path, const char *cfg) {
    if (CCharacter *entered = Entered(path)) {
        return entered;
    }
    Store &store = State();
    if (store.live == static_cast<int>(store.slots.size())) {
        store.slots.emplace_back();
    }
    Slot &slot = store.slots[store.live];
    if (!Loader()(slot.chara, slot.arena, path, cfg)) {
        slot.chara.Initialize();
        return nullptr;
    }
    slot.path = path;
    std::fprintf(stderr, "floor characters: %s entered (%d of %d units)\n", path, slot.arena.used, slot.arena.limit);
    store.live++;
    return &slot.chara;
}

void FloorCharactersClear() {
    Store &store = State();
    for (int i = 0; i < store.live; i++) {
        store.slots[i].chara.Initialize();
        store.slots[i].arena.used = 0;
        store.slots[i].path.clear();
    }
    store.live = 0;
    TexManager.DeleteTextureBlock(kFloorCharacterBlock);
}

void SetFloorCharacterLoader(FloorCharacterLoader loader) {
    Loader() = loader != nullptr ? loader : PackLoad;
}

} // namespace expanded
