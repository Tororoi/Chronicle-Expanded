#include "expanded/weapons/ungaga/mirage/heat_haze.hpp"

#include <random>

#include "expanded/core/random.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "runeffect.hpp"
#include "snd.hpp"
#include "texture.hpp"

namespace expanded::mirage {

namespace {

constexpr float kFireRise = 3.0f; // DrawRaster lifts the fire's own point by this

float phase = 0.0f;

} // namespace

void HazeStep() {
    phase += kHazeWaveStep * std::uniform_real_distribution<float>(0.0f, 1.0f)(ModRandom());
    if (!(phase < kHazeWaveCycle)) {
        phase = 0.0f;
    }
}

// CFireOmni::DrawRaster at the copy, with the shimmer's size and sway.
void HazeDraw(const float *root, float gain) {
    if (gain <= 0.0f) {
        return;
    }
    CTexture *mask = TexManager.GetTexture(const_cast<char *>("alpha01"), -1);
    CTexture *blender = TexManager.GetTexture(const_cast<char *>("blender"), -1);
    if (mask == nullptr || blender == nullptr) {
        return; // no fires in this dungeon: nothing to blend with
    }
    sceVu0FVECTOR world = {root[0], root[1] + kHazeLift + kFireRise, root[2], 1.0f};
    int           top_left[4];
    int           bottom_right[4];
    if (MGRotTransPers3DSprite(top_left, bottom_right, world, kHazeWidth, kHazeHeight, 0) != 1) {
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
    sceGsTex0 blend = *reinterpret_cast<sceGsTex0 *>(&blender->tex0);
    float     depth = static_cast<float>(top_left[2] >> 4) * kHazeSway * gain;
    blendTextuerTest(Vif1Packet, blend.TBP0, blend.TBW, blend.PSM, CRect_i_(x, y >> 1, width, height), mask,
                     CRect_i_(0, 0, width, height), CRect_i_(0, 0, 64, 64), depth, phase);
    set2DSprite(Vif1Packet, blender, CRect_i_(x, y, width, height * 2), CRect_i_(1, 1, width - 2, height - 2));
}

float HazePhase() {
    return phase;
}

void HazeReset() {
    phase = 0.0f;
}

} // namespace expanded::mirage
