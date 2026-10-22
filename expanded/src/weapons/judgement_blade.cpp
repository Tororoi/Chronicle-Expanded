#include "expanded/weapons/judgement_blade.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "character.hpp"
#include "dataalloc.hpp"
#include "dun/gameloop.hpp"
#include "frame.hpp"
#include "mds.hpp"
#include "mglib.hpp"
#include "monstorunit.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

constexpr float kFrameSeconds = 1.0f / 60.0f;
constexpr int   kWeaponTextureBlock = 0x1D;
constexpr int   kArenaQuads = 0x4000; // 256 KB for the copy's frames

enum class Phase {
    kNone,
    kHanging,
    kFading,
    kFalling,
    kLanded,
};

struct Blade {
    Phase         phase = Phase::kNone;
    CMonstorUnit *unit = nullptr;
    int           monster = -1;
    float         shown = 0.0f; // 0..1
    float         grip[3] = {}; // where the grip is
    float         ground = 0.0f;
    float         speed = 0.0f; // falling, units a frame
    float         yaw = 0.0f;
    bool          copied = false;
};

Blade      blade;
CCharacter copy;

alignas(64) u_long128 arena_store[kArenaQuads];
CDataAlloc2<1> arena;

// The sword in hand, copied: the same model and textures, a frame tree of its own.
bool Copy() {
    if (MainWeapon.frame == nullptr) {
        return false;
    }
    arena.base = reinterpret_cast<u_char *>(arena_store);
    arena.limit = kArenaQuads;
    arena.used = 0;
    copy = MainWeapon;
    copy.motion[0] = &copy.motion_type;
    copy.frame = reinterpret_cast<CFrame *>(CopyFrameVu1(reinterpret_cast<CFrameVu1 *>(MainWeapon.frame), &arena));
    if (copy.frame == nullptr) {
        return false;
    }
    copy.frame->DeleteReference();
    copy.frame->parent = nullptr;
    copy.SetScale(kJudgementScale, kJudgementScale, kJudgementScale);
    blade.copied = true;
    return true;
}

bool Alive(const Blade &b) {
    return b.unit != nullptr && b.monster >= 0 && b.unit->monster[b.monster].state != -1 && b.unit->monster[b.monster].hp > 0;
}

float Reach() {
    return kJudgementLength * kJudgementScale;
}

// The grip's height over the monster: its tip kJudgementMargin over the monster's head.
void PlaceOver(Blade &b) {
    sceVu0FVECTOR at;
    b.unit->chara[b.monster][0].GetPosition(at);
    b.grip[0] = at[0];
    b.grip[1] = at[1] + b.unit->chara[b.monster][0].body_height + kJudgementMargin + Reach();
    b.grip[2] = at[2];
    b.ground = at[1];
    b.yaw = CharaMain.rotation.y;
}

} // namespace

void JudgementHover(CMonstorUnit &unit, int monster) {
    if (blade.phase == Phase::kFalling || blade.phase == Phase::kLanded) {
        return;
    }
    if (!blade.copied) {
        Copy(); // without a sword to copy it hangs unseen
    }
    if (blade.monster != monster || blade.unit != &unit) {
        blade.shown = 0.0f;
    }
    blade.unit = &unit;
    blade.monster = monster;
    blade.phase = Phase::kHanging;
    if (Alive(blade)) {
        PlaceOver(blade);
    }
}

void JudgementRelease() {
    if (blade.phase == Phase::kHanging) {
        blade.phase = Phase::kFading;
    }
}

bool JudgementDrop() {
    if (blade.phase != Phase::kHanging || !Alive(blade)) {
        return false;
    }
    blade.phase = Phase::kFalling;
    blade.speed = 0.0f;
    return true;
}

bool JudgementHanging() {
    return blade.phase == Phase::kHanging;
}

bool JudgementFalling() {
    return blade.phase == Phase::kFalling;
}

bool JudgementLanded(float *point) {
    if (blade.phase != Phase::kLanded) {
        return false;
    }
    point[0] = blade.grip[0];
    point[1] = blade.ground;
    point[2] = blade.grip[2];
    point[3] = 1.0f;
    blade.phase = Phase::kNone;
    blade.shown = 0.0f;
    blade.monster = -1;
    return true;
}

void JudgementReset() {
    blade = {};
}

bool JudgementGlowPoint(float *point) {
    if (blade.phase == Phase::kNone || blade.shown <= 0.0f) {
        return false;
    }
    point[0] = blade.grip[0];
    point[1] = blade.grip[1] - Reach() * 0.5f;
    point[2] = blade.grip[2];
    point[3] = 1.0f;
    return true;
}

bool JudgementIsCopy(const CCharacter &character) {
    return blade.copied && &character == &copy;
}

float JudgementShown() {
    return blade.phase == Phase::kNone ? 0.0f : blade.shown;
}

void JudgementStep() {
    switch (blade.phase) {
        case Phase::kHanging:
            blade.shown = std::min(1.0f, blade.shown + 1.0f / kJudgementFadeFrames);
            if (!Alive(blade)) {
                blade.phase = Phase::kFading;
            }
            break;
        case Phase::kFading:
            blade.shown = std::max(0.0f, blade.shown - 1.0f / kJudgementFadeFrames);
            if (blade.shown <= 0.0f) {
                blade.phase = Phase::kNone;
                blade.monster = -1;
            }
            break;
        case Phase::kFalling: {
            // A blade let go still fading in finishes on the way down.
            blade.shown = std::min(1.0f, blade.shown + 1.0f / kJudgementFadeFrames);
            if (Alive(blade)) {
                sceVu0FVECTOR at;
                blade.unit->chara[blade.monster][0].GetPosition(at);
                blade.grip[0] = at[0];
                blade.grip[2] = at[2];
                blade.ground = at[1];
            }
            blade.speed += kJudgementGravity * kFrameSeconds * kFrameSeconds;
            blade.grip[1] -= blade.speed;
            float stop = blade.ground + Reach();
            if (blade.grip[1] <= stop) {
                blade.grip[1] = stop;
                blade.phase = Phase::kLanded;
            }
            break;
        }
        default:
            break;
    }
}

void JudgementDraw() {
    if (!blade.copied || blade.phase == Phase::kNone || blade.shown <= 0.0f || MainWeapon.frame == nullptr) {
        return;
    }
    // Point-down over the grip, its flat the way Toan faces.
    copy.SetPosition(blade.grip[0], blade.grip[1], blade.grip[2]);
    copy.SetRotation(static_cast<float>(M_PI) * 0.5f, blade.yaw, 0.0f);
    copy.ambient_offset[3] = 128.0f * blade.shown;
    TexManager.ReloadTexture(Vif1Packet, kWeaponTextureBlock);
    copy.Draw();
}

} // namespace expanded
