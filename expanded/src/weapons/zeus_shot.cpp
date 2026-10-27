#include "expanded/weapons/zeus_shot.hpp"

#include <algorithm>
#include <cmath>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/scene_light.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/charge.hpp"
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

struct State {
    Charge      charge = Charge::kIdle;
    int         frames = 0;               // in the phase
    GuardCharge guard;                    // the guard's charge
    ShotCharge  draw{kZeusChargedFrames}; // the slingshot drawn and held
    bool        dimmed = false;
    bool        struck = false; // a bolt came down in the window
    float       sling = 0.0f;   // the slingshot's white, 0..1
};

State             state;
PelletSlots<bool> bolts; // the pellets out that call a bolt where they come down, and whether each was charged

void Flash(const float *point) {
    SolarBlastFlash(kZeusFlash, point, 0.0f);
}

void Spend() {
    if (state.charge != Charge::kIdle || state.dimmed) {
        SceneDimEnd();
    }
    state = {}; // the pellets out still owe their bolts
}

bool ChargedOut() {
    return bolts.Any([](bool charged) { return charged; });
}

} // namespace

bool ZeusShotActive() {
    return Playing(CHARA_XIAO) && SuperSteveSphere() == ITEM_WEAPON_SWORD_OF_ZEUS;
}

void ZeusShotStep() {
    if (!ZeusShotActive()) {
        if (state.charge != Charge::kIdle || state.dimmed) {
            state = {};
            bolts.Clear();
            SceneDimEnd();
        }
        return;
    }
    // How long the slingshot was last held drawn: the shot it lets go of reads it; the room darkens as it builds.
    bool was_drawn = state.draw.Holding();
    state.draw.Step(true);
    if (state.draw.Holding()) {
        if (state.charge == Charge::kIdle || state.charge == Charge::kChain) {
            state.dimmed = true;
            SceneDim(kZeusFlash.prime_dim * std::min(1.0f, static_cast<float>(state.draw.Drawn()) / kZeusChargedFrames));
        }
    } else if (!was_drawn && state.dimmed && state.charge == Charge::kIdle && !ChargedOut()) {
        state.dimmed = false; // let go short of the charge, or its bolt came: the dim lifts
        SceneDimEnd();
    }
    state.frames++;

    switch (state.charge) {
        case Charge::kIdle:
            state.sling = std::max(0.0f, state.sling - 1.0f / kSolarFadeFrames);
            if (state.guard.Start()) {
                state.charge = Charge::kCharging;
                state.frames = 0;
            }
            break;
        case Charge::kCharging: {
            GuardCharge::Step step = state.guard.Charge(kZeusGuardFrames);
            if (step == GuardCharge::Step::kDropped) {
                Spend();
                break;
            }
            float k = state.guard.Progress(kZeusGuardFrames);
            state.sling = k;
            SceneDim(kZeusFlash.prime_dim * k);
            if (step == GuardCharge::Step::kFull) {
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
    bolts.Clear();
}

void ZeusShotFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots) {
        return;
    }
    bolts.Drop(slot); // a pellet fired into this slot before is gone, ended or not
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
            state.draw.MarkReady();
        }
    }
    bool charged = state.draw.Ready();
    state.draw.Reset();
    if (!charged && state.charge != Charge::kChain) {
        return;
    }
    // A pellet that calls a bolt hurts nothing itself.
    shots.damage[slot] = 0;
    bolts.Keep(shots, slot, charged);
}

void ZeusShotEnded(const CSHOT &shots, int slot, const float *point, bool struck) {
    std::optional<bool> kept = bolts.Take(shots, slot);
    if (!kept) {
        return;
    }
    bool charged = *kept;
    int  monster = struck && NowMonstorUnit != nullptr ? NearestMonster(*NowMonstorUnit, point, kZeusStruckReach) : -1;
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
    return bolts.Of(shots, slot) != nullptr;
}

} // namespace expanded
