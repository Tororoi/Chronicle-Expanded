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
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/judgement_blade.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/big_bang.hpp"
#include "expanded/weapons/toan/swings.hpp"
#include "expanded/weapons/toan/sword_of_zeus.hpp"
#include "expanded/weapons/weapon_hit.hpp"
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

namespace {

// About how long the blade's fall takes, for the room's darkening along it.
constexpr int kSolarDropFrames = 20;

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
    int                 frames = 0;         // in the phase
    GuardCharge         guard;              // the guard's charge
    float               blade = 0.0f;       // the blade's white, 0..1
    float               wielder[3] = {};    // the wielder's own tint
    float               glow = 0.0f;        // the glow's size, 0..1
    float               glow_shrink = 0.0f; // what it loses each frame while it shrinks away
    const CSHOT        *shots = nullptr;    // the charged pellet's pool and slot while it flies
    int                 pellet = -1;
    sceVu0FVECTOR       landed = {};         // where it ended
    int                 struck = -1;         // the monster it struck there, or -1
    int                 chain_action = -1;   // the Sword of Zeus's combo: the swing that last came forward …
    bool                chain_fired = false; // … and whether its bolt came down
};

State state;

const SolarProfile *ActiveProfile() {
    if (Playing(CHARA_TOAN) && WeaponWielded(ITEM_WEAPON_SUN_SWORD)) {
        return &kSunSwordFlash;
    }
    if (Playing(CHARA_TOAN) && WeaponWielded(ITEM_WEAPON_BIG_BANG)) {
        return &kBigBangFlash;
    }
    if (Playing(CHARA_TOAN) && WeaponWielded(ITEM_WEAPON_SWORD_OF_ZEUS)) {
        return &kZeusFlash;
    }
    if (Playing(CHARA_XIAO) && SuperSteveSphere() == ITEM_WEAPON_SUN_SWORD) {
        return &kSolarShotFlash;
    }
    return nullptr;
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

// The flash's white from point and the light hits of the profile's share (PlantSolarHits): false with no monsters' unit,
// which leaves the bill and the blinding out.
bool FlashWhite(const SolarProfile &profile, const float *point, int except, float spared) {
    SceneFlash(profile.look);
    setUnitAmbientAnime(kFlashPulseSpeed, 1.0f, profile.look.light[0], profile.look.light[1], profile.look.light[2]);
    if (NowMonstorUnit == nullptr) {
        return false;
    }
    if (profile.damage_share > 0.0f && NowWeaponHave != nullptr) {
        int base = std::max(1, static_cast<int>(std::lround(NowWeaponHave->attack * profile.damage_share)));
        PlantSolarHits(*NowMonstorUnit, point, base, except, spared);
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
        if (monster == except || !MonsterInPlay(live)) {
            continue;
        }
        sceVu0FVECTOR position;
        unit.chara[monster][0].GetPosition(position);
        if (std::hypot(position[0] - point[0], position[2] - point[2]) > kSolarFlashRadius ||
            (spared > 0.0f && BlastDistance(unit, monster, point) <= spared)) {
            continue;
        }
        if (!HitRecordFree(*NowColData)) {
            break;
        }
        PlantWeaponHit(position, base_damage, kSolarHitLife, kSolarHitRadius);
        // A monster the game cannot move takes the hit where it stands.
        bool rooted = live.knockback[2] <= 0.0f;
        NowColData->SetKickBack(const_cast<float *>(point), rooted ? 0.0f : kSolarKickSpeed, kSolarKickDecay, 2);
        struck++;
    }
    return struck;
}

void SolarFlashAt(const SolarProfile &profile, const float *point, int except, float whp) {
    if (FlashWhite(profile, point, except, 0.0f)) {
        WearWeapon(whp < 0.0f ? profile.flash_whp : whp);
        Blind(*NowMonstorUnit);
    }
}

void SolarBlastFlash(const SolarProfile &profile, const float *point, float whp) {
    if (FlashWhite(profile, point, -1, kBlastFalloff[0].reach)) {
        WearWeapon(whp);
        BlindAfter(kBlastThrowFrames);
    }
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
            if (s.guard.Start()) {
                s.phase = SolarPhase::kCharging;
                s.profile = profile;
                s.frames = 0;
            }
            break;

        case SolarPhase::kCharging: {
            const int         full = profile->charge_frames;
            GuardCharge::Step step = s.guard.Charge(full);
            if (step == GuardCharge::Step::kDropped) {
                Clear();
                break;
            }
            float k = s.guard.Progress(full);
            s.blade = k;
            SceneDim(profile->prime_dim * k);
            // The cyan build-up over the charge's last frames.
            float ramp = std::clamp(1.0f - static_cast<float>(full - s.guard.Charged()) / kSolarRampFrames, 0.0f, 1.0f);
            SetWielder(kChargeColour[0] * kChargeRampPeak * ramp, kChargeColour[1] * kChargeRampPeak * ramp,
                       kChargeColour[2] * kChargeRampPeak * ramp);
            // The glow grows with the charge, full the moment it primes.
            s.glow = profile->glow ? k : 0.0f;
            s.glow_shrink = 0.0f;
            if (step == GuardCharge::Step::kFull) {
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
            if (profile->shot || !ToanAttacking()) {
                s.phase = SolarPhase::kPrimed;
                if (s.frames >= kSolarPrimedFrames) {
                    s.phase = SolarPhase::kFading;
                    s.frames = 0;
                    GlowShrink();
                }
                break;
            }
            s.phase = SolarPhase::kWindup;
            if (profile->judgement && JudgementHanging() && ToanSwingDrops() && JudgementDrop(profile->drop_frames, profile->to_the_hilt)) {
                s.phase = SolarPhase::kDropping;
                s.frames = 0;
                break;
            }
            if (ToanSwingForward()) {
                if (profile->bolts) {
                    BoltSwing(*profile);
                } else {
                    Flash(*profile);
                }
            } else if (profile->bolts && ToanComboSwing(BtActStatus.action_no)) {
                // The room plunges to black along the swing, the bolt as it comes forward.
                SceneDimRamp(profile->prime_dim, ToanSwingWindUp(BtActStatus.action_no));
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
            bool swinging = ToanComboSwing(action);
            bool restarted = swinging && (action < s.chain_action ||
                                          (action == s.chain_action && s.chain_fired &&
                                           CharaMain.motion_type.state.time < kToanCombo[action - kToanFirstSwing].hit - 1.0f));
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
            if (!ToanSwingForward()) {
                SceneDimRamp(profile->prime_dim, ToanSwingWindUp(action));
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
    state.struck = struck && NowMonstorUnit != nullptr ? NearestMonster(*NowMonstorUnit, point, kSolarStruckReach) : -1;
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
