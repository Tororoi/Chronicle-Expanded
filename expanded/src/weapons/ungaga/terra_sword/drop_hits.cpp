#include "expanded/weapons/ungaga/terra_sword/drop_hits.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <vector>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/ungaga/terra_sword.hpp"
#include "expanded/weapons/weapon_hit.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded::terra_sword {

namespace {

constexpr int kHitReserve = 16; // free hit records a drop's hit leaves the game
constexpr int kHitLife = 1;     // planted before the monsters step: CheckDmg tests it once

// What a hit throws its monster with.
enum class Shove {
    kNone,
    kFromRock,   // away from the boulder's spot
    kFromPlayer, // away from the player, as a swing
};

struct Plant {
    int         monster;
    float       share;
    Shove       shove;
    float       spot[3]; // the boulder's (x, floor, z)
    const char *what;
};

struct Watched {
    Plant                plant;
    const COLLISION_HIT *record;
    int                  plants;
    bool                 taken;
};

std::vector<Watched> watched;

int FreeHits() {
    int free = 0;
    for (int active : NowColData->active) {
        free += active == 0 ? 1 : 0;
    }
    return free;
}

// Where the boulder throws a monster from: its spot, or, for one right under it (no way across the floor away from it),
// kRockKickLead past the monster on the far side from the player, so it is thrown toward him.
void RockOrigin(const float *spot, const float *at, float *origin) {
    origin[0] = spot[0], origin[1] = spot[1], origin[2] = spot[2], origin[3] = 1.0f;
    if (std::hypot(at[0] - spot[0], at[2] - spot[2]) >= kRockKickUnder) {
        return;
    }
    float px = CharaMain.pos[0] - at[0], pz = CharaMain.pos[2] - at[2], length = std::hypot(px, pz);
    if (length < 1e-3f) {
        px = 1.0f, pz = 0.0f, length = 1.0f;
    }
    origin[0] = at[0] - px / length * kRockKickLead;
    origin[2] = at[2] - pz / length * kRockKickLead;
}

// One plant of a hit: its record, or null with no record to spare.
const COLLISION_HIT *PlantHit(CMonstorUnit &unit, const Plant &plant) {
    if (NowColData == nullptr || NowWeaponHave == nullptr || FreeHits() <= kHitReserve) {
        return nullptr;
    }
    float          centre[4];
    float          radius = std::max(kDropHitRadius, MonsterBodyCentre(unit, plant.monster, centre));
    int            damage = std::max(1, static_cast<int>(std::nearbyint(static_cast<double>(NowWeaponHave->attack * plant.share))));
    COLLISION_HIT *record = PlantWeaponHit(centre, damage, kHitLife, radius);
    if (record == nullptr) {
        return nullptr;
    }
    record->weapon_flags = kRockMark; // none of the weapon's abilities
    float at[4];
    unit.chara[plant.monster][0].GetPosition(at);
    if (plant.shove == Shove::kFromRock) {
        float origin[4];
        RockOrigin(plant.spot, at, origin);
        NowColData->SetKickBack(origin, kRockKick.speed, kRockKick.decay, kKnockbackFromOrigin);
    } else if (plant.shove == Shove::kFromPlayer) {
        NowColData->SetKickBack(CharaMain.pos, kNutKick.speed, kNutKick.decay, kKnockbackFromOrigin);
    }
    std::fprintf(stderr, "big rock: %s monster %d for %d, at (%.0f, %.0f, %.0f) r %.1f\n", plant.what, plant.monster, damage, centre[0], centre[1],
                 centre[2], radius);
    return record;
}

bool PlantWatched(CMonstorUnit &unit, const Plant &plant) {
    const COLLISION_HIT *record = PlantHit(unit, plant);
    if (record == nullptr) {
        return false;
    }
    watched.push_back({plant, record, 1, false});
    return true;
}

} // namespace

int RockBlast(CMonstorUnit &unit, const float *spot) {
    int hit = 0;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (!MonsterLiving(unit.monster[monster]) || MonsterBodyEdge(unit, monster, spot[0], spot[2]) > kRockBlastRadius) {
            continue;
        }
        if (PlantWatched(unit, {
                                   monster, kRockBlastShare, Shove::kFromRock, {spot[0], spot[1], spot[2]},
                                      "crushed"
        })) {
            hit++;
        }
    }
    return hit;
}

void RockPin(CMonstorUnit &unit, const float *spot) {
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (!MonsterLiving(unit.monster[monster])) {
            continue;
        }
        float at[4];
        unit.chara[monster][0].GetPosition(at);
        if (std::hypot(at[0] - spot[0], at[2] - spot[2]) > kRockRadius) {
            continue;
        }
        if (PlantHit(unit, {
                               monster, kRockPinShare, Shove::kNone, {spot[0], spot[1], spot[2]},
                                  "pinned under the boulder"
        }) != nullptr) {
            LogEvent("big_rock", std::format("\"step\":\"pinned\",\"monster\":{}", monster));
        }
    }
}

bool NutBonk(CMonstorUnit &unit, int monster) {
    return PlantWatched(unit, {monster, kNutBonkShare, Shove::kFromPlayer, {}, "bonked"});
}

bool DropHit(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kRockMark) != 0;
}

void DropHitsStep(CMonstorUnit &unit) {
    std::vector<Watched> left;
    for (Watched &hit : watched) {
        if (hit.taken || !MonsterLiving(unit.monster[hit.plant.monster])) {
            continue;
        }
        std::fprintf(stderr, "big rock: %s monster %d took nothing (plant %d of %d)\n", hit.plant.what, hit.plant.monster, hit.plants, kDropHitTries);
        if (hit.plants >= kDropHitTries) {
            continue;
        }
        if (const COLLISION_HIT *record = PlantHit(unit, hit.plant)) {
            left.push_back({hit.plant, record, hit.plants + 1, false});
        }
    }
    watched = std::move(left);
}

void DropHitTaken(int monster, const COLLISION_HIT &hit) {
    for (Watched &w : watched) {
        if (w.record == &hit && w.plant.monster == monster) {
            w.taken = true;
        }
    }
}

void DropHitsReset() {
    watched.clear();
}

} // namespace expanded::terra_sword
