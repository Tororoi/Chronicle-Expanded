#pragma once

struct BT_SHOT_EFFECT;

namespace expanded::terra_sword {

// The boulder's shockwave (terra_sword.hpp): the game's syougekiha (kShockFile) as one of Expanded's shot effects,
// burst on the floor where the boulder lands at kShockScale, its one clip played at kShockRate and held there until the
// effect ends it at the clip's end. It is the look alone: it hits nothing.
BT_SHOT_EFFECT *ShockwaveEffect();

// A burst at spot (its floor); one already playing is put out. False when it is not loaded on this floor.
bool ShockwavePlay(const float *spot);
// One step: held at its spot, its size and rate.
void ShockwaveStep();
// Put out now.
void ShockwaveStop();
bool ShockwavePlaying();
// Forgotten without touching the effect (its pack goes with the floor).
void ShockwaveReset();

} // namespace expanded::terra_sword
