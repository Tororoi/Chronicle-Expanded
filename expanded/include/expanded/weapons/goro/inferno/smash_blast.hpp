#pragma once

namespace dc::game {
class CSHOT_EFFECT;
struct BT_SHOT_EFFECT;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// The Inferno's smash landing: Mr. Blare's self-destruct blast (kSmashBlastPack: its shot's one clip, played once where
// it is put) as one of Expanded's shot effects, kSmashBlastScale times its size, where his weapon meets the ground, in
// place of his impact's own model, which is hidden (its shake and kick are as ever; its shockwave reaches
// kSmashShockwaveRadius, not its native 15). The blast is the visual alone: it hits nothing.
inline constexpr const char *kSmashBlastPack = "dun/effect/zibaku_f2.chr";
inline constexpr float       kSmashBlastScale = 1.5f;
inline constexpr int         kSmashBlasts = 1;
inline constexpr float       kSmashShockwaveRadius = 20.0f;

// The blast's description (registered for the Inferno: shot_effects.hpp).
BT_SHOT_EFFECT *SmashBlastEffect();

// The blast at at (Xiao's charged pellet's end too: inferno/fire_shot.hpp), in orange or, for Satan's Ax's sphere, pink
// red (data/pink_fire.hpp): false when it is not loaded or has no room.
enum class BlastLook {
    kOrange,
    kPink,
};
// size times its own (the hammer's grown size: goro/last_judgement.hpp).
bool SmashBlastAt(const float *at, BlastLook look = BlastLook::kOrange, float size = 1.0f);

// Hook: his charge attack's impact set in slot of effect at at (GoroKey_Play); with the Inferno, the blast there.
void InfernoImpact(CSHOT_EFFECT &effect, int slot, const float *at);
// Hook: a shot effect planting its hit (ShotEffectHitSet); the Inferno's impact reaching kSmashShockwaveRadius.
void InfernoImpactHit(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit);
// One step: the impact's model put back to its size once its slot is done.
void SmashBlastStep();

} // namespace expanded
