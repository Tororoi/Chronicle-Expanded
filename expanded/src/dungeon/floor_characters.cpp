#include "expanded/dungeon/floor_characters.hpp"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <string>

#include "btmisc.hpp"
#include "character.hpp"
#include "dataalloc.hpp"
#include "expanded/dungeon/pack_files.hpp"
#include "mglib.hpp"
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

FloorSlots<Slot> &State() {
    static FloorSlots<Slot> store;
    return store;
}

bool PackLoad(CCharacter &chara, CDataAlloc2<1> &arena, const char *path, const char *cfg, bool images) {
    u_int *pack = ReadPackIntoArena(path, arena, 4);
    if (pack == nullptr || (images && !PackTexturesFit(pack, kFloorCharacterBlock, path))) {
        return false;
    }
    chara.Initialize();
    // Without an image arena the pack's images are passed over; none left from an earlier character may stand in for them.
    std::fill(std::begin(chara.images), std::end(chara.images), nullptr);
    // Every floor character's images go in the one block: loading keeps those already there, or the characters entered
    // before would draw with this one's.
    chara.LoadPackData3(pack, const_cast<char *>(cfg), &arena, kFloorCharacterBlock, images ? &arena : nullptr, 1, 0);
    return chara.frame != nullptr;
}

FloorCharacterLoader &Loader() {
    static FloorCharacterLoader loader = nullptr;
    return loader;
}

CCharacter *Entered(const char *path, bool images) {
    for (Slot &slot : State()) {
        if (slot.path == path && slot.images == images) {
            return &slot.chara;
        }
    }
    return nullptr;
}

CCharacter *Enter(const char *path, const char *cfg, bool images) {
    if (CCharacter *entered = Entered(path, images)) {
        return entered;
    }
    FloorSlots<Slot> &store = State();
    Slot             &slot = store.Next();
    bool              loaded = Loader() != nullptr ? Loader()(slot.chara, slot.arena, path, cfg) : PackLoad(slot.chara, slot.arena, path, cfg, images);
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

std::string WeaponPackPath(int weapon_no, std::string &cfg) {
    char chr_name[64] = {}, cfg_name[64] = {};
    BtGetWeaponNamePath3(chr_name, cfg_name, weapon_no);
    cfg = cfg_name;
    return std::string("commenu/weapon/") + chr_name;
}

void DrawFloorCharacter(CCharacter &chara) {
    TexManager.ReloadTexture(Vif1Packet, kFloorCharacterBlock);
    chara.Draw();
}

void FloorCharactersClear() {
    for (Slot &slot : State()) {
        slot.chara.Initialize();
        slot.arena.used = 0;
        slot.path.clear();
    }
    State().live = 0;
    TexManager.DeleteTextureBlock(kFloorCharacterBlock);
}

void SetFloorCharacterLoader(FloorCharacterLoader loader) {
    Loader() = loader;
}

} // namespace expanded
