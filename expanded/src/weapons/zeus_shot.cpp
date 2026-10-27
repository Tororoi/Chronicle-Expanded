#include "expanded/weapons/zeus_shot.hpp"

#include <algorithm>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/scene_light.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/sword_of_zeus.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"


namespace expanded {

namespace {


enum class Charge {
    kIdle,
    kCharging,
    kPrimed,
    kChain, // locked on: pellets call bolts
};

struct Flight {
    const CSHOT *shots = nullptr;
    int          pellet = -1;
    bool         charged = false;
};

struct State {
    Charge        charge = Charge::kIdle;
    int           frames = 0; // in the phase
    int           charged = 0;
    int           unguarded = 0;
    int           hold = 0; // frames the slingshot has been held drawn
    int           held = 0; // how long it was held drawn before it last let go
    bool          dimmed = false;
    bool          struck = false; // a bolt came down in the window
    float         sling = 0.0f;   // the slingshot's white, 0..1
    Flight        flights[kPelletSlots];
};

State state;

// The live monster nearest point, within reach, or -1.
int MonsterAt(const float *point, float reach) {
    if (NowMonstorUnit == nullptr) {
        return -1;
    }
    int   nearest = -1;
    float best = reach;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        const MONSTOR &live = NowMonstorUnit->monster[monster];
        if (live.state == -1 || live.hp <= 0) {
            continue;
        }
        sceVu0FVECTOR at;
        NowMonstorUnit->chara[monster][0].GetPosition(at);
        float distance = std::hypot(at[0] - point[0], at[2] - point[2]);
        if (distance <= best) {
            best = distance;
            nearest = monster;
        }
    }
    return nearest;
}

void Flash(const float *point) {
    SolarBlastFlash(kZeusFlash, point, 0.0f);
}

void Spend() {
    if (state.charge != Charge::kIdle || state.dimmed) {
        SceneDimEnd();
    }
    Flight flights[kPelletSlots];
    std::copy(std::begin(state.flights), std::end(state.flights), std::begin(flights));
    state = {};
    std::copy(std::begin(flights), std::end(flights), std::begin(state.flights)); // the pellets out still owe their bolts
}

bool ChargedOut() {
    return std::any_of(std::begin(state.flights), std::end(state.flights), [](const Flight &f) { return f.shots != nullptr && f.charged; });
}

} // namespace

bool ZeusShotActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && WeaponWielded(ITEM_WEAPON_SUPER_STEVE) &&
           SuperSteveSphere() == ITEM_WEAPON_SWORD_OF_ZEUS;
}

void ZeusShotStep() {
    if (!ZeusShotActive()) {
        if (state.charge != Charge::kIdle || state.dimmed) {
            state = {};
            SceneDimEnd();
        }
        return;
    }
    // How long the slingshot was last held drawn: the shot it lets go of reads it; the room darkens as it builds.
    if (BtActStatus.action_on != 0 && (BtActStatus.action_no == kXiaoDraw || BtActStatus.action_no == kXiaoHold)) {
        if (++state.hold == kZeusChargedFrames) {
            ChargeCompleteFlash();
        }
        if (state.charge == Charge::kIdle || state.charge == Charge::kChain) {
            state.dimmed = true;
            SceneDim(kZeusFlash.prime_dim * std::min(1.0f, static_cast<float>(state.hold) / kZeusChargedFrames));
        }
    } else if (state.hold > 0) {
        state.held = state.hold;
        state.hold = 0;
    } else if (state.dimmed && state.charge == Charge::kIdle && !ChargedOut()) {
        state.dimmed = false; // let go short of the charge, or its bolt came: the dim lifts
        SceneDimEnd();
    }
    state.frames++;

    switch (state.charge) {
        case Charge::kIdle:
            state.sling = std::max(0.0f, state.sling - 1.0f / kSolarFadeFrames);
            if (Guarding() && !Blinding()) {
                state.charge = Charge::kCharging;
                state.charged = state.unguarded = state.frames = 0;
            }
            break;
        case Charge::kCharging: {
            state.unguarded = Guarding() ? 0 : state.unguarded + 1;
            if (state.unguarded > kSolarGuardGrace || Blinding()) {
                Spend();
                break;
            }
            if (state.unguarded == 0) {
                state.charged++;
            }
            float k = std::min(1.0f, static_cast<float>(state.charged) / kZeusGuardFrames);
            state.sling = k;
            SceneDim(kZeusFlash.prime_dim * k);
            if (state.charged >= kZeusGuardFrames) {
                state.charge = Charge::kPrimed;
                state.frames = 0;
                ChargeCompleteFlash();
            }
            break;
        }
        case Charge::kPrimed:
            state.sling = 1.0f;
            SceneDim(kZeusFlash.prime_dim);
            if (state.frames >= kZeusPrimedFrames) {
                Spend();
            }
            break;
        case Charge::kChain:
            state.sling = 1.0f;
            if (!state.struck) {
                SceneDim(kZeusFlash.prime_dim); // until the first bolt's flash takes it over
            }
            if (state.frames >= kZeusChainFrames) {
                bool struck = state.struck;
                Spend();
                if (struck) {
                    state.dimmed = false;
                }
            }
            break;
    }
}

