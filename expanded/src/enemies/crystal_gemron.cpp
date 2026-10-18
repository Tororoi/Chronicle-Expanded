#include "expanded/enemies/crystal_gemron.hpp"

#include <cmath>
#include <cstring>

#include "dun/gameloop.hpp"
#include "expanded/core/frame_tint.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/species.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"

namespace expanded {

namespace {

// Holy Gemron's record (its script name too, for its damage: the built script loads in its place) on the built
// model, weak to ice and immune to the rest.
[[maybe_unused]] const bool kSpecies = AddSpecies({
    .kind = Monster::CRYSTAL_GEMRON,
    .stand_in = Monster::GEMRON_HOLY,
    .model_file = kCrystalGemronModelFile,
    .script_file = kCrystalGemronScriptFile,
    .model_name = "e168a",
    .elements = {0, 150, 0, 0, 0},
    .abs = 80,
    .name_no = 321,
    .steal_item = ITEM_POWERUP_POWDER,
    .rare_item = ITEM_CRYSTAL_EYEBALL,
});

// Its shot, Holy Gemron's own on the Ice Queen's arrow (flying from the moment it is fired at 1.2, Freeze and the Ice
// element, a guardable knockback), and her prison (no hit, held while the player stays frozen).
// clang-format off
BT_SHOT_EFFECT arrow_shot = {"info", 0, 1, {1.4f, 1.2f, 0.0f, 0.0f}, {2.6f, 7.0f, 8.0f, 0.0f}, 360, 58, 0x102, 2, 1,
                             {-1, 0, 1, -1}, -1, {}, 0.0f, 0, {-1, -1, -1, -1}};
// clang-format on
BT_SHOT_EFFECT prison_shot = IcePrisonShot();
constexpr int kArrows = 4;

// An arrow spent within this of the player's chest, while he is frozen, encases him (its hit takes a couple of steps
// to freeze him).
constexpr float kPrisonReach = 20.0f;
constexpr int   kArrowSettle = 2;

// The eyes' glow, on the ambient for their own draw.
constexpr float kEyeGlow[3] = {45.0f, 70.0f, 50.0f};

struct Shots {
    int   arrow = -1;
    int   prison = -1;
    bool  flying[kArrows] = {};
    float last[kArrows][4] = {};
    int   settle = 0;    // steps until a spent arrow near the player is checked for a freeze
    int   standing = -1; // the prison's slot
    bool  breaking = false;
    int   owner = -1;
};

Shots shots;

void OnFloor() {
    shots = {};
}

void OnModel(CMonstorUnit &unit, int model_slot, int model_no) {
    if (model_no != No(Monster::CRYSTAL_GEMRON)) {
        return;
    }
    shots.arrow = EnterOwnShot(unit, model_slot, &arrow_shot, kArrows, kIceArrowFile);
    shots.prison = EnterShotEffect(&prison_shot, 1, kIcePrisonFile);
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), AddEnemyModelListener(OnModel), true);

float Distance(const float *a, const float *b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

void Imprison(CMonstorUnit &unit) {
    if (shots.prison < 0 || shots.standing >= 0) {
        return;
    }
    CSHOT_EFFECT *effect = ShotEffectAt(shots.prison);
    float feet[4];
    PlayerFeet(feet);
    NowShotEffect->Set(shots.prison, feet, feet);
    if (effect->current_slot < 0) {
        return;
    }
    if (shots.owner >= 0 && IsKind(unit, shots.owner, Monster::CRYSTAL_GEMRON)) {
        NowShotEffect->SetUserID2(shots.owner);
    }
    shots.standing = effect->current_slot;
    shots.breaking = false;
    PlayIceFreeze();
}

} // namespace

void CrystalGemronStep(CMonstorUnit &unit) {
    CSHOT_EFFECT *arrows = shots.arrow >= 0 ? ShotEffectAt(shots.arrow) : nullptr;
    if (arrows == nullptr) {
        return;
    }
    for (int monster = 0; monster < kFloorMonsters && shots.owner < 0; monster++) {
        if (IsKind(unit, monster, Monster::CRYSTAL_GEMRON)) {
            shots.owner = monster;
        }
    }
    // Arrows home in while they fly; one spent near the player may encase him.
    float chest[4];
    PlayerChest(chest);
    for (int slot = 0; slot < kArrows; slot++) {
        bool flying = arrows->active[slot] != 0 && arrows->phase[slot] == 1;
        if (flying) {
            SteerIceArrow(*arrows, slot);
            arrows->chara[slot].GetPosition(shots.last[slot]);
        } else if (shots.flying[slot] && Distance(shots.last[slot], chest) <= kPrisonReach) {
            shots.settle = kArrowSettle;
        }
        shots.flying[slot] = flying;
    }
    if (shots.settle > 0 && --shots.settle == 0 && PlayerFrozen()) {
        Imprison(unit);
    }
    // The prison holds while the freeze lasts, then shatters.
    CSHOT_EFFECT *prison = shots.prison >= 0 ? ShotEffectAt(shots.prison) : nullptr;
    if (prison == nullptr || shots.standing < 0) {
        return;
    }
    int slot = shots.standing;
    if (prison->active[slot] == 0) {
        shots.standing = -1;
        return;
    }
    if (!shots.breaking && HoldIcePrison(*prison, slot) && !PlayerAfflicted()) {
        shots.breaking = true;
        ShatterIcePrison(*prison, slot);
    }
}

void CrystalGemronDrawing(CMonstorUnit &unit, int monster) {
    if (!IsKind(unit, monster, Monster::CRYSTAL_GEMRON) || unit.chara[monster][0].frame == nullptr) {
        return;
    }
    if (CFrame *eyes = unit.chara[monster][0].frame->SearchFrame(const_cast<char *>(kCrystalEyeFrame))) {
        SetFrameTint(eyes, kEyeGlow[0], kEyeGlow[1], kEyeGlow[2]);
    }
}

} // namespace expanded
