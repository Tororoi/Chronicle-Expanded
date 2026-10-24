#pragma once

class CSHOT;

namespace expanded {

// Double Impact: with the Double Impact in hand, or Super Steve carrying its SynthSphere, every pellet Xiao fires is
// joined by a twin side by side with it, kDoubleImpactWidth apart across its flight, each at kDoubleImpactDamage of the
// shot's attack. Each lands its own hit, so each rolls the weapon's abilities, and both are drawn as the Steel
// Slingshot's stone. Both ricochet as the Hardshooter's pellets do, at different monsters.
inline constexpr float kDoubleImpactDamage = 0.75f;
inline constexpr float kDoubleImpactWidth = 1.0f;

bool DoubleImpactActive();

// Hook: a pellet fired into slot (BattleActionPlay_Jinn). The twin's slot, or -1.
int DoubleImpactFired(CSHOT &shots, int slot);

} // namespace expanded
