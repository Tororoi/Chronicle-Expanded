#pragma once

namespace dc::game {
class CSHOT_EFFECT;
struct BT_SHOT_EFFECT;
} // namespace dc::game

namespace expanded {

// The Inferno's smash landing: Mr. Blare's self-destruct blast (kSmashBlastPack: its shot's one clip, played once where
// it is put) as one of Expanded's shot effects, kSmashBlastScale times its size, where his weapon meets the ground, in
// place of his impact's own model, which is hidden (its hit, shake and kick are as ever). It is the visual alone: it
// hits nothing.
inline constexpr const char *kSmashBlastPack = "dun/effect/zibaku_f2.chr";
inline constexpr float       kSmashBlastScale = 1.0f;
inline constexpr int         kSmashBlasts = 1;

// The blast's description (registered for the Inferno: shot_effects.hpp).
BT_SHOT_EFFECT *SmashBlastEffect();

// Hook: his charge attack's impact set in slot of effect at at (GoroKey_Play); with the Inferno, the blast there.
void InfernoImpact(CSHOT_EFFECT &effect, int slot, const float *at);
// One step: the impact's model put back to its size once its slot is done.
void SmashBlastStep();

} // namespace expanded
