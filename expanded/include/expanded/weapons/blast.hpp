#pragma once

#include "expanded/weapons/kick_back.hpp"

class CMonstorUnit;
struct COLLISION_HIT;

namespace expanded {

// The falloff blast: one hit sphere on each live monster in reach, its damage the weapon's attack times the step for
// that monster's distance from the blast (to the nearest edge of its body spheres), no element, the weapon's own
// abilities, a throw away from the blast, and no guard to stop it. Big Bang's whirlwind and judgement blade and Super
// Steve's bombs plant it.

struct BlastStep {
    float reach;
    float times;
};

// Outermost first: an enemy takes the innermost step its distance is within.
inline constexpr BlastStep kBlastFalloff[] = {
    {50.0f, 1.0f},
    {40.0f, 2.0f},
    {25.0f, 3.0f},
    {10.0f, 4.0f},
};
// About 50 units at full strength (speed^2 / (2 decay)).
inline constexpr KickBack kBlastKick = {3.5f, 0.12f};
inline constexpr int      kBlastHitLife = 3;
// The frames a full throw takes to slow to a stop.
inline constexpr int kBlastThrowFrames = static_cast<int>(kBlastKick.speed / kBlastKick.decay) + 1;

struct BlastOptions {
    float damage = 1.0f; // of the steps
    float reach = 1.0f;  // of the step reaches
    float kick = 1.0f;   // of the throw's speed
    int   except = -1;   // a monster the blast leaves alone
    int   still = -1;    // a monster that takes the hit where it stands
};

// The blast at point with attack: how many monsters it reached.
int PlantBlast(CMonstorUnit &unit, const float *point, int attack, const BlastOptions &options = {});

// The distance from point to the nearest edge of a monster's body spheres, across the floor (no height).
float BlastDistance(const CMonstorUnit &unit, int monster, const float *point);

// What an explosion sounds as: the game's bomb; Toan's blasts, the big explosion the Dark Genie's attack on Norune makes
// in the opening; Super Steve's bombs, the Pirate's Chariot's cannon. Those two duck the game's other sounds under them
// (OwnSound). The game's bomb stands in for a bank that cannot be read.
enum class BlastSound {
    kBomb,
    kToan,
    kSteve,
};

void PlayBlastSound(BlastSound sound);

// The game's bomb explosion at point, its puffs scale times their size, and a shock ring growing out to ring (a blast's
// reach; none at 0), sounding as sound: the look alone, hurting no one. False when the game's three explosions are all
// in use.
bool ExplosionAt(const float *point, float scale, float ring = 0.0f, BlastSound sound = BlastSound::kBomb);

// The game's shock ring at point growing out to ring, over the frames the game's bomb of scale takes (15 × scale).
void ShockRingAt(const float *point, float ring, float scale);

// Whether a hit is a blast's, which no guard stops.
bool BlastHit(const COLLISION_HIT &hit);

} // namespace expanded
