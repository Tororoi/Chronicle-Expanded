#pragma once

namespace expanded {

// Mirage "Decoy", as the mod has it: with the Mirage or Hercules' Wrath in Ungaga's hand, or Super Steve carrying either's
// SynthSphere, the guard pose held kMirageGuardFrames (the clock starting on its first frame and running while the guard
// button stays down) flashes the wielder with the charge-complete flash and plants a decoy where they stand: a copy of them
// holding the guard (mirage/clone.hpp) under a heat shimmer (mirage/heat_haze.hpp). One plant a guard: letting the guard
// button go readies the next. The decoy stands kMirageFrames, kHerculesMirageFrames when Hercules' Wrath plants it.
//   - While it stands, every monster's script finds the player at the decoy, so it walks to it and attacks it there; a
//     monster the player's side hits finds the player again, until the next decoy. A monster set up while it stands goes
//     for it too.
//   - A plant with a decoy standing hands off to a new one at once (its time and its shimmer start): the old copy stays and
//     dissolves over kMirageHandoffFrames, then the copy comes back at the new spot. The monsters keep to the old spot until
//     the new copy has fully appeared (kMirageAggroHoldFrames), when all of them, those hit since the old one too, take to
//     the new one. A plant during a hand-off does nothing.
//   - Its shimmer rises over kMirageHazeRampFrames, then the copy fades in over kMirageFadeFrames; at the end the copy fades
//     out first and the shimmer last. The shimmer sits on the copy, the old one during a hand-off.
//   - It goes when its time is up, the weapon leaves the hand (or its sphere Super Steve), the active character changes or
//     the floor does. Paused, it stands still, its time with it.
//   - While it stands it owns where the monsters find the player: confusion's target waits until it goes.
// Times are game frames.
inline constexpr int kMirageGuardFrames = 15;
inline constexpr int kMirageFrames = 720;
inline constexpr int kHerculesMirageFrames = 1080;
inline constexpr int kMirageHazeRampFrames = 15;
inline constexpr int kMirageFadeFrames = 30;
inline constexpr int kMirageHandoffFrames = 15;
inline constexpr int kMirageAggroHoldFrames = kMirageHazeRampFrames + kMirageFadeFrames;
// The guard pose's loop, Ungaga's and Xiao's alike: the motion the copy holds.
inline constexpr int kMirageGuardLoop = 9;

// The Mirage line, the Mirage and what is built from it (the Terra Sword, Hercules' Wrath, Babel's Spear), in hand or as
// Super Steve's sphere: monsters can be locked on from kMirageLineReach times as far.
inline constexpr float kMirageLineReach = 2.0f;

// Whether the decoy can be planted: the Mirage or Hercules' Wrath in hand, or as Super Steve's sphere.
bool MirageActive();
bool MirageLineReachActive();

// Hooks: a dungeon step, after the monsters' shots stepped; a monster took a hit (CheckDmg); where a monster's script finds
// the player (_GET_POSITION, _GET_DISTANCE), true when the decoy sets it; the active character's lock-on range scale
// (SetNearLockOnTarget, setTargetCursor); the copy's draw, with the dungeon's characters, and the shimmer's, after the map's
// heat rasters.
void  MirageStep();
void  MirageHitTaken(int monster);
bool  MirageTarget(int monster, float *position);
float MirageLineReachRate(float rate);
void  MirageCloneDraw();
void  MirageHazeDraw();

// Whether a decoy stands (a hand-off included), whether it is handing off, where the monsters find it (the old spot while
// they keep to it) and where it was planted, the frames it has stood and its life.
bool MirageDecoyUp();
bool MirageHandingOff();
void MirageDecoyTarget(float *out);
void MirageDecoyPlace(float *out);
int  MirageDecoyAge();
int  MirageDecoyLife();
// How solid the copy is (0 to 1) and how strong the shimmer is (0 to 1).
float MirageCloneAlpha();
float MirageHazeGain();

// Taken down (the floor changed), for tests too.
void MirageReset();

} // namespace expanded
