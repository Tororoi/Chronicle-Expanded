#include "expanded/weapons/heavens_cloud_shot.hpp"
#include "sound_effect.h"

#include <algorithm>
#include <cmath>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kWindGem = 3; // MasekiEffect's Wind Gem burst

// The frame a stage comes at: the first whole frame at or past its share of kTyphoonShotFrames.
constexpr int StageFrame(float share) {
    float at = share * static_cast<float>(kTyphoonShotFrames);
    int   whole = static_cast<int>(at);
    return static_cast<float>(whole) < at ? whole + 1 : whole;
}

struct State {
    ShotCharge         draw{StageFrame(kTyphoonShotFull), StageFrame(kTyphoonShotEmpowered)}; // a flash at each stage
    PelletSlots<float> charges;                                                               // the empowered pellets out, and how far each was
};

State state;

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
    state.draw.Step(TyphoonShotActive());
}

void TyphoonShotFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots) {
        return;
    }
    state.charges.Drop(slot); // a pellet fired into this slot before is gone, ended or not
    if (!TyphoonShotActive()) {
        return;
    }
    float charge = TyphoonShotCharge(state.draw.Held());
    state.draw.Reset();
    if (charge <= 0.0f) {
        return;
    }
    ChargedShotLetGo(); // an empowered shot's wear is a charged one's
    shots.size[slot] *= 1.0f + (kTyphoonPellet - 1.0f) * charge;
    shots.damage[slot] = static_cast<int>(static_cast<float>(shots.damage[slot]) * (1.0f + (kTyphoonShotDamage - 1.0f) * charge));
    state.charges.Keep(shots, slot, charge);
}

void TyphoonShotEnded(const CSHOT &shots, int slot, const float *point, bool struck) {
    std::optional<float> charge = state.charges.Take(shots, slot);
    bool                 near = NowMonstorUnit != nullptr && NearestMonster(*NowMonstorUnit, point, kTyphoonBurstNear) >= 0;
    if (charge && (struck || near)) {
        Blast(point, *charge);
    }
}

} // namespace expanded
