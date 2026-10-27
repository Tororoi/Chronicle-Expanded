#include "expanded/dungeon/shot_effects.hpp"

#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "collisiondata.hpp"
#include "dataalloc.hpp"
#include "dataread.hpp"
#include "dataset.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/pack_files.hpp"
#include "frame.hpp"
#include "itembombeffect.hpp"
#include "itemdata.hpp"
#include "main.hpp"
#include "mathutil.hpp"
#include "mglib.hpp"
#include "monstorunit.hpp"
#include "nowload.hpp"
#include "shot_effect_pack.hpp"
#include "snd.hpp"
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

struct Store : FloorSlots<Slot> {
    bool floor = false;
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

std::string PathOf(const BT_SHOT_EFFECT *effect, const char *path) {
    return path != nullptr ? std::string(path) : "dun/effect/" + std::string(effect->model_name) + ".chr";
}

// Reads the effect's pack from the cache into its own arena and sets the effect up from it.
bool DiscLoad(CSHOT_EFFECT &effect, CDataAlloc2<1> &arena, BT_SHOT_EFFECT *description, int shots, const char *path) {
    std::string file = PathOf(description, path);
    u_int      *pack = ReadPackIntoArena(file, arena, 4 + 2 * shots);
    if (pack == nullptr || !PackTexturesFit(pack, kShotEffectBlock, file.c_str())) {
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
    int index = 0;
    for (Slot &slot : State()) {
        if (slot.effect.effect_data == effect) {
            return index;
        }
        index++;
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

CSHOT_EFFECT *LoadedShotEffect(int index) {
    if (index < 0 || index >= ShotEffectCount() || (index < kRetailShotEffects && NowShotEffect == nullptr)) {
        return nullptr;
    }
    return ShotEffectAt(index);
}

int FireShotEffect(int index, float *position, float *target, int owner) {
    CSHOT_EFFECT *effect = LoadedShotEffect(index);
    if (effect == nullptr) {
        return -1;
    }
    NowShotEffect->Set(index, position, target);
    if (effect->current_slot >= 0 && owner >= 0) {
        NowShotEffect->SetUserID2(owner);
    }
    return effect->current_slot;
}

int SpawnStill(CSHOT_EFFECT &effect, const float *at, int user, int phase) {
    float from[4] = {at[0], at[1], at[2], 1.0f};
    float to[4] = {at[0], at[1] + 1.0f, at[2], 1.0f};
    effect.Set(from, to, user, -1, 0, nullptr, phase);
    int slot = effect.current_slot;
    if (slot >= 0) {
        StopShot(effect, slot);
    }
    return slot;
}

void ShotEffectStrike(CSHOT_EFFECT &effect, int slot, float *at) {
    BT_SHOT_EFFECT *data = effect.effect_data;
    effect.chara[slot].SetPosition(at);
    effect.phase[slot] = 2;
    int motion = data->motion[2];
    if (motion == -1) {
        effect.active[slot] = 0;
        if (data->end_effect == SHOT_END_EFFECT_BOMB) {
            ShotEffectBomb(at, data, effect.user_id_2[slot]); // the owner's damage and the shot's radius
        }
        return;
    }
    effect.chara[slot].motion_type.state.time = (float) effect.chara[slot].motion_type.motion_info[motion].start;
    effect.chara[slot].SetMotion(motion, 6);
    sceVu0Normalize(effect.velocity[slot], effect.velocity[slot]);
    sceVu0ScaleVectorXYZ(effect.velocity[slot], effect.velocity[slot], data->speed[2]);
    if (data->sound[2] != -1 && effect.no_sound[slot] == 0) {
        SndSePlay(data->sound[2], -1, 0);
    }
}

bool ShotEffectStrikeHit(int hit) {
    COLLISION_HIT &record = NowColData->hit[hit];
    if (NowShotEffect == nullptr || record.monster_no < 0) {
        return false;
    }
    CSHOT_EFFECT *found = nullptr;
    int           slot = -1;
    float         best = 0.0f;
    for (int index = 0; index < ShotEffectCount(); index++) {
        CSHOT_EFFECT &effect = *ShotEffectAt(index);
        if (effect.effect_data == nullptr) {
            continue;
        }
        for (int i = 0; i < effect.slot_count && i < 8; i++) {
            if (effect.active[i] == 0 || effect.phase[i] >= 2 || effect.user_id_2[i] != record.monster_no) {
                continue;
            }
            sceVu0FVECTOR at;
            effect.chara[i].GetPosition(at);
            float distance = DistVector(at, record.pos);
            float reach = 1.0f + std::abs(effect.effect_data->speed[effect.phase[i]]);
            if (distance <= reach && (found == nullptr || distance < best)) {
                found = &effect;
                slot = i;
                best = distance;
            }
        }
    }
    if (found == nullptr) {
        return false;
    }
    ShotEffectStrike(*found, slot, record.pos);
    return true;
}

void StopShot(CSHOT_EFFECT &effect, int slot) {
    effect.velocity[slot][0] = effect.velocity[slot][1] = effect.velocity[slot][2] = 0.0f;
}

void EnterShotPhase(CSHOT_EFFECT &effect, int slot, int phase) {
    effect.phase[slot] = static_cast<s16>(phase);
    int motion = effect.effect_data->motion[phase];
    if (motion == -1) {
        effect.active[slot] = 0;
        return;
    }
    effect.chara[slot].motion_type.state.time = static_cast<float>(effect.chara[slot].motion_type.motion_info[motion].start);
    effect.chara[slot].SetMotion(motion, 6);
    StopShot(effect, slot);
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
    Slot  &slot = store.Next();
    if (!Loader()(slot.effect, slot.arena, effect, shots, path)) {
        slot.effect.Initialize();
        return -1;
    }
    std::fprintf(stderr, "shot effects: %s in pack %d (%d shots)\n", effect->model_name, kRetailShotEffects + store.live, shots);
    return kRetailShotEffects + store.live++;
}

int EnterOwnShot(CMonstorUnit &unit, int model_slot, BT_SHOT_EFFECT *effect, int shots, const char *path) {
    int index = EnterShotEffect(effect, shots, path);
    if (index >= 0) {
        unit.model[model_slot].shot_effect[0] = static_cast<s16>(index);
    }
    return index;
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
    for (Slot &slot : store) {
        slot.effect.Initialize();
        slot.arena.used = 0;
    }
    store.live = 0;
    store.floor = false;
    TexManager.DeleteTextureBlock(kShotEffectBlock);
}

void ShotEffectsReset() {
    for (Slot &slot : State()) {
        slot.effect.OffEffect(-1);
    }
}

void ShotEffectsFloorLoaded() {
    State().floor = true;
    // Every registered file is read now, so a load mid-floor reads no disc.
    for (Registered &entry : Registry()) {
        if (Loader() == DiscLoad) {
            CachedPackFile(PathOf(entry.effect, entry.path.empty() ? nullptr : entry.path.c_str()));
        }
    }
    ShotEffectsWeaponsChanged();
}

void ShotEffectsStep() {
    for (Slot &slot : State()) {
        slot.effect.Step();
    }
}

void ShotEffectsDraw() {
    for (Slot &slot : State()) {
        slot.effect.Draw();
    }
}

void SetShotEffectLoader(ShotEffectLoader loader) {
    Loader() = loader != nullptr ? loader : DiscLoad;
}

namespace {

constexpr float kUnlitFull = 128.0f; // an unlit frame's own colour

void FadeUnlit(CFrame *frame, float k, int depth) {
    for (; frame != nullptr && depth <= 16; frame = frame->brother) {
        if (frame->attr.use_color != 0) {
            for (float &channel : frame->attr.color) {
                channel = kUnlitFull * k;
            }
        }
        FadeUnlit(frame->child, k, depth + 1);
    }
}

std::map<const BT_SHOT_EFFECT *, BlastDamage> &BlastDamages() {
    static std::map<const BT_SHOT_EFFECT *, BlastDamage> damages;
    return damages;
}

} // namespace

void SetBlastDamage(const BT_SHOT_EFFECT *effect, BlastDamage damage) {
    if (damage != nullptr) {
        BlastDamages()[effect] = damage;
    } else {
        BlastDamages().erase(effect);
    }
}

void FadeUnlitFrames(CFrame *frame, float k) {
    FadeUnlit(frame, k, 0);
}

int ShotEffectBomb(float *position, const BT_SHOT_EFFECT *effect, int owner) {
    int  damage = effect->bomb_damage;
    auto found = BlastDamages().find(effect);
    if (found != BlastDamages().end() && owner >= 0) {
        damage = found->second(owner);
    }
    int slot = SetBombEffect(position, effect->target, damage, effect->bomb_scale);
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
