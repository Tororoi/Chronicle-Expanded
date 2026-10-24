#include "expanded/weapons/toan/heavens_cloud.hpp"

#include <algorithm>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/blade_scale.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// Toan's charge (ToanKey_Play): the wind-up, the whirlwind's charge level and the whirlwind itself.
constexpr int kWindup = 0xE;
constexpr int kWhirlCharge = 2;
constexpr int kWhirlwind = 0x18;
constexpr int kPhases = 4;

struct Typhoon {
    int   frames = 0;   // into the whirlwind's charge
    float scale = 1.0f; // the sword's size now
    bool  flashed = false;
    int   tornado = -1; // the whirlwind's slot in NowMainEffect, while it is grown
    float radius[kPhases] = {};
};

Typhoon typhoon;

bool Active() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_TOAN && NowWeapon == &MainWeapon &&
           WeaponWielded(ITEM_WEAPON_HEAVEN_S_CLOUD);
}

// The tornado back to its own size, hits and look.
void Shrink() {
    if (typhoon.tornado >= 0 && NowMainEffect != nullptr && NowMainEffect->effect_data != nullptr) {
        std::copy_n(typhoon.radius, kPhases, NowMainEffect->effect_data->radius);
        NowMainEffect->chara[typhoon.tornado].SetScale(1.0f, 1.0f, 1.0f);
    }
    typhoon.tornado = -1;
}

// The whirlwind's hit on every monster in its reach, each thrown clear of it.
void Sweep() {
    if (NowMonstorUnit == nullptr || NowWeaponHave == nullptr) {
        return;
    }
    const float  reach = kTyphoonReach * typhoon.scale;
    BlastOptions options;
    options.damage = kTyphoonDamage;
    options.reach = reach / kBlastFalloff[0].reach;
    options.flat = true;
    options.throw_to = reach + kTyphoonClear;
    options.element = NowWeaponHave->best_elem >= 0 && NowWeaponHave->best_elem < 5 ? NowWeaponHave->best_elem : -1;
    sceVu0FVECTOR feet;
    sceVu0CopyVector(feet, CharaMain.pos);
    PlantBlast(*NowMonstorUnit, feet, NowWeaponHave->attack, options);
}

void Reset() {
    Shrink();
    if (typhoon.scale != 1.0f) {
        ScaleSwordInHand(1.0f);
    }
    typhoon = {};
}

} // namespace

float TyphoonScale() {
    return typhoon.scale;
}

void TyphoonStep() {
    if (!Active()) {
        Reset();
        return;
    }
    if (BtActStatus.action_no == kWindup && BtActStatus.charge_level == kWhirlCharge) {
        typhoon.frames = std::min(typhoon.frames + 1, kTyphoonFrames);
        float t = static_cast<float>(typhoon.frames) / static_cast<float>(kTyphoonFrames);
        typhoon.scale = 1.0f + (kTyphoonScale - 1.0f) * std::pow(t, kTyphoonCurve);
        if (typhoon.frames == kTyphoonFrames && !typhoon.flashed) {
            typhoon.flashed = true;
            ChargeCompleteFlash();
        }
        ScaleSwordInHand(typhoon.scale);
        return;
    }
    if (BtActStatus.action_no == kWhirlwind && typhoon.scale > 1.0f) {
        // The whirlwind keeps the size the charge reached, its tornado with it.
        ScaleSwordInHand(typhoon.scale);
        if (typhoon.tornado >= 0 && NowMainEffect != nullptr && NowMainEffect->active[typhoon.tornado] != 0) {
            NowMainEffect->chara[typhoon.tornado].SetScale(typhoon.scale, typhoon.scale, typhoon.scale);
        }
        return;
    }
    Reset();
}

void TyphoonHits(const float *hit_pos) {
    if (Active() && BtActStatus.action_no == kWhirlwind) {
        PlantAlongBlade(hit_pos, typhoon.scale);
    }
}

void TyphoonWhirlwind() {
    if (!Active() || typhoon.scale <= 1.0f || NowMainEffect == nullptr || NowMainEffect->effect_data == nullptr) {
        return;
    }
    int slot = NowMainEffect->current_slot;
    if (slot < 0 || slot >= 8 || NowMainEffect->active[slot] == 0) {
        return;
    }
    Shrink();
    typhoon.tornado = slot;
    float *radius = NowMainEffect->effect_data->radius;
    std::copy_n(radius, kPhases, typhoon.radius);
    for (int phase = 0; phase < kPhases; phase++) {
        radius[phase] *= typhoon.scale;
    }
    NowMainEffect->chara[slot].SetScale(typhoon.scale, typhoon.scale, typhoon.scale);
    Sweep();
}

} // namespace expanded
