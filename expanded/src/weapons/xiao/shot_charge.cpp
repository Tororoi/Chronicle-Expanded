#include "expanded/weapons/xiao/shot_charge.hpp"

#include "btactstatus.hpp"
#include "expanded/weapons/solar_flash.hpp"

namespace expanded {

namespace {

constexpr int kDrawAction = 11; // Xiao drawing the slingshot back
constexpr int kHoldAction = 12; // and holding it drawn

bool charged_shot = false; // the shot let go now was charged

} // namespace

void ShotCharge::Step(bool active) {
    if (!active) {
        Reset();
        return;
    }
    if (BtActStatus.action_on != 0 && (BtActStatus.action_no == kDrawAction || BtActStatus.action_no == kHoldAction)) {
        if (++hold_ == frames_) {
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
