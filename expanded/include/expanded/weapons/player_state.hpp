#pragma once

namespace expanded {

// The player as the weapons' effects read him: BtActStatus's actions (each played by the motion of the same number), the
// guard and the lock-on.

// Toan (ToanKey_Play): the five combo swings, the charge's wind-up and lunge, the whirlwind.
inline constexpr int kToanFirstSwing = 0x24, kToanLastSwing = 0x28;
inline constexpr int kToanWindup = 0xE;
inline constexpr int kToanLunge = 0xF;
inline constexpr int kToanWhirlwind = 0x18;

// Xiao (the slingshot): drawing it back, holding it drawn, the shot.
inline constexpr int kXiaoDraw = 11, kXiaoHold = 12, kXiaoShot = 13;

// Ungaga (UngagaKey_On): the three combo swings.
inline constexpr int kUngagaFirstSwing = 0x25, kUngagaLastSwing = 0x27;

// BtActStatus.guard_mode while the guard is up.
inline constexpr int kGuardUp = 5;
bool                 Guarding();

// The monster the player is locked on to while it is on the floor and alive, or -1.
int LockedTarget();

} // namespace expanded
