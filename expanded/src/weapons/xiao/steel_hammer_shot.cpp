#include "expanded/weapons/xiao/steel_hammer_shot.hpp"

#include "bt_shot_effect.hpp"
#include "expanded/data/steel_hammer_head.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/model_pellets.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"

namespace expanded {

namespace {

// The head alone: the apple shot drawing the hammer's head, at no speed, put where it is wanted, no hit (as the Kitchen
// Knife's Aquamarine).
// clang-format off
BT_SHOT_EFFECT head_shot = {"ringo_ex", 0, 0, {}, {}, -1, 0, 0, 3, 0, {-1, 0, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_STEEL_HAMMER, WeaponShotForm::Sphere, &head_shot, kSteelHeadSlots, kSteelHeadThrowFile), true);

ModelPellets heads(&head_shot, SteelHammerPellets, kSteelHeadScale, nullptr, ModelPellets::Turn::Spin);

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener([] { heads.Reset(); }), true);

} // namespace

bool SteelHammerPellets() {
    return SuperSteveSphere() == ITEM_WEAPON_STEEL_HAMMER;
}

void SteelHeadShotStep() {
    heads.Step();
}

void SteelHeadShotReset() {
    heads.Reset();
}

void SteelHeadShotFired(CSHOT &shots, int slot) {
    if (slot >= 0 && slot < kPelletSlots && SteelHammerPellets()) {
        for (int c = 0; c < 3; c++) {
            shots.vector[slot][c] *= kSteelHeadSpeed;
        }
        shots.life[slot] = static_cast<int>(static_cast<float>(shots.life[slot]) / kSteelHeadSpeed);
    }
    heads.Fired(shots, slot);
}

void SteelHeadShotEnded(const CSHOT &shots, int slot) {
    heads.Ended(shots, slot);
}

} // namespace expanded
