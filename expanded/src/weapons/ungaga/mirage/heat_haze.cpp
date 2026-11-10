#include "common.h"

#include <random>

#include "expanded/core/random.hpp"
#include "expanded/dungeon/torch_fire.hpp"
#include "expanded/weapons/ungaga/mirage/heat_haze.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "runeffect.hpp"
#include "snd.hpp"
#include "texture.hpp"

namespace expanded::mirage {

namespace {

constexpr float kFireRise = 3.0f; // DrawRaster lifts the fire's own point by this

float phase = 0.0f;

// The dungeon's own mask, or the shared fire's on a floor without one.
CTexture *Mask() {
    CTexture *own = TexManager.GetTexture(const_cast<char *>("alpha01"), -1);
    return own != nullptr || !TorchFireReady() ? own : TorchFireTexture("alpha01");
}

// The raster over the screen's x by y (in DrawRaster's own units: y in half rows, height in half the rows it covers),
// swaying by depth.
void Raster(int x, int y, int width, int height, float depth) {
    CTexture *mask = Mask();
    CTexture *blender = TexManager.GetTexture(const_cast<char *>("blender"), -1);
    if (mask == nullptr || blender == nullptr) {
        return;
    }
    sceGsTex0 blend = *reinterpret_cast<sceGsTex0 *>(&blender->tex0);
    blendTextuerTest(Vif1Packet, blend.TBP0, blend.TBW, blend.PSM, CRect_i_(x, y >> 1, width, height), mask,
                     CRect_i_(0, 0, width, height), CRect_i_(0, 0, 64, 64), depth, phase);
    set2DSprite(Vif1Packet, blender, CRect_i_(x, y, width, height * 2), CRect_i_(1, 1, width - 2, height - 2));
}

// Where root projects, or false off the screen.
bool Project(const float *root, float lift, float width, float height, int *top_left, int *bottom_right) {
    sceVu0FVECTOR world = {root[0], root[1] + lift + kFireRise, root[2], 1.0f};
    return MGRotTransPers3DSprite(top_left, bottom_right, world, width, height, 0) == 1;
}

} // namespace

void HazeStep(float step) {
    phase += step * std::uniform_real_distribution<float>(0.0f, 1.0f)(ModRandom());
    if (!(phase < kHazeWaveCycle)) {
        phase = 0.0f;
    }
}

// CFireOmni::DrawRaster at root, with the shimmer's size and sway.
void HazeDraw(const float *root, float gain, const HazeLook &look) {
    int top_left[4];
    int bottom_right[4];
    if (gain <= 0.0f || !Project(root, look.lift, look.width, look.height, top_left, bottom_right)) {
        return;
    }

    int x = (top_left[0] - 0x6C00) >> 4;
    int y = (top_left[1] - GS_Y_OFFSET) >> 3;
    int width = (bottom_right[0] - top_left[0]) >> 4;
    int height = (bottom_right[1] - top_left[1]) >> 4;
    height >>= 1;

    if (x + width > 639 && x < 640) {
        width = 639 - x;
    }

    if (x < 0 && x + width > 0) {
        width = x + width;
        x = 0;
    }

    if (y < 1) {
        height += y >> 1;
        y = 0;
    }

    // The sway is the depth the raster passes times the fire's scale and its gain: the shimmer's scale and gain ride it.
    Raster(x, y, width, height, static_cast<float>(top_left[2] >> 4) * look.sway * gain);
}

void HazeDrawOver(const float *root, float gain, float sway) {
    int top_left[4];
    int bottom_right[4];
    if (gain <= 0.0f || !Project(root, 0.0f, 1.0f, 1.0f, top_left, bottom_right)) {
        return;
    }
    Raster(0, 0, 639, SCREEN_HEIGHT / 2, static_cast<float>(top_left[2] >> 4) * sway * gain);
}

float HazePhase() {
    return phase;
}

void HazeReset() {
    phase = 0.0f;
}

} // namespace expanded::mirage
