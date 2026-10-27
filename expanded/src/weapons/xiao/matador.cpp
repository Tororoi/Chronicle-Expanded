#include "expanded/weapons/xiao/matador.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dataalloc.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/charge.hpp"
#include "frame.hpp"
#include "framevu1.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "mds.hpp"
#include "mdt.hpp"
#include "mglib.hpp"
#include "shot_effect.hpp"
#include "texture.hpp"
#include "userstatus.hpp"
#include "visualvu1.hpp"

namespace expanded {

namespace {

constexpr int   kWeaponTextureBlock = 0x1D;
constexpr int   kArenaQuads = 0x4000; // 256 KB for the copy's frames
constexpr float kGlowPull = 6.0f;
constexpr float kHalfPi = 1.57079632679f;
constexpr char  kPouch[] = "null24"; // the elastic's pouch bone
constexpr float kCupNudge[2] = {-0.3f, -0.3f}; // from the pouch bone to its cup: up, and along the flight
constexpr float kOrange[3] = {120.0f, 45.0f, 0.0f};

struct State {
    ShotCharge    charge{kChargingBullFrames};
    const CSHOT  *shots = nullptr;
    bool          charged[kPelletSlots] = {};
    bool          copied = false;
    const CFrame *source = nullptr; // the slingshot in hand it was copied from
};

// A mesh of the copy the elastic deforms: its vertices in the catch pose, swapped in only while the copy draws (the
// mesh is the slingshot in hand's own).
struct Pose {
    CFrameVu1         *frame = nullptr;
    sceVu0FVECTOR     *vertices = nullptr;
    std::vector<float> taut;
    std::vector<float> own; // the slingshot in hand's, while the copy draws
};

State             state;
CCharacter        copy;
std::vector<Pose> poses;

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

void CollectMeshes(CFrame *frame) {
    for (; frame != nullptr; frame = frame->brother) {
        CVisualVu1 *visual = reinterpret_cast<CFrameVu1 *>(frame)->GetVisual();
        u_int      *data = visual != nullptr ? visual->GetMDTDataAddress() : nullptr;
        if (data != nullptr) {
            MDT_HEADER *mdt = reinterpret_cast<MDT_HEADER *>(data);
            Pose        pose;
            pose.frame = reinterpret_cast<CFrameVu1 *>(frame);
            pose.vertices = reinterpret_cast<sceVu0FVECTOR *>(reinterpret_cast<u_char *>(mdt) + mdt->vertex_ofs);
            pose.taut.assign(reinterpret_cast<float *>(pose.vertices), reinterpret_cast<float *>(pose.vertices + mdt->vertex_num));
            poses.push_back(std::move(pose));
        }
        CollectMeshes(frame->child);
    }
}

// The copy posed at the catch: its bones at kChargingBullPose of the weapon's motion, and the elastic's vertices as
// that pose deforms them, the slingshot in hand's own put back.
void PoseCatch() {
    poses.clear();
    int             index = -1;
    tagMOTION_TYPE *source = NowWeapon->GetMotionParam(NowWeapon->motion_no, &index, nullptr, nullptr, nullptr);
    if (source == nullptr || source->motion_info == nullptr || index < 0) {
        return;
    }
    CollectMeshes(copy.frame);
    tagMOTION_TYPE motion = *source;
    motion.state.motion_no = motion.state.playing_no = index;
    motion.state.blending = false;
    motion.state.time = kChargingBullPose;
    SetMotionEX(copy.frame, &motion, motion.motion_info, &motion.state, motion.frame_info);
    for (Pose &pose : poses) {
        // The vertices now hold the catch; the slingshot in hand's own were kept in taut.
        std::vector<float> own = std::move(pose.taut);
        pose.taut.assign(reinterpret_cast<float *>(pose.vertices), reinterpret_cast<float *>(pose.vertices) + own.size());
        std::copy(own.begin(), own.end(), reinterpret_cast<float *>(pose.vertices));
    }
}

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
    PoseCatch();
    state.copied = true;
    state.source = NowWeapon->frame;
    return true;
}

bool Ours(const CSHOT &shots, int slot) {
    return slot >= 0 && slot < kPelletSlots && &shots == state.shots && state.charged[slot];
}

} // namespace

bool ChargingBullActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && WeaponActive(ITEM_WEAPON_MATADOR);
}

void ChargingBullStep() {
    state.charge.Step(ChargingBullActive());
}

void ChargingBullFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots) {
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
    hit.weapon_flags |= kChargingBullMark;
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
    hit.knockback_mode = kKnockbackFromOrigin;
}

void ChargingBullEnded(const CSHOT &shots, int slot) {
    if (Ours(shots, slot)) {
        state.charged[slot] = false;
    }
}

bool ChargingBullHit(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kChargingBullMark) != 0;
}

bool ChargingBullCharged(const CSHOT &shots, int slot) {
    return Ours(shots, slot);
}

void ChargingBullDraw() {
    if (state.shots == nullptr) {
        return;
    }
    for (int slot = 0; slot < kPelletSlots; slot++) {
        if (!state.charged[slot] || state.shots->used[slot] == 0) {
            continue;
        }
        sceVu0FVECTOR at;
        sceVu0CopyVector(at, const_cast<float *>(state.shots->pos[slot]));
        if (state.copied && state.source == (NowWeapon != nullptr ? NowWeapon->frame : nullptr)) {
            const float *v = state.shots->vector[slot];
            // Upright, fork up, its face turned along its flight as it is held; then slid so its pouch is on the pellet.
            copy.SetPosition(at[0], at[1], at[2]);
            copy.SetRotation(-kHalfPi, std::atan2(v[0], v[2]) + kHalfPi, 0.0f);
            if (CFrame *pouch = FrameNamed(copy.frame, kPouch)) {
                sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, on;
                pouch->GetWorldPosition(on, origin);
                // The pouch bone sits a little over and ahead of the cup the pellet shows in.
                float flat = std::hypot(v[0], v[2]);
                float back = flat > 0.0f ? kCupNudge[1] / flat : 0.0f;
                copy.SetPosition(2.0f * at[0] - on[0] + v[0] * back, 2.0f * at[1] - on[1] + kCupNudge[0],
                                 2.0f * at[2] - on[2] + v[2] * back);
            }
            for (Pose &pose : poses) {
                float *vertices = reinterpret_cast<float *>(pose.vertices);
                pose.own.assign(vertices, vertices + pose.taut.size());
                std::copy(pose.taut.begin(), pose.taut.end(), vertices);
                pose.frame->attr.remake_pending = 1;
            }
            TexManager.ReloadTexture(Vif1Packet, kWeaponTextureBlock);
            copy.Draw();
            // The slingshot in hand's own back (its motion rebuilds its mesh from them each step).
            for (Pose &pose : poses) {
                std::copy(pose.own.begin(), pose.own.end(), reinterpret_cast<float *>(pose.vertices));
            }
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
