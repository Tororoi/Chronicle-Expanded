#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "mathconst.h"

#include <algorithm>
#include <cstdio>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "frame.hpp"

namespace expanded {

namespace {

CCharacter *model = nullptr;
bool        failed = false; // not tried again until the next floor
int         risen_frames = 0;
int         grown_frames = 0; // of kRisingFlameGrow

CCharacter *Load() {
    if (model != nullptr || failed) {
        return model;
    }
    model = EnterFloorCharacter(kRisingFlamePack, kRisingFlameCfg);
    if (model == nullptr || model->frame == nullptr) {
        std::fprintf(stderr, "inferno: no flame, %s could not be loaded\n", kRisingFlamePack);
        model = nullptr;
        failed = true;
        return nullptr;
    }
    model->motion_no = kRisingFlameKey;
    model->motion_speed = kRisingFlameMotionSpeed;
    return model;
}

} // namespace

void RisingFlameStep(bool up, bool smashing) {
    risen_frames = std::clamp(risen_frames + (up ? 1 : -1), 0, kRisingFlameRise);
    grown_frames = risen_frames == 0 ? 0 : std::min(kRisingFlameGrow, grown_frames + (smashing ? 1 : 0));
    if (risen_frames > 0 && model != nullptr) {
        model->Step();
    }
}

float RisingFlameRisen() {
    return static_cast<float>(risen_frames) / kRisingFlameRise;
}

float RisingFlameScale() {
    return kRisingFlameScale + (kRisingFlameSmashScale - kRisingFlameScale) * static_cast<float>(grown_frames) / kRisingFlameGrow;
}

void RisingFlameDraw() {
    if (risen_frames == 0 || BtEventMode != 0 || Load() == nullptr) {
        return;
    }
    const float at[3] = {CharaMain.pos[0], CharaMain.pos[1] + kRisingFlameLift - kRisingFlameDepth * (1.0f - RisingFlameRisen()),
                         CharaMain.pos[2]};
    // Its flight is along its z: turned onto -y, head down.
    model->SetPosition(at[0], at[1], at[2]);
    model->SetRotation(HALF_PI, 0.0f, 0.0f);
    const float scale = RisingFlameScale();
    model->SetScale(scale, scale, scale);
    model->frame->SetPosition(at[0], at[1], at[2]);
    model->frame->SetRotation(HALF_PI, 0.0f, 0.0f);
    model->frame->SetScale(scale, scale, scale);
    DrawFloorCharacter(*model);
}

void RisingFlameReset() {
    model = nullptr; // the floor's characters go with it
    failed = false;
    risen_frames = 0;
    grown_frames = 0;
}

} // namespace expanded
