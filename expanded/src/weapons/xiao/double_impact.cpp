#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/xiao/double_impact.hpp"

#include <cmath>

#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/hardshooter.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {


} // namespace

bool DoubleImpactActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && WeaponActive(ITEM_WEAPON_DOUBLE_IMPACT);
}

int DoubleImpactFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots || shots.used[slot] == 0 || !DoubleImpactActive()) {
        return -1;
    }
    int twin = -1;
    for (int other = 0; other < kPelletSlots; other++) {
        if (other != slot && shots.used[other] == 0) {
            twin = other;
            break;
        }
    }
    shots.damage[slot] = static_cast<int>(std::lround(static_cast<float>(shots.damage[slot]) * kDoubleImpactDamage));
    if (twin < 0) {
        return -1;
    }

    // Side by side: half the width each way, level and across the flight.
    const float *v = shots.vector[slot];
    float        across[2] = {v[2], -v[0]};
    float        level = std::sqrt(across[0] * across[0] + across[1] * across[1]);
    if (level > 0.0f) {
        across[0] *= 0.5f * kDoubleImpactWidth / level;
        across[1] *= 0.5f * kDoubleImpactWidth / level;
    }
    for (int i = 0; i < 4; i++) {
        shots.pos[twin][i] = shots.pos[slot][i];
        shots.unk_100[twin][i] = shots.unk_100[slot][i];
        shots.vector[twin][i] = v[i];
    }
    shots.pos[slot][0] += across[0];
    shots.pos[slot][2] += across[1];
    shots.pos[twin][0] -= across[0];
    shots.pos[twin][2] -= across[1];
    shots.halted[twin] = shots.halted[slot];
    shots.life[twin] = shots.life[slot];
    shots.damage[twin] = shots.damage[slot];
    shots.size[twin] = shots.size[slot];
    shots.unk_340[twin] = shots.unk_340[slot];
    shots.unk_370[twin] = shots.unk_370[slot];
    shots.start_flag[twin] = 0;
    shots.used[twin] = 1;
    RicochetPair(slot, twin);
    return twin;
}

} // namespace expanded
