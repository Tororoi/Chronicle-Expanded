#include "expanded/weapons/goro/inferno/smash_blast.hpp"

#include <cstdio>
#include <iterator>

#include "bt_shot_effect.hpp"
#include "collisiondata.hpp"
#include "expanded/data/pink_fire.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/last_judgement.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"

namespace expanded {

namespace {

// Its one clip playing once where it is put, no flight and no hit.
// clang-format off
BT_SHOT_EFFECT blast = {"zibaku_f2", 0, 0, {}, {}, -1, 0, 0, 3, 0, {0, -1, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
BT_SHOT_EFFECT pink = {"zibaku_f2", 0, 0, {}, {}, -1, 0, 0, 3, 0, {0, -1, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_INFERNO, WeaponShotForm::Weapon, &blast, kSmashBlasts, kSmashBlastPack),
     RegisterWeaponShotEffect(ITEM_WEAPON_INFERNO, WeaponShotForm::Sphere, &blast, kSmashBlasts, kSmashBlastPack),
     RegisterWeaponShotEffect(ITEM_WEAPON_SATAN_S_AX, WeaponShotForm::Sphere, &pink, kSmashBlasts, kPinkBlastFile), true);

// The impact whose model is hidden, its shockwave widened.
CSHOT_EFFECT *hidden = nullptr;
int           hidden_slot = -1;

void OnFloor() {
    hidden = nullptr;
    hidden_slot = -1;
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

} // namespace

BT_SHOT_EFFECT *SmashBlastEffect() {
    return &blast;
}

bool SmashBlastAt(const float *at, BlastLook look, float size) {
    int index = ShotEffectIndex(look == BlastLook::kPink ? &pink : &blast);
    if (index < 0) {
        std::fprintf(stderr, "inferno: no blast, not loaded on this floor\n");
        return false;
    }
    CSHOT_EFFECT *blasts = ShotEffectAt(index);
    int           shot = SpawnStill(*blasts, at);
    if (shot < 0) {
        return false;
    }
    blasts->chara[shot].SetScale(kSmashBlastScale * size, kSmashBlastScale * size, kSmashBlastScale * size);
    return true;
}

void InfernoImpact(CSHOT_EFFECT &effect, int slot, const float *at) {
    if (!InfernoWielded() || slot < 0 || slot >= static_cast<int>(std::size(effect.chara)) ||
        !SmashBlastAt(at, BlastLook::kOrange, JudgementHammerScale())) {
        return;
    }
    effect.chara[slot].SetScale(0.0f, 0.0f, 0.0f);
    hidden = &effect;
    hidden_slot = slot;
}

void InfernoImpactHit(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit) {
    if (&effect == hidden && slot == hidden_slot) {
        hit.radius = kSmashShockwaveRadius;
    }
}

void SmashBlastStep() {
    if (hidden != nullptr && hidden->active[hidden_slot] == 0) {
        // Its slot done: whatever plays there next is seen.
        hidden->chara[hidden_slot].SetScale(1.0f, 1.0f, 1.0f);
        hidden = nullptr;
        hidden_slot = -1;
    }
}

} // namespace expanded
