#include "expanded/weapons/xiao/shot_charge.hpp"

#include "btactstatus.hpp"
#include "expanded/weapons/solar_flash.hpp"

namespace expanded {

namespace {

constexpr int kHoldAction = 12; // Xiao drawing the slingshot back and holding it drawn

} // namespace

void ShotCharge::Step(bool active) {
    if (!active) {
        Reset();
        return;
    }
    if (BtActStatus.action_on != 0 && BtActStatus.action_no == kHoldAction) {
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
    return charged;
}

} // namespace expanded
