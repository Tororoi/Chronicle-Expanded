#pragma once

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded {

// Shamshir "Swift Strikes", inherited by the Dusack, 7 Branch Sword, Atlamillia Sword and Chronicle Sword.
// - In hand: Toan's five combo swings play kSwiftStrikesFactor times as fast; charge attacks keep their speed.
// - Super Steve carrying one of their spheres, or a Partisan's: Xiao's draw plays kSwiftDrawFactor times as fast and
//   her shot at kSwiftShotSpeed, under 1 so the pellet's one-frame release window is never stepped over.
inline constexpr float kSwiftStrikesFactor = 4.0f / 3.0f;
inline constexpr float kSwiftDrawFactor = 1.6f;
inline constexpr float kSwiftShotSpeed = 0.95f;

bool SwiftStrikesSword();
bool SwiftStrikesSphere();

// Hook: motionDrive, before the player character steps: the speed its motion plays at.
void SwiftStrikesMotion(CCharacter &chara, int chara_no);

} // namespace expanded
