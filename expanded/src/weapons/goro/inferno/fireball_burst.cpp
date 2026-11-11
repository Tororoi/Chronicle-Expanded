#include "expanded/weapons/goro/inferno/fireball_burst.hpp"

#include <cstdio>
#include <iterator>

#include "bt_shot_effect.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"

namespace expanded {

namespace {

// Its burst playing once where it is put, no flight and no hit.
// clang-format off
BT_SHOT_EFFECT burst = {"f_boll_3", 0, 0, {}, {}, -1, 0, 0, 3, 0, {kFireballBurstKey, -1, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_INFERNO, WeaponShotForm::Weapon, &burst, kFireballBursts, kRisingFlamePack), true);

// The impact whose model is hidden.
CSHOT_EFFECT *hidden = nullptr;
int           hidden_slot = -1;

void OnFloor() {
    hidden = nullptr;
    hidden_slot = -1;
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

} // namespace

BT_SHOT_EFFECT *FireballBurstEffect() {
    return &burst;
}

void InfernoImpact(CSHOT_EFFECT &effect, int slot, const float *at) {
    if (!InfernoWielded() || slot < 0 || slot >= static_cast<int>(std::size(effect.chara))) {
        return;
    }
    int index = ShotEffectIndex(&burst);
    if (index < 0) {
        std::fprintf(stderr, "inferno: no burst, not loaded on this floor\n");
        return;
    }
    CSHOT_EFFECT *bursts = ShotEffectAt(index);
    int           shot = SpawnStill(*bursts, at);
    if (shot < 0) {
        return;
    }
    bursts->chara[shot].SetScale(kFireballBurstScale, kFireballBurstScale, kFireballBurstScale);
    effect.chara[slot].SetScale(0.0f, 0.0f, 0.0f);
    hidden = &effect;
    hidden_slot = slot;
}

void FireballBurstStep() {
    if (hidden != nullptr && hidden->active[hidden_slot] == 0) {
        // Its slot done: whatever plays there next is seen.
        hidden->chara[hidden_slot].SetScale(1.0f, 1.0f, 1.0f);
        hidden = nullptr;
        hidden_slot = -1;
    }
}

} // namespace expanded
