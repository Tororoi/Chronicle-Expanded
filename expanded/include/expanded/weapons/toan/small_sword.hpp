#pragma once

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded {

// Small Sword "Quick Draw", inherited by Tsukikage and Heaven's Cloud.
// - In hand: the opening combo swing skips its wind-up.
// - Super Steve carrying one of their spheres: the shot fires the instant the button is let go, and the action gauge
//   refills as at speed kQuickDrawShotSpeed.
inline constexpr int kQuickDrawShotSpeed = 350;

bool QuickDrawSword();
bool QuickDrawSphere();

// Hook: ToanKey_Play, before it reads the motion time: past the opening swing's step-in, jumps to just before its
// trail and hit.
void QuickDrawSwing(CCharacter &chara);

// Hook: BattleActionPlay_Jinn, before it reads the motion time: a draw or hold let go jumps to the shot.
void QuickDrawShot(CCharacter &chara, int aimed);

// Hook: motionDrive's action gauge refill: the speed it refills at.
int QuickDrawGaugeSpeed(int speed);

} // namespace expanded
