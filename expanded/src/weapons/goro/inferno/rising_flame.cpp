#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "mathconst.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "character.hpp"
#include "charaheight.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/pink_fire.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "frame.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// Each colour's fireball: entered once a floor, not tried again until the next once it fails.
struct Fireball {
    const char *pack;
    CCharacter *model = nullptr;
    bool        failed = false;
};

Fireball    fireballs[2] = {{kRisingFlamePack}, {kPinkFireballFile}}; // by FlameLook
FlameLook   look = FlameLook::kOrange;
CCharacter *model = nullptr; // the look's, once loaded
int         risen_frames = 0;
int         grown_frames = 0;  // of kRisingFlameGrow
int         fired_frames = -1; // since Xiao let a charged pellet go, or -1
int         fade_from = 0;     // its risen frames as it began to die down
float       shrink = 1.0f;     // of its size, as it dies down

// Its size and depth for who wears it: Goro's at his height, the rest at theirs.
float Body() {
    return UserStatus != nullptr ? CharaHeight(UserStatus) / kRisingFlameHeight : 1.0f;
}

CCharacter *Load() {
    Fireball &fireball = fireballs[static_cast<int>(look)];
    if (fireball.model == nullptr && !fireball.failed) {
        fireball.model = EnterFloorCharacter(fireball.pack, kRisingFlameCfg);
        if (fireball.model == nullptr || fireball.model->frame == nullptr) {
            std::fprintf(stderr, "inferno: no flame, %s could not be loaded\n", fireball.pack);
            fireball.model = nullptr;
            fireball.failed = true;
        } else {
            fireball.model->motion_no = kRisingFlameKey;
            fireball.model->motion_speed = kRisingFlameMotionSpeed;
        }
    }
    model = fireball.model;
    return model;
}

} // namespace

void RisingFlameLookAs(FlameLook as) {
    look = as;
    model = fireballs[static_cast<int>(look)].model;
}

void RisingFlameStep(bool up, bool smashing, bool burning_elsewhere) {
    if (fired_frames >= 0 && up) {
        fired_frames = -1; // drawn again: it rises on from where it is
        grown_frames = 0;
        shrink = 1.0f;
    }
    if (fired_frames >= 0) {
        // It flares to its full size, then shrinks and sinks.
        fired_frames++;
        if (fired_frames <= kRisingFlameFlare) {
            grown_frames = kRisingFlameGrow * fired_frames / kRisingFlameFlare;
        } else {
            float gone = std::min(1.0f, static_cast<float>(fired_frames - kRisingFlameFlare) / kRisingFlameFade);
            risen_frames = static_cast<int>(std::lround(static_cast<float>(fade_from) * (1.0f - gone)));
            shrink = 1.0f - gone;
            if (gone >= 1.0f) {
                risen_frames = grown_frames = 0;
                fired_frames = -1;
                shrink = 1.0f;
            }
        }
    } else {
        risen_frames = std::clamp(risen_frames + (up ? 1 : -1), 0, kRisingFlameRise);
        grown_frames = risen_frames == 0 ? 0 : std::min(kRisingFlameGrow, grown_frames + (smashing ? 1 : 0));
    }
    if ((risen_frames > 0 || burning_elsewhere) && model != nullptr) {
        model->motion_speed = RisingFlameMotionSpeed();
        model->Step();
    }
}

float RisingFlameRisen() {
    return static_cast<float>(risen_frames) / kRisingFlameRise;
}

float RisingFlameGrown() {
    return static_cast<float>(grown_frames) / kRisingFlameGrow;
}

void RisingFlameFired() {
    if (risen_frames > 0) {
        fired_frames = 0;
        fade_from = risen_frames;
    }
}

float RisingFlameScale() {
    return (kRisingFlameScale + (kRisingFlameSmashScale - kRisingFlameScale) * RisingFlameGrown()) * shrink * Body();
}

float RisingFlameMotionSpeed() {
    return kRisingFlameMotionSpeed + (kRisingFlameSmashMotionSpeed - kRisingFlameMotionSpeed) * RisingFlameGrown();
}

void RisingFlameDrawAt(const float *at, float pitch, float yaw, float scale) {
    if (BtEventMode != 0 || Load() == nullptr) {
        return;
    }
    model->SetPosition(at[0], at[1], at[2]);
    model->SetRotation(pitch, yaw, 0.0f);
    model->SetScale(scale, scale, scale);
    model->frame->SetPosition(at[0], at[1], at[2]);
    model->frame->SetRotation(pitch, yaw, 0.0f);
    model->frame->SetScale(scale, scale, scale);
    DrawFloorCharacter(*model);
}

void RisingFlameDraw() {
    if (risen_frames == 0) {
        return;
    }
    const float at[3] = {CharaMain.pos[0], CharaMain.pos[1] + kRisingFlameLift - kRisingFlameDepth * Body() * (1.0f - RisingFlameRisen()),
                         CharaMain.pos[2]};
    // Its flight is along its z: turned onto -y, head down.
    RisingFlameDrawAt(at, HALF_PI, 0.0f, RisingFlameScale());
}

void RisingFlameLight(const CCharacter &character, float *ambient) {
    if (&character != &CharaMain || risen_frames == 0) {
        return;
    }
    float risen = RisingFlameRisen(), grown = RisingFlameGrown();
    for (int c = 0; c < 3; c++) {
        ambient[c] += risen * (kRisingFlameGlow[c] + (kRisingFlameSmashGlow[c] - kRisingFlameGlow[c]) * grown);
    }
}

void RisingFlameReset() {
    for (Fireball &fireball : fireballs) {
        fireball.model = nullptr; // the floor's characters go with it
        fireball.failed = false;
    }
    model = nullptr;
    risen_frames = 0;
    grown_frames = 0;
    fired_frames = -1;
    shrink = 1.0f;
}

} // namespace expanded
