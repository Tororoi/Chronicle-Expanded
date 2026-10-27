#pragma once

#include <libvu0.h>

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded {

// The dungeon's light for a weapon's flash: darkened before it, washed to white by it and eased back after, by the
// colours DunMainDraw hands the renderer each frame (the floor's own values are never written). Times are game frames.

// How a flash looks: the light's and the fog's colour at the peak, how much of the fog it washes (0..1), how many
// frames the white takes to recede, and the dim it recedes onto and holds (0 = none) until release frames before
// rest frames have passed.
struct FlashLook {
    float light[3] = {255.0f, 240.0f, 200.0f};
    float fog[3] = {255.0f, 255.0f, 255.0f};
    float fog_amount = 1.0f;
    int   ease_frames = 300;
    float rest_dim = 0.0f;
    int   rest_frames = 0;
    int   release_frames = 60;
};

inline constexpr float kDimKeep = 0.15f;    // how much of the floor's light is left at full dim
inline constexpr float kDimFogPull = 0.35f; // the fog's reach at full dim, as a fraction of its own
inline constexpr int   kRampFrames = 4;     // a strike's darkening, the frames before it

// The room darkened to k (0 = its own light, 1 = full dim); a flash running takes no dim until it is over.
void SceneDim(float k);
// The darkening before a strike: from `from` to full black along u (0..1), barely moving at first and plunging at the end.
void SceneDimRamp(float from, float u);
// No dim: the floor's own light back.
void SceneDimEnd();
// The flash: white at once, receding as look says.
void SceneFlash(const FlashLook &look);
// Everything back to the floor's own light, at once (the floor changed, the weapon was put away).
void SceneLightReset();
// The darkness last applied (0 = the floor's own light).
float SceneDarkness();
bool  SceneFlashing();

// One step of the flash's ease (once a frame the monsters step unpaused).
void SceneLightStep();

// Hooks: DunMainDraw, the light set it hands the renderer, changed in place; CCharacter::Draw, the light a character
// takes on as the room darkens (a slight white that keeps it out of the dark), added to ambient.
void SceneLighting(float *ambient, sceVu0FMATRIX colours, float *fog_rate, unsigned char *fog_colour);
void SceneCharacterLight(const CCharacter &character, float *ambient);

} // namespace expanded
