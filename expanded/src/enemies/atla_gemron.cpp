#include "dungeon.h"
#include "expanded/enemies/atla_gemron.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <random>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/core/frame_tint.hpp"
#include "expanded/data/atla_gemron_recipe.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/scaling.hpp"
#include "expanded/enemies/species.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

namespace rc = atla_recipe;

constexpr int kMonsters = 16;

// The record: Holy Gemron's (its script name too, for its damage: the built script loads in its place), its model the
// built one, weak to wind (150 %) and immune to the rest.
constexpr int kName = 322;
constexpr s16 kAbs = 40;

void BuildRecord(MONSTOR_MODEL &record) {
    std::memset(record.model_name[0], 0, sizeof(record.model_name[0]));
    std::strcpy(record.model_name[0], "e169a");
    for (int i = 0; i < 5; i++) {
        record.attachment_weight[i] = i == 3 ? 150 : 0;
    }
    record.shot_effect[0] = record.shot_effect[1] = -1; // its own, entered when it is set up
    record.exp = kAbs;
    record.name_no = kName;
    record.steal_item = ITEM_DRAN_S_FEATHER;
    record.rare_item = ITEM_POTATO_CAKE;
}

[[maybe_unused]] const bool kSpecies =
    AddSpecies({Monster::ATLA_GEMRON, Monster::GEMRON_HOLY, kAtlaGemronModelFile, kAtlaGemronScriptFile, BuildRecord});

// Its shot: Holy Gemron's (its damage, reaction, element, target and sounds) on the Atlamillia's burst, one phase (the
// shot ends with its motion), turned to its aim. It flies the burst's own pace a frame whatever it meets (the engine's
// flight is only its aim), its hit growing from 1 to 18 between attack frames 140 and 145 of the script's fire (138, at
// 0.2): 10 to 35 frames on.
constexpr int   kHolyShot = 25;
constexpr float kAim = 1.0f / (1 << 20);
constexpr float kRadiusFrom = 1.0f, kRadiusTo = 18.0f;
constexpr float kGrowFrom = kAtlaShotFrom + (140 - 138) / 0.2f * kAtlaShotSpeed;
constexpr float kGrowTo = kAtlaShotFrom + (145 - 138) / 0.2f * kAtlaShotSpeed;
constexpr float kGrowth = (kRadiusTo - kRadiusFrom) / (kGrowTo - kGrowFrom);
constexpr int   kShots = 2;

BT_SHOT_EFFECT atla_shot;
int            shot = -1;

// The death: motion 11, its copy from 200; the held atla leaves the grip on the swap and settles 11 frames on.
constexpr int         kDeathMotion = 11;
constexpr float       kSettle = 11.0f;
constexpr const char *kHeldAtla = "ball_0";
// The forehead Atlamillia's glow, on the ambient for its own draw.
constexpr float kLensGlow = 50.0f;

// The drop: 5 % the Atlamillia Sword (from Sun & Moon Temple on; before it a weapon's sphere), 15 % its SynthSphere,
// 10 % the dungeon's featured weapon's (Demon Shaft has none), else a weapon's from the tiers the dungeon draws from.
constexpr int kSwordPercent = 5, kAtlamilliaPercent = 20, kFeaturedPercent = 30;
constexpr int kFeatured[DUNGEON_COUNT] = {ITEM_WEAPON_BONE_RAPIER, ITEM_WEAPON_WISE_OWL_SWORD, ITEM_WEAPON_DUSACK,
                                          ITEM_WEAPON_MANEATER, ITEM_WEAPON_7BRANCH_SWORD, ITEM_WEAPON_DARK_CLOUD, 0};
