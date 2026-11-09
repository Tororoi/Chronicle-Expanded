#include "expanded/weapons/goro/inferno/rising_flame.hpp"

#include <algorithm>

#include "camerafollow.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "fireomni.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "render/draw2d.hpp"
#include "snd.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

CFireOmni flame;
int       risen_frames = 0;

// A torch's layer: no alpha test, depth tested but not written, Cs * As + Cd.
void SetLayerRegisters() {
    const draw2d::Services &services = draw2d::Get();
    sceGsTest               test = mgPixelTest;
    test.bits.ate = 0;
    test.bits.zte = 1;
    services.set_test(&test);
    sceGsZbuf zbuffer = mgZBuffer;
    zbuffer.bits.zmsk = 1;
    services.set_zbuf(&zbuffer);
    sceGsAlpha alpha = mgAlpha;
    alpha.bits.a = 0;
    alpha.bits.b = 2;
    alpha.bits.c = 0;
    alpha.bits.d = 1;
    services.set_alpha(&alpha);
}

} // namespace

void RisingFlameStep(bool up) {
    risen_frames = std::clamp(risen_frames + (up ? 1 : -1), 0, kRisingFlameRise);
    if (risen_frames > 0) {
        flame.FireStep();
    }
}

float RisingFlameRisen() {
    return static_cast<float>(risen_frames) / kRisingFlameRise;
}

void RisingFlameDraw() {
    CTexture *core = TexManager.GetTexture(const_cast<char *>("lightling"), -1);
    CTexture *glow = TexManager.GetTexture(const_cast<char *>("blender"), -1);
    if (risen_frames == 0 || BtEventMode != 0 || core == nullptr || glow == nullptr ||
        TexManager.GetTexture(const_cast<char *>("d01e02"), -1) == nullptr) {
        return; // a floor without torches has no flame to show
    }
    flame.FireCreate(); // the flame's frame, into the blender the torches have finished with

    sceVu0FVECTOR world = {CharaMain.pos[0],
                           CharaMain.pos[1] + kRisingFlameLift - kRisingFlameSize * (1.0f - RisingFlameRisen()),
                           CharaMain.pos[2], 1.0f};
    sceVu0FVECTOR feet = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2], 1.0f};
    sceVu0FVECTOR toward, ref;
    NowCamera__3->GetPos(toward);
    NowCamera__3->GetRef(ref);
    sceVu0SubVector(toward, toward, ref);
    sceVu0Normalize(toward, toward);
    sceVu0FVECTOR near = {world[0] + kRisingFlameNear * toward[0], world[1] + kRisingFlameNear * toward[1],
                          world[2] + kRisingFlameNear * toward[2], 1.0f};
    int           top_left[4], bottom_right[4], feet_at[4], feet_end[4], near_at[4], near_end[4];
    // A sprite's height is not squeezed as the field's rows are: half the units cover the same height.
    if (MGRotTransPers3DSprite(top_left, bottom_right, world, kRisingFlameSize, kRisingFlameSize / 2.0f, 1) != 1 ||
        MGRotTransPers3DSprite(feet_at, feet_end, feet, 1.0f, 1.0f, 0) != 1 ||
        MGRotTransPers3DSprite(near_at, near_end, near, 1.0f, 1.0f, 0) != 1) {
        return;
    }
    // Cut off at his feet: what is still under the floor shows nothing.
    const int   floor_y = (feet_at[1] + feet_end[1]) / 2;
    const float shown = std::min(1.0f, static_cast<float>(floor_y - top_left[1]) / static_cast<float>(bottom_right[1] - top_left[1]));
    if (shown <= 0.0f) {
        return;
    }
    bottom_right[1] = std::min(bottom_right[1], floor_y);
    // Over him, not behind his back: the depth of a point toward the camera.
    top_left[2] = bottom_right[2] = near_at[2];
    SetLayerRegisters();
    spRGBA half = {.r = 0x80, .g = 0x80, .b = 0x80, .a = kRisingFlameAlpha};
    set3DSpriteFog(Vif1Packet, core, CRect_i_(0, 0, 64, static_cast<int>(64.0f * shown)), top_left, bottom_right, &half);
    set3DSpriteFog(Vif1Packet, glow, CRect_i_(2, 2, 124, static_cast<int>(124.0f * shown)), top_left, bottom_right, &half);
    const draw2d::Services &services = draw2d::Get();
    services.set_test(nullptr);
    services.set_zbuf(nullptr);
    services.set_alpha(nullptr);
}

void RisingFlameReset() {
    risen_frames = 0;
}

} // namespace expanded
