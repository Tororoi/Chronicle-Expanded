#include "expanded/weapons/xiao/angel_shooter.hpp"
#include "sound_effect.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/abilities.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "healeffect.hpp"
#include "itemdata.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace dc::game {
extern CCharacter NewChangeFx;
extern s32        NewChangeFxFlag;
} // namespace dc::game

namespace expanded {

namespace {

constexpr float kFlashSpeed = 60.0f, kFlashCount = 1.0f;

struct Grace {
    bool open = false; // on as of the last step
    bool gear = false; // in the Angel Gear's look
    int  counter = -1; // the tick counter as last held, or -1
    int  period = 0;
    int  ticks = 0;         // since it opened: the Angel Gear's swell spans two
    bool following = false; // the sparkle is ours and follows her
};

Grace grace;

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(GuardianGraceReset), true);

// The frames from tick to tick while it is on.
int Frames() {
    return grace.gear ? kGuardianGraceGearFrames : kGuardianGraceFrames;
}

// The least the counter is held at after its step: Frames() steps from the tick.
int Floor(int period) {
    return period - Frames() + 1;
}

// Whether the Heal ability comes from Super Steve's sphere rather than the weapon's own record.
bool Inherited() {
    return SuperSteveSphere() == ITEM_WEAPON_ANGEL_SHOOTER || SuperSteveSphere() == ITEM_WEAPON_ANGE_GEAR;
}

// Whether it comes in the Angel Gear's look.
bool Gear() {
    return WeaponActive(ITEM_WEAPON_ANGE_GEAR);
}

} // namespace

bool GuardianGraceCarried() {
    return WeaponWielded(ITEM_WEAPON_ANGEL_SHOOTER) || WeaponWielded(ITEM_WEAPON_ANGE_GEAR) || Inherited();
}

bool GuardianGraceOn() {
    if (UserStatus == nullptr || UserStatus->cur_chara != CHARA_XIAO || NowWeaponHave == nullptr || !GuardianGraceCarried()) {
        return false;
    }
    bool ability = (NowWeaponHave->flags & kAbilityHeal) != 0 || Inherited();
    return ability && UserStatus->hp[CHARA_XIAO] > 0 && Guarding();
}

void GuardianGraceStep() {
    bool on = GuardianGraceOn();
    if (on && !grace.open) {
        grace.gear = Gear();
        grace.ticks = 0;
        if (grace.gear) {
            // The guard's moment: the golden burst of a character change (armed as the game arms it), a warm flash and its
            // jingle.
            NewChangeFx.motion_no = 0;
            NewChangeFx.motion_flags = 6;
            NewChangeFx.motion_speed = -1.0f;
            NewChangeFx.motion_type.state.time = 1.0f;
            NewChangeFxFlag = 1;
            setUnitAmbientAnime(kFlashSpeed, kFlashCount, kGuardianGraceGearFlash[0], kGuardianGraceGearFlash[1], kGuardianGraceGearFlash[2]);
            SndSePlay(SE_SPARKLE, -1, 0);
        } else {
            // The guard's moment: the healing spring's sparkle, a soft white flash and the heal chime.
            float at[4] = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2], 1.0f};
            HealEffect.Set(at);
            grace.following = true;
            setUnitAmbientAnime(kFlashSpeed, kFlashCount, kGuardianGraceFlash[0], kGuardianGraceFlash[1], kGuardianGraceFlash[2]);
            SndSePlay(SE_CHARA_RECOVER, -1, 0);
        }
    }
    if (!on) {
        grace.counter = -1;
    }
    grace.open = on;
    if (grace.following && HealEffect.active != 0) {
        HealEffect.position[0] = CharaMain.pos[0];
        HealEffect.position[1] = CharaMain.pos[1];
        HealEffect.position[2] = CharaMain.pos[2];
    } else {
        grace.following = false;
    }
}

int GuardianGraceCounter(int counter, int period) {
    if (!grace.open) {
        return counter;
    }
    counter = std::max(counter, Floor(period)); // the tick Frames() from the last at the latest
    if (grace.counter >= 0 && counter < grace.counter) {
        grace.ticks++; // it healed: the counter began again
    }
    grace.counter = counter;
    grace.period = period;
    return counter;
}

bool GuardianGraceHeals() {
    return grace.open && Inherited();
}

void GuardianGraceLight(const CCharacter &character, float *ambient) {
    if (!grace.open || grace.counter < 0 || &character != &CharaMain) {
        return;
    }
    // One smooth swell a second: at its height as the tick heals (every other tick of the Angel Gear's), lowest between.
    int          frames = Frames(), per_swell = kGuardianGraceFrames / frames;
    int          into = grace.ticks % per_swell * frames + grace.counter - Floor(grace.period);
    float        phase = std::clamp(static_cast<float>(into) / (per_swell * frames - 1), 0.0f, 1.0f);
    float        ease = 0.5f + 0.5f * std::cos(2.0f * std::numbers::pi_v<float> * phase);
    const float *low = grace.gear ? kGuardianGraceGearLow : kGuardianGraceLow;
    const float *high = grace.gear ? kGuardianGraceGearHigh : kGuardianGraceHigh;
    for (int i = 0; i < 3; i++) {
        ambient[i] += low[i] + (high[i] - low[i]) * ease;
    }
}

void GuardianGraceReset() {
    grace = Grace{};
}

} // namespace expanded
