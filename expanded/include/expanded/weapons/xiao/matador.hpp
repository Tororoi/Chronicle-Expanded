#pragma once

class CCharacter;
class CSHOT;
struct COLLISION_HIT;

namespace expanded {

// Matador "Charging Bull": with the Matador in hand, or Super Steve carrying its SynthSphere, a shot held drawn
// kChargingBullFrames (the game's charge-complete flash) lets go a charged pellet: kChargingBullDamage times the
// damage, through a guard, and a shove along its flight as Goro's hammer swing gives (kChargingBullKick, from
// kChargingBullBehind units back down its line). It flies as an orange copy of the slingshot in hand in a fire glow, the
// pellet hidden inside it, upright as it is held. Times are game frames.
inline constexpr int   kChargingBullFrames = 60;
inline constexpr float kChargingBullDamage = 1.5f;
inline constexpr float kChargingBullKickSpeed = 2.5f;
inline constexpr float kChargingBullKickDecay = 0.1f;
inline constexpr float kChargingBullBehind = 30.0f;
inline constexpr float kChargingBullGlow = 14.0f; // the glow's diameter

bool ChargingBullActive();

// Hooks: a dungeon step; a pellet let go into slot; its hit planted (CSHOT::step); a pellet ended; whether a pellet's own
// sprite is left undrawn (CSHOT::draw); the copies' draw; the copy's orange (CCharacter::Draw).
void ChargingBullStep();
void ChargingBullFired(CSHOT &shots, int slot);
void ChargingBullHit(const CSHOT &shots, int slot, COLLISION_HIT &hit);
void ChargingBullEnded(const CSHOT &shots, int slot);
bool ChargingBullHidden(const CSHOT &shots, int slot);
void ChargingBullDraw();
void ChargingBullLight(const CCharacter &character, float *ambient);

// Whether a hit is a charged pellet's (a guard does not stop it); whether slot of shots carries a charge (for tests).
bool ChargingBullHit(const COLLISION_HIT &hit);
bool ChargingBullCharged(const CSHOT &shots, int slot);

} // namespace expanded
