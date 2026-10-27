#pragma once

namespace dc::game {
class CCharacter;
class CSHOT;
class CSHOT_EFFECT;
struct BT_SHOT_EFFECT;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Dragon's Y: locked on, Xiao moves kLockOnSpeed times as fast (her strafes and her guarded steps playing that much
// faster with her), with the Dragon's Y in hand or a slingshot built from it (Divine Beast Title, Angel Shooter, Angel
// Gear), or Super Steve carrying any of their SynthSpheres. With the Dragon's Y itself, or Super Steve carrying its
// SynthSphere, a shot held drawn kDragonShotFrames (the charge-complete flash) fires the Gemron ball of the weapon's
// selected element (the Black Dragon's with none) in place of the pellet, at kDragonShotDamage times its damage, turned
// on the monsters. The weapon's abilities ride on it as on a pellet; the ball's own ailment is left off. It hits a
// monster once, and a monster it hits is thrown away from a point kDragonShotBehind units back down its flight
// (kDragonShotKickSpeed, kDragonShotKickDecay). Times are game frames.
inline constexpr float kLockOnSpeed = 1.3f;
inline constexpr int   kDragonShotFrames = 60;
inline constexpr float kDragonShotDamage = 1.5f;
inline constexpr float kDragonShotBehind = 4.0f;
inline constexpr float kDragonShotKickSpeed = 2.5f;
inline constexpr float kDragonShotKickDecay = 0.1f;
inline constexpr int   kDragonShots = 6;

bool LockOnSpeedActive();
bool DragonShotActive();

// The ball the selected element fires (element as NowWeaponHave->best_elem: -1 none, 0-4 fire to holy).
const BT_SHOT_EFFECT *DragonShot(int element);

// Hooks: MoveChara, locked on (what the stick's move is scaled by); motionDrive, before the player character steps (its
// motion's speed); a dungeon step; a pellet let go into slot; a shot effect's hit planted (CSHOT_EFFECT::Step).
float LockOnMoveScale();
void  LockOnSpeedMotion(CCharacter &chara, int chara_no);
void  DragonShotStep();
void  DragonShotFired(CSHOT &shots, int slot);
void  DragonShotHit(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit);
// Hook: CheckDmg taking a hit on monster: whether it refuses it (a ball's hit on a monster that ball has hit already).
bool DragonShotRefused(int monster, const COLLISION_HIT &hit);

} // namespace expanded
