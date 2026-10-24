#include "expanded/weapons/heavens_cloud_shot.hpp"
#include "sound_effect.h"

#include <algorithm>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kDrawAction = 11; // Xiao drawing the slingshot back
constexpr int kHoldAction = 12; // and holding it drawn
constexpr int kWindGem = 3;     // MasekiEffect's Wind Gem burst

struct Flight {
    const CSHOT *shots = nullptr;
    int          pellet = -1;
    float        charge = 0.0f;
};

struct State {
    int    hold = 0; // frames the slingshot has been held drawn
    int    held = 0; // how long it was held drawn before it last let go
    bool   flashed[2] = {};
    Flight flights[12];
};

State state;

bool MonsterNear(const float *point, float reach) {
    if (NowMonstorUnit == nullptr) {
        return false;
    }
    for (int monster = 0; monster < 16; monster++) {
        const MONSTOR &live = NowMonstorUnit->monster[monster];
        if (live.state == -1 || live.hp <= 0) {
            continue;
        }
        sceVu0FVECTOR at;
        NowMonstorUnit->chara[monster][0].GetPosition(at);
        if (std::hypot(at[0] - point[0], at[2] - point[2]) <= reach) {
            return true;
        }
    }
    return false;
}

// The Wind Gem's burst at point, scale times its size: the look alone.
void BurstAt(const float *point, float scale) {
    CSHOT_EFFECT &wind = MasekiEffect[kWindGem];
    if (wind.effect_data == nullptr) {
        return;
    }
    sceVu0FVECTOR at = {point[0], point[1] - kTyphoonBurstRise * (scale - 1.0f), point[2], 1.0f};
    sceVu0FVECTOR up = {0.0f, 1.0f, 0.0f, 0.0f};
    wind.Set(at, up, -1, -1, 0, nullptr, -1);
    int slot = wind.current_slot;
    if (slot < 0 || slot >= 8) {
        return;
    }
    wind.life_time[slot] = 0; // it hurts nothing itself
    wind.chara[slot].SetScale(scale, scale, scale);
    SndSePlay(SE_WIND_GEM, -1, 0);
    SndSePlay(SE_EXPLOSION, -1, 0);
}

void Blast(const float *point, float charge) {
    if (NowMonstorUnit == nullptr || NowWeaponHave == nullptr) {
        return;
    }
    const float reach = kTyphoonBlastReach + (kTyphoonBlastReachFull - kTyphoonBlastReach) * charge;
    // Thrown from a little short of where it landed, towards Xiao: a monster struck behind its middle is still thrown
    // away from her.
    sceVu0FVECTOR origin = {point[0], point[1], point[2], 1.0f};
    float         dx = CharaMain.pos[0] - point[0], dz = CharaMain.pos[2] - point[2];
    float         length = std::hypot(dx, dz);
    if (length > kTyphoonBlastPullback) {
        origin[0] += dx / length * kTyphoonBlastPullback;
        origin[2] += dz / length * kTyphoonBlastPullback;
    }
    BlastOptions options;
    options.damage = kTyphoonBlastDamage * (0.5f + 0.5f * charge);
    options.reach = reach / kBlastFalloff[0].reach;
    options.flat = true;
    options.guardable = true;
    options.throw_to = reach;
    options.element = NowWeaponHave->best_elem >= 0 && NowWeaponHave->best_elem < 5 ? NowWeaponHave->best_elem : -1;
    PlantBlast(*NowMonstorUnit, origin, NowWeaponHave->attack, options);
    BurstAt(point, kTyphoonBurstLook + (kTyphoonBurstLookFull - kTyphoonBurstLook) * charge);
}

} // namespace

bool TyphoonShotActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && WeaponWielded(ITEM_WEAPON_SUPER_STEVE) &&
           SuperSteveSphere() == ITEM_WEAPON_HEAVEN_S_CLOUD;
}

float TyphoonShotCharge(int frames) {
    float hold = std::min(1.0f, static_cast<float>(frames) / static_cast<float>(kTyphoonShotFrames));
    if (hold < kTyphoonShotEmpowered) {
        return 0.0f;
    }
    return std::min(1.0f, (hold - kTyphoonShotEmpowered) / (kTyphoonShotFull - kTyphoonShotEmpowered));
}

void TyphoonShotStep() {
    if (!TyphoonShotActive()) {
        state.hold = state.held = 0;
        state.flashed[0] = state.flashed[1] = false;
        return;
    }
    bool drawn = BtActStatus.action_on != 0 && (BtActStatus.action_no == kDrawAction || BtActStatus.action_no == kHoldAction);
    if (!drawn) {
        if (state.hold > 0) {
            state.held = state.hold;
        }
        state.hold = 0;
        state.flashed[0] = state.flashed[1] = false;
        return;
    }
    state.hold++;
    float hold = static_cast<float>(state.hold) / static_cast<float>(kTyphoonShotFrames);
    for (int stage = 0; stage < 2; stage++) {
        if (!state.flashed[stage] && hold >= (stage == 0 ? kTyphoonShotEmpowered : kTyphoonShotFull)) {
            state.flashed[stage] = true;
            ChargeCompleteFlash();
        }
    }
}

void TyphoonShotFired(CSHOT &shots, int slot) {
    if (!TyphoonShotActive() || slot < 0 || slot >= 12) {
        return;
    }
    float charge = TyphoonShotCharge(std::max(state.hold, state.held));
    state.held = 0;
    if (charge <= 0.0f) {
        return;
    }
    shots.size[slot] *= 1.0f + (kTyphoonPellet - 1.0f) * charge;
    shots.damage[slot] = static_cast<int>(static_cast<float>(shots.damage[slot]) * (1.0f + (kTyphoonShotDamage - 1.0f) * charge));
    state.flights[slot] = {&shots, slot, charge};
}

void TyphoonShotEnded(const CSHOT &shots, int slot, const float *point, bool struck) {
    if (slot < 0 || slot >= 12) {
        return;
    }
    Flight &flight = state.flights[slot];
    if (flight.shots != &shots || flight.pellet != slot) {
        return;
    }
    float charge = flight.charge;
    flight = {};
    if (struck || MonsterNear(point, kTyphoonBurstNear)) {
        Blast(point, charge);
    }
}

} // namespace expanded
