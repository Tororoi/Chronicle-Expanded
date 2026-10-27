#include "expanded/status/monster_ailments.hpp"

#include "expanded/status/poison.hpp"
#include "monstorunit.hpp"

namespace expanded {

void GiveMonsterAilments(MONSTOR &live, int monster, int given) {
    if (live.status_chance == 0) {
        return;
    }
    if ((given & kMonsterPoison) != 0 && live.stop_timer == 0 && live.anger_timer == 0) {
        Poison(&live, monster);
    }
    if ((given & kMonsterFreeze) != 0) {
        if (live.stop_timer <= 0) {
            live.stop_timer = 300;
            live.poison_timer = 0;
            live.slow_timer = 0;
            live.anger_timer = 0;
            live.movement_speed = 0.0f;
        } else {
            live.stop_timer = 0;
        }
    }
    if ((given & kMonsterGoo) != 0 && live.stop_timer == 0 && live.poison_timer == 0 && live.anger_timer == 0) {
        live.slow_timer = 180;
    }
}

} // namespace expanded
