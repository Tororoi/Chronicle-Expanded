#pragma once

class CSHOT;
class CMonstorUnit;

namespace expanded {

// Hardshooter "Ricochet": with the Hardshooter or the Double Impact built from it in hand, or Super Steve carrying
// either SynthSphere, a pellet that lands on a monster sends a second pellet from it at the living monster nearest the
// one struck (within kRicochetRange), aimed at that monster's largest hurt sphere taking hits now, or in a random level
// direction when none is that near. The second pellet carries the first's damage, speed and size, leaves the monster
// struck before the game's collision takes it, so it never comes back at it, and does not ricochet again. A pellet
// ending on a wall, or running out, sends none.
inline constexpr float kRicochetRange = 120.0f;
inline constexpr int   kRicochetLife = 120;

bool RicochetActive();

// Whether slot of shots holds a ricochet.
bool Ricochet(int slot);

// The monster whose hurt sphere's centre is at point (checkCollision's hit position on a monster), or -1.
int RicochetStruck(const CMonstorUnit &unit, const float *point);

// The slot the ricochet took, or -1 (for tests).
int  RicochetShotEnded(CSHOT &shots, int slot, const float *point, bool struck);
void RicochetShotFired(int slot);

// Two pellets fired as a pair (Double Impact): their ricochets take different monsters.
void RicochetPair(int first, int second);

// A shot's step (CSHOT::step, before its collision): a ricochet still leaving the monster it came from is moved along
// its way, the game's collision held off, until it stands clear of that monster by a frame of travel.
void RicochetStep(CSHOT &shots, int slot);

} // namespace expanded
