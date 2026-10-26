#include "expanded/dungeon/floor_characters.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <iterator>
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
    bool           images = true; // its pack's images loaded, not the original's textures shared
};

struct Store {
    std::deque<Slot> slots; // only grows, so no character moves
    int              live = 0;
};

Store &State() {
    static Store store;
    return store;
}

bool PackLoad(CCharacter &chara, CDataAlloc2<1> &arena, const char *path, const char *cfg, bool images) {
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
    if (images && !PackTexturesFit(pack, kFloorCharacterBlock, path)) {
        return false;
    }
    chara.Initialize();
    // Without an image arena the pack's images are passed over; none left from an earlier character may stand in for them.
    std::fill(std::begin(chara.images), std::end(chara.images), nullptr);
    chara.LoadPackData2(pack, const_cast<char *>(cfg), &arena, kFloorCharacterBlock, images ? &arena : nullptr, 0);
    return chara.frame != nullptr;
}

FloorCharacterLoader &Loader() {
    static FloorCharacterLoader loader = nullptr;
    return loader;
}

CCharacter *Entered(const char *path, bool images) {
    Store &store = State();
    for (int i = 0; i < store.live; i++) {
        if (store.slots[i].path == path && store.slots[i].images == images) {
            return &store.slots[i].chara;
        }
    }
    return nullptr;
}

CCharacter *Enter(const char *path, const char *cfg, bool images) {
    if (CCharacter *entered = Entered(path, images)) {
        return entered;
    }
    Store &store = State();
    if (store.live == static_cast<int>(store.slots.size())) {
        store.slots.emplace_back();
    }
    Slot &slot = store.slots[store.live];
    bool  loaded = Loader() != nullptr ? Loader()(slot.chara, slot.arena, path, cfg) : PackLoad(slot.chara, slot.arena, path, cfg, images);
    if (!loaded) {
        slot.chara.Initialize();
        return nullptr;
    }
    slot.path = path;
    slot.images = images;
    std::fprintf(stderr, "floor characters: %s entered%s (%d of %d units)\n", path, images ? "" : " as a copy", slot.arena.used,
                 slot.arena.limit);
    store.live++;
    return &slot.chara;
}

} // namespace

CCharacter *EnterFloorCharacter(const char *path, const char *cfg) {
    return Enter(path, cfg, true);
}

CCharacter *EnterFloorCopy(const char *path, const char *cfg) {
    return Enter(path, cfg, false);
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
    Loader() = loader;
}

} // namespace expanded