void ZeusShotReset() {
    state = {};
}

void ZeusShotFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots) {
        return;
    }
    state.flights[slot] = {}; // a pellet fired into this slot before is gone, ended or not
    if (!ZeusShotActive()) {
        return;
    }
    if (state.charge == Charge::kPrimed) {
        state.frames = 0;
        if (LockedTarget() >= 0) {
            state.charge = Charge::kChain; // the window opens with this shot, which is the first to call a bolt
            state.struck = false;
        } else {
            // Not locked on: this pellet calls its bolt where it lands, as a charged shot's does; the prime dim holds
            // until it comes down.
            state.charge = Charge::kIdle;
            state.dimmed = true;
            state.held = kZeusChargedFrames;
        }
    }
    bool charged = std::max(state.hold, state.held) >= kZeusChargedFrames;
    state.held = 0;
    if (!charged && state.charge != Charge::kChain) {
        return;
    }
    // A pellet that calls a bolt hurts nothing itself.
    shots.damage[slot] = 0;
    state.flights[slot] = {&shots, slot, charged};
}

void ZeusShotEnded(const CSHOT &shots, int slot, const float *point, bool struck) {
    if (slot < 0 || slot >= kPelletSlots) {
        return;
    }
    Flight &flight = state.flights[slot];
    if (flight.shots != &shots || flight.pellet != slot) {
        return;
    }
    bool charged = flight.charged;
    flight = {};
    int monster = struck ? MonsterAt(point, kZeusStruckReach) : -1;
    // A pellet that left in the window owes its bolt even if the window lapsed while it flew; one that met nothing
    // calls none, unless it was charged.
    // The window's first bolt opens a combo, its later ones follow on.
    Thunder thunder = state.charge == Charge::kChain && state.struck ? Thunder::kCombo : Thunder::kFirst;
    bool    bolt = monster >= 0 ? BoltOn(monster, true, thunder) : charged && BoltAt(point, -1, true, thunder);
    if (!bolt) {
        if (charged && state.dimmed && state.charge == Charge::kIdle) {
            state.dimmed = false;
            SceneDimEnd();
        }
        return;
    }
    state.struck = true;
    if (state.charge != Charge::kChain) {
        state.dimmed = false;
    }
    Flash(point);
}

void ZeusShotLight(const CCharacter &character, float *ambient) {
    if (state.sling <= 0.0f || &character != NowWeapon || !ZeusShotActive()) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] += kSolarBladeWhite * state.sling;
    }
}

bool ZeusShotCallsBolt(const CSHOT &shots, int slot) {
    return slot >= 0 && slot < kPelletSlots && state.flights[slot].shots == &shots && state.flights[slot].pellet == slot;
}

} // namespace expanded
