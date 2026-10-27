#include "expanded/weapons/toan/big_bang.hpp"

#include <algorithm>

#include "bt_shot_effect.hpp"
#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/wear.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr float kRingLift = 2.0f; // the shock ring over the blast's point, as the bomb drop's

// Toan's charge: the meter runs from 1 to 3 while the wind-up builds it.
constexpr float kMeterFloor = 1.0f;
constexpr float kMeterCap = 3.0f;
// A bomb's blast: kind 3, waiting on phase 10 (SetBombEffect).
constexpr int kBombKind = 3;
constexpr int kBombPhase = 10;

// explosion.chr: one motion playing once from where it is put, no flight and no hit.
// clang-format off
BT_SHOT_EFFECT explosion_shot = {"explosion", 0, 0, {}, {}, -1, 0, 0, 3, 0, {0, -1, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on
constexpr int kExplosions = 3;

[[maybe_unused]] const bool kRegistered = (RegisterWeaponShotEffect(ITEM_WEAPON_BIG_BANG, WeaponShotForm::Weapon, &explosion_shot, kExplosions), true);

bool Toan() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_TOAN;
}

// The blast is the whirlwind's while no Solar Flash charge holds the blade.
bool Detonates() {
    return Toan() && WeaponWielded(ITEM_WEAPON_BIG_BANG) && SolarFlashPhase() == SolarPhase::kIdle;
}

} // namespace

bool DetonateActive() {
    return WeaponActive(ITEM_WEAPON_BIG_BANG);
}

void DetonationAt(const float *point, float scale, float ring) {
    int index = ShotEffectIndex(&explosion_shot);
    if (index < 0) {
        ExplosionAt(point, scale, ring, BlastSound::kToan);
        return;
    }
    // A shot that does not fly: aimed straight up at no speed (Set lets it go of the model it was copied from).
    CSHOT_EFFECT *explosions = ShotEffectAt(index);
    sceVu0FVECTOR from, to;
    sceVu0CopyVector(from, const_cast<float *>(point));
    from[3] = 1.0f;
    sceVu0CopyVector(to, from);
    to[1] += 1.0f;
    explosions->Set(from, to, 0, -1, 0, nullptr, 0);
    int slot = explosions->current_slot;
    if (slot < 0) {
        ExplosionAt(point, scale, ring, BlastSound::kToan);
        return;
    }
    explosions->velocity[slot][0] = explosions->velocity[slot][1] = explosions->velocity[slot][2] = 0.0f;
    explosions->chara[slot].SetScale(scale, scale, scale);
    // The ring a little over the ground, so the floor does not cut it.
    sceVu0FVECTOR lifted = {point[0], point[1] + kRingLift, point[2], 1.0f};
    ShockRingAt(lifted, ring, scale);
    PlayBlastSound(BlastSound::kToan);
}

void WhirlwindStarted() {
    if (!Detonates() || NowMainEffect == nullptr || NowMonstorUnit == nullptr || NowWeaponHave == nullptr) {
        return;
    }
    // The tornado goes, look and hits: the explosion is the whirlwind's.
    int slot = NowMainEffect->current_slot;
    if (slot >= 0 && slot < 8) {
        NowMainEffect->active[slot] = 0;
    }
    sceVu0FVECTOR feet;
    sceVu0CopyVector(feet, CharaMain.pos);
    PlantBlast(*NowMonstorUnit, feet, NowWeaponHave->attack);
    DetonationAt(feet, kWhirlExplosion, kWhirlRing);
    WearWeapon(kDetonateWhp);
}

bool WhirlSwordHits() {
    return !Detonates();
}

bool PlayerHitSkipped(const COLLISION_HIT &hit) {
    return DetonateActive() && hit.kind == kBombKind && hit.ready_phase == kBombPhase;
}

void DetonateChargeLight(const CCharacter &character, float *ambient) {
    if (&character != NowWeapon || !Detonates() || BtActStatus.action_on == 0 || BtActStatus.action_no != kToanWindup) {
        return;
    }
    float k = std::clamp((BtActStatus.charge_time - kMeterFloor) / (kMeterCap - kMeterFloor), 0.0f, 1.0f);
    for (int c = 0; c < 3; c++) {
        ambient[c] += kSolarBladeWhite * k;
    }
}

} // namespace expanded
