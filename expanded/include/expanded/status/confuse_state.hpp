#pragma once

#include <cstring>

#include "expanded/enemies/enemy_events.hpp"

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded::confuse {

// What Confuse keeps of the floor's monsters, shared by its files: the confusion (confuse.cpp), the hits between
// monsters (confuse_hit.cpp) and the stars (confuse_stars.cpp).

// The dungeon steps 60 times a second.
inline constexpr int kStepsPerSecond = 60;

struct Confusion {
    CMonstorUnit *unit = nullptr;
    // Steps of play since the floor was set up.
    int tick = 0;
    // The step each monster's confusion ends on, 0 for none.
    int until[kFloorMonsters] = {};
    // The confused monsters that hit each monster, most recent first, -1 past the last.
    int hit_by[kFloorMonsters][kFloorMonsters];
    // Each monster's logged target: -1 the player, -2 none.
    int target[kFloorMonsters];
    // The step each attacker last hurt each victim.
    int contact[kFloorMonsters][kFloorMonsters] = {};

    Confusion() {
        std::memset(hit_by, -1, sizeof(hit_by));
        for (int i = 0; i < kFloorMonsters; i++) {
            target[i] = -2;
        }
    }

    void Forget(int slot) {
        until[slot] = 0;
        target[slot] = -2;
        std::memset(hit_by[slot], -1, sizeof(hit_by[slot]));
    }
};

Confusion &State();

// A monster taking part: on the floor near the player, life left, out of its chest.
bool InPlay(const CMonstorUnit &unit, int slot);

// A confused monster in play.
bool Confused(const CMonstorUnit &unit, int slot);

// The last still-confused monster to hit an unconfused monster, or -1.
int Provoker(const CMonstorUnit &unit, int slot);

// The monster that planted a hit (sphere owner slot * 5 + 200, or a shot's monster), or -1.
int Attacker(const COLLISION_HIT &hit);

void Position(CMonstorUnit &unit, int slot, float *position);

// The stars: loaded with the floor's monsters and dropped with them; taken off one monster, or off all.
void StarsLoad();
void StarsDrop();
void StarOff(int slot);
void StarsOff();

} // namespace expanded::confuse
