#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/status/confuse.hpp"

#include <libvu0.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bt_shot_effect.hpp"
#include "collisiondata.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/status/confuse_icon.hpp"
#include "expanded/status/poison.hpp"
#include "itembombeffect.hpp"
#include "itemdata.hpp"
#include "mathutil.hpp"
#include "monstorunit.hpp"
#include "nowload.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "snd.hpp"
#include "texture_port.hpp"

namespace expanded {

namespace {

// The dungeon steps 60 times a second.
constexpr int   kStepsPerSecond = 60;
constexpr int   kConfuseSteps = 20 * kStepsPerSecond;
constexpr float kConfuseChance = 5.0f;
// How often the same attacker may hurt the same monster.
constexpr int kContactSteps = kStepsPerSecond / 2;

// The stars (e114ex): looped, grown in over a quarter second to 1.5x, 3 above the head.
constexpr int   kStars = 8;
constexpr int   kStarsBlock = 0x26;
constexpr float kStarsLift = 3.0f;
constexpr float kStarsScale = 1.5f;
constexpr int   kStarsGrowSteps = kStepsPerSecond / 4;
constexpr float kStarsLead = 1.5f;

// The stars as an effect that only plays.
BT_SHOT_EFFECT StarsDescription = {
    "e114ex", 1, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, 0, 0, 0, 0, 0, {0, -1, -1, -1}, -1, {0, 0}, 0.0f, 0,
    {-1, -1, -1, -1}};

struct Confusion {
    CMonstorUnit *unit = nullptr;
    // Steps of play since the floor was set up.
    int tick = 0;
    // The step each monster's confusion ends on, 0 for none.
    int until[16] = {};
    // The confused monsters that hit each monster, most recent first, -1 past the last.
    int hit_by[16][16];
    // Each monster's logged target: -1 the player, -2 none.
    int target[16];
    // The step each attacker last hurt each victim.
    int contact[16][16] = {};
    // Each monster's star (a slot of the stars effect, or -1) and the step it came on.
    int star[16];
    int star_from[16] = {};

    Confusion() {
        std::memset(hit_by, -1, sizeof(hit_by));
        for (int i = 0; i < 16; i++) {
            target[i] = -2;
            star[i] = -1;
        }
    }

