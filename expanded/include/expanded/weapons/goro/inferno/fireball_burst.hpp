#pragma once

namespace dc::game {
class CSHOT_EFFECT;
struct BT_SHOT_EFFECT;
} // namespace dc::game

namespace expanded {

// The Inferno's smash landing: the fireball's burst (kRisingFlamePack's key kFireballBurstKey, the one a monster's
// fireball plays as it strikes) as one of Expanded's shot effects, kFireballBurstScale times its size, where his weapon
// meets the ground, in place of his impact's own model, which is hidden (its hit, shake and kick are as ever). It is the
// visual alone: it hits nothing. The burst spreads far beyond the fireball: at its own size it already fills the space
// around him that the flame does at its smash size (rising_flame.hpp), and any larger it closes over the camera.
inline constexpr int   kFireballBurstKey = 1;
inline constexpr float kFireballBurstScale = 1.0f;
inline constexpr int   kFireballBursts = 1;

// The burst's description (registered for the Inferno: shot_effects.hpp).
BT_SHOT_EFFECT *FireballBurstEffect();

// Hook: his charge attack's impact set in slot of effect at at (GoroKey_Play); with the Inferno, the burst there.
void InfernoImpact(CSHOT_EFFECT &effect, int slot, const float *at);
// One step: the impact's model put back to its size once its slot is done.
void FireballBurstStep();

} // namespace expanded
