#pragma once

namespace dc::game {
class CSHOT;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Xiao's charged pellet with the Inferno's SynthSphere on Super Steve (satans_ax/underworld_shot.hpp's, its pull and the
// Inferno's Burning Aura with it: inferno.hpp): a fire hit, whatever element is selected (her shots not charged keep
// it), drawn as the rising flame's fireball (inferno/rising_flame.hpp) flying
// head first, kFireShotScale times its size, in place of the pellet. Where it ends (a wall or its life: it goes on
// through monsters) Mr. Blare's blast goes off on the floor beneath it (inferno/smash_blast.hpp) with the explosion's sound, and a fire hit of
// the pellet's damage reaches kFireShotBlast around it (Goro's smash's shockwave's reach), throwing what it hits
// kFireShotKick of a blast's throw.
inline constexpr float kFireShotScale = 2.0f;
inline constexpr float kFireShotBlast = 20.0f;
inline constexpr float kFireShotKick = 0.7f;
// With Satan's Ax's SynthSphere it is the same in pink red (data/pink_fire.hpp), the fireball at its own size and its
// blast's hit kSatansShotBlast around (Mr. Blare's own reach), in the selected element.
inline constexpr float kSatansShotScale = 1.0f;
inline constexpr float kSatansShotBlast = 14.0f;

// Whether any is flying.
bool FireShotsFlying();
// Hooks: whether slot of shots is one, its pellet not drawn (CSHOT::draw); each drawn, with the dungeon's effects; one
// ended at point (CSHOT::step), before its pull lets it go.
bool FireShotHidden(const CSHOT &shots, int slot);
void FireShotHit(const CSHOT &shots, int slot, COLLISION_HIT &hit);
void FireShotsDraw();
void FireShotEnded(const CSHOT &shots, int slot, const float *point);

} // namespace expanded