    void Forget(int slot) {
        until[slot] = 0;
        target[slot] = -2;
        std::memset(hit_by[slot], -1, sizeof(hit_by[slot]));
    }
};

Confusion &State() {
    static Confusion state;
    return state;
}

// The stars' effect, loaded into the floor's monster memory.
CSHOT_EFFECT &Stars() {
    static CSHOT_EFFECT stars;
    return stars;
}

bool g_stars_loaded = false;

bool InPlay(const CMonstorUnit &unit, int slot) {
    const MONSTOR &monster = unit.monster[slot];
    return monster.state == 2 && monster.hp > 0 && monster.revealed != 0;
}

bool Confused(const CMonstorUnit &unit, int slot) {
    return slot >= 0 && slot < 16 && State().until[slot] != 0 && InPlay(unit, slot);
}

// The last still-confused monster to hit an unconfused monster, or -1.
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

// The monster that planted a hit (sphere owner slot * 5 + 200, or a shot's monster), or -1.
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

void StarOff(int slot) {
    Confusion &state = State();
    if (state.star[slot] >= 0 && g_stars_loaded) {
        Stars().active[state.star[slot]] = 0;
    }
    state.star[slot] = -1;
}

// Ends the shot that planted hit where it struck, as CSHOT_EFFECT::Step ends one on a target.
void ShotImpact(int hit) {
    COLLISION_HIT &record = NowColData->hit[hit];
    if (NowShotEffect == nullptr || record.monster_no < 0) {
        return;
    }
    CSHOT_EFFECT *found = nullptr;
    int           slot = -1;
    float         best = 0.0f;
    for (int index = 0; index < ShotEffectCount(); index++) {
        CSHOT_EFFECT &effect = *ShotEffectAt(index);
        if (effect.effect_data == nullptr) {
            continue;
        }
        for (int i = 0; i < effect.slot_count && i < 8; i++) {
            if (effect.active[i] == 0 || effect.phase[i] >= 2 || effect.user_id_2[i] != record.monster_no) {
                continue;
            }
            sceVu0FVECTOR at;
            effect.chara[i].GetPosition(at);
            float distance = DistVector(at, record.pos);
            float reach = 1.0f + std::abs(effect.effect_data->speed[effect.phase[i]]);
            if (distance <= reach && (found == nullptr || distance < best)) {
                found = &effect;
                slot = i;
                best = distance;
            }
        }
    }
    if (found == nullptr) {
        return;
    }
    BT_SHOT_EFFECT *data = found->effect_data;
    found->chara[slot].SetPosition(record.pos);
    found->phase[slot] = 2;
    int motion = data->motion[2];
    if (motion == -1) {
        found->active[slot] = 0;
        if (data->end_effect == SHOT_END_EFFECT_BOMB) {
            ShotEffectBomb(record.pos, data, found->user_id_2[slot]); // the owner's damage and the shot's radius
        }
        return;
    }
    found->chara[slot].motion_type.state.time = (float) found->chara[slot].motion_type.motion_info[motion].start;
    found->chara[slot].SetMotion(motion, 6);
    sceVu0Normalize(found->velocity[slot], found->velocity[slot]);
    sceVu0ScaleVectorXYZ(found->velocity[slot], found->velocity[slot], data->speed[2]);
    if (data->sound[2] != -1 && found->no_sound[slot] == 0) {
        SndSePlay(data->sound[2], -1, 0);
    }
}

} // namespace

int ConfuseOptionMessage(int bit) {
    return bit == kConfuseOptionBit ? kConfuseNameMessage : bit + 69;
}

void ConfuseTick() {
    if (!MonstersStepping()) {
        return;
    }
    Confusion &state = State();
    state.tick++;
    for (int slot = 0; slot < 16; slot++) {
        if (state.until[slot] == 0) {
            continue;
        }
        if (state.unit != nullptr && state.unit->monster[slot].hp <= 0) {
            state.Forget(slot);
            continue;
        }
        if (state.tick >= state.until[slot]) {
            state.Forget(slot);
            std::fprintf(stderr, "confuse: monster %d comes to its senses\n", slot);
        }
    }
}

void ConfuseFloorReset(bool drop_stars) {
    for (int slot = 0; slot < 16; slot++) {
        StarOff(slot);
    }
    State() = Confusion();
    if (drop_stars) {
        Stars().Initialize();
        g_stars_loaded = false;
    }
}

void ConfuseFloorLoaded() {
    ConfuseFloorReset(true);
    LoadFile(const_cast<char *>("gedit/s04/chara/e114ex.chr"), read_buffer, NULL);
    wait_now_loading_vsync();
    g_stars_loaded = Stars().Entry2(&StarsDescription, read_buffer, kStarsBlock, &MonstorModelBuffer, kStars) != 0;
}

void ConfuseStep(CMonstorUnit &unit, int slot) {
    Confusion &state = State();
    if (state.unit != &unit) {
        for (int i = 0; i < 16; i++) {
            StarOff(i);
        }
        int tick = state.tick;
        state = Confusion();
        state.unit = &unit;
        state.tick = tick;
    }
    if (unit.monster[slot].hp <= 0) {
        state.Forget(slot);
        return;
    }
    int provoker = Provoker(unit, slot);
    if (provoker >= 0) {
        LogTarget(slot, provoker);
    } else if (state.until[slot] == 0 && state.target[slot] != -2) {
        state.target[slot] = -2;
        std::fprintf(stderr, "confuse: monster %d goes back to the player\n", slot);
    }
}

void ConfuseRoll(CMonstorUnit &unit, int slot) {
    if (!(unit.effect[slot].hit_attributes & kAbilityConfuse)) {
        return;
    }
    MONSTOR &monster = unit.monster[slot];
    if (monster.kind == MONSTER_KIND_NO_LOCK_ON || monster.hp <= 0) {
        return;
    }
    if (!(100.0f * GameFraction() < kConfuseChance)) {
        return;
    }
    Confusion &state = State();
    if (state.until[slot] == 0) {
        state.target[slot] = -2;
        std::fprintf(stderr, "confuse: monster %d is confused\n", slot);
    }
    state.until[slot] = state.tick + kConfuseSteps;
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

void ConfuseScriptPlayerPosition(float *position) {
    if (!ConfuseTarget(*NowMonstorUnit, NowMonstorUnit->current_monster, position)) {
        sceVu0CopyVector(position, CharaMain.pos);
    }
}

int ConfuseFriendlyHit(CMonstorUnit &unit, int victim, const float *position, float radius) {
    Confusion &state = State();
    if (state.unit != &unit || NowColData == nullptr) {
        return -1;
    }
    for (int i = 0; i < kHitRecords; i++) {
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
        if (state.contact[attacker][victim] != 0 && state.tick - state.contact[attacker][victim] < kContactSteps) {
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
    state.contact[attacker][victim] = state.tick;
    if (NowColData->hit[hit].monster_no >= 0) {
        std::fprintf(stderr, "confuse: monster %d's shot hits monster %d\n", attacker, victim);
        ShotImpact(hit);
        NowColData->active[hit] = 0;
    } else {
        std::fprintf(stderr, "confuse: monster %d hits monster %d\n", attacker, victim);
    }
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

void ConfuseStarsDraw() {
    Confusion &state = State();
    CMonstorUnit *unit = state.unit;
    if (!g_stars_loaded || unit == nullptr || unit != NowMonstorUnit) {
        return;
    }
    CSHOT_EFFECT &stars = Stars();
    // The confused monsters nearest the player, one star each.
    int   order[16];
    float distance[16];
    int   count = 0;
    for (int slot = 0; slot < 16; slot++) {
        if (!Confused(*unit, slot)) {
            continue;
        }
        sceVu0FVECTOR at;
        Position(*unit, slot, at);
        float d = DistVector(at, CharaMain.pos);
        int   i = count++;
        while (i > 0 && distance[i - 1] > d) {
            order[i] = order[i - 1];
            distance[i] = distance[i - 1];
            i--;
        }
        order[i] = slot;
        distance[i] = d;
    }
    bool wanted[16] = {};
    for (int i = 0; i < count && i < kStars; i++) {
        wanted[order[i]] = true;
    }
    for (int slot = 0; slot < 16; slot++) {
        if (!wanted[slot]) {
            StarOff(slot);
        }
    }
    for (int slot = 0; slot < 16; slot++) {
        if (!wanted[slot]) {
            continue;
        }
        MONSTOR      &monster = unit->monster[slot];
        // Over the body's height, or the lock-on point where higher.
        sceVu0FVECTOR head;
        Position(*unit, slot, head);
        head[1] += unit->chara[slot][0].body_height;
        if (monster.lockon_frame != NULL && monster.lockon_position[1] > head[1]) {
            head[1] = monster.lockon_position[1];
        }
        head[1] += kStarsLift;
        head[3] = 1.0f;
        int star = state.star[slot];
        if (star < 0 || stars.active[star] == 0) {
            star = stars.Set(head, head, -1, -1, 0, NULL, -1);
            if (star < 0) {
                state.star[slot] = -1;
                continue;
            }
            stars.SetLoop(0);
            state.star[slot] = star;
            state.star_from[slot] = state.tick;
        }
        CCharacter  &model = stars.chara[star];
        MOTION_INFO &key = model.motion_type.motion_info[0];
        if (model.motion_type.state.time >= (float) key.end - kStarsLead) {
            model.motion_type.state.time = (float) key.start;
        }
        float grown = static_cast<float>(state.tick - state.star_from[slot]) / kStarsGrowSteps;
        float scale = kStarsScale * (grown < 1.0f ? (grown > 0.01f ? grown : 0.01f) : 1.0f);
        model.SetPosition(head);
        model.SetScale(scale, scale, scale);
    }
    stars.Draw();
}

void ConfuseIconTexture(const char *name, PortDecodedTexture &texture) {
    // Bit 14's tile: the column of bits 8 to 14, seventh row.
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
    // Only into an empty tile, one colour throughout.
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

} // namespace expanded
