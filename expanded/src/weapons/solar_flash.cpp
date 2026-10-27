#include "expanded/weapons/solar_flash.hpp"

#include <algorithm>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/judgement_blade.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/big_bang.hpp"
#include "expanded/weapons/toan/sword_of_zeus.hpp"
#include "expanded/weapons/wear.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"


namespace expanded {

const SolarProfile kSunSwordFlash = {
    .weapon_no = ITEM_WEAPON_SUN_SWORD,
    .charge_frames = kSolarChargeFrames,
    .shot = false,
    .damage_share = 0.25f,
    .flash_whp = 5.0f,
    .prime_dim = 0.35f,
    .glow = true,
    .glow_size = 20.0f,
    .glow_along = 0.0f,
    .glow_axis = 2.0f, // the round emblem on the hilt
    .look = {.ease_frames = kBlindFrames},
};

const SolarProfile kSolarShotFlash = {
    .weapon_no = ITEM_WEAPON_SUPER_STEVE,
    .charge_frames = kSolarShotChargeFrames,
    .shot = true,
    .damage_share = 0.25f,
    .flash_whp = 5.0f,
    .prime_dim = 0.35f,
    .glow = true,
    .glow_size = 14.0f,
    .glow_along = 0.5f,
    .glow_axis = 0.0f,
    .look = {.ease_frames = kBlindFrames},
};

const SolarProfile kBigBangFlash = {
    .weapon_no = ITEM_WEAPON_BIG_BANG,
    .charge_frames = kSolarChargeFrames,
    .shot = false,
    .damage_share = 0.5f,
    .flash_whp = 5.0f,
    .prime_dim = 0.35f,
    .glow = true,
    .glow_size = 20.0f,
    .glow_along = 0.5f,
    .glow_axis = 0.0f,
    .look = {.light = {236.0f, 226.0f, 255.0f}, .fog = {242.0f, 236.0f, 255.0f}, .fog_amount = 0.8f, .ease_frames = kBlindFrames},
    .glow_look = GlowLook::kBlue,
    .judgement = true,
    .land_whp = 20.0f,
};

namespace {

// The first combo swing's frame the judgement blade is let go on, and the frame it comes forward on: the Sword of
// Zeus's blade is paced to land between them, at the swing's own step (0.3 a frame).
constexpr float kZeusDropFrom = 824.0f;
constexpr float kZeusLandAt = 828.0f;
constexpr float kSwingStep = 0.3f;

} // namespace

const SolarProfile kZeusFlash = {
    .weapon_no = ITEM_WEAPON_SWORD_OF_ZEUS,
    .charge_frames = kSolarChargeFrames,
    .shot = false,
    .damage_share = 0.0f,
    .flash_whp = 0.0f, // each bolt bills its own
    .prime_dim = 0.5f,
    .glow = true,
    .glow_size = 20.0f,
    .glow_along = 0.5f,
    .glow_axis = 0.0f,
    .look = {.light = {228.0f, 240.0f, 255.0f}, .fog = {238.0f, 246.0f, 255.0f}, .fog_amount = 0.8f, .ease_frames = 120},
    .glow_look = GlowLook::kRing,
    .judgement = true,
    .land_whp = 0.0f,
    .primed_white = false,
    .drop_frames = static_cast<int>((kZeusLandAt - kZeusDropFrom) / kSwingStep + 0.999f),
    .to_the_hilt = true,
    .bolts = true,
};

namespace {

// Each combo swing's clip and the frame it comes forward on.
struct Swing {
    float start, end, hit;
};

// The first combo swing's frame the judgement blade is let go on.
constexpr float kDropFrame = 824.0f;
// About how long the blade's fall takes, for the room's darkening along it.
constexpr int kSolarDropFrames = 20;

constexpr Swing kCombo[] = {
    {820.0f, 830.0f, 826.0f},
    {830.0f, 838.0f, 835.0f},
    {838.0f, 847.0f, 844.0f},
    {847.0f, 857.0f, 853.0f},
    {856.0f, 884.0f, 867.0f},
};

// The game's charge-complete colour, which the build-up ramps toward (half of it at the last frame).
constexpr float kChargeRampPeak = 0.5f;
// The flash's own pulse in its light.
constexpr float kFlashPulseSpeed = 30.0f;

// How far toward the camera the glow disc's depth is tested from.
constexpr float kGlowPull = 1.0f;
constexpr float kJudgementGlowNearer = 4.0f; // the hanging blade's glow before its hilt, toward the camera

struct State {
    SolarPhase          phase = SolarPhase::kIdle;
    const SolarProfile *profile = nullptr;
    int                 frames = 0;      // in the phase
    int                 charged = 0;     // frames guarded in the charge
    int                 unguarded = 0;   // frames since the guard was last up
    float               blade = 0.0f;    // the blade's white, 0..1
    float               wielder[3] = {}; // the wielder's own tint
    float               glow = 0.0f;     // the glow's size, 0..1
    float               glow_shrink = 0.0f; // what it loses each frame while it shrinks away
    const CSHOT        *shots = nullptr; // the charged pellet's pool and slot while it flies
    int                 pellet = -1;
    sceVu0FVECTOR       landed = {}; // where it ended
    int                 struck = -1; // the monster it struck there, or -1
    int                 chain_action = -1;   // the Sword of Zeus's combo: the swing that last came forward …
    bool                chain_fired = false; // … and whether its bolt came down
};

State state;

const SolarProfile *ActiveProfile() {
    if (UserStatus == nullptr) {
        return nullptr;
    }
    if (UserStatus->cur_chara == CHARA_TOAN && WeaponWielded(ITEM_WEAPON_SUN_SWORD)) {
        return &kSunSwordFlash;
    }
    if (UserStatus->cur_chara == CHARA_TOAN && WeaponWielded(ITEM_WEAPON_BIG_BANG)) {
        return &kBigBangFlash;
    }
    if (UserStatus->cur_chara == CHARA_TOAN && WeaponWielded(ITEM_WEAPON_SWORD_OF_ZEUS)) {
        return &kZeusFlash;
    }
    if (UserStatus->cur_chara == CHARA_XIAO && WeaponWielded(ITEM_WEAPON_SUPER_STEVE) && SuperSteveSphere() == ITEM_WEAPON_SUN_SWORD) {
        return &kSolarShotFlash;
    }
    return nullptr;
}

// The live monster nearest point, within reach, or -1.
int MonsterAt(const float *point, float reach) {
    if (NowMonstorUnit == nullptr) {
        return -1;
    }
    int   nearest = -1;
    float best = reach;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        const MONSTOR &live = NowMonstorUnit->monster[monster];
        if (live.state == -1 || live.hp <= 0) {
            continue;
        }
        sceVu0FVECTOR position;
        NowMonstorUnit->chara[monster][0].GetPosition(position);
        float distance = std::hypot(position[0] - point[0], position[2] - point[2]);
        if (distance <= best) {
            best = distance;
            nearest = monster;
        }
    }
    return nearest;
}

bool Attacking() {
    int action = BtActStatus.action_no;
    return BtActStatus.action_on != 0 && ((action >= kToanFirstSwing && action <= kToanLastSwing) || action == kToanLunge || action == kToanWhirlwind);
}

// The swing lets a hanging judgement blade go: the first combo swing at its drop frame, a lunge or whirlwind at once.
bool SwingDrops() {
    int action = BtActStatus.action_no;
    if (action == kToanLunge || action == kToanWhirlwind) {
        return true;
    }
    float time = CharaMain.motion_type.state.time;
    return action == kToanFirstSwing && time >= kDropFrame && time <= kCombo[0].end;
}

// The swing has come forward: a lunge or whirlwind at once, a combo hit at its own frame.
bool SwingForward() {
    int action = BtActStatus.action_no;
    if (action == kToanLunge || action == kToanWhirlwind) {
        return true;
    }
    const Swing &swing = kCombo[action - kToanFirstSwing];
    float        time = CharaMain.motion_type.state.time;
    return time >= swing.hit && time <= swing.end;
}

bool Combo(int action) {
    return BtActStatus.action_on != 0 && action >= kToanFirstSwing && action <= kToanLastSwing;
}

// A combo swing winding up: how far it is from its start to coming forward (0..1).
float WindUp(int action) {
    const Swing &swing = kCombo[action - kToanFirstSwing];
    return std::clamp((CharaMain.motion_type.state.time - swing.start) / (swing.hit - swing.start), 0.0f, 1.0f);
}

void SetWielder(float red, float green, float blue) {
    state.wielder[0] = red;
    state.wielder[1] = green;
    state.wielder[2] = blue;
}

void GlowStep() {
    if (state.glow_shrink > 0.0f) {
        state.glow = std::max(0.0f, state.glow - state.glow_shrink);
        state.glow_shrink = state.glow > 0.0f ? state.glow_shrink : 0.0f;
    }
}

// The glow shrinks away from whatever size it has, over kSolarFadeFrames.
void GlowShrink() {
    state.glow_shrink = state.glow / kSolarFadeFrames;
}

bool HitFree() {
    for (int i = 0; i < kHitRecords; i++) {
        if (NowColData->active[i] == 0) {
            return true;
        }
    }
    return false;
}

// The charge dropped: the glow shrinks away rather than going at once.
void Clear() {
    if (state.phase != SolarPhase::kIdle) {
        SceneDimEnd();
    }
    JudgementRelease();
    float glow = state.glow;
    state = {};
    state.glow = glow;
    GlowShrink();
}

// The judgement blade's landing at point: the blast (the full falloff, its explosion), then the flash from there,
// billed as the landing.
void Land(const SolarProfile &profile, const float *point) {
    state = {};
    if (NowMonstorUnit != nullptr && NowWeaponHave != nullptr) {
        PlantBlast(*NowMonstorUnit, point, NowWeaponHave->attack);
    }
    DetonationAt(point, kLandingExplosion, kLandingRing);
    SolarBlastFlash(profile, point, profile.land_whp);
}

void Flash(const SolarProfile &profile) {
    sceVu0FVECTOR point;
    sceVu0CopyVector(point, CharaMain.pos);
    state = {};
    SolarFlashAt(profile, point);
}

// The Sword of Zeus's flash: no hit of its own, the blinding once the bolts' throws are over.
void BoltFlash(const SolarProfile &profile, const float *point) {
    SolarBlastFlash(profile, point, profile.flash_whp);
}

// The bolt on the locked target, its flash, and the combo chained from this swing: true when it came down.
bool ChainFrom(const SolarProfile &profile, int target) {
    if (target < 0 || !BoltOn(target)) {
        return false;
    }
    BoltFlash(profile, CharaMain.pos);
    state.phase = SolarPhase::kChain;
    state.frames = 0;
    state.chain_action = BtActStatus.action_no;
    state.chain_fired = true;
    return true;
}

// A primed Sword of Zeus swing come forward: locked on, the bolt and the chain; else the volley.
void BoltSwing(const SolarProfile &profile) {
    if (ChainFrom(profile, LockedTarget())) {
        return;
    }
    sceVu0FVECTOR point;
    sceVu0CopyVector(point, CharaMain.pos);
    state = {};
    BoltVolley(point);
    BoltFlash(profile, point);
}

// Where a profile's glow sits on the weapon: on its model's length axis, or along between its first two hit points
// when it has them, else at the weapon's own origin in the wielder's hand.
bool WeaponGlowPoint(const SolarProfile &profile, float *point) {
    if (NowWeapon == nullptr || NowWeapon->frame == nullptr) {
        return false;
    }
    if (profile.glow_axis != 0.0f) {
        sceVu0FVECTOR on_axis = {0.0f, 0.0f, profile.glow_axis, 1.0f};
        NowWeapon->frame->GetWorldPosition(point, on_axis);
        return true;
    }
    float         along = profile.glow_along;
    sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 0.0f};
    CFrame       *hilt = FrameNamed(NowWeapon->frame, "dcol0");
    CFrame       *tip = FrameNamed(NowWeapon->frame, "dcol1");
    if (hilt == nullptr || tip == nullptr) {
        NowWeapon->frame->GetWorldPosition(point, origin);
        return true;
    }
    sceVu0FVECTOR end;
    hilt->GetWorldPosition(point, origin);
    tip->GetWorldPosition(end, origin);
    for (int c = 0; c < 3; c++) {
        point[c] += (end[c] - point[c]) * along;
    }
    return true;
}

} // namespace

