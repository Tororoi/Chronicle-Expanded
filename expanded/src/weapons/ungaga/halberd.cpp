#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/ungaga/halberd.hpp"

#include <cmath>

#include "bt_shot_effect.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/shot_charge.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

extern CSHOT_EFFECT CharaMainEffect;

namespace expanded {

namespace {

constexpr int kPhases = 4;
constexpr int kSlots = 8;

// Ungaga's charge effect at a tier: his main effect reads a copy of its own description, scaled.
struct Charge {
    BT_SHOT_EFFECT     scaled = {};
    BT_SHOT_EFFECT    *own = nullptr;  // the description the effect was entered with, while the copy stands in
    const TornadoTier *tier = nullptr; // the copy's
};

Charge     charge;
ShotCharge shot{kTornadoShotFrames};

void OnFloor() {
    shot.Reset();
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

void SizeSlots(CSHOT_EFFECT &effect, float size) {
    for (int slot = 0; slot < effect.slot_count && slot < kSlots; slot++) {
        effect.chara[slot].SetScale(size, size, size);
    }
}

// The effect's own description and size back, where it still reads the copy (a floor load, a menu or another character
// enters it again with its own).
void PutBack() {
    if (charge.own != nullptr && CharaMainEffect.effect_data == &charge.scaled) {
        CharaMainEffect.effect_data = charge.own;
        SizeSlots(CharaMainEffect, 1.0f);
    }
    charge.own = nullptr;
    charge.tier = nullptr;
}

// The effect at tier: entered again since it last read the copy, the copy is taken from what it was entered with.
void Apply(const TornadoTier &tier) {
    CSHOT_EFFECT &effect = CharaMainEffect;
    if (effect.effect_data == nullptr) {
        return;
    }
    if (effect.effect_data != &charge.scaled) {
        charge.own = effect.effect_data;
        charge.tier = nullptr;
    }
    if (charge.tier == &tier) {
        return;
    }
    charge.scaled = *charge.own;
    for (int phase = 0; phase < kPhases; phase++) {
        charge.scaled.speed[phase] *= tier.travel;
        charge.scaled.radius[phase] *= tier.size;
    }
    charge.tier = &tier;
    effect.effect_data = &charge.scaled;
    // Set never rewrites a slot's scale, and the draw folds it into the model each frame.
    SizeSlots(effect, tier.size);
}

} // namespace

const TornadoTier *TornadoTierOf(int weapon_no) {
    switch (weapon_no) {
        case ITEM_WEAPON_HALBERT:
            return &kTornadoHalberd;
        case ITEM_WEAPON_SCORPION:
            return &kTornadoScorpion;
        case ITEM_WEAPON_MIRAGE:
        case ITEM_WEAPON_CACTUS:
            return &kTornadoMirage;
        case ITEM_WEAPON_HERCULES_WRATH:
        case ITEM_WEAPON_TERRA_SWORD:
        case ITEM_WEAPON_BABEL_S_SPEAR:
            return &kTornadoHercules;
        default:
            return nullptr;
    }
}

const TornadoTier *TornadoChargeTier() {
    if (UserStatus == nullptr || UserStatus->cur_chara != CHARA_UNGAGA || NowWeaponHave == nullptr || NowMainEffect != &CharaMainEffect) {
        return nullptr;
    }
    return TornadoTierOf(HeldWeapon());
}

const TornadoTier *TornadoShotTier() {
    if (UserStatus == nullptr || UserStatus->cur_chara != CHARA_XIAO) {
        return nullptr;
    }
    return TornadoTierOf(SuperSteveSphere());
}

void TornadoChargeStep() {
    shot.Step(TornadoShotTier() != nullptr);
    const TornadoTier *tier = TornadoChargeTier();
    if (tier == nullptr) {
        PutBack();
        return;
    }
    Apply(*tier);
}

void TornadoShotFired(CSHOT &shots, int slot) {
    const TornadoTier *tier = TornadoShotTier();
    if (slot < 0 || slot >= kPelletSlots || tier == nullptr || !shot.TakeCharged()) {
        return;
    }
    shots.size[slot] *= tier->size;
    for (int axis = 0; axis < 3; axis++) {
        shots.vector[slot][axis] *= tier->travel;
    }
    if (shots.damage[slot] > 0) {
        // Halves to even, as the mod rounds.
        shots.damage[slot] = static_cast<int>(std::nearbyint(static_cast<float>(shots.damage[slot]) * kTornadoShotDamage));
    }
}

} // namespace expanded
