#pragma once

struct BT_SHOT_EFFECT;

namespace expanded {

// The poof a summon appears in: gedit/e03's cutscene smoke (e228ex) as one of Expanded's shot effects, burst at a spot at
// the summoner's scale, turned to the summoner's facing, its one clip (frames kSmokeClipStart to kSmokeClipEnd, authored
// at 0.2 a frame) played at kSmokeRate, held at its spot, and from frame kSmokeFadeFrom faded until it is gone at
// kSmokeClipEnd, where the effect ends it. It is the visual alone: it hits nothing. One plays at a time. What rises in it
// is timed on its frames.
inline constexpr const char *kSummonSmokeFile = "gedit/e03/chara/e228ex.chr";
inline constexpr float       kSmokeClipStart = 1.0f, kSmokeClipEnd = 50.0f, kSmokeFadeFrom = 32.0f;
inline constexpr float       kSmokeRate = 0.6f;
inline constexpr int         kSmokeShots = 1;

// The smoke's description, for the weapons that summon in it to register (shot_effects.hpp).
BT_SHOT_EFFECT *SummonSmokeEffect();

// A poof at spot (its ground), turned to yaw, scale times its own size, for owner (the summon, by any pointer of its
// own); one already playing is put out. False when the smoke is not loaded on this floor.
bool SummonSmokeStart(const float *spot, float yaw, float scale, const void *owner);
// One step (once a frame the monsters step unpaused): held at its spot, at its rate, faded past kSmokeFadeFrom.
void SummonSmokeStep();
// Put out now, if it plays for owner: another summon's smoke plays on.
void SummonSmokeEnd(const void *owner);
bool SummonSmokePlaying();
// The frame of its clip, or -1 when none plays.
float SummonSmokeFrame();

} // namespace expanded