// The SynthSphere tiers, by the mod's rebalanced attack, magic, elements and antis: a dungeon draws from its own and
// every tier below (Demon Shaft all). Not listed: the Atlamillia (its own drops).
// clang-format off
const std::vector<int> kTiers[DUNGEON_COUNT] = {
    {ITEM_WEAPON_DAGGER, ITEM_WEAPON_WOODEN_SLINGSHOT, ITEM_WEAPON_BASELARD, ITEM_WEAPON_BONE_SLINGSHOT, ITEM_WEAPON_STEEL_SLINGSHOT, ITEM_WEAPON_MALLET, ITEM_WEAPON_GLADIUS, ITEM_WEAPON_MACHINE_GUN, ITEM_WEAPON_CRYSKNIFE, ITEM_WEAPON_BONE_RAPIER, ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_FIGHTING_STICK, ITEM_WEAPON_STEEL_HAMMER, ITEM_WEAPON_GOLD_RING},
    {ITEM_WEAPON_BUSTER_SWORD, ITEM_WEAPON_BANDIT_SLINGSHOT, ITEM_WEAPON_POCKLEKUL, ITEM_WEAPON_BANDIT_S_RING, ITEM_WEAPON_BIG_BUCKS_HAMMER, ITEM_WEAPON_FROZEN_TUNA, ITEM_WEAPON_PLATE_HAMMER, ITEM_WEAPON_DOUBLE_IMPACT, ITEM_WEAPON_SMALL_SWORD, ITEM_WEAPON_STEVE, ITEM_WEAPON_JAVELIN, ITEM_WEAPON_FLAMINGO, ITEM_WEAPON_PLATINUM_RING, ITEM_WEAPON_JACKAL, ITEM_WEAPON_SNAIL, ITEM_WEAPON_SAX, ITEM_WEAPON_WISE_OWL_SWORD, ITEM_WEAPON_TURTLE_SHELL, ITEM_WEAPON_CHOPPER, ITEM_WEAPON_TRIAL_HAMMER, ITEM_WEAPON_HARDSHOOTER},
    {ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_HALBERT, ITEM_WEAPON_5_FOOT_NAIL, ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_MACHO_SWORD, ITEM_WEAPON_KITCHEN_KNIFE, ITEM_WEAPON_PARTISAN, ITEM_WEAPON_DE_SANGA, ITEM_WEAPON_BLESSING_GUN, ITEM_WEAPON_FAIRY_S_RING, ITEM_WEAPON_BATTLE_AX, ITEM_WEAPON_ANTIQUE_SWORD, ITEM_WEAPON_DUSACK, ITEM_WEAPON_SCORPION, ITEM_WEAPON_TSUKIKAGE, ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_SWALLOW, ITEM_WEAPON_SERPENT_SWORD, ITEM_WEAPON_MATADOR, ITEM_WEAPON_CHOORA, ITEM_WEAPON_THORN_ARMLET, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_GAIA_HAMMER, ITEM_WEAPON_CACTUS, ITEM_WEAPON_LAMB_S_SWORD},
    {ITEM_WEAPON_SKUNK, ITEM_WEAPON_MAGICAL_HAMMER, ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_MIRAGE, ITEM_WEAPON_MANEATER},
    {ITEM_WEAPON_DIVINEBEASTTITLE, ITEM_WEAPON_SUN_SWORD, ITEM_WEAPON_CROSS_HINDER, ITEM_WEAPON_GODDESS_RING, ITEM_WEAPON_SATAN_S_RING, ITEM_WEAPON_7BRANCH_SWORD, ITEM_WEAPON_BRAVE_ARK, ITEM_WEAPON_AGA_S_SWORD},
    {ITEM_WEAPON_DARK_CLOUD, ITEM_WEAPON_DESTRUCTION_RING, ITEM_WEAPON_LAST_JUDGEMENT, ITEM_WEAPON_HEAVEN_S_CLOUD, ITEM_WEAPON_DRAIN_SEEKER, ITEM_WEAPON_TERRA_SWORD, ITEM_WEAPON_HEXA_BLASTER},
    {ITEM_WEAPON_MARDAN_EINS, ITEM_WEAPON_MARDAN_TWEI, ITEM_WEAPON_ARISE_MARDAN, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_ATHENA_S_ARMLET, ITEM_WEAPON_TALL_HAMMER, ITEM_WEAPON_SATAN_S_AX, ITEM_WEAPON_STAR_BREAKER, ITEM_WEAPON_HERCULES_WRATH, ITEM_WEAPON_ANGEL_SHOOTER, ITEM_WEAPON_MOBIUS_RING, ITEM_WEAPON_SUPERNOVA, ITEM_WEAPON_7THHEAVEN, ITEM_WEAPON_BIG_BANG, ITEM_WEAPON_BABEL_S_SPEAR, ITEM_WEAPON_ANGE_GEAR, ITEM_WEAPON_INFERNO, ITEM_WEAPON_SECRET_ARMLET, ITEM_WEAPON_SWORD_OF_ZEUS, ITEM_WEAPON_CHRONICLE2, ITEM_WEAPON_CHRONICLE_SWORD},
};
// clang-format on

