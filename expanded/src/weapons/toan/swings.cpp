#include "expanded/weapons/toan/swings.hpp"

#include <algorithm>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/player_state.hpp"

namespace expanded {

bool ToanAttacking() {
    int action = BtActStatus.action_no;
    return BtActStatus.action_on != 0 && ((action >= kToanFirstSwing && action <= kToanLastSwing) || action == kToanLunge || action == kToanWhirlwind);
}

bool ToanComboSwing(int action) {
    return BtActStatus.action_on != 0 && action >= kToanFirstSwing && action <= kToanLastSwing;
}

bool ToanSwingDrops() {
    int action = BtActStatus.action_no;
    if (action == kToanLunge || action == kToanWhirlwind) {
        return true;
    }
    float time = CharaMain.motion_type.state.time;
    return action == kToanFirstSwing && time >= kToanFirstSwingTrail && time <= kToanCombo[0].end;
}

bool ToanSwingForward() {
    int action = BtActStatus.action_no;
    if (action == kToanLunge || action == kToanWhirlwind) {
        return true;
    }
    const ToanSwing &swing = kToanCombo[action - kToanFirstSwing];
    float            time = CharaMain.motion_type.state.time;
    return time >= swing.hit && time <= swing.end;
}

float ToanSwingWindUp(int action) {
    const ToanSwing &swing = kToanCombo[action - kToanFirstSwing];
    return std::clamp((CharaMain.motion_type.state.time - swing.start) / (swing.hit - swing.start), 0.0f, 1.0f);
}

} // namespace expanded
