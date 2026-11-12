#include "expanded/weapons/kitchen_knife_shot.hpp"

#include "bt_shot_effect.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/model_pellets.hpp"
#include "expanded/weapons/toan/kitchen_knife.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

// The gem alone: the apple shot drawing the Aquamarine, at no speed, put where it is wanted, no hit (as Big Bang's bomb).
// clang-format off
BT_SHOT_EFFECT gem_shot = {"ringo_ex", 0, 0, {}, {}, -1, 0, 0, 3, 0, {-1, 0, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_KITCHEN_KNIFE, WeaponShotForm::Sphere, &gem_shot, kGemShotSlots, kAquamarineThrowFile), true);

ModelPellets gems(&gem_shot, KitchenKnifePellets, 1.0f, kGemTint);

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener([] { gems.Reset(); }), true);

} // namespace

void GemShotStep() {
    gems.Step();
}

void GemShotReset() {
    gems.Reset();
}

void GemShotFired(CSHOT &shots, int slot) {
    gems.Fired(shots, slot);
}

void GemShotEnded(const CSHOT &shots, int slot) {
    gems.Ended(shots, slot);
}

void GemShotLight(const CCharacter &character, float *ambient) {
    gems.Light(character, ambient);
}

} // namespace expanded