SolarPhase SolarFlashPhase() {
    return state.phase;
}

float SolarGlowSize() {
    return state.glow;
}

int PlantSolarHits(CMonstorUnit &unit, const float *point, int base_damage, int except, float spared) {
    if (NowColData == nullptr) {
        return 0;
    }
    int struck = 0;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        const MONSTOR &live = unit.monster[monster];
        if (monster == except || live.state == -1 || live.hp <= 0 || live.revealed != -1) {
            continue;
        }
        sceVu0FVECTOR position;
        unit.chara[monster][0].GetPosition(position);
        if (std::hypot(position[0] - point[0], position[2] - point[2]) > kSolarFlashRadius ||
            (spared > 0.0f && BlastDistance(unit, monster, point) <= spared)) {
            continue;
        }
        if (!HitFree()) {
            break;
        }
        int            hit = NowColData->Set(position, base_damage, kSolarHitLife, kSolarHitRadius, 0.0f, 2, 2, 0, 0);
        COLLISION_HIT &record = NowColData->hit[hit];
        record.owner = UserStatus != nullptr ? UserStatus->cur_chara : 0;
        record.attack_no = 0;
        if (NowWeaponHave != nullptr) {
            record.vs_monster = NowWeaponHave->vs_monster;
            record.weapon_flags = NowWeaponHave->flags;
        }
        // A monster the game cannot move takes the hit where it stands.
        bool rooted = live.knockback[2] <= 0.0f;
        NowColData->SetKickBack(const_cast<float *>(point), rooted ? 0.0f : kSolarKickSpeed, kSolarKickDecay, 2);
        struck++;
    }
    return struck;
}

