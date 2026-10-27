#include "expanded/enemies/crystal_gemron.hpp"

#include <cstring>

#include "dun/gameloop.hpp"
#include "expanded/core/frame_tint.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/enemies/monster_actions.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/species.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"

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

// Its shot, the ice arrow, and her prison (no hit, held while the player stays frozen).
BT_SHOT_EFFECT arrow_shot = IceArrowShot();
BT_SHOT_EFFECT prison_shot = IcePrisonShot();
constexpr int  kArrows = 4;

// An arrow spent within this of the player's chest, while he is frozen, encases him once its hit has frozen him.
constexpr float kPrisonReach = 20.0f;

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

void Imprison(CMonstorUnit &unit) {
    if (shots.prison < 0 || shots.standing >= 0) {
        return;
    }
    float feet[4];
    PlayerFeet(feet);
    int owner = shots.owner >= 0 && IsKind(unit, shots.owner, Monster::CRYSTAL_GEMRON) ? shots.owner : -1;
    int slot = FireShotEffect(shots.prison, feet, feet, owner);
    if (slot < 0) {
        return;
    }
    shots.standing = slot;
    shots.breaking = false;
    PlayIceFreeze();
}

} // namespace

void CrystalGemronStep(CMonstorUnit &unit) {
    CSHOT_EFFECT *arrows = LoadedShotEffect(shots.arrow);
    if (arrows == nullptr) {
        return;
    }
    for (int monster = 0; monster < kFloorMonsters && shots.owner < 0; monster++) {
        if (IsKind(unit, monster, Monster::CRYSTAL_GEMRON)) {
            shots.owner = monster;
        }
    }
    // Arrows home in while they fly; one spent near the player may encase him.
    for (int slot = 0; slot < kArrows; slot++) {
        bool flying = arrows->active[slot] != 0 && arrows->phase[slot] == 1;
        if (flying) {
            SteerIceArrow(*arrows, slot);
            arrows->chara[slot].GetPosition(shots.last[slot]);
        } else if (shots.flying[slot] && NearPlayerChest(shots.last[slot], kPrisonReach)) {
            shots.settle = kIceArrowSettle;
        }
        shots.flying[slot] = flying;
    }
    if (shots.settle > 0 && --shots.settle == 0 && PlayerFrozen()) {
        Imprison(unit);
    }
    // The prison holds while the freeze lasts, then shatters.
    CSHOT_EFFECT *prison = LoadedShotEffect(shots.prison);
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
    if (!IsKind(unit, monster, Monster::CRYSTAL_GEMRON)) {
        return;
    }
    if (CFrame *eyes = MonsterFrame(unit, monster, kCrystalEyeFrame)) {
        SetFrameTint(eyes, kEyeGlow[0], kEyeGlow[1], kEyeGlow[2]);
    }
}

} // namespace expanded
