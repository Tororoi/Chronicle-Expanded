#include "expanded/weapons/xiao/matador.hpp"

#include <cmath>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dataalloc.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/shot_charge.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "mds.hpp"
#include "mglib.hpp"
#include "shot_effect.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int   kShots = 12;
constexpr int   kMark = 0x20000000; // in a hit's weapon flags: a charged pellet's
constexpr int   kWeaponTextureBlock = 0x1D;
constexpr int   kArenaQuads = 0x4000; // 256 KB for the copy's frames
constexpr float kGlowPull = 6.0f;
constexpr float kHalfPi = 1.57079632679f;
constexpr float kOrange[3] = {120.0f, 45.0f, 0.0f};

struct State {
    ShotCharge    charge{kChargingBullFrames};
    const CSHOT  *shots = nullptr;
    bool          charged[kShots] = {};
    bool          copied = false;
    const CFrame *source = nullptr; // the slingshot in hand it was copied from
};

State      state;
CCharacter copy;

alignas(64) u_long128 arena_store[kArenaQuads];
CDataAlloc2<1> arena;

void OnFloor() {
    for (bool &charged : state.charged) {
        charged = false;
    }
    state.charge.Reset();
    state.copied = false;
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

// The slingshot in hand, copied: the same model and textures, a frame tree of its own.
bool Copy() {
    if (NowWeapon == nullptr || NowWeapon->frame == nullptr) {
        return false;
    }
    if (state.copied && state.source == NowWeapon->frame) {
        return true;
    }
    arena.base = reinterpret_cast<u_char *>(arena_store);
    arena.limit = kArenaQuads;
    arena.used = 0;
    copy = *NowWeapon;
    copy.motion[0] = &copy.motion_type;
    copy.frame = reinterpret_cast<CFrame *>(CopyFrameVu1(reinterpret_cast<CFrameVu1 *>(NowWeapon->frame), &arena));
    if (copy.frame == nullptr) {
        state.copied = false;
        return false;
    }
    copy.frame->DeleteReference();
    copy.frame->parent = nullptr;
    copy.SetScale(1.0f, 1.0f, 1.0f);
    state.copied = true;
    state.source = NowWeapon->frame;
    return true;
}

bool Ours(const CSHOT &shots, int slot) {
    return slot >= 0 && slot < kShots && &shots == state.shots && state.charged[slot];
}

} // namespace

bool ChargingBullActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && WeaponActive(ITEM_WEAPON_MATADOR);
}

void ChargingBullStep() {
    state.charge.Step(ChargingBullActive());
}

void ChargingBullFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kShots) {
        return;
    }
    if (&shots == state.shots) {
        state.charged[slot] = false;
    }
    if (!ChargingBullActive() || !state.charge.TakeCharged()) {
        return;
    }
    if (&shots != state.shots) {
        state.shots = &shots;
        for (bool &charged : state.charged) {
            charged = false;
        }
    }
    shots.damage[slot] = static_cast<int>(std::lround(static_cast<float>(shots.damage[slot]) * kChargingBullDamage));
    state.charged[slot] = true;
    Copy();
}

void ChargingBullHit(const CSHOT &shots, int slot, COLLISION_HIT &hit) {
    if (!Ours(shots, slot)) {
        return;
    }
    hit.weapon_flags |= kMark;
    // Shoved along its flight, from a point back down its line.
    const float *v = shots.vector[slot];
    float        length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length <= 0.0f) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        hit.knockback_origin[i] = shots.pos[slot][i] - v[i] / length * kChargingBullBehind;
    }
    hit.knockback_origin[3] = 1.0f;
    hit.knockback_speed = kChargingBullKickSpeed;
    hit.knockback_decay = kChargingBullKickDecay;
    hit.knockback_mode = 2;
}

void ChargingBullEnded(const CSHOT &shots, int slot) {
    if (Ours(shots, slot)) {
        state.charged[slot] = false;
    }
}

bool ChargingBullHidden(const CSHOT &shots, int slot) {
    return Ours(shots, slot) && state.copied;
}

bool ChargingBullHit(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kMark) != 0;
}

bool ChargingBullCharged(const CSHOT &shots, int slot) {
    return Ours(shots, slot);
}

void ChargingBullDraw() {
    if (state.shots == nullptr) {
        return;
    }
    for (int slot = 0; slot < kShots; slot++) {
        if (!state.charged[slot] || state.shots->used[slot] == 0) {
            continue;
        }
        sceVu0FVECTOR at;
        sceVu0CopyVector(at, const_cast<float *>(state.shots->pos[slot]));
        if (state.copied && state.source == (NowWeapon != nullptr ? NowWeapon->frame : nullptr)) {
            const float *v = state.shots->vector[slot];
            copy.SetPosition(at[0], at[1], at[2]);
            // Upright, fork up, its face turned along its flight as it is held.
            copy.SetRotation(-kHalfPi, std::atan2(v[0], v[2]) + kHalfPi, 0.0f);
            TexManager.ReloadTexture(Vif1Packet, kWeaponTextureBlock);
            copy.Draw();
        }
        DrawGlowDisc(GlowLook::kFire, at, kChargingBullGlow, kGlowPull);
    }
}

void ChargingBullLight(const CCharacter &character, float *ambient) {
    if (!state.copied || &character != &copy) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        ambient[i] += kOrange[i];
    }
}

} // namespace expanded
