#include "expanded/weapons/pellets.hpp"

#include "dun/gameloop.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kXiaoShot = 1; // a shot's user that is Xiao's, as her pellets' hits are

} // namespace

int FreePelletSlot(const CSHOT &shots, int slot) {
    for (int other = 0; other < kPelletSlots; other++) {
        if (other != slot && shots.used[other] == 0) {
            return other;
        }
    }
    return -1;
}

void CopyPellet(CSHOT &shots, int from, int to) {
    for (int i = 0; i < 4; i++) {
        shots.pos[to][i] = shots.pos[from][i];
        shots.unk_100[to][i] = shots.unk_100[from][i];
        shots.vector[to][i] = shots.vector[from][i];
    }
    shots.halted[to] = shots.halted[from];
    shots.life[to] = shots.life[from];
    shots.damage[to] = shots.damage[from];
    shots.size[to] = shots.size[from];
    shots.unk_340[to] = shots.unk_340[from];
    shots.unk_370[to] = shots.unk_370[from];
    shots.start_flag[to] = 0;
    shots.used[to] = 1;
}

int FireInPelletsPlace(CSHOT_EFFECT &effect, CSHOT &shots, int slot, int damage) {
    float from[4] = {shots.pos[slot][0], shots.pos[slot][1], shots.pos[slot][2], 1.0f};
    float to[4] = {from[0] + shots.vector[slot][0], from[1] + shots.vector[slot][1], from[2] + shots.vector[slot][2], 1.0f};
    effect.Set(from, to, kXiaoShot, 0, -1, nullptr, -1);
    int fired = effect.current_slot;
    if (fired < 0) {
        return -1;
    }
    effect.SetDmg(damage);
    if (NowWeaponHave != nullptr) {
        // Her weapon's abilities ride with it, as on a pellet.
        effect.SetWepStatus(NowWeaponHave->flags);
        effect.SetVsMonster(NowWeaponHave->vs_monster);
    }
    shots.used[slot] = 0;
    return fired;
}

} // namespace expanded
