#include "expanded/weapons/judgement_blade.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <cstdint>

#include "character.hpp"
#include "dataalloc.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/enemies/species.hpp"
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
    bool          at_point = false; // hung over a point, no monster under it
    int           paced = 0;        // frames left in a paced fall; 0 under gravity
    bool          to_the_hilt = false;
    float         length = kJudgementLength; // the copied sword's reach from its grip, at its own size
    const CFrame *source = nullptr;          // the sword in hand it was copied from
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
    blade.source = MainWeapon.frame;
    // Its reach: grip to its tip's hit point (dcol1), as the sword in hand has it.
    CFrame *tip = FrameNamed(MainWeapon.frame, "dcol1");
    if (tip != nullptr) {
        sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, grip, end;
        MainWeapon.frame->GetWorldPosition(grip, origin);
        tip->GetWorldPosition(end, origin);
        float length = std::hypot(end[0] - grip[0], end[1] - grip[1], end[2] - grip[2]);
        blade.length = length > 1.0f ? length : kJudgementLength;
    }
    return true;
}

bool Alive(const Blade &b) {
    if (b.at_point) {
        return true;
    }
    return b.unit != nullptr && b.monster >= 0 && b.unit->monster[b.monster].state != -1 && b.unit->monster[b.monster].hp > 0;
}

float Reach() {
    return blade.length * kJudgementScale;
}

// The grip's height over the monster: its tip kJudgementMargin over the kind's height (grown with a grown monster).
void PlaceOver(Blade &b) {
    sceVu0FVECTOR at;
    b.unit->chara[b.monster][0].GetPosition(at);
    b.grip[0] = at[0];
    b.grip[1] = at[1] + MonsterHeight(*b.unit, b.monster) + kJudgementMargin + Reach();
    b.grip[2] = at[2];
    b.ground = at[1];
    b.yaw = CharaMain.rotation.y;
}

} // namespace

void JudgementHover(CMonstorUnit &unit, int monster) {
    if (blade.phase == Phase::kFalling || blade.phase == Phase::kLanded) {
        return;
    }
    if (!blade.copied || blade.source != MainWeapon.frame) {
        Copy(); // without a sword to copy it hangs unseen
    }
    if (blade.monster != monster || blade.unit != &unit) {
        blade.shown = 0.0f;
    }
    blade.unit = &unit;
    blade.monster = monster;
    blade.at_point = false;
    blade.phase = Phase::kHanging;
    if (Alive(blade)) {
        PlaceOver(blade);
    }
}

void JudgementHoverAt(const float *point, float height, float shown) {
    if (blade.phase == Phase::kFalling || blade.phase == Phase::kLanded) {
        return;
    }
    if (!blade.copied || blade.source != MainWeapon.frame) {
        Copy();
    }
    blade.unit = nullptr;
    blade.monster = -1;
    blade.at_point = true;
    blade.phase = Phase::kHanging;
    blade.grip[0] = point[0];
    blade.grip[1] = point[1] + height;
    blade.grip[2] = point[2];
    blade.ground = point[1];
    blade.yaw = CharaMain.rotation.y;
    blade.shown = std::clamp(shown, 0.0f, 1.0f);
}

void JudgementRelease() {
    if (blade.phase == Phase::kHanging) {
        blade.phase = Phase::kFading;
    }
}

bool JudgementDrop(int frames, bool to_the_hilt) {
    if (blade.phase != Phase::kHanging || !Alive(blade)) {
        return false;
    }
    blade.phase = Phase::kFalling;
    blade.speed = 0.0f;
    blade.paced = std::max(0, frames);
    blade.to_the_hilt = to_the_hilt;
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
    blade.at_point = false;
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
            if (!blade.at_point) { // a point's hover is shown as its owner says
                blade.shown = std::min(1.0f, blade.shown + 1.0f / kJudgementFadeFrames);
            }
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
            if (!blade.at_point && Alive(blade)) {
                sceVu0FVECTOR at;
                blade.unit->chara[blade.monster][0].GetPosition(at);
                blade.grip[0] = at[0];
                blade.grip[2] = at[2];
                blade.ground = at[1];
            }
            float stop = blade.ground + (blade.to_the_hilt ? 0.0f : Reach());
            if (blade.paced > 0) {
                // The rest of the way in the frames left, evenly.
                blade.grip[1] -= (blade.grip[1] - stop) / static_cast<float>(blade.paced--);
            } else {
                blade.speed += kJudgementGravity * kFrameSeconds * kFrameSeconds;
                blade.grip[1] -= blade.speed;
            }
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
    copy.SetRotation(std::numbers::pi_v<float> * 0.5f, blade.yaw, 0.0f);
    copy.ambient_offset[3] = 128.0f * blade.shown;
    TexManager.ReloadTexture(Vif1Packet, kWeaponTextureBlock);
    copy.Draw();
}

} // namespace expanded
