#pragma once

#include "expanded/weapons/kick_back.hpp"

class CMonstorUnit;
struct COLLISION_HIT;

namespace expanded {

// The falloff blast: one hit sphere on each live monster in reach, its damage the weapon's attack times the step for
// that monster's distance from the blast (to the nearest edge of its body spheres), no element unless asked, the weapon's own
// abilities, a throw away from the blast, and no guard to stop it unless asked. Big Bang's whirlwind and judgement
// blade, Super Steve's bombs and the Sword of Zeus's bolts plant it.

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
// A monster this near the blast's centre, across the floor, is thrown a random way.
inline constexpr float kBlastCentred = 1.0f;
// The frames a full throw takes to slow to a stop.
inline constexpr int kBlastThrowFrames = static_cast<int>(kBlastKick.speed / kBlastKick.decay) + 1;

struct BlastOptions {
    float damage = 1.0f; // of the steps
    float reach = 1.0f;  // of the step reaches
    float kick = 1.0f;   // of the throw's speed
    int   except = -1;   // a monster the blast leaves alone …
    int   spared = 0;    // … and these, one bit each
    int   element = -1;  // the element the hit carries (WEAPON_ELEMENT_*), the weapon's own strength in it; -1 for none
    int   still = -1;    // a monster that takes the hit where it stands
    bool  guardable = false; // a monster's guard stops it, as any hit's
    bool  flat = false;      // every monster in reach takes the outermost step's share, however near
    float throw_to = 0.0f;   // each monster thrown out to this far from the blast, instead of kBlastKick
    bool  rumble = true;     // felt as the knockdown rumble
};

// A throw_to throw: from a monster's distance to the blast, the speed that carries it to the edge (decay as
// kBlastKick's: it slides speed^2 / (2 decay)), capped at kBlastThrowMost.
inline constexpr float kBlastThrowMost = 6.0f;
float                  BlastThrowSpeed(float distance, float edge);

// The blast at point with attack, felt as the knockdown rumble unless asked not to be: how many monsters it reached.
int PlantBlast(CMonstorUnit &unit, const float *point, int attack, const BlastOptions &options = {});

// How a blast is felt in the hands: the rumble of a knockdown blow to the player (a blast that hurts), or a lighter
// hit's (a shot's small burst). Both on the big motor, and only when the player has vibration on.
enum class BlastRumble {
    kKnockdown,
    kLight,
};
void RumbleBlast(BlastRumble rumble);
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

// Whether a hit is an unguardable blast's, which no guard stops.
bool BlastHit(const COLLISION_HIT &hit);

// Whether a hit is an unguardable blast of Ungaga's, which wears no weapon HP as it lands: what set it off bills its own
// (Hercules' Wrath's Air Strike).
bool UngagaBlastHit(const COLLISION_HIT &hit);

} // namespace expanded