void SolarFlashAt(const SolarProfile &profile, const float *point, int except, float whp) {
    SceneFlash(profile.look);
    setUnitAmbientAnime(kFlashPulseSpeed, 1.0f, profile.look.light[0], profile.look.light[1], profile.look.light[2]);
    if (NowMonstorUnit == nullptr) {
        return;
    }
    if (profile.damage_share > 0.0f && NowWeaponHave != nullptr) {
        int base = std::max(1, static_cast<int>(std::lround(NowWeaponHave->attack * profile.damage_share)));
        PlantSolarHits(*NowMonstorUnit, point, base, except);
    }
    WearWeapon(whp < 0.0f ? profile.flash_whp : whp);
    Blind(*NowMonstorUnit);
}

void SolarBlastFlash(const SolarProfile &profile, const float *point, float whp) {
    SceneFlash(profile.look);
    setUnitAmbientAnime(kFlashPulseSpeed, 1.0f, profile.look.light[0], profile.look.light[1], profile.look.light[2]);
    if (NowMonstorUnit == nullptr) {
        return;
    }
    if (profile.damage_share > 0.0f && NowWeaponHave != nullptr) {
        int base = std::max(1, static_cast<int>(std::lround(NowWeaponHave->attack * profile.damage_share)));
        PlantSolarHits(*NowMonstorUnit, point, base, -1, kBlastFalloff[0].reach);
    }
    WearWeapon(whp);
    BlindAfter(kBlastThrowFrames);
}

