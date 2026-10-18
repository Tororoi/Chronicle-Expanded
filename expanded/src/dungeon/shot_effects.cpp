#include "expanded/dungeon/shot_effects.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include "collisiondata.hpp"
#include "dataalloc.hpp"
#include "dataread.hpp"
#include "dataset.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/species.hpp"
#include "itembombeffect.hpp"
#include "itemdata.hpp"
#include "main.hpp"
#include "mglib.hpp"
#include "monstorunit.hpp"
#include "nowload.hpp"
#include "platform/arena.hpp"
#include "shot_effect_pack.hpp"
#include "texture.hpp"
#include "tim2.hpp"
#include "userstatus.hpp"

extern s32 mode;

namespace expanded {
namespace {

// One of Expanded's effects, with the arena its model and images live in.
struct Slot {
    CSHOT_EFFECT   effect;
    CDataAlloc2<1> arena{-1};
};

struct Store {
    std::deque<Slot> slots; // only grows, so no effect moves
    int              live = 0;
    bool             floor = false;
};

Store &State() {
    static Store store;
    return store;
}

struct Registered {
    int             weapon_no;
    WeaponShotForm  form;
    BT_SHOT_EFFECT *effect;
    int             shots;
    std::string     path;
};

std::vector<Registered> &Registry() {
    static std::vector<Registered> registry;
    return registry;
}

std::unordered_map<std::string, std::vector<unsigned char>> &FileCache() {
    static std::unordered_map<std::string, std::vector<unsigned char>> cache;
    return cache;
}

std::string PathOf(const BT_SHOT_EFFECT *effect, const char *path) {
    return path != nullptr ? std::string(path) : "dun/effect/" + std::string(effect->model_name) + ".chr";
}

// The file's bytes, read through read_buffer the first time and kept for the session.
const std::vector<unsigned char> *CachedFile(const std::string &path) {
    auto found = FileCache().find(path);
    if (found != FileCache().end()) {
        return &found->second;
    }
    int size = 0;
    if (LoadFile2(const_cast<char *>(path.c_str()), read_buffer, &size, 0) == 0) {
        std::fprintf(stderr, "shot effects: cannot read %s\n", path.c_str());
        return nullptr;
    }
    wait_now_loading_vsync();
    auto *bytes = reinterpret_cast<unsigned char *>(read_buffer);
    return &FileCache().emplace(path, std::vector<unsigned char>(bytes, bytes + size)).first->second;
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

// Reads the effect's pack from the cache into its own arena and sets the effect up from it.
bool DiscLoad(CSHOT_EFFECT &effect, CDataAlloc2<1> &arena, BT_SHOT_EFFECT *description, int shots, const char *path) {
    std::string                       file = PathOf(description, path);
    const std::vector<unsigned char> *bytes = CachedFile(file);
    if (bytes == nullptr) {
        return false;
    }
    std::size_t pack_bytes = (bytes->size() + 2048 + 63) & ~std::size_t{63};
    // The pack, then the model, motions and images it copies out, then each shot's frame copy.
    int quads = static_cast<int>((pack_bytes / 16) * (4 + 2 * shots) * kArenaHeadroom) + 4096;
    arena.base = ArenaBlock(&arena, static_cast<std::size_t>(quads) * 16);
    arena.limit = quads;
    arena.used = 0;
    auto *pack = reinterpret_cast<u_int *>(arena.Alloc64(static_cast<int>(pack_bytes / 16)));
    std::memcpy(pack, bytes->data(), bytes->size());

    TextureNeed need = NeedOf(pack);
    int         free = FreeTextures() - kTextureReserve;
    int         vram_end = TexManager.blocks[kShotEffectBlock].vram_end;
    int         vram_left = TexManager.vram_max - (vram_end > mgTopVRAM ? vram_end : mgTopVRAM);
    if (need.textures > free || need.vram > vram_left) {
        std::fprintf(stderr, "shot effects: no texture room for %s: needs %d textures (%d free) and %d VRAM blocks (%d left)\n",
                     file.c_str(), need.textures, free, need.vram, vram_left);
        return false;
    }
    effect.Initialize();
    return effect.Entry2(description, pack, kShotEffectBlock, &arena, shots) != 0;
}

ShotEffectLoader &Loader() {
    static ShotEffectLoader loader = DiscLoad;
    return loader;
}

int StoreIndex(const BT_SHOT_EFFECT *effect) {
    Store &store = State();
    for (int i = 0; i < store.live; i++) {
        if (store.slots[i].effect.effect_data == effect) {
            return i;
        }
    }
    return -1;
}

bool InDungeon() {
    return State().floor && mode == GAME_MODE_DUNGEON && UserStatus != nullptr;
}

void EnterRegistered(int weapon_no, WeaponShotForm form) {
    for (Registered &entry : Registry()) {
        if (entry.weapon_no == weapon_no && entry.form == form) {
            EnterShotEffect(entry.effect, entry.shots, entry.path.empty() ? nullptr : entry.path.c_str());
        }
    }
}

} // namespace

int ShotEffectCount() {
    return kRetailShotEffects + State().live;
}

CSHOT_EFFECT *ShotEffectAt(int index) {
    if (index >= 0 && index < kRetailShotEffects) {
        return &NowShotEffect->effect[index];
    }
    Store &store = State();
    if (index >= kRetailShotEffects && index - kRetailShotEffects < store.live) {
        return &store.slots[index - kRetailShotEffects].effect;
    }
    static CSHOT_EFFECT empty;
    empty.Initialize();
    return &empty;
}

int ShotEffectIndex(const BT_SHOT_EFFECT *effect) {
    if (effect == nullptr) {
        return -1;
    }
    if (NowShotEffect != nullptr) {
        for (int i = 0; i < kRetailShotEffects; i++) {
            if (NowShotEffect->effect[i].effect_data == effect) {
                return i;
            }
        }
    }
    int index = StoreIndex(effect);
    return index < 0 ? -1 : kRetailShotEffects + index;
}

int EnterShotEffect(BT_SHOT_EFFECT *effect, int shots, const char *path) {
    int index = ShotEffectIndex(effect);
    if (index >= 0 || effect == nullptr) {
        return index;
    }
    Store &store = State();
    if (store.live == static_cast<int>(store.slots.size())) {
        store.slots.emplace_back();
    }
    Slot &slot = store.slots[store.live];
    if (!Loader()(slot.effect, slot.arena, effect, shots, path)) {
        slot.effect.Initialize();
        return -1;
    }
    std::fprintf(stderr, "shot effects: %s in pack %d (%d shots)\n", effect->model_name, kRetailShotEffects + store.live, shots);
    return kRetailShotEffects + store.live++;
}

int EnterTableShotEffect(int config, int shots) {
    if (config < 0 || config >= static_cast<int>(std::size(BtEntryEffectTbl))) {
        return -1;
    }
    return EnterShotEffect(BtEntryEffectTbl[config], shots);
}

int EnterMonsterShotEffect(int model_no, int which) {
    if (Species(model_no) == nullptr) {
        return -1;
    }
    const MONSTOR_MODEL &model = *Species(model_no);
    return EnterTableShotEffect(model.shot_effect[which], model.kind == MONSTER_KIND_NO_LOCK_ON ? 6 : 2);
}

void EnterModelShotEffects(CMonstorUnit &unit, int model_slot, BT_SHOT_EFFECT *first, BT_SHOT_EFFECT *second, int shots) {
    BT_SHOT_EFFECT *effects[2] = {first, second};
    for (int i = 0; i < 2; i++) {
        if (effects[i] != nullptr) {
            int index = EnterShotEffect(effects[i], shots);
            if (index >= 0) {
                unit.model[model_slot].shot_effect[i] = static_cast<s16>(index);
            }
        }
    }
}

void RegisterWeaponShotEffect(int weapon_no, WeaponShotForm form, BT_SHOT_EFFECT *effect, int shots, const char *path) {
    Registry().push_back({weapon_no, form, effect, shots, path != nullptr ? path : ""});
}

void ShotEffectsWeaponsChanged() {
    if (!InDungeon()) {
        return;
    }
    for (auto &chara : UserStatus->chara_weapons) {
        for (WEAPON_HAVE &weapon : chara) {
            if (weapon.item_no < ITEM_WEAPON_START) {
                continue;
            }
            EnterRegistered(weapon.item_no, WeaponShotForm::Weapon);
            if (weapon.item_no != ITEM_WEAPON_SUPER_STEVE) {
                continue;
            }
            for (ATTACH_LIST &attach : weapon.attach) {
                if (attach.item_no == ITEM_ATTACH_SYNTHESIS_SPHERE) {
                    EnterRegistered(attach.sphere_weapon_no, WeaponShotForm::Sphere);
                }
            }
        }
    }
}

void ShotEffectsClear() {
    Store &store = State();
    for (int i = 0; i < store.live; i++) {
        store.slots[i].effect.Initialize();
        store.slots[i].arena.used = 0;
    }
    store.live = 0;
    store.floor = false;
    TexManager.DeleteTextureBlock(kShotEffectBlock);
}

void ShotEffectsReset() {
    Store &store = State();
    for (int i = 0; i < store.live; i++) {
        store.slots[i].effect.OffEffect(-1);
    }
}

void ShotEffectsFloorLoaded() {
    State().floor = true;
    // Every registered file is read now, so a load mid-floor reads no disc.
    for (Registered &entry : Registry()) {
        if (Loader() == DiscLoad) {
            CachedFile(PathOf(entry.effect, entry.path.empty() ? nullptr : entry.path.c_str()));
        }
    }
    ShotEffectsWeaponsChanged();
}

void ShotEffectsStep() {
    Store &store = State();
    for (int i = 0; i < store.live; i++) {
        store.slots[i].effect.Step();
    }
}

void ShotEffectsDraw() {
    Store &store = State();
    for (int i = 0; i < store.live; i++) {
        store.slots[i].effect.Draw();
    }
}

void SetShotEffectLoader(ShotEffectLoader loader) {
    Loader() = loader != nullptr ? loader : DiscLoad;
}

int ShotEffectBomb(float *position, const BT_SHOT_EFFECT *effect) {
    int slot = SetBombEffect(position, effect->target, effect->bomb_damage, effect->bomb_scale);
    int per_scale = effect->unk_056[0] | (effect->unk_056[1] << 8);
    if (slot == -1 || per_scale == 0) {
        return slot;
    }
    COLLISION_HIT &hit = NowColData->hit[NowColData->now_hit];
    hit.radius = per_scale * effect->bomb_scale;
    if (effect->bomb_scale > 1.0f) {
        NowShockWave->base_radius = NowShockWave->expand_radius = per_scale * effect->bomb_scale;
    }
    // Away from the blast; a blast on the player throws him straight back from the way he faces.
    float dx = CharaMain.pos[0] - position[0], dz = CharaMain.pos[2] - position[2];
    float length = std::sqrt(dx * dx + dz * dz);
    if (length < 0.01f) {
        dx = -std::sin(CharaMain.rotation.y), dz = -std::cos(CharaMain.rotation.y), length = 1.0f;
    }
    hit.velocity[0] = dx / length;
    hit.velocity[1] = 0.0f;
    hit.velocity[2] = dz / length;
    return slot;
}

} // namespace expanded

// Retail's search and fill of its five slots, then Expanded's part of the pack in place of a refusal.
PORT_OVERRIDE int CSHOT_EFFECT_PACK::Entry(BT_SHOT_EFFECT *description, unsigned int *resource, int texture_block, CDataAlloc2<1> *allocator, int slots) {
    int index = expanded::ShotEffectIndex(description);
    if (index >= 0) {
        return index;
    }
    for (index = 0; index < expanded::kRetailShotEffects; index++) {
        if (effect[index].Entry(description, resource, texture_block, allocator, slots) != 0) {
            return index;
        }
    }
    return expanded::EnterShotEffect(description, slots);
}
