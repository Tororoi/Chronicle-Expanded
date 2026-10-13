#include "confuse.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <libvu0.h>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "itemdata.hpp"
#include "mathutil.hpp"
#include "monstorunit.hpp"
#include "texture_port.hpp"

#include "icon/confuse.hpp"

extern WEAPON_DATA WeaponList[120];

namespace {

// The game steps the dungeon once a tick, 60 times a second.
constexpr int   kStepsPerSecond = 60;
constexpr int   kConfuseSteps = 20 * kStepsPerSecond;
constexpr float kConfuseChance = 5.0f;
// One hurt of a monster by the same attacker this often: an attack sphere is planted again every step it is
// open.
constexpr int kContactSteps = kStepsPerSecond / 2;

// Babel's Spear carries the ability from its weapon data.
const bool kBabelConfuses = [] {
    WeaponList[ITEM_WEAPON_BABEL_S_SPEAR - ITEM_WEAPON_START].flags |= kWeaponFlagConfuse;
    return true;
}();

struct Confusion {
    CMonstorUnit *unit = nullptr;
    // Damage checks of the monsters in play run in slot order once a step: a slot not above the last one
    // starts the next step. A slot not checked in the step before has been out of play and starts afresh.
    int pass = 0;
    int last_slot = 16;
    int seen[16] = {};
    // Steps left of each monster's confusion.
    int timer[16] = {};
    // The confused monsters that hit each monster, most recent first, -1 past the last.
    int hit_by[16][16];
    // The target each monster goes after, for the log: -1 the player, -2 nothing chosen.
    int target[16];
    // The step each attacker last hurt each victim.
    int contact[16][16] = {};

    Confusion() { Clear(); }

    void Clear() {
        std::memset(timer, 0, sizeof(timer));
        std::memset(hit_by, -1, sizeof(hit_by));
        std::memset(contact, 0, sizeof(contact));
        for (int &t : target) {
            t = -2;
        }
    }

    void Forget(int slot) {
        timer[slot] = 0;
        target[slot] = -2;
        std::memset(hit_by[slot], -1, sizeof(hit_by[slot]));
    }
};

Confusion &State() {
    static Confusion state;
    return state;
}

bool InPlay(const CMonstorUnit &unit, int slot) {
    const MONSTOR &monster = unit.monster[slot];
    return monster.state == 2 && monster.hp > 0 && monster.revealed != 0;
}

bool Confused(const CMonstorUnit &unit, int slot) {
    return slot >= 0 && slot < 16 && State().timer[slot] > 0 && InPlay(unit, slot);
}

// The confused monster a monster that is not confused goes after: the last one to hit it still confused,
// or -1.
int Provoker(const CMonstorUnit &unit, int slot) {
    if (Confused(unit, slot)) {
        return -1;
    }
    for (int attacker : State().hit_by[slot]) {
        if (attacker < 0) {
            break;
        }
        if (Confused(unit, attacker)) {
            return attacker;
        }
    }
    return -1;
}

// The monster whose attack planted a hit: an attack sphere's owner is its slot * 5 + 200, a projectile names
// its monster. -1 for anything else.
int Attacker(const COLLISION_HIT &hit) {
    if (hit.monster_no >= 0 && hit.monster_no < 16) {
        return hit.monster_no;
    }
    if (hit.owner >= 200 && (hit.owner - 200) % 5 == 0 && (hit.owner - 200) / 5 < 16) {
        return (hit.owner - 200) / 5;
    }
    return -1;
}

void Position(CMonstorUnit &unit, int slot, float *position) {
    unit.chara[slot][0].GetPosition(position);
}

void LogTarget(int slot, int target) {
    if (State().target[slot] == target) {
        return;
    }
    State().target[slot] = target;
    if (target >= 0) {
        std::fprintf(stderr, "confuse: monster %d goes after monster %d\n", slot, target);
    } else {
        std::fprintf(stderr, "confuse: monster %d goes after the player\n", slot);
    }
}

} // namespace

