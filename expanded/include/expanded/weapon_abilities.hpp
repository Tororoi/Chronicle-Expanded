#pragma once

namespace expanded {

// The weapon ability bits; Confuse is Expanded's own.
inline constexpr int kAbilityPoor = 0x0004;
inline constexpr int kAbilityStop = 0x0040;
inline constexpr int kAbilityFragile = 0x0100;
inline constexpr int kAbilityDrain = 0x0400;
inline constexpr int kAbilityHeal = 0x0800;
inline constexpr int kAbilityCritical = 0x1000;
inline constexpr int kAbilityAbsUp = 0x2000;
inline constexpr int kAbilityConfuse = 0x4000;

// A weapon's abilities as the game builds it: its table flags plus those it gains, the chance ones rolled now.
int WeaponAbilities(int weapon_no, int flags);

// The same, with a roll of 0-99 deciding the chance abilities.
int WeaponAbilities(int weapon_no, int flags, int roll);

} // namespace expanded
