#pragma once

class CSHOT_EFFECT;

namespace expanded {

// The Ice Queen's ice arrow and prison as shot effects, shared by her and the Crystal Gemron: the packs of her
// companions (the korinoya and the kori).
inline constexpr const char *kIceArrowFile = "dun/monstor/korinoya.chr";
inline constexpr const char *kIcePrisonFile = "dun/monstor/kori.chr";

// The player's chest, which the arrow is aimed and steered at.
void PlayerChest(float *chest);

// Whether the player is frozen, and whether any of his ailments still runs.
bool PlayerFrozen();
bool PlayerAfflicted();

// Steers a flying arrow (phase 1) as the korinoya flies: within 60 degrees of its heading it turns toward the
// player's chest by a 0.12 chord a step, at 1.2 a step, its model facing its flight.
void SteerIceArrow(CSHOT_EFFECT &effect, int slot);

// Moves a shot to a phase as its impact would, ending it when the phase has no motion.
void EnterShotPhase(CSHOT_EFFECT &effect, int slot, int phase);

// Holds a formed prison (phase 1) on its last frame; true while it stands formed.
bool HoldIcePrison(CSHOT_EFFECT &effect, int slot);

} // namespace expanded