void SolarFlashStep() {
    GlowStep();
    const SolarProfile *profile = ActiveProfile();
    if (profile == nullptr || (state.profile != nullptr && state.profile != profile)) {
        Clear();
        return;
    }
    State &s = state;
    s.frames++;
    switch (s.phase) {
        case SolarPhase::kIdle:
            // One flash at a time.
            if (Guarding() && !Blinding()) {
                s.phase = SolarPhase::kCharging;
                s.profile = profile;
                s.frames = s.charged = s.unguarded = 0;
            }
            break;

        case SolarPhase::kCharging: {
            s.unguarded = Guarding() ? 0 : s.unguarded + 1;
            if (s.unguarded > kGuardGrace || Blinding()) {
                Clear();
                break;
            }
            if (s.unguarded == 0) {
                s.charged++;
            }
            const int full = profile->charge_frames;
            float     k = std::min(1.0f, static_cast<float>(s.charged) / full);
            s.blade = k;
            SceneDim(profile->prime_dim * k);
            // The cyan build-up over the charge's last frames.
            float ramp = std::clamp(1.0f - static_cast<float>(full - s.charged) / kSolarRampFrames, 0.0f, 1.0f);
            SetWielder(kChargeColour[0] * kChargeRampPeak * ramp, kChargeColour[1] * kChargeRampPeak * ramp,
                       kChargeColour[2] * kChargeRampPeak * ramp);
            // The glow grows with the charge, full the moment it primes.
            s.glow = profile->glow ? k : 0.0f;
            s.glow_shrink = 0.0f;
            if (s.charged >= full) {
                s.phase = SolarPhase::kPrimed;
                s.frames = 0;
                ChargeCompleteFlash();
            }
            break;
        }

        case SolarPhase::kPrimed:
        case SolarPhase::kWindup:
            s.blade = 1.0f;
            SceneDim(profile->prime_dim);
            if (profile->primed_white) {
                SetWielder(kSolarPrimedWhite, kSolarPrimedWhite, kSolarPrimedWhite);
            } else {
                SetWielder(0.0f, 0.0f, 0.0f);
            }
            if (profile->judgement) {
                int target = LockedTarget();
                if (target >= 0 && NowMonstorUnit != nullptr) {
                    JudgementHover(*NowMonstorUnit, target);
                } else {
                    JudgementRelease();
                }
            }
            // A shot's charge waits on the pellet (SolarShotFired).
            if (profile->shot || !Attacking()) {
                s.phase = SolarPhase::kPrimed;
                if (s.frames >= kSolarPrimedFrames) {
                    s.phase = SolarPhase::kFading;
                    s.frames = 0;
                    GlowShrink();
                }
                break;
            }
            s.phase = SolarPhase::kWindup;
            if (profile->judgement && JudgementHanging() && SwingDrops() && JudgementDrop(profile->drop_frames, profile->to_the_hilt)) {
                s.phase = SolarPhase::kDropping;
                s.frames = 0;
                break;
            }
            if (SwingForward()) {
                if (profile->bolts) {
                    BoltSwing(*profile);
                } else {
                    Flash(*profile);
                }
            } else if (profile->bolts && Combo(BtActStatus.action_no)) {
                // The room plunges to black along the swing, the bolt as it comes forward.
                SceneDimRamp(profile->prime_dim, WindUp(BtActStatus.action_no));
            }
            break;

        case SolarPhase::kDropping: {
            // The room darkens along the fall, from the prime dim to black at the landing.
            int fall = profile->drop_frames > 0 ? profile->drop_frames : kSolarDropFrames;
            SceneDimRamp(profile->prime_dim, std::min(1.0f, static_cast<float>(s.frames) / fall));
            sceVu0FVECTOR point;
            if (profile->bolts) {
                // In the ground to the hilt as the swing comes forward: gone, and the bolt comes down on the target.
                if (JudgementLanded(point) || !JudgementFalling()) {
                    BoltSwing(*profile);
                }
                break;
            }
            if (JudgementLanded(point)) {
                Land(*profile, point);
            } else if (!JudgementFalling()) {
                Flash(*profile); // the target went before it landed: the flash, as ever
            }
            break;
        }

        case SolarPhase::kChain: {
            // The combo carries the charge: each swing darkens the room as it winds up and brings a bolt down on the
            // target as it comes forward. A swing outside the combo, a new combo (an earlier swing, or the same one
            // back before its hit frame) or the lock let go spends it.
            int  action = BtActStatus.action_no;
            bool swinging = Combo(action);
            bool restarted = swinging && (action < s.chain_action ||
                                          (action == s.chain_action && s.chain_fired &&
                                           CharaMain.motion_type.state.time < kCombo[action - kToanFirstSwing].hit - 1.0f));
            int  target = LockedTarget();
            if (!swinging || restarted || target < 0) {
                if (!SceneFlashing()) {
                    SceneDimEnd();
                }
                state = {};
                break;
            }
            s.blade = 1.0f;
            if (action != s.chain_action) {
                s.chain_action = action;
                s.chain_fired = false;
            }
            if (s.chain_fired) {
                break;
            }
            if (!SwingForward()) {
                SceneDimRamp(profile->prime_dim, WindUp(action));
                break;
            }
            s.chain_fired = true;
            if (BoltOn(target, true, Thunder::kCombo)) {
                BoltFlash(*profile, CharaMain.pos);
            }
            break;
        }

        case SolarPhase::kFlying:
            // Held to the flash, as a swing's are; a pellet lives 120 frames, so one never seen to end is gone.
            if (s.frames > kSolarPrimedFrames) {
                Clear();
            }
            break;

        case SolarPhase::kLanding:
            SceneDimRamp(profile->prime_dim, static_cast<float>(s.frames) / kRampFrames);
            if (s.frames >= kRampFrames) {
                sceVu0FVECTOR point;
                int           struck = s.struck;
                sceVu0CopyVector(point, s.landed);
                state = {};
                SolarFlashAt(*profile, point, struck);
            }
            break;

        case SolarPhase::kFading: {
            float k = 1.0f - static_cast<float>(s.frames) / kSolarFadeFrames;
            if (k <= 0.0f) {
                Clear();
                break;
            }
            s.blade = k;
            SceneDim(profile->prime_dim * k);
            SetWielder(kSolarPrimedWhite * k, kSolarPrimedWhite * k, kSolarPrimedWhite * k);
            break;
        }
    }
}

