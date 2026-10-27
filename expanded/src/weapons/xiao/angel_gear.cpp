#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/xiao/angel_gear.hpp"
#include "sound_effect.h"

#include <algorithm>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/angel_gear/shield_prop.hpp"
#include "expanded/weapons/xiao/angel_gear/shield_ring.hpp"
#include "expanded/weapons/xiao/angel_gear/shot_reflect.hpp"
#include "expanded/weapons/xiao/angel_shooter.hpp"
#include "itemdata.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

using namespace angel_gear;

constexpr int   kHealAbility = 0x800; // the weapon flags' Heal
constexpr float kFullGauge = 100.0f;
constexpr float kXiaoGaugeRate = 1.5f; // the refill's own rate for Xiao
constexpr float kOutOfReach = 1.0e6f;

struct Reflector {
    bool  live = false; // on as of the last step
    float alpha = 0.0f;
    bool  jingled = false;
    bool  dispelling = false;
    bool  cooling = false;
    bool  solid = false;
    int   hits = 0;
    int   pull = -1; // frames into the fire cycle, or -1
    int   wait = 0;  // frames the first waiting shot has waited to be fired
};

Reflector reflector;

void OnFloor() {
    GuardianReflectorReset();
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

float HerYaw() {
    float rotation[4];
    CharaMain.GetRotation(rotation);
    return rotation[1];
}

// Everything put back as the game has it.
void TakeDown() {
    RingUp(false);
    PropDown();
    ReflectReset();
    PropOrbitTarget(0.0f);
    reflector = Reflector{};
}

// A strike on the shield: its health off the gauge, and the last one breaks it.
void Struck() {
    reflector.hits = std::max(0, reflector.hits - 1);
    BtActStatus.action_gauge = kFullGauge * reflector.hits / kReflectorHits;
    if (reflector.hits == 0) {
        reflector.dispelling = true;
        reflector.cooling = true;
        SndSePlay(SE_WEAPON_BREAK, -1, 0);
    }
}

// The shield follows the guard: up for the whole guard on a full gauge, clearing away when she lets go, and fast when broken.
void FollowGuard(bool guarding) {
    if (reflector.dispelling) {
        reflector.alpha -= 1.0f / kReflectorDispel;
        if (reflector.alpha <= 0.0f || !PropUp()) {
            reflector.alpha = 0.0f;
            PropDown();
            reflector.dispelling = false;
        }
    } else if (guarding) {
        bool ready = !reflector.cooling && BtActStatus.action_gauge >= kReflectorFullGauge;
        if (!PropUp() && ready && PropSpawn()) {
            reflector.alpha = 0.0f;
            reflector.jingled = false;
            reflector.hits = kReflectorHits;
            BtActStatus.action_gauge = kFullGauge;
        }
        if (PropUp()) {
            reflector.alpha = std::min(1.0f, reflector.alpha + 1.0f / kReflectorFadeIn);
            if (!reflector.jingled && reflector.alpha >= 1.0f) {
                reflector.jingled = true;
                SndSePlay(SE_SPARKLE, -1, 0);
            }
        }
    } else if (PropUp()) {
        reflector.alpha -= 1.0f / kReflectorFadeOut;
        if (reflector.alpha <= 0.0f) {
            reflector.alpha = 0.0f;
            PropDown();
        }
    }
}

// The fire cycle, the weapon's own keys: draw, hold, shoot (the shot leaving), back to the catch pose. It starts once
// nothing more is coming, or the first waiting shot has waited long enough.
void FireCycle(bool guarding, const float *her) {
    if (guarding && PropUp() && reflector.alpha >= 1.0f && (PendingCount() > 0 || reflector.pull >= 0)) {
        if (reflector.pull < 0 && (++reflector.wait > kReflectorFireWait || ClaimCount() == 0)) {
            PropKey(kPropKeyDraw);
            reflector.pull = 0;
            reflector.wait = 0;
        }
        if (reflector.pull >= 0) {
            reflector.pull++;
            if (reflector.pull == kReflectorDraw) {
                PropKey(kPropKeyHold);
            }
            if (reflector.pull == kReflectorDraw + kReflectorHold) {
                PropKey(kPropKeyShoot);
                float muzzle[3];
                PropMuzzle(muzzle);
                FireReturn(her, muzzle);
            }
            if (reflector.pull >= kReflectorDraw + kReflectorHold + kReflectorShoot) {
                PropKey(kPropKeyIdle);
                reflector.pull = -1;
            }
        }
    } else if (reflector.pull >= 0) {
        PropKey(kPropKeyIdle);
        reflector.pull = -1;
    }
    if (PendingCount() == 0) {
        reflector.wait = 0;
    }
    if (!guarding) {
        DropPending();
    }
}

} // namespace

