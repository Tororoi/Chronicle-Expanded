#include <libvu0.h>

#include <algorithm>

#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "expanded/status/confuse.hpp"
#include "expanded/status/confuse_state.hpp"
#include "mathutil.hpp"
#include "monstorunit.hpp"
#include "nowload.hpp"
#include "shot_effect.hpp"

namespace expanded {

using namespace confuse;

namespace {

// The stars (e114ex): looped, grown in over a quarter second to 1.5x, 3 above the head.
constexpr int   kStars = 8;
constexpr int   kStarsBlock = 0x26;
constexpr float kStarsLift = 3.0f;
constexpr float kStarsScale = 1.5f;
constexpr int   kStarsGrowSteps = kStepsPerSecond / 4;
constexpr float kStarsLead = 1.5f;

// The stars as an effect that only plays.
BT_SHOT_EFFECT StarsDescription = {
    "e114ex", 1, 0, {0.0f, 0.0f, 0.0f, 0.0f},
       {0.0f, 0.0f, 0.0f, 0.0f},
       0, 0, 0, 0, 0, {0, -1, -1, -1},
       -1, {0, 0},
       0.0f, 0, {-1, -1, -1, -1}
};

// The stars' effect, loaded into the floor's monster memory.
CSHOT_EFFECT &Stars() {
    static CSHOT_EFFECT stars;
    return stars;
}

bool  loaded = false;
float fade = 1.0f; // the stars' share of their size

// Each monster's star (a slot of the stars effect, or -1) and the step it came on.
struct On {
    int star[kFloorMonsters];
    int from[kFloorMonsters] = {};

    On() {
        for (int &slot : star) {
            slot = -1;
        }
    }
};

On on;

} // namespace

namespace confuse {

void StarsLoad() {
    LoadFile(const_cast<char *>("gedit/s04/chara/e114ex.chr"), read_buffer, NULL);
    wait_now_loading_vsync();
    loaded = Stars().Entry2(&StarsDescription, read_buffer, kStarsBlock, &MonstorModelBuffer, kStars) != 0;
}

void StarsDrop() {
    Stars().Initialize();
    loaded = false;
}

void StarOff(int slot) {
    if (on.star[slot] >= 0 && loaded) {
        Stars().active[on.star[slot]] = 0;
    }
    on.star[slot] = -1;
}

void StarsOff() {
    for (int slot = 0; slot < kFloorMonsters; slot++) {
        StarOff(slot);
    }
}

} // namespace confuse

void ConfuseStarsDraw() {
    Confusion    &state = State();
    CMonstorUnit *unit = state.unit;
    if (!loaded || unit == nullptr || unit != NowMonstorUnit) {
        return;
    }
    CSHOT_EFFECT &stars = Stars();
    // The confused monsters nearest the player, one star each.
    int   order[kFloorMonsters];
    float distance[kFloorMonsters];
    int   count = 0;
    for (int slot = 0; slot < kFloorMonsters; slot++) {
        if (!Confused(*unit, slot)) {
            continue;
        }
        sceVu0FVECTOR at;
        Position(*unit, slot, at);
        float d = DistVector(at, CharaMain.pos);
        int   i = count++;
        while (i > 0 && distance[i - 1] > d) {
            order[i] = order[i - 1];
            distance[i] = distance[i - 1];
            i--;
        }
        order[i] = slot;
        distance[i] = d;
    }
    bool wanted[kFloorMonsters] = {};
    for (int i = 0; i < count && i < kStars; i++) {
        wanted[order[i]] = true;
    }
    for (int slot = 0; slot < kFloorMonsters; slot++) {
        if (!wanted[slot]) {
            StarOff(slot);
        }
    }
    for (int slot = 0; slot < kFloorMonsters; slot++) {
        if (!wanted[slot]) {
            continue;
        }
        MONSTOR &monster = unit->monster[slot];
        // Over the body's height, or the lock-on point where higher.
        sceVu0FVECTOR head;
        Position(*unit, slot, head);
        head[1] += unit->chara[slot][0].body_height;
        if (monster.lockon_frame != NULL && monster.lockon_position[1] > head[1]) {
            head[1] = monster.lockon_position[1];
        }
        head[1] += kStarsLift;
        head[3] = 1.0f;
        int star = on.star[slot];
        if (star < 0 || stars.active[star] == 0) {
            star = stars.Set(head, head, -1, -1, 0, NULL, -1);
            if (star < 0) {
                on.star[slot] = -1;
                continue;
            }
            stars.SetLoop(0);
            on.star[slot] = star;
            on.from[slot] = state.tick;
        }
        CCharacter  &model = stars.chara[star];
        MOTION_INFO &key = model.motion_type.motion_info[0];
        if (model.motion_type.state.time >= (float) key.end - kStarsLead) {
            model.motion_type.state.time = (float) key.start;
        }
        float grown = static_cast<float>(state.tick - on.from[slot]) / kStarsGrowSteps;
        float scale = kStarsScale * std::clamp(std::min(grown, 1.0f) * fade, 0.01f, 1.0f);
        model.SetPosition(head);
        model.SetScale(scale, scale, scale);
    }
    stars.Draw();
}

void ConfuseStarsFade(float share) {
    fade = share;
}

} // namespace expanded