void SolarShotFired(CSHOT &shots, int slot) {
    if (state.phase != SolarPhase::kPrimed || state.profile == nullptr || !state.profile->shot) {
        return;
    }
    shots.size[slot] = kSolarPelletSize;
    state.phase = SolarPhase::kFlying;
    state.frames = 0;
    state.shots = &shots;
    state.pellet = slot;
}

void SolarShotEnded(const CSHOT &shots, int slot, const float *point, bool struck) {
    if (state.phase != SolarPhase::kFlying || state.shots != &shots || state.pellet != slot) {
        return;
    }
    sceVu0CopyVector(state.landed, const_cast<float *>(point));
    state.landed[3] = 1.0f;
    state.struck = struck ? MonsterAt(point, kSolarStruckReach) : -1;
    state.phase = SolarPhase::kLanding;
    state.frames = 0;
    state.shots = nullptr;
    state.pellet = -1;
    state.glow = 0.0f;
}

void SolarFlashReset() {
    state = {};
}

bool SolarFlashOwnsLight(const CCharacter &character) {
    return state.phase != SolarPhase::kIdle && &character == &CharaMain;
}

void SolarFlashLight(const CCharacter &character, float *ambient) {
    if (state.phase == SolarPhase::kIdle) {
        return;
    }
    if (&character == &CharaMain) {
        for (int c = 0; c < 3; c++) {
            ambient[c] += state.wielder[c];
        }
    } else if (&character == NowWeapon) {
        for (int c = 0; c < 3; c++) {
            ambient[c] += kSolarBladeWhite * state.blade;
        }
    } else if (JudgementIsCopy(character)) {
        for (int c = 0; c < 3; c++) {
            ambient[c] += kSolarBladeWhite * JudgementShown();
        }
    }
}

void SolarGlowDraw() {
    const SolarProfile *profile = ActiveProfile();
    if (profile != nullptr && profile->judgement) {
        // Its glow is the judgement blade's alone, and goes with it.
        sceVu0FVECTOR at;
        if (JudgementGlowPoint(at)) {
            JudgementDraw();
            DrawGlowDisc(profile->glow_look, at, profile->glow_size * kJudgementScale * JudgementShown(), kGlowPull,
                         kJudgementGlowNearer);
        }
        return;
    }
    if (state.glow <= 0.0f || profile == nullptr) {
        return;
    }
    // On the charged pellet while it flies, else on the weapon.
    sceVu0FVECTOR at;
    if (state.shots != nullptr) {
        sceVu0CopyVector(at, const_cast<float *>(state.shots->pos[state.pellet]));
    } else if (!WeaponGlowPoint(*ActiveProfile(), at)) {
        return;
    }
    DrawGlowDisc(profile->glow_look, at, profile->glow_size * state.glow, kGlowPull);
}

} // namespace expanded