struct Gemron {
    bool          atla = false;
    bool          dropped = false; // its death has placed its atla
    int           placed = -1;     // the atla, while it bounces
    bool          held = false;    // the held atla's pose seen
    sceVu0FMATRIX world{};         // the held atla's, as the unit last drew it
    AtlaPath      path{};
};

Gemron gemrons[kMonsters];

std::minstd_rand &Rng() {
    static std::minstd_rand rng{std::random_device{}()};
    return rng;
}

bool Atla(const CMonstorUnit &unit, int monster) {
    return monster >= 0 && monster < kMonsters && unit.monster[monster].state != -1 &&
           ModelKind(unit.monster[monster].base_model) == No(Monster::ATLA_GEMRON);
}

void OnModel(CMonstorUnit &unit, int model_slot, int model_no) {
    if (model_slot == 0) {
        shot = -1;
        for (Gemron &g : gemrons) {
            g = {};
        }
    }
    if (model_no != No(Monster::ATLA_GEMRON) || BtEntryEffectTbl[kHolyShot] == nullptr) {
        return;
    }
    atla_shot = *BtEntryEffectTbl[kHolyShot];
    std::memset(atla_shot.model_name, 0, sizeof(atla_shot.model_name));
    std::strcpy(atla_shot.model_name, kAtlaShotModel);
    atla_shot.stationary = 0;
    atla_shot.face_flight = 1;
    atla_shot.speed[0] = kAim;
    atla_shot.radius[0] = kRadiusFrom;
    atla_shot.motion[0] = 0;
    for (int i = 1; i < 4; i++) {
        atla_shot.speed[i] = atla_shot.radius[i] = 0.0f;
        atla_shot.motion[i] = -1;
    }
    shot = EnterShotEffect(&atla_shot, kShots, kAtlaShotFile);
    if (shot >= 0) {
        unit.model[model_slot].shot_effect[0] = static_cast<s16>(shot);
    }
}

