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

// The action gauge (BtActStatus.action_gauge) full, and the rate motionDrive refills Xiao's at (the weapon's speed / 30,
// at least 1, times kXiaoGaugeRate a frame).
inline constexpr float kFullGauge = 100.0f;
inline constexpr float kXiaoGaugeRate = 1.5f;

// The moves MoveChara plays every member by: the run and the walk (keyCtrl), and locked on, the stance and its strafes.
inline constexpr int kMoveRun = 1, kMoveWalk = 2;
inline constexpr int kLockOnStance = 0x12, kStrafeRight = 0x13, kStrafeLeft = 0x14, kStrafeForward = 0x15, kStrafeBack = 0x16;

// Whether chara (CHARA_*) is the member in play.
bool Playing(int chara);

// The way the player faces: CharaMain's yaw.
float PlayerYaw();

// The guard (movechara's BtActStatus): the guarding action (action_on), its hold (action and motion 9), and the mode
// (guard_mode) the hold sets while the guard is up.
inline constexpr int kGuardAction = 6;
inline constexpr int kGuardHold = 9;
inline constexpr int kGuardUp = 5;
bool                 Guarding();

// A monster's hit kind (COLLISION_HIT::kind): a guard-breaking one, which the game lets through all but the guard's hold
// and knocks the player down with, a knockback, which any guard stops and an unguarded one throws the player back
// with, and a light one, which any guard stops and an unguarded one only makes him flinch.
inline constexpr int kGuardBreakingHit = 3;
inline constexpr int kKnockbackHit = 2;
inline constexpr int kLightHit = 4;

// The monster the player is locked on to while it is on the floor and alive, or -1.
int LockedTarget();

// The element selected on the weapon in hand (WEAPON_ELEMENT_FIRE to _HOLY), or -1 with none.
int SelectedElement();

} // namespace expanded
