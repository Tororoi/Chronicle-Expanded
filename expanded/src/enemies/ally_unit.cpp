#include "expanded/enemies/ally_unit.hpp"

#include <libgraph.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include "dataalloc.hpp"
#include "dataread_port.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/sounds.hpp"
#include "monstorunit.hpp"
#include "platform/arena.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "texture.hpp"
#include "texture_port.hpp"
#include "textureanime.hpp"

namespace expanded {

namespace {

// Its kind's model slot in its unit.
constexpr int kModelSlot = 0;

// The texture blocks its load enters images into: the monsters' (its model's, SetupBaseModel's own) and the shot
// effects' (its shots', as BtLoadMonstor has the floor's kinds enter theirs).
constexpr int kMonsterBlock = 0x2A, kShotBlock = 0x26;

// Its script's working memory, a floor monster's (MonstorScriptBuffer's), and the memory its model, motions, images,
// script copy and shot data load into: a pockle takes about 35,000 quadwords of it, this over three times that.
constexpr int kScriptQuads = 1875;
constexpr int kModelQuads = 0x20000;

constexpr int kTextureEntries = sizeof(CTextureManager::textures) / sizeof(CTextureManager::textures[0]);
constexpr int kTexAnimeRecords = sizeof(MonsterTexAnim) / sizeof(MonsterTexAnim[0]);

// What its load added: a texture entry (where, its name and its keys, to know it is still its own), a texture
// animation record, a shot pack slot it filled (and the description it filled it with), and a block's VRAM ends before
// and after.
struct Texture {
    int    index;
    char   name[32];
    u_long tex0;
};

struct ShotSlot {
    int             index;
    BT_SHOT_EFFECT *config;
};

struct BlockEnds {
    int block;
    int top, end;             // before its load
    int after_top, after_end; // after it
};

struct Ally {
    std::unique_ptr<CMonstorUnit> unit;
    CMonstorUnit                 *floor = nullptr;
    CDataAlloc2<1>                model{-1};
    CDataAlloc2<1>                script{-1};
    bool                          standing = false;
    std::vector<Texture>          textures;
    std::vector<int>              anims;
    std::vector<ShotSlot>         shots;
    std::vector<BlockEnds>        ends;
};

Ally &State() {
    static Ally ally;
    return ally;
}

// NowMonstorUnit on the ally's unit for a scope: its script commands read it, and the floor's features pass it by.
class OnAlly {
public:
    explicit OnAlly(CMonstorUnit &unit) : saved_(NowMonstorUnit) { NowMonstorUnit = &unit; }

    ~OnAlly() { NowMonstorUnit = saved_; }

