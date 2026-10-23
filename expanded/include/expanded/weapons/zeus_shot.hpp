#pragma once

class CCharacter;
class CSHOT;

namespace expanded {

// Super Steve carrying the Sword of Zeus's SynthSphere: the sword's lightning from Xiao's slingshot. A guard held
// kZeusGuardFrames primes it, the slingshot whitening and the room darkening as it builds. Her next shot not locked on
// brings a bolt down where it lands; locked on it opens a kZeusChainFrames window in which every pellet that reaches a
// monster brings a bolt down on it. A shot held drawn kZeusChargedFrames is a charged
// one: its bolt comes down where it ends. A pellet that calls a bolt hurts nothing itself; the bolt is its hit. A charge
// unused for kZeusPrimedFrames is spent. Times are game frames.

inline constexpr int   kZeusGuardFrames = 180;
inline constexpr int   kZeusPrimedFrames = 600;
inline constexpr int   kZeusChainFrames = 300;
inline constexpr int   kZeusChargedFrames = 60;
inline constexpr float kZeusStruckReach = 30.0f; // a pellet that met a monster struck the nearest within this

bool ZeusShotActive();

// One step (once a frame the monsters step unpaused); the floor's monsters put back.
void ZeusShotStep();
void ZeusShotReset();

// Hooks: Xiao fired a pellet into slot of shots; a pellet ended at point (on a monster when struck); the white the
// charge puts on the slingshot.
void ZeusShotFired(CSHOT &shots, int slot);
void ZeusShotEnded(const CSHOT &shots, int slot, const float *point, bool struck);
void ZeusShotLight(const CCharacter &character, float *ambient);
// Whether a pellet out calls a bolt (and so hurts nothing itself).
bool ZeusShotCallsBolt(const CSHOT &shots, int slot);

} // namespace expanded
