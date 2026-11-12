#pragma once

namespace dc::game {
class CSHOT;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Super Steve carrying the Steel Hammer's SynthSphere: every pellet flies as the hammer's head (its spiked ball, without
// the handle, a spike where it was: data/steel_hammer_head.hpp) at kSteelHeadScale its size, tumbling as it flies
// (model_pellets.hpp) at kSteelHeadSpeed its speed (as long in the air for it, so as far), and its hit throws the
// monster back as the Baselard's Heavy Hand does (toan/baselard.hpp). A pellet fired with every one of the effect's
// kSteelHeadSlots in the air flies as a plain pellet, as slow, still throwing back.
inline constexpr int   kSteelHeadSlots = 8;
inline constexpr float kSteelHeadScale = 0.75f;
inline constexpr float kSteelHeadSpeed = 0.5f;

// Whether Xiao's pellets are the hammer's heads.
bool SteelHammerPellets();

// Hooks: one step (once a frame the monsters step unpaused); the floor's monsters put back; Xiao fired a pellet into
// slot of shots (slowed here); a pellet ended.
void SteelHeadShotStep();
void SteelHeadShotReset();
void SteelHeadShotFired(CSHOT &shots, int slot);
void SteelHeadShotEnded(const CSHOT &shots, int slot);

} // namespace expanded
