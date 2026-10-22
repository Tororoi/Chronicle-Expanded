#include "expanded/weapons/blast.hpp"

#include <algorithm>
#include <cmath>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "itembombeffect.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// A blast's hit carries this bit in its weapon flags, above the ones a weapon's record has.
constexpr int kBlastMark = 0x40000000;

bool HitFree() {
    for (int i = 0; i < 96; i++) {
        if (NowColData->active[i] == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

float BlastDistance(const CMonstorUnit &unit, int monster, const float *point) {
    const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
    float                       nearest = -1.0f;
    for (int i = 0; i < 16; i++) {
        if (body.timer[i] == 0) {
            continue;
        }
        float edge = std::max(0.0f, std::hypot(body.position[i][0] - point[0], body.position[i][2] - point[2]) - body.radius[i]);
        if (nearest < 0.0f || edge < nearest) {
            nearest = edge;
        }
    }
    if (nearest >= 0.0f) {
        return nearest;
    }
    // No spheres yet: its root.
    sceVu0FVECTOR position;
    const_cast<CMonstorUnit &>(unit).chara[monster][0].GetPosition(position);
    return std::hypot(position[0] - point[0], position[2] - point[2]);
}

int PlantBlast(CMonstorUnit &unit, const float *point, int attack, const BlastOptions &options) {
    if (NowColData == nullptr) {
        return 0;
    }
    int reached = 0;
    for (int monster = 0; monster < 16; monster++) {
        const MONSTOR &live = unit.monster[monster];
        if (monster == options.except || live.state == -1 || live.hp <= 0 || live.revealed != -1) {
            continue;
        }
        float distance = BlastDistance(unit, monster, point);
        float times = 0.0f;
        for (const BlastStep &step : kBlastFalloff) {
            if (distance <= step.reach * options.reach) {
                times = step.times;
            }
        }
        if (times <= 0.0f || !HitFree()) {
            continue;
        }
        sceVu0FVECTOR centre;
        unit.chara[monster][0].GetPosition(centre);
        centre[1] += unit.chara[monster][0].body_height * 0.5f;
        int            damage = std::max(1, static_cast<int>(std::lround(attack * times * options.damage)));
        int            hit = NowColData->Set(centre, damage, kBlastHitLife, std::max(10.0f, unit.chara[monster][0].body_height), 0.0f, 2, 2, 0, 0);
        COLLISION_HIT &record = NowColData->hit[hit];
        record.owner = UserStatus != nullptr ? UserStatus->cur_chara : 0;
        record.attack_no = 0;
        record.weapon_flags = kBlastMark;
        if (NowWeaponHave != nullptr) {
            record.vs_monster = NowWeaponHave->vs_monster;
            record.weapon_flags |= NowWeaponHave->flags;
        }
        bool  still = monster == options.still || live.knockback[2] <= 0.0f;
        float speed = still ? 0.0f : kBlastKick.speed * options.kick;
        NowColData->SetKickBack(const_cast<float *>(point), speed, kBlastKick.decay, 2);
        reached++;
    }
    return reached;
}

namespace {

// The opening's explosion bank, program 40: note 67 is the big explosion with the long tail. Shipwreck's monster bank,
// program 77: note 25 is the Pirate's Chariot's cannon.
constexpr OwnSound kToanSound = {"opdat/norn2/majin.pak", "o03b_i.hd", 40, 67, 108};
constexpr OwnSound kSteveSound = {"sound/set/snd102.snd", "d03a_g.hd", 77, 25, 66};

// Read with the first floor, not as the first blast goes off.
[[maybe_unused]] const bool kRegistered = (RegisterOwnSound(kToanSound), RegisterOwnSound(kSteveSound), true);

} // namespace

void PlayBlastSound(BlastSound sound) {
    const OwnSound *own = sound == BlastSound::kToan ? &kToanSound : sound == BlastSound::kSteve ? &kSteveSound
                                                                                                 : nullptr;
    if (own == nullptr || !PlayOwnSound(*own, true)) {
        SndSePlay(SE_EXPLOSION, -1, 0);
    }
}

void ShockRingAt(const float *point, float ring, float scale) {
    if (NowShockWave == nullptr) {
        return;
    }
    CShockWave *wave = NowShockWave;
    sceVu0CopyVector(wave->position, const_cast<float *>(point));
    wave->position[3] = 1.0f;
    // It grows from base out to base + expand.
    wave->base_radius = wave->expand_radius = ring * 0.5f;
    wave->radius = 0.0f;
    wave->expand_steps = 15.0f * scale;
    wave->phase = 0.0f;
    wave->alpha = 0.0f;
    wave->active = true;
}

bool ExplosionAt(const float *point, float scale, float ring, BlastSound sound) {
    if (NowBombEffect == nullptr) {
        return false;
    }
    for (int bomb = 0; bomb < 3; bomb++) {
        if (NowBombEffect[bomb].CheckBomb() != 0) {
            continue;
        }
        // SetBombEffect without its hit.
        NowBombEffect[bomb].SetBomb(const_cast<float *>(point), scale);
        PlayBlastSound(sound);
        if (ring > 0.0f) {
            ShockRingAt(point, ring, scale);
        }
        return true;
    }
    return false;
}

bool BlastHit(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kBlastMark) != 0;
}

} // namespace expanded
