#pragma once

namespace expanded {

// Tsukikage "Moonlit Focus", inherited by Heaven's Cloud (both also have Quick Draw, small_sword.hpp).
// - In hand: Toan's charge builds twice as fast (lunge ~0.25 s, whirlwind ~0.75 s).
// - Super Steve carrying either sphere: pellets fly twice as fast.
inline constexpr float kMoonlitFocusRate = 2.0f;

bool MoonlitFocusSword();
bool MoonlitFocusSphere();

// Hook: ToanKey_Play's wind-up: what step adds to the charge time this frame.
float MoonlitFocusCharge(float step);

// Hook: BattleActionPlay_Jinn, a pellet just fired: its velocity.
void MoonlitFocusShot(float *vector);

} // namespace expanded
