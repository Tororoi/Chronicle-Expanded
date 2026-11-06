#include "expanded/weapons/goro/satans_ax/underworld_tint.hpp"

#include <algorithm>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/player_state.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

int frames = 0; // of kUnderworldTintFade

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(UnderworldTintReset), true);

} // namespace

void UnderworldTintStep() {
    // Goro's pull: not Xiao's pellet's.
    bool on = Playing(CHARA_GORO) && UnderworldStrength() > 0.0f;
    frames = std::clamp(frames + (on ? 1 : -1), 0, kUnderworldTintFade);
}

float UnderworldTint() {
    return static_cast<float>(frames) / kUnderworldTintFade;
}

void UnderworldLight(const CCharacter &character, float *ambient) {
    if (frames == 0 || &character != NowWeapon) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] += kUnderworldTint[c] * UnderworldTint();
    }
}

void UnderworldTintReset() {
    frames = 0;
}

} // namespace expanded
