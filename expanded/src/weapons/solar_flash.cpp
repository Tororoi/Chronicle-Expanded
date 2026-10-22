#include "expanded/weapons/solar_flash.hpp"

#include <algorithm>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/weapons/super_steve.hpp"
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
    .glow_along = 0.2f, // low on the blade, near the hilt
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
    .glow_along = 0.5f,
    .look = {.ease_frames = kBlindFrames},
};

namespace {

// Toan's swings: the combo's five hits, the lunge and the whirlwind.
constexpr int kComboFirst = 0x24;
constexpr int kComboLast = 0x28;
constexpr int kLunge = 0xF;
constexpr int kWhirlwind = 0x18;

// Each combo swing's clip and the frame it comes forward on.
struct Swing {
    float start, end, hit;
};

constexpr Swing kCombo[] = {
    {820.0f, 830.0f, 826.0f},
    {830.0f, 838.0f, 835.0f},
    {838.0f, 847.0f, 844.0f},
    {847.0f, 857.0f, 853.0f},
    {856.0f, 884.0f, 867.0f},
};

// The game's charge-complete colour, which the build-up ramps toward (half of it at the last frame).
constexpr float kChargeColour[3] = {0.0f, 122.0f, 208.0f};
constexpr float kChargeRampPeak = 0.5f;
// The game's own pulses: the charge-complete one at priming, the flash's own in its light.
constexpr float kPulseSpeed = 15.0f;
constexpr float kFlashPulseSpeed = 30.0f;

// The glow disc at full charge, and how far toward the camera its depth is tested from.
constexpr float kGlowDiameter = 14.0f;
constexpr float kGlowPull = 1.0f;

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
};

State state;

const SolarProfile *ActiveProfile() {
    if (UserStatus == nullptr) {
        return nullptr;
    }
    if (UserStatus->cur_chara == CHARA_TOAN && WeaponWielded(ITEM_WEAPON_SUN_SWORD)) {
        return &kSunSwordFlash;
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
    for (int monster = 0; monster < 16; monster++) {
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

bool Guarding() {
    return BtActStatus.guard_mode == 5;
}

bool Attacking() {
    int action = BtActStatus.action_no;
    return BtActStatus.action_on != 0 && ((action >= kComboFirst && action <= kComboLast) || action == kLunge || action == kWhirlwind);
}

// The swing has come forward: a lunge or whirlwind at once, a combo hit at its own frame.
bool SwingForward() {
    int action = BtActStatus.action_no;
    if (action == kLunge || action == kWhirlwind) {
        return true;
    }
    const Swing &swing = kCombo[action - kComboFirst];
    float        time = CharaMain.motion_type.state.time;
    return time >= swing.hit && time <= swing.end;
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
    for (int i = 0; i < 96; i++) {
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
    float glow = state.glow;
    state = {};
    state.glow = glow;
    GlowShrink();
}

void Flash(const SolarProfile &profile) {
    sceVu0FVECTOR point;
    sceVu0CopyVector(point, CharaMain.pos);
    state = {};
    SolarFlashAt(profile, point);
}

// Where the glow sits on the weapon: along between its first two hit points when it has them, else the weapon's own
// origin in the wielder's hand.
bool WeaponGlowPoint(float along, float *point) {
    if (NowWeapon == nullptr || NowWeapon->frame == nullptr) {
        return false;
    }
    sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 0.0f};
    CFrame       *hilt = NowWeapon->frame->SearchFrame(const_cast<char *>("dcol0"));
    CFrame       *tip = NowWeapon->frame->SearchFrame(const_cast<char *>("dcol1"));
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

int PlantSolarHits(CMonstorUnit &unit, const float *point, int base_damage, int except) {
    if (NowColData == nullptr) {
        return 0;
    }
    int struck = 0;
    for (int monster = 0; monster < 16; monster++) {
        const MONSTOR &live = unit.monster[monster];
        if (monster == except || live.state == -1 || live.hp <= 0 || live.revealed != -1) {
            continue;
        }
        sceVu0FVECTOR position;
        unit.chara[monster][0].GetPosition(position);
        if (std::hypot(position[0] - point[0], position[2] - point[2]) > kSolarFlashRadius) {
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

void SolarFlashAt(const SolarProfile &profile, const float *point, int except) {
    SceneFlash(profile.look);
    setUnitAmbientAnime(kFlashPulseSpeed, 1.0f, profile.look.light[0], profile.look.light[1], profile.look.light[2]);
    if (NowMonstorUnit == nullptr) {
        return;
    }
    if (profile.damage_share > 0.0f && NowWeaponHave != nullptr) {
        int base = std::max(1, static_cast<int>(std::lround(NowWeaponHave->attack * profile.damage_share)));
        PlantSolarHits(*NowMonstorUnit, point, base, except);
    }
    WearWeapon(profile.flash_whp);
    Blind(*NowMonstorUnit);
}

void SolarFlashStep() {
    BlindTick();
    SceneLightStep();
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
            if (s.unguarded > kSolarGuardGrace || Blinding()) {
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
                setUnitAmbientAnime(kPulseSpeed, 1.0f, kChargeColour[0], kChargeColour[1], kChargeColour[2]);
            }
            break;
        }

        case SolarPhase::kPrimed:
        case SolarPhase::kWindup:
            s.blade = 1.0f;
            SceneDim(profile->prime_dim);
            SetWielder(kSolarPrimedWhite, kSolarPrimedWhite, kSolarPrimedWhite);
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
            if (SwingForward()) {
                Flash(*profile);
            }
            break;

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
    BlindEnd();
    SceneLightReset();
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
    }
}

void SolarGlowDraw() {
    if (state.glow <= 0.0f || ActiveProfile() == nullptr) {
        return;
    }
    // On the charged pellet while it flies, else on the weapon.
    sceVu0FVECTOR at;
    if (state.shots != nullptr) {
        sceVu0CopyVector(at, const_cast<float *>(state.shots->pos[state.pellet]));
    } else if (!WeaponGlowPoint(ActiveProfile()->glow_along, at)) {
        return;
    }
    DrawGlowDisc(GlowLook::kGold, at, kGlowDiameter * state.glow, kGlowPull);
}

} // namespace expanded
