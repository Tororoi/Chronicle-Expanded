#include "expanded/enemies/crystal_gemron.hpp"

#include <cmath>
#include <cstring>

#include "dun/gameloop.hpp"
#include "expanded/core/frame_tint.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/scaling.hpp"
#include "expanded/enemies/species.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "snd.hpp"

namespace expanded {

namespace {

constexpr int kMonsters = 16;

// The record: Holy Gemron's (its script name too, for its damage: the built script loads in its place), its model the
// built one, weak to ice (150 %) and immune to the rest.
constexpr int kName = 321;
constexpr s16 kAbs = 80;

void BuildRecord(MONSTOR_MODEL &record) {
    std::memset(record.model_name[0], 0, sizeof(record.model_name[0]));
    std::strcpy(record.model_name[0], "e168a");
    for (int i = 0; i < 5; i++) {
        record.attachment_weight[i] = i == 1 ? 150 : 0;
    }
    record.shot_effect[0] = record.shot_effect[1] = -1; // its own, entered when it is set up
    record.exp = kAbs;
    record.name_no = kName;
    record.steal_item = ITEM_POWERUP_POWDER;
    record.rare_item = ITEM_CRYSTAL_EYEBALL;
}

[[maybe_unused]] const bool kSpecies =
    AddSpecies({Monster::CRYSTAL_GEMRON, Monster::GEMRON_HOLY, kCrystalGemronModelFile, kCrystalGemronScriptFile, BuildRecord});

// Its shot, Holy Gemron's own on the Ice Queen's arrow (flying from the moment it is fired at 1.2, Freeze and the Ice
// element, a guardable knockback), and her prison (no hit, held while the player stays frozen).
// clang-format off
BT_SHOT_EFFECT arrow_shot = {"info", 0, 1, {1.4f, 1.2f, 0.0f, 0.0f}, {2.6f, 7.0f, 8.0f, 0.0f}, 360, 58, 0x102, 2, 1,
                             {-1, 0, 1, -1}, -1, {}, 0.0f, 0, {-1, -1, -1, -1}};
BT_SHOT_EFFECT prison_shot = {"info", 1, 0, {}, {-100.0f, -100.0f, -100.0f, -100.0f}, 1800, 0, 0, 2, 1,
                              {0, 0, 1, 1}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
// clang-format on
constexpr int kArrows = 4;

// An arrow spent within this of the player's chest, while he is frozen, encases him (its hit takes a couple of steps
// to freeze him).
constexpr float kPrisonReach = 20.0f;
constexpr int   kArrowSettle = 2;
constexpr float kShatterSpeed = 0.5f;
constexpr int   kFreezeSounds[] = {1772, 1777};
constexpr int   kShatterSound = 1776;

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

bool Crystal(const CMonstorUnit &unit, int monster) {
    return monster >= 0 && monster < kMonsters && unit.monster[monster].state != -1 &&
           ModelKind(unit.monster[monster].base_model) == No(Monster::CRYSTAL_GEMRON);
}

void OnModel(CMonstorUnit &unit, int model_slot, int model_no) {
    if (model_slot == 0) {
        shots = {};
    }
    if (model_no != No(Monster::CRYSTAL_GEMRON)) {
        return;
    }
    shots.arrow = EnterShotEffect(&arrow_shot, kArrows, kIceArrowFile);
    shots.prison = EnterShotEffect(&prison_shot, 1, kIcePrisonFile);
    if (shots.arrow >= 0) {
        unit.model[model_slot].shot_effect[0] = static_cast<s16>(shots.arrow);
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyModelListener(OnModel), true);

float Distance(const float *a, const float *b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

void Imprison(CMonstorUnit &unit) {
    CSHOT_EFFECT *effect = ShotEffectAt(shots.prison);
    if (effect == nullptr || shots.standing >= 0) {
        return;
    }
    float feet[4];
    sceVu0CopyVector(feet, CharaMain.pos);
    feet[3] = 1.0f;
    NowShotEffect->Set(shots.prison, feet, feet);
    if (effect->current_slot < 0) {
        return;
    }
    if (shots.owner >= 0 && Crystal(unit, shots.owner)) {
        NowShotEffect->SetUserID2(shots.owner);
    }
    shots.standing = effect->current_slot;
    shots.breaking = false;
    for (int sound : kFreezeSounds) {
        SndSePlay(sound, -1, 0);
    }
}

} // namespace

void CrystalGemronStep(CMonstorUnit &unit) {
    CSHOT_EFFECT *arrows = shots.arrow >= 0 ? ShotEffectAt(shots.arrow) : nullptr;
    if (arrows == nullptr) {
        return;
    }
    for (int monster = 0; monster < kMonsters && shots.owner < 0; monster++) {
        if (Crystal(unit, monster)) {
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
        SndSePlay(kShatterSound, -1, 0);
        EnterShotPhase(*prison, slot, 2);
        prison->chara[slot].SetMotionSpeed(kShatterSpeed);
    }
}

void CrystalGemronDrawing(CMonstorUnit &unit, int monster) {
    if (!Crystal(unit, monster) || unit.chara[monster][0].frame == nullptr) {
        return;
    }
    if (CFrame *eyes = unit.chara[monster][0].frame->SearchFrame(const_cast<char *>(kCrystalEyeFrame))) {
        SetFrameTint(eyes, kEyeGlow[0], kEyeGlow[1], kEyeGlow[2]);
    }
}

} // namespace expanded
