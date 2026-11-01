#include "expanded/weapons/toan/sand_breaker.hpp"

#include <format>

#include "btactstatus.hpp"
#include "character.hpp"
#include "charakey.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/status/confuse_state.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/swings.hpp"
#include "expanded/weapons/xiao/angel_gear/shot_reflect.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

// The guard (movechara's BtActStatus.action_on) and the hit kind that breaks it, which a counter blocks as a knockback.
constexpr int kGuarding = 6;
constexpr int kGuardBreaking = 3, kKnockback = 2;

// A returned shot leaves from Toan's chest.
constexpr float kMuzzleLift = 10.0f;

int  ready = 0;
bool caught = false; // the hit reaching him now is the one the counter caught
bool countering = false;

// X out of the guard: Toan's swing, or Xiao's draw.
void Attack() {
    if (Playing(CHARA_XIAO)) {
        BattleActionOn_Jinn();
    } else {
        ToanKey_On();
    }
}

// The actions a counter plays: Toan's opening swing, or Xiao's draw, hold and shot.
bool CounterAction(int action) {
    return action == kToanFirstSwing || action == kXiaoDraw || action == kXiaoHold || action == kXiaoShot;
}

} // namespace

bool SandCounterActive() {
    return (Playing(CHARA_TOAN) || Playing(CHARA_XIAO)) && WeaponActive(ITEM_WEAPON_SAND_BREAKER);
}

int SandCounterReady() {
    return ready;
}

bool SandCountering() {
    return countering;
}

void SandCounterPress() {
    if (BtActStatus.action_on == kGuarding && SandCounterActive()) {
        ready = kCounterWindow;
    }
}

void SandCounterStep() {
    // No blow caught in time: the press is the swing it would have been out of the guard, wind-up and all.
    if (ready > 0 && --ready == 0 && BtActStatus.action_on == kGuarding && SandCounterActive()) {
        BtActStatus.action_on = 0;
        Attack();
    }
    if (countering && (BtActStatus.action_on != 1 || !CounterAction(BtActStatus.action_no))) {
        countering = false;
    }
}

void SandCounterHit(COLLISION_HIT &hit) {
    // The game has taken him out of his guard by now; a counter is readied only in it, and the reaction says whether the
    // guard stopped the blow.
    caught = ready > 0 && SandCounterActive();
    if (caught && hit.kind == kGuardBreaking) {
        hit.kind = kKnockback;
    }
}

void SandCounterBlocked(bool blocked, const COLLISION_HIT &hit) {
    if (!caught) {
        return;
    }
    caught = false;
    ready = 0;
    if (!blocked) {
        return;
    }

    int   attacker = confuse::Attacker(hit);
    float her[4] = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2], 1.0f};
    float muzzle[4] = {her[0], her[1] + kMuzzleLift, her[2], 1.0f};
    bool  returned = angel_gear::ReturnShotHit(hit, her, muzzle, attacker);
    LogEvent("counter", std::format("\"monster\":{},\"returned\":{}", attacker, returned));
    if (!returned && attacker >= 0 && NowMonstorUnit != nullptr) {
        KickMonster(*NowMonstorUnit, attacker, her, kCounterKick);
    }

    if (Playing(CHARA_XIAO)) {
        // Super Steve's shot, fired at once as Quick Draw fires it (small_sword.hpp).
        BtActStatus.action_on = 0;
        BattleActionOn_Jinn();
        countering = BtActStatus.action_on == 1;
        return;
    }

    // The opening swing, skipping its wind-up as Quick Draw plays it.
    sceVu0FVECTOR rotation;
    CharaMain.GetRotation(rotation);
    BtActStatus.action_on = 1;
    BtActStatus.action_no = kToanFirstSwing;
    BtActStatus.motion_no = kToanFirstSwing;
    BtActStatus.action_step = 1;
    BtActStatus.charge_time = 1.0f;
    BtActStatus.charge_level = 0;
    BtActStatus.combo_window = 0;
    BtActStatus.combo_queued = 0;
    BtActStatus.frames_since_attack = 0;
    BtActStatus.refill_frames = 0;
    BtActStatus.swing_heading = rotation[1];
    driveNoInterpolate = 1;
    countering = true; // Quick Draw's jump past the wind-up (small_sword.hpp)
}

int SandCounterAttack(int attack) {
    return countering ? static_cast<int>(static_cast<float>(attack) * kCounterDamage) : attack;
}

} // namespace expanded
