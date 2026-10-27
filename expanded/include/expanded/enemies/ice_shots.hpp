#pragma once

namespace dc::game {
class CSHOT_EFFECT;
struct BT_SHOT_EFFECT;
} // namespace dc::game

namespace expanded {

// The Ice Queen's ice arrow and prison as shot effects, shared by her and the Crystal Gemron: the packs of her
// companions (the korinoya and the kori).
inline constexpr const char *kIceArrowFile = "dun/monstor/korinoya.chr";
inline constexpr const char *kIcePrisonFile = "dun/monstor/kori.chr";

// The Crystal Gemron's arrow: Holy Gemron's shot on the Ice Queen's arrow, flying from the moment it is fired at 1.2 a
// step for 360 steps, with Freeze, the Ice element and a guardable knockback. Each user enters its own copy.
BT_SHOT_EFFECT IceArrowShot();

// The prison's description (no hit; phase 1 formed, 2 shattering), each user entering its own copy.
BT_SHOT_EFFECT IcePrisonShot();

// The player's chest, which the arrow is aimed and steered at, and feet, where the prison forms.
void PlayerChest(float *chest);
void PlayerFeet(float *feet);

// Whether a point is within reach of the player's chest.
bool NearPlayerChest(const float *point, float reach);

// Steps a spent arrow's hit takes to reach the player (and freeze him).
inline constexpr int kIceArrowSettle = 2;

// Whether the player is frozen, and whether any of his ailments still runs.
bool PlayerFrozen();
bool PlayerAfflicted();

// Steers a flying arrow (phase 1) as the korinoya flies: within 60 degrees of its heading it turns toward the
// player's chest by a 0.12 chord a step, at 1.2 a step, its model facing its flight.
void SteerIceArrow(CSHOT_EFFECT &effect, int slot);

// Holds a formed prison (phase 1) on its last frame; true while it stands formed.
bool HoldIcePrison(CSHOT_EFFECT &effect, int slot);

// The freeze's sounds, as a prison forms, and a prison's shatter: its sound and phase, at half pace.
void PlayIceFreeze();
void ShatterIcePrison(CSHOT_EFFECT &effect, int slot);

} // namespace expanded