int ConfuseOptionMessage(int bit) {
    return bit == kConfuseOptionBit ? kConfuseNameMessage : bit + 69;
}

void ConfuseStep(CMonstorUnit &unit, int slot) {
    Confusion &state = State();
    if (state.unit != &unit) {
        state = Confusion();
        state.unit = &unit;
    }
    if (slot <= state.last_slot) {
        state.pass++;
    }
    state.last_slot = slot;
    if (state.seen[slot] != state.pass - 1 && state.seen[slot] != state.pass) {
        state.Forget(slot);
    }
    state.seen[slot] = state.pass;

    MONSTOR &monster = unit.monster[slot];
    if (monster.hp <= 0) {
        state.Forget(slot);
        return;
    }
    if (state.timer[slot] > 0 && --state.timer[slot] == 0) {
        state.target[slot] = -2;
        std::fprintf(stderr, "confuse: monster %d comes to its senses\n", slot);
    }

    int provoker = Provoker(unit, slot);
    if (provoker >= 0) {
        LogTarget(slot, provoker);
    } else if (state.timer[slot] == 0 && state.target[slot] != -2) {
        state.target[slot] = -2;
        std::fprintf(stderr, "confuse: monster %d goes back to the player\n", slot);
    }

    if (state.timer[slot] > 0) {
        // A slow pulse of yellow over the monster's ambient light.
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(state.pass) * (2.0f * 3.14159265f / kStepsPerSecond));
        monster.palette_override[0] = 110.0f + 30.0f * pulse;
        monster.palette_override[1] = 100.0f + 25.0f * pulse;
        monster.palette_override[2] = 20.0f;
        monster.palette_override_pending = true;
    }
}

void ConfuseRoll(CMonstorUnit &unit, int slot) {
    MONSTOR &monster = unit.monster[slot];
    if (monster.kind == MONSTER_KIND_NO_LOCK_ON || monster.hp <= 0) {
        return;
    }
    if (!(100.0f * static_cast<float>(rand()) / 2147483648.0f < kConfuseChance)) {
        return;
    }
    Confusion &state = State();
    if (state.timer[slot] == 0) {
        state.target[slot] = -2;
        std::fprintf(stderr, "confuse: monster %d is confused\n", slot);
    }
    state.timer[slot] = kConfuseSteps;
}

bool ConfuseTarget(CMonstorUnit &unit, int slot, float *position) {
    if (slot < 0 || slot >= 16 || State().unit != &unit) {
        return false;
    }
    if (Confused(unit, slot)) {
        sceVu0FVECTOR self;
        sceVu0FVECTOR other;
        Position(unit, slot, self);
        int   best = -1;
        float best_distance = DistVector(self, CharaMain.pos);
        for (int i = 0; i < 16; i++) {
            if (i == slot || !InPlay(unit, i)) {
                continue;
            }
            Position(unit, i, other);
            float distance = DistVector(self, other);
            if (distance < best_distance) {
                best_distance = distance;
                best = i;
            }
        }
        LogTarget(slot, best);
        if (best < 0) {
            return false;
        }
        Position(unit, best, position);
        return true;
    }
    int provoker = Provoker(unit, slot);
    if (provoker < 0) {
        return false;
    }
    Position(unit, provoker, position);
    return true;
}

int ConfuseFriendlyHit(CMonstorUnit &unit, int victim, const float *position, float radius) {
    Confusion &state = State();
    if (state.unit != &unit || NowColData == nullptr) {
        return -1;
    }
    for (int i = 0; i < 96; i++) {
        const COLLISION_HIT &hit = NowColData->hit[i];
        if (NowColData->active[i] == 0 || hit.phase != hit.ready_phase) {
            continue;
        }
        int attacker = Attacker(hit);
        if (attacker < 0 || attacker == victim || !InPlay(unit, attacker)) {
            continue;
        }
        // A confused monster hurts any other; a provoked one only the monster it goes after.
        if (!Confused(unit, attacker) && Provoker(unit, attacker) != victim) {
            continue;
        }
        if (state.contact[attacker][victim] != 0 && state.pass - state.contact[attacker][victim] < kContactSteps) {
            continue;
        }
        sceVu0FVECTOR at;
        sceVu0CopyVector(at, const_cast<float *>(position));
        if (DistVector(at, const_cast<float *>(hit.pos)) <= radius + hit.radius) {
            return i;
        }
    }
    return -1;
}

