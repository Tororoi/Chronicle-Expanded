#include "expanded/weapons/charge.hpp"

#include <algorithm>

#include "btactstatus.hpp"
#include "dun/gameloop.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/weapons/player_state.hpp"

namespace expanded {

namespace {

constexpr float kPulseSpeed = 15.0f; // the game's charge-complete pulse

bool charged_shot = false; // the shot let go now was charged

} // namespace

void ChargeCompleteFlash() {
    setUnitAmbientAnime(kPulseSpeed, 1.0f, kChargeColour[0], kChargeColour[1], kChargeColour[2]);
}

void GuardHold::Step(bool button) {
    if (!button) {
        Reset();
    } else if (held_ >= 0) {
        held_++;
    } else if (Guarding()) {
        held_ = 0;
    }
}

bool GuardHold::Reached(int frames) const {
    return !latched_ && held_ >= frames && Guarding();
}

bool GuardCharge::Start() {
    if (!Guarding() || Blinding()) {
        return false;
    }
    charged_ = unguarded_ = 0;
    return true;
}

GuardCharge::Step GuardCharge::Charge(int frames) {
    unguarded_ = Guarding() ? 0 : unguarded_ + 1;
    if (unguarded_ > kGuardGrace || Blinding()) {
        return Step::kDropped;
    }
    if (unguarded_ == 0) {
        charged_++;
    }
    return charged_ >= frames ? Step::kFull : Step::kHolding;
}

float GuardCharge::Progress(int frames) const {
    return std::min(1.0f, static_cast<float>(charged_) / static_cast<float>(frames));
}

void ShotCharge::Step(bool active) {
    if (!active) {
        Reset();
        return;
    }
    if (BtActStatus.action_on != 0 && (BtActStatus.action_no == kXiaoDraw || BtActStatus.action_no == kXiaoHold)) {
        ++hold_;
        if (hold_ == frames_ || (early_ > 0 && hold_ == early_)) {
            ChargeCompleteFlash();
        }
    } else if (hold_ > 0) {
        held_ = hold_;
        hold_ = 0;
    }
}

bool ShotCharge::TakeCharged() {
    bool charged = Ready();
    Reset();
    if (charged) {
        ChargedShotLetGo();
    }
    return charged;
}

void ChargedShotLetGo() {
    charged_shot = true;
}

float ChargedShotWear(float wear) {
    bool charged = charged_shot;
    charged_shot = false;
    return charged ? wear * kChargedShotWear : wear;
}

} // namespace expanded
