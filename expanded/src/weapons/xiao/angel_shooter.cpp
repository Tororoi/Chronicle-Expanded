#include "expanded/weapons/xiao/angel_shooter.hpp"
#include "sound_effect.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "healeffect.hpp"
#include "itemdata.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int   kHealAbility = 0x800; // the weapon flags' Heal: the bit the game's tick heals for
constexpr int   kGuarding = 5;        // BtActStatus.guard_mode while the guard is up
constexpr float kFlashSpeed = 60.0f, kFlashCount = 1.0f;

struct Grace {
    bool open = false; // on as of the last step
    int  counter = -1; // the tick counter as last held, or -1
    int  period = 0;
    bool following = false; // the sparkle is ours and follows her
};

Grace grace;

void OnFloor() {
    GuardianGraceReset();
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

// The least the counter is held at after its step: kGuardianGraceFrames steps from the tick.
int Floor(int period) {
    return period - kGuardianGraceFrames + 1;
}

// Whether the Heal ability comes from Super Steve's sphere rather than the weapon's own record.
bool Inherited() {
    return SuperSteveSphere() == ITEM_WEAPON_ANGEL_SHOOTER;
}

} // namespace

bool GuardianGraceCarried() {
    return WeaponWielded(ITEM_WEAPON_ANGEL_SHOOTER) || Inherited();
}

bool GuardianGraceOn() {
    if (UserStatus == nullptr || UserStatus->cur_chara != CHARA_XIAO || NowWeaponHave == nullptr || !GuardianGraceCarried()) {
        return false;
    }
    bool ability = (NowWeaponHave->flags & kHealAbility) != 0 || Inherited();
    return ability && UserStatus->hp[CHARA_XIAO] > 0 && BtActStatus.guard_mode == kGuarding;
}

void GuardianGraceStep() {
    bool on = GuardianGraceOn();
    if (on && !grace.open) {
        // The guard's moment: the healing spring's sparkle, a soft white flash and the heal chime.
        float at[4] = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2], 1.0f};
        HealEffect.Set(at);
        grace.following = true;
        setUnitAmbientAnime(kFlashSpeed, kFlashCount, kGuardianGraceFlash, kGuardianGraceFlash, kGuardianGraceFlash);
        SndSePlay(SE_CHARA_RECOVER, -1, 0);
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
    counter = std::max(counter, Floor(period)); // the tick kGuardianGraceFrames from the last at the latest
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
    // One smooth swell a tick: at its height as the tick heals, lowest between.
    int   floor = Floor(grace.period);
    float phase = std::clamp(static_cast<float>(grace.counter - floor) / (grace.period - floor), 0.0f, 1.0f);
    float ease = 0.5f + 0.5f * std::cos(2.0f * std::numbers::pi_v<float> * phase);
    for (int i = 0; i < 3; i++) {
        ambient[i] += kGuardianGraceLow + (kGuardianGraceHigh - kGuardianGraceLow) * ease;
    }
}

void GuardianGraceReset() {
    grace = Grace{};
}

} // namespace expanded