void ConfuseLanded(CMonstorUnit &unit, int hit, int victim) {
    Confusion &state = State();
    int        attacker = Attacker(NowColData->hit[hit]);
    if (attacker < 0) {
        return;
    }
    state.contact[attacker][victim] = state.pass;
    std::fprintf(stderr, "confuse: monster %d hits monster %d\n", attacker, victim);
    if (!Confused(unit, attacker)) {
        return;
    }
    // The victim remembers it, most recent first.
    int *list = state.hit_by[victim];
    if (list[0] == attacker) {
        return;
    }
    int end = 0;
    while (end < 15 && list[end] >= 0 && list[end] != attacker) {
        end++;
    }
    for (int i = end; i > 0; i--) {
        list[i] = list[i - 1];
    }
    list[0] = attacker;
    if (!Confused(unit, victim) && Provoker(unit, victim) == attacker) {
        std::fprintf(stderr, "confuse: monster %d turns on monster %d\n", victim, attacker);
    }
}

void ConfuseIconTexture(const char *name, PortDecodedTexture &texture) {
    // The tile of bit 14: the column of bits 8 to 14, the seventh row.
    constexpr int kTileX = 0xD8;
    constexpr int kTileY = 0x50 + 0x14 * 6;
    if (name == nullptr || std::strcmp(name, "charaface") != 0 || texture.width != 256 || texture.height != 320 ||
        texture.levels.empty()) {
        return;
    }
    std::vector<uint8_t> &pixels = texture.levels[0];
    bool                  indexed = texture.format == gfx::TextureFormat::Index8;
    size_t                bytes = indexed ? 1 : 4;
    if (pixels.size() < static_cast<size_t>(texture.width) * texture.height * bytes) {
        return;
    }
    auto at = [&](int x, int y) { return (static_cast<size_t>(y) * texture.width + x) * bytes; };
    // Only a sheet whose tile is empty: one colour throughout.
    for (int y = 0; y < kConfuseIconSize; y++) {
        for (int x = 0; x < kConfuseIconSize; x++) {
            if (std::memcmp(&pixels[at(kTileX + x, kTileY + y)], &pixels[at(kTileX, kTileY)], bytes) != 0) {
                return;
            }
        }
    }
    for (int y = 0; y < kConfuseIconSize; y++) {
        for (int x = 0; x < kConfuseIconSize; x++) {
            const uint8_t *rgba = &kConfuseIconRgba[(y * kConfuseIconSize + x) * 4];
            if (rgba[3] < 128) {
                continue;
            }
            uint8_t *out = &pixels[at(kTileX + x, kTileY + y)];
            if (!indexed) {
                std::memcpy(out, rgba, 3);
                out[3] = 255;
                continue;
            }
            // The palette's nearest opaque colour.
            int best = -1;
            int best_distance = 0;
            for (int i = 0; i < 256; i++) {
                uint32_t entry = texture.palette[i];
                int      r = entry & 0xFF;
                int      g = (entry >> 8) & 0xFF;
                int      b = (entry >> 16) & 0xFF;
                int      a = entry >> 24;
                if (a < 128) {
                    continue;
                }
                int distance = (r - rgba[0]) * (r - rgba[0]) + (g - rgba[1]) * (g - rgba[1]) + (b - rgba[2]) * (b - rgba[2]);
                if (best < 0 || distance < best_distance) {
                    best = i;
                    best_distance = distance;
                }
            }
            if (best >= 0) {
                *out = static_cast<uint8_t>(best);
            }
        }
    }
}

