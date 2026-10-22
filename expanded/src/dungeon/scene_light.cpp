#include "expanded/dungeon/scene_light.hpp"

#include <algorithm>
#include <cmath>

#include "character.hpp"

namespace expanded {

namespace {

// The white's fall: exponential, normalised to reach nothing at the end, so most of it goes in the first moments.
constexpr double kDecay = 4.0;
constexpr int    kFogFrames = 60; // the fog lifts in a second; the light takes the whole ease
constexpr double kRampSharpness = 4.0;
// The slight white everyone in the room takes as it darkens, at its fullest at a dim of kTintFullDim.
constexpr float kTintMax = 30.0f;
constexpr float kTintFullDim = 0.5f;

float     dim = 0.0f;
bool      flashing = false;
int       frames = 0;
FlashLook look;

float Lerp(float a, float b, float k) { return a + (b - a) * k; }

float Fall(double t) {
    t = std::clamp(t, 0.0, 1.0);
    const double end = std::exp(-kDecay);
    return static_cast<float>((std::exp(-kDecay * t) - end) / (1.0 - end));
}

// What the flash leaves: white k of the light, kf of the fog, over the dim.
void Levels(float &white, float &fog_white, float &darkness) {
    white = fog_white = 0.0f;
    darkness = dim;
    if (!flashing) {
        return;
    }
    white = Fall(static_cast<double>(frames) / std::max(1, look.ease_frames));
    fog_white = Fall(static_cast<double>(frames) / kFogFrames) * std::clamp(look.fog_amount, 0.0f, 1.0f);
    darkness = 0.0f;
    if (look.rest_dim > 0.0f) {
        const int left = look.rest_frames - frames;
        darkness = left > look.release_frames ? look.rest_dim : look.rest_dim * std::max(0, left) / std::max(1, look.release_frames);
    }
}

} // namespace

void SceneDim(float k) {
    if (!flashing) {
        dim = std::clamp(k, 0.0f, 1.0f);
    }
}

void SceneDimRamp(float from, float u) {
    if (flashing) {
        // A strike inside a flash: the dark takes over from the white.
        flashing = false;
    }
    u = std::clamp(u, 0.0f, 1.0f);
    const float ramp = static_cast<float>((std::exp(kRampSharpness * u) - 1.0) / (std::exp(kRampSharpness) - 1.0));
    dim = std::clamp(from + (1.0f - from) * ramp, 0.0f, 1.0f);
}

void SceneDimEnd() {
    if (!flashing) {
        dim = 0.0f;
    }
}

void SceneFlash(const FlashLook &flash) {
    look = flash;
    flashing = true;
    frames = 0;
    dim = 0.0f;
}

void SceneLightReset() {
    flashing = false;
    dim = 0.0f;
    frames = 0;
}

float SceneDarkness() {
    float white, fog_white, darkness;
    Levels(white, fog_white, darkness);
    return darkness;
}

bool SceneFlashing() { return flashing; }

void SceneLightStep() {
    if (!flashing) {
        return;
    }
    frames++;
    const int total = look.rest_dim > 0.0f ? std::max(look.rest_frames, look.ease_frames) : look.ease_frames;
    if (frames >= total) {
        flashing = false;
        frames = 0;
    }
}

void SceneLighting(float *ambient, sceVu0FMATRIX colours, float *fog_rate, unsigned char *fog_colour) {
    float white, fog_white, darkness;
    Levels(white, fog_white, darkness);
    if (white <= 0.0f && fog_white <= 0.0f && darkness <= 0.0f) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] = Lerp(Lerp(ambient[c], ambient[c] * kDimKeep, darkness), look.light[c], white);
        for (int row = 0; row < 4; row++) {
            colours[row][c] = Lerp(Lerp(colours[row][c], colours[row][c] * kDimKeep, darkness), look.light[c], white);
        }
    }
    // A floor without fog keeps none: the light alone carries the flash there.
    if (fog_rate[1] > fog_rate[0]) {
        fog_rate[0] = Lerp(Lerp(fog_rate[0], fog_rate[0] * kDimFogPull, darkness), 0.0f, fog_white);
        fog_rate[1] = Lerp(Lerp(fog_rate[1], fog_rate[1] * kDimFogPull, darkness), 1.0f, fog_white);
    }
    for (int c = 0; c < 3; c++) {
        fog_colour[c] = static_cast<unsigned char>(std::lround(Lerp(Lerp(fog_colour[c], 0.0f, darkness), look.fog[c], fog_white)));
    }
}

void SceneCharacterLight(const CCharacter &, float *ambient) {
    const float add = kTintMax * std::clamp(SceneDarkness() / kTintFullDim, 0.0f, 1.0f);
    for (int c = 0; c < 3; c++) {
        ambient[c] += add;
    }
}

} // namespace expanded