bool GuardianReflectorActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && BtEventMode == 0 && NowMonstorUnit != nullptr &&
           WeaponActive(ITEM_WEAPON_ANGE_GEAR);
}

void GuardianReflectorStep() {
    if (!GuardianReflectorActive()) {
        if (reflector.live) {
            TakeDown();
        }
        return;
    }
    reflector.live = true;
    if (PropStale()) {
        TakeDown();
        reflector.live = true;
    }
    PropLoad(); // ahead of her guard
    bool  guarding = Guarding();
    float her[4] = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2], 1.0f};
    float yaw = HerYaw();

    RingStep();
    // The gauge is the standing shield's health (a hit on her fills it: put back); after a break it refills, then is free.
    if (PropUp() && !reflector.dispelling) {
        float health = kFullGauge * reflector.hits / kReflectorHits;
        if (std::fabs(BtActStatus.action_gauge - health) > 0.5f) {
            BtActStatus.action_gauge = health;
        }
    }
    if (reflector.cooling && !reflector.dispelling && BtActStatus.action_gauge >= kReflectorFullGauge) {
        reflector.cooling = false;
    }

    FollowGuard(guarding);
    TrackReturns();
    bool shield_up = guarding && PropUp() && !reflector.dispelling;
    if (shield_up) {
        ClaimShots(her);
    }
    if (PropUp()) {
        float bearing;
        if (ChooseBearing(her, yaw, reflector.pull >= 0, &bearing)) {
            PropOrbitTarget(bearing - yaw);
        }
    }
    PropPlace();
    reflector.solid = shield_up && reflector.alpha >= 1.0f;
    RingUp(PropUp());
    float pouch[3];
    PropPouch(pouch);
    SustainClaims(shield_up, reflector.solid, her, pouch);
    FireCycle(guarding, her);
    PropAnimate(reflector.alpha);
}

void GuardianReflectorDraw() {
    if (reflector.live) {
        PropDraw();
    }
}

void GuardianReflectorSpend() {
    if (!reflector.live || !PropUp() || (reflector.alpha < 1.0f && !reflector.dispelling)) {
        return;
    }
    if (SpendOnShield(reflector.dispelling) && !reflector.dispelling) {
        Struck();
    }
}

float GuardianReflectorGaugeGain(float gain) {
    if (!reflector.live) {
        return gain;
    }
    if (PropUp() || reflector.dispelling) {
        return 0.0f; // held as the shield's health
    }
    if (reflector.cooling) {
        // In place of Xiao's 1.5, the rate that fills the gauge from empty over kReflectorCooldown at the weapon's speed.
        float speed = NowWeaponHave != nullptr ? NowWeaponHave->speed / 30.0f : 1.0f;
        return gain / kXiaoGaugeRate * kFullGauge / (kReflectorCooldown * std::max(1.0f, speed));
    }
    return gain;
}

void GuardianReflectorPlayerPoint(float *point) {
    if (ReturnTesting()) {
        point[1] += kOutOfReach; // a returned shot never meets her
    } else if (reflector.live && reflector.solid) {
        PropCatchPoint(point);
    }
}

void AngelGearHealTick() {
    if (UserStatus == nullptr || UserStatus->cur_chara != CHARA_XIAO || !WeaponActive(ITEM_WEAPON_ANGE_GEAR) || NowWeaponHave == nullptr ||
        (NowWeaponHave->flags & kHealAbility) == 0) {
        return;
    }
    if (UserStatus->step_disable != 0 || BtActStatus.in_presentation != 0 || BtEventMode != 0) {
        return; // a chest opening, a talk or an event: the mod's heal waits them out
    }
    // The tick heals Xiao itself; the rest of the party with her.
    CUserStatus &status = *UserStatus;
    for (int chara = 0; chara < 6; chara++) {
        if (chara == CHARA_XIAO || status.hp[chara] <= 0 || status.hp[chara] >= status.max_hp[chara]) {
            continue;
        }
        status.AddNowLife(chara, 1, 0.0f);
    }
}

void GuardianReflectorLight(const CCharacter &character, float *ambient) {
    if (!PropUp() || &character != PropCharacter()) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        ambient[i] += kReflectorTint[i];
    }
}

bool GuardianReflectorUp() {
    return PropUp();
}

float GuardianReflectorAlpha() {
    return reflector.alpha;
}

int GuardianReflectorHits() {
    return reflector.hits;
}

bool GuardianReflectorCooling() {
    return reflector.cooling;
}

void GuardianReflectorReset() {
    TakeDown();
}

} // namespace expanded