    OnAlly(const OnAlly &) = delete;
    OnAlly &operator=(const OnAlly &) = delete;

private:
    CMonstorUnit *saved_;
};

// What is taken before a load, to tell what the load added.
struct Taken {
    std::array<bool, kTextureEntries>    textures{};
    std::array<bool, kTexAnimeRecords>   anims{};
    std::array<bool, kRetailShotEffects> shots{};
    int                                  ends[2][2] = {};
};

constexpr int kBlocks[2] = {kMonsterBlock, kShotBlock};

Taken TakenNow() {
    Taken taken;
    for (int i = 0; i < kTextureEntries; i++) {
        taken.textures[i] = TexManager.textures[i].name[0] != 0;
    }
    for (int i = 0; i < kTexAnimeRecords; i++) {
        taken.anims[i] = MonsterTexAnim[i].kind != -1;
    }
    for (int i = 0; NowShotEffect != nullptr && i < kRetailShotEffects; i++) {
        taken.shots[i] = NowShotEffect->effect[i].effect_data != nullptr;
    }
    for (int b = 0; b < 2; b++) {
        taken.ends[b][0] = TexManager.blocks[kBlocks[b]].vram_top;
        taken.ends[b][1] = TexManager.blocks[kBlocks[b]].vram_end;
    }
    return taken;
}

void NoteAdded(Ally &ally, const Taken &before) {
    for (int i = 0; i < kTextureEntries; i++) {
        const CTexture &texture = TexManager.textures[i];
        if (!before.textures[i] && texture.name[0] != 0) {
            Texture added = {i, {}, texture.tex0};
            std::strncpy(added.name, texture.name, sizeof(added.name) - 1);
            ally.textures.push_back(added);
        }
    }
    for (int i = 0; i < kTexAnimeRecords; i++) {
        if (!before.anims[i] && MonsterTexAnim[i].kind != -1) {
            ally.anims.push_back(i);
        }
    }
    for (int i = 0; NowShotEffect != nullptr && i < kRetailShotEffects; i++) {
        if (!before.shots[i] && NowShotEffect->effect[i].effect_data != nullptr) {
            ally.shots.push_back({i, NowShotEffect->effect[i].effect_data});
        }
    }
    for (int b = 0; b < 2; b++) {
        const CTextureBlock &block = TexManager.blocks[kBlocks[b]];
        ally.ends.push_back({kBlocks[b], before.ends[b][0], before.ends[b][1], block.vram_top, block.vram_end});
    }
}

// A texture entry emptied and its renderer keys let go, as DeleteTextureBlock empties a block's.
void ReleaseTexture(CTexture &texture) {
    const sceGsTex0 &tex0 = *reinterpret_cast<const sceGsTex0 *>(&texture.tex0);
    PortReleaseKey(tex0.TBP0);
    if (tex0.CBP != 0) {
        PortReleaseKey(tex0.CBP);
    }
    texture.Initialize();
}

// What its load added, taken back where it is still its own: a floor's reload and an event empty the texture blocks and
// the shot pack first (and reset the texture animations) before anything new loads.
void Release(Ally &ally) {
    for (const Texture &added : ally.textures) {
        CTexture &texture = TexManager.textures[added.index];
        if (texture.tex0 == added.tex0 && std::strncmp(texture.name, added.name, sizeof(added.name)) == 0) {
            ReleaseTexture(texture);
        }
    }
    for (const BlockEnds &ends : ally.ends) {
        CTextureBlock &block = TexManager.blocks[ends.block];
        if (block.vram_top == ends.after_top && block.vram_end == ends.after_end) {
            block.vram_top = ends.top;
            block.vram_end = ends.end;
        }
    }
    for (int index : ally.anims) {
        MonsterTexAnim[index].Initialize();
    }
    for (const ShotSlot &slot : ally.shots) {
        if (NowShotEffect != nullptr && NowShotEffect->effect[slot.index].effect_data == slot.config) {
            NowShotEffect->effect[slot.index].Initialize();
        }
    }
    ally.textures.clear();
    ally.anims.clear();
    ally.shots.clear();
    ally.ends.clear();
}

bool LoadKind(CMonstorUnit &unit, int kind, const float *position) {
    Ally &ally = State();
    if (ally.model.base == nullptr) {
        ally.model.base = ArenaBlock(&ally.model, static_cast<std::size_t>(kModelQuads) * 16);
        ally.model.limit = kModelQuads;
    }
    DataReadForgetPackCapacities(&ally.model);
    ally.model.used = 0;
    if (unit.SetupBaseModel(kModelSlot, kind, kShotBlock, &ally.model) == 0) {
        return false;
    }
    std::fprintf(stderr, "ally: kind %d loaded (%d of %d units)\n", kind, ally.model.used, ally.model.limit);
    MonsterSoundsAddKind(kind); // its sounds, wherever it stands
    float at[4] = {position[0], position[1], position[2], 1.0f};
    return unit.SetupViewMonstor(kModelSlot, at, -1) != 0;
}

AllyLoader &Loader() {
    static AllyLoader loader = LoadKind;
    return loader;
}

bool Standing(const Ally &ally) {
    return ally.standing && ally.unit != nullptr && ally.floor != nullptr && NowMonstorUnit == ally.floor;
}

} // namespace

bool AllyUnit(const CMonstorUnit &unit) {
    return State().unit != nullptr && &unit == State().unit.get();
}

bool AllyStepping() {
    return NowMonstorUnit != nullptr && AllyUnit(*NowMonstorUnit);
}

CMonstorUnit *AllyStanding() {
    return State().standing ? State().unit.get() : nullptr;
}

CMonstorUnit *AllyFloor() {
    return State().standing ? State().floor : nullptr;
}

bool AllySummon(int kind, const float *position) {
    AllyDismiss();
    Ally &ally = State();
    if (NowMonstorUnit == nullptr || AllyStepping()) {
        return false;
    }
    ally.floor = NowMonstorUnit;
    ally.unit = std::make_unique<CMonstorUnit>();
    CMonstorUnit &unit = *ally.unit;
    if (ally.script.base == nullptr) {
        ally.script.base = ArenaBlock(&ally.script, static_cast<std::size_t>(kScriptQuads) * 16 * kArenaHeadroom);
        ally.script.limit = kScriptQuads * kArenaHeadroom;
    }
    ally.script.used = 0;
    // Its own script memory alone: CleanViewMonstor empties every unit's script memory it holds.
    unit.script[kAllyMonster] = &ally.script;
    unit.collision = ally.floor->collision;
    unit.model_count = 0;
    unit.current_monster = 0;
    unit.requested_event = -1;

    Taken before = TakenNow();
    bool  stood = false;
    {
        OnAlly now(unit);
        unit.CleanViewMonstor(0);
        stood = Loader()(unit, kind, position);
    }
    NoteAdded(ally, before);
    if (!stood) {
        unit.monster[kAllyMonster].state = -1;
        Release(ally);
        return false;
    }
    // No floor kind: the features that tell a monster by its model slot find none.
    unit.monster[kAllyMonster].base_model = -1;
    ally.standing = true;
    return true;
}

void AllyDismiss() {
    Ally &ally = State();
    if (ally.unit != nullptr) {
        ally.unit->monster[kAllyMonster].state = -1;
    }
    ally.standing = false;
    Release(ally);
}

void AllyForget() {
    Ally &ally = State();
    if (ally.unit != nullptr) {
        ally.unit->monster[kAllyMonster].state = -1;
    }
    ally.standing = false;
    ally.textures.clear();
    ally.anims.clear();
    ally.shots.clear();
    ally.ends.clear();
}

void AllyStep(int pause) {
    Ally &ally = State();
    if (!Standing(ally)) {
        return;
    }
    OnAlly now(*ally.unit);
    ally.unit->Step(pause);
}

void AllyDraw() {
    Ally &ally = State();
    if (!Standing(ally) || CMonUnitHyde != 0 || BtEventMode != 0) {
        return;
    }
    OnAlly now(*ally.unit);
    ally.unit->DrawMonstor();
}

void AllyShadow() {
    Ally &ally = State();
    if (!Standing(ally) || CMonUnitHyde != 0 || BtEventMode != 0) {
        return;
    }
    OnAlly now(*ally.unit);
    ally.unit->DrawShadowMonstor();
}

void AllyPlayerMove(const float *position, float *velocity) {
    Ally &ally = State();
    if (Standing(ally)) {
        ally.unit->MoveCheck(const_cast<float *>(position), velocity, lockOnTargetFlag);
    }
}

void AllyShotFired() {
    if (!AllyStepping() || NowShotEffect == nullptr || NowShotEffect->current_effect == -1) {
        return;
    }
    ShotEffectAt(NowShotEffect->current_effect)->SetUserID2(kAllyShooter);
}

void SetAllyLoader(AllyLoader loader) {
    Loader() = loader != nullptr ? loader : LoadKind;
}

} // namespace expanded