void OnSpawn(CMonstorUnit &unit, int monster) {
    if (monster >= 0 && monster < kMonsters) {
        gemrons[monster] = {};
        gemrons[monster].atla = Atla(unit, monster);
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyModelListener(OnModel), AddEnemySpawnListener(OnSpawn), true);

int RandomWeapon(int dungeon) {
    std::vector<int> pool;
    int              tiers = std::clamp(dungeon + 1, 1, int(DUNGEON_COUNT));
    for (int t = 0; t < tiers; t++) {
        for (int weapon : kTiers[t]) {
            WEAPON_DATA *data = GetWeaponData(weapon);
            if (data != nullptr && (data->owner == 0 || data->owner < UserStatus->party_size)) { // its owner has joined
                pool.push_back(weapon);
            }
        }
    }
    return pool.empty() ? ITEM_WEAPON_DAGGER : pool[std::uniform_int_distribution<std::size_t>(0, pool.size() - 1)(Rng())];
}

AtlaDrop RollDrop() {
    int dungeon = std::clamp(int(selectMapNo), 0, DUNGEON_COUNT - 1);
    int roll = std::uniform_int_distribution<int>(0, 99)(Rng());
    if (roll < kSwordPercent && dungeon >= DUNGEON_SUN_MOON_TEMPLE) {
        return {ITEM_WEAPON_ATLAMILLIA_SWORD, false};
    }
    if (roll >= kSwordPercent && roll < kAtlamilliaPercent) {
        return {ITEM_WEAPON_ATLAMILLIA_SWORD, true};
    }
    if (roll >= kAtlamilliaPercent && roll < kFeaturedPercent && kFeatured[dungeon] != 0) {
        return {kFeatured[dungeon], true};
    }
    return {RandomWeapon(dungeon), true};
}

// The rotation's quaternion (w, x, y, z), as CFrame::SetTransMatrix turns it back into rows.
void QuatOfRows(const float m[3][3], float *q) {
    float tr = m[0][0] + m[1][1] + m[2][2], s;
    if (tr > 0) {
        s = 2 * std::sqrt(1 + tr);
        q[0] = s / 4, q[1] = (m[1][2] - m[2][1]) / s, q[2] = (m[2][0] - m[0][2]) / s, q[3] = (m[0][1] - m[1][0]) / s;
    } else if (m[0][0] >= m[1][1] && m[0][0] >= m[2][2]) {
        s = 2 * std::sqrt(1 + m[0][0] - m[1][1] - m[2][2]);
        q[0] = (m[1][2] - m[2][1]) / s, q[1] = s / 4, q[2] = (m[1][0] + m[0][1]) / s, q[3] = (m[2][0] + m[0][2]) / s;
    } else if (m[1][1] >= m[2][2]) {
        s = 2 * std::sqrt(1 + m[1][1] - m[0][0] - m[2][2]);
        q[0] = (m[2][0] - m[0][2]) / s, q[1] = (m[1][0] + m[0][1]) / s, q[2] = s / 4, q[3] = (m[2][1] + m[1][2]) / s;
    } else {
        s = 2 * std::sqrt(1 + m[2][2] - m[0][0] - m[1][1]);
        q[0] = (m[0][1] - m[1][0]) / s, q[1] = (m[2][0] + m[0][2]) / s, q[2] = (m[2][1] + m[1][2]) / s, q[3] = s / 4;
    }
}

// The held atla's pose to a real atla's: from its world turn, size and place (the sphere's centre less the root's
// offset) to the turn and place a real atla has over the same point of the unit's floor.
AtlaPath PathFrom(const sceVu0FMATRIX world, float floor) {
    AtlaPath path{};
    float    rows[3][3], centre[3] = {}, held[3] = {}, rest[3] = {}, length = 0;
    for (int r = 0; r < 3; r++) {
        float l = std::sqrt(world[r][0] * world[r][0] + world[r][1] * world[r][1] + world[r][2] * world[r][2]);
        for (int c = 0; c < 3; c++) {
            rows[r][c] = world[r][c] / l;
        }
        length += l;
    }
    float g = static_cast<float>(rc::kGraftScale), scale = length / 3 * g;
    for (int j = 0; j < 3; j++) {
        for (int k = 0; k < 3; k++) {
            centre[j] += g * static_cast<float>(rc::kAtlaCentre[k]) * world[k][j];
            held[j] += static_cast<float>(rc::kAtlaCentre[k]) * rows[k][j];
            rest[j] += static_cast<float>(rc::kAtlaCentre[k] * rc::kAtlaRootRows[k][j]);
        }
        centre[j] += world[3][j];
    }
    QuatOfRows(rows, path.q0);
    for (int i = 0; i < 4; i++) {
        path.q1[i] = static_cast<float>(rc::kAtlaRootQuat[i]);
    }
    path.scale = scale;
    for (int j = 0; j < 3; j++) {
        path.from[j] = centre[j] - scale * held[j] - static_cast<float>(rc::kAtlaRootT[j]);
    }
    path.to[0] = centre[0] - rest[0];
    path.to[1] = floor;
    path.to[2] = centre[2] - rest[2];
    return path;
}

void StepShot() {
    CSHOT_EFFECT *effect = shot >= 0 ? ShotEffectAt(shot) : nullptr;
    if (effect == nullptr || effect->effect_data != &atla_shot) {
        return;
    }
    float most = 0.0f;
    for (int slot = 0; slot < kShots; slot++) {
        if (effect->active[slot] == 0) {
            continue;
        }
        CCharacter &burst = effect->chara[slot];
        most = std::max(most, std::clamp(kRadiusFrom + (burst.GetNowTime() - kGrowFrom) * kGrowth, kRadiusFrom, kRadiusTo));
        float at[4];
        burst.GetPosition(at);
        for (int i = 0; i < 3; i++) {
            at[i] += effect->velocity[slot][i] * static_cast<float>(rc::kShotFlight / kAim);
        }
        burst.SetPosition(at);
    }
    atla_shot.radius[0] = most;
}

void StepDeath(CMonstorUnit &unit, int monster) {
    Gemron           &g = gemrons[monster];
    const CCharacter &body = unit.chara[monster][0];
    bool              dying = unit.monster[monster].state != -1 && unit.monster[monster].hp <= 0 && body.motion_no == kDeathMotion;
    float             frame = body.motion_type.state.time;
    if (!g.dropped) {
        if (!dying || frame < kAtlaGemronDeathStart) {
            return;
        }
        g.dropped = true;
        float at[4];
        unit.chara[monster][0].GetPosition(at);
        if (g.held) {
            g.path = PathFrom(g.world, at[1]);
        }
        const float *rest = g.held ? g.path.to : at;
        AtlaDrop     drop = RollDrop();
        int          atla = PlaceAtlaDrop(rest, drop);
        if (atla < 0) {
            atla = TakeOverAtla(rest); // every place holds an atla: one comes to it, its own content
        }
        if (atla < 0) {
            GiveAtlaDrop(drop);
            return;
        }
        if (g.held) {
            g.placed = atla;
            MoveAtla(atla, g.path, -1.0f);
        }
        return;
    }
    if (dying && frame >= kAtlaGemronSwap) {
        unit.monster[monster].shadow_enabled = false; // the body's shadow would fall on the atla rising out of it
    }
    if (g.placed < 0) {
        return;
    }
    if (dying && frame < kAtlaGemronSwap) {
        float at[4];
        unit.chara[monster][0].GetPosition(at);
        g.path = PathFrom(g.world, at[1]); // it may still be sliding
        MoveAtla(g.placed, g.path, -1.0f);
        return;
    }
    float progress = dying ? (frame - kAtlaGemronSwap) / kSettle : 1.0f;
    MoveAtla(g.placed, g.path, progress);
    if (progress >= 1.0f) {
        g.placed = -1;
    }
}

} // namespace

void AtlaGemronStep(CMonstorUnit &unit) {
    StepShot();
    for (int monster = 0; monster < kMonsters; monster++) {
        if (gemrons[monster].atla) {
            StepDeath(unit, monster);
        }
    }
}

void AtlaGemronDrawing(CMonstorUnit &unit, int monster) {
    if (!Atla(unit, monster) || unit.chara[monster][0].frame == nullptr) {
        return;
    }
    if (CFrame *lens = unit.chara[monster][0].frame->SearchFrame(const_cast<char *>(kAtlaGemronLens))) {
        SetFrameTint(lens, kLensGlow, kLensGlow, kLensGlow);
    }
}

void AtlaGemronDrawn(CMonstorUnit &unit, int monster) {
    if (!Atla(unit, monster) || unit.chara[monster][0].frame == nullptr) {
        return;
    }
    const CCharacter &body = unit.chara[monster][0];
    if (body.motion_no == kDeathMotion && body.motion_type.state.time >= kAtlaGemronSwap) {
        return; // out of the draw
    }
    if (CFrame *held = body.frame->SearchFrame(const_cast<char *>(kHeldAtla))) {
        held->GetLWMatrix(gemrons[monster].world);
        gemrons[monster].held = true;
    }
}

} // namespace expanded
