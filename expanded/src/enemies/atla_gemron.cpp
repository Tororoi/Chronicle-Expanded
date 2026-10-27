#include "expanded/enemies/atla_gemron.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>

#include "dun/gameloop.hpp"
#include "expanded/core/frame_tint.hpp"
#include "expanded/data/atla_gemron_recipe.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/atla_gemron_loot.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_actions.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/species.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"

namespace expanded {

namespace {

namespace rc = atla_recipe;

// Holy Gemron's record (its script name too, for its damage: the built script loads in its place) on the built
// model, weak to wind and immune to the rest.
[[maybe_unused]] const bool kSpecies = AddSpecies({
    .kind = Monster::ATLA_GEMRON,
    .stand_in = Monster::GEMRON_HOLY,
    .model_file = kAtlaGemronModelFile,
    .script_file = kAtlaGemronScriptFile,
    .model_name = "e169a",
    .elements = {0, 0, 0, 150, 0},
    .abs = 40,
    .name_no = 322,
    .steal_item = ITEM_DRAN_S_FEATHER,
    .rare_item = ITEM_POTATO_CAKE,
});

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

struct Gemron {
    bool          atla_gemron = false;
    bool          dropped = false; // its death has placed its atla
    int           placed = -1;     // the atla, while it bounces
    int           atla = -1;       // the atla, while the body lies over it
    bool          held = false;    // the held atla's pose seen
    sceVu0FMATRIX world{};         // the held atla's, as the unit last drew it
    AtlaPath      path{};
};

Gemron gemrons[kFloorMonsters];

void OnFloor() {
    shot = -1;
    for (Gemron &g : gemrons) {
        g = {};
    }
}

void OnModel(CMonstorUnit &unit, int model_slot, int model_no) {
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
    shot = EnterOwnShot(unit, model_slot, &atla_shot, kShots, kAtlaShotFile);
}

void OnSpawn(CMonstorUnit &unit, int monster) {
    if (monster >= 0 && monster < kFloorMonsters) {
        gemrons[monster] = {};
        gemrons[monster].atla_gemron = IsKind(unit, monster, Monster::ATLA_GEMRON);
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), AddEnemyModelListener(OnModel), AddEnemySpawnListener(OnSpawn), true);

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
        AtlaDrop     drop = RollAtlaGemronDrop(selectMapNo);
        int          atla = PlaceAtlaDrop(rest, drop);
        if (atla < 0) {
            atla = TakeOverAtla(rest); // every place holds an atla: one comes to it, its own content
        }
        if (atla < 0) {
            GiveAtlaDrop(drop);
            return;
        }
        g.atla = atla;
        DrawAtlaOverShadows(atla, true); // the body's shadow on the ground under it, not on it
        if (g.held) {
            g.placed = atla;
            MoveAtla(atla, g.path, -1.0f);
        }
        return;
    }
    if (!dying && g.atla >= 0) {
        DrawAtlaOverShadows(g.atla, false);
        g.atla = -1;
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
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (gemrons[monster].atla_gemron) {
            StepDeath(unit, monster);
        }
    }
}

void AtlaGemronDrawing(CMonstorUnit &unit, int monster) {
    if (!IsKind(unit, monster, Monster::ATLA_GEMRON)) {
        return;
    }
    if (CFrame *lens = MonsterFrame(unit, monster, kAtlaGemronLens)) {
        SetFrameTint(lens, kLensGlow, kLensGlow, kLensGlow);
    }
}

void AtlaGemronDrawn(CMonstorUnit &unit, int monster) {
    if (!IsKind(unit, monster, Monster::ATLA_GEMRON)) {
        return;
    }
    const CCharacter &body = unit.chara[monster][0];
    if (body.motion_no == kDeathMotion && body.motion_type.state.time >= kAtlaGemronSwap) {
        return; // out of the draw
    }
    if (CFrame *held = MonsterFrame(unit, monster, kHeldAtla)) {
        held->GetLWMatrix(gemrons[monster].world);
        gemrons[monster].held = true;
    }
}

} // namespace expanded
