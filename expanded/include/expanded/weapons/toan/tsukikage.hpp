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

// Tsukikage "Moonlit Bait": while a Tsukikage is owned (any bag or the storage), at night (NowTime in EdGetTime's
// ED_TIME_NIGHT, 5.5-8.5; a town whose clock stands, as Brownboo's and Yellow Drops', is day) a fish with any liking
// for the bait on the hook likes it kMoonlitBait; one with none still has none.
inline constexpr float kMoonlitBait = 1.0f;

bool MoonlitBaitActive();

// A fish's affinity for the bait on the hook, affinity the game's.
float MoonlitBaitAffinity(float affinity);

} // namespace expanded
