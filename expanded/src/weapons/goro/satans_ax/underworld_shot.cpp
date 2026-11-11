#include "expanded/weapons/goro/satans_ax/underworld_shot.hpp"

#include <cmath>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"

namespace expanded {

namespace {

// A charged pellet in flight: the order it was let go in, and the monsters it has hit (a bit each).
struct Flight {
    int order = 0;
    u32 struck = 0;
};

ShotCharge          draw{kUnderworldShotFrames}; // the slingshot drawn and held
PelletSlots<Flight> flying;
int                 fired = 0;
int                 planted[kHitRecords] = {}; // the slot each marked hit record was planted by

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(UnderworldShotReset), true);

} // namespace

bool UnderworldShotActive() {
    return Playing(CHARA_XIAO) && (SuperSteveSphere() == ITEM_WEAPON_SATAN_S_AX || SuperSteveSphere() == ITEM_WEAPON_INFERNO);
}

bool UnderworldShotInferno() {
    return Playing(CHARA_XIAO) && SuperSteveSphere() == ITEM_WEAPON_INFERNO;
}

bool UnderworldShotDrawn() {
    return UnderworldShotInferno() && draw.Holding();
}

bool UnderworldShotHolding() {
    return UnderworldShotActive() && draw.Holding();
}

int UnderworldShotsFired() {
    return fired;
}

int UnderworldShotSlots() {
    return kPelletSlots;
}

const CSHOT *UnderworldShotIn(int i, int &slot) {
    const CSHOT *found = nullptr;
    flying.ForEach([&](const CSHOT &shots, int s, const Flight &) {
        if (s == i && shots.used[s] != 0) {
            found = &shots;
            slot = s;
        }
    });
    return found;
}

bool UnderworldShotAt(float *at) {
    const CSHOT *last = nullptr;
    int          slot = -1, order = -1;
    flying.ForEach([&](const CSHOT &shots, int s, const Flight &flight) {
        if (shots.used[s] != 0 && flight.order > order) {
            last = &shots;
            slot = s;
            order = flight.order;
        }
    });
    if (last == nullptr) {
        return false;
    }
    if (at != nullptr) {
        for (int i = 0; i < 3; i++) {
            at[i] = last->pos[slot][i];
        }
    }
    return true;
}

void UnderworldShotStep() {
    draw.Step(UnderworldShotActive());
}

void UnderworldShotFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots) {
        return;
    }
    flying.Drop(slot);
    if (!UnderworldShotActive() || !draw.TakeCharged()) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        shots.vector[slot][i] *= kUnderworldShotSpeed;
    }
    shots.size[slot] *= kUnderworldShotSize;
    shots.life[slot] = static_cast<int>(std::lround(static_cast<float>(shots.life[slot]) * kUnderworldShotLife));
    flying.Keep(shots, slot, Flight{.order = ++fired});
}

void UnderworldShotHit(const CSHOT &shots, int slot, COLLISION_HIT &hit) {
    int record = NowColData != nullptr ? static_cast<int>(&hit - NowColData->hit) : -1;
    if (flying.Of(shots, slot) == nullptr || record < 0 || record >= kHitRecords) {
        return;
    }
    hit.weapon_flags |= kUnderworldShotMark;
    planted[record] = slot;
}

void UnderworldShotEnded(const CSHOT &shots, int slot) {
    flying.Take(shots, slot);
}

bool UnderworldShotPierces(const CSHOT &shots, int slot) {
    return flying.Of(shots, slot) != nullptr;
}

bool UnderworldShotRefused(int monster, const COLLISION_HIT &hit) {
    int record = NowColData != nullptr ? static_cast<int>(&hit - NowColData->hit) : -1;
    if ((hit.weapon_flags & kUnderworldShotMark) == 0 || record < 0 || record >= kHitRecords || monster < 0 || monster >= 32) {
        return false;
    }
    Flight *flight = flying.InSlot(planted[record]);
    if (flight == nullptr) {
        return false;
    }
    u32 bit = 1u << monster;
    if ((flight->struck & bit) != 0) {
        return true;
    }
    flight->struck |= bit;
    return false;
}

bool UnderworldShotStruck(int monster) {
    if (monster < 0 || monster >= 32) {
        return false;
    }
    const Flight *last = nullptr;
    flying.ForEach([&](const CSHOT &shots, int s, const Flight &flight) {
        if (shots.used[s] != 0 && (last == nullptr || flight.order > last->order)) {
            last = &flight;
        }
    });
    return last != nullptr && (last->struck & (1u << monster)) != 0;
}

void UnderworldShotReset() {
    draw.Reset();
    flying.Clear();
    fired = 0;
}

} // namespace expanded
