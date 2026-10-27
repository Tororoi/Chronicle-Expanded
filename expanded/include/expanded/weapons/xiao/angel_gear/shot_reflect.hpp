#pragma once

#include "shot_utils.hpp"

namespace dc::game {
class CSHOT_EFFECT;
} // namespace dc::game

namespace expanded::angel_gear {

// The Guardian Reflector's catch and return, a frame at a time:
//   - Claim: a monster's shot in flight within kClaimRadius of Xiao, closing on her, with the flight left to reach the pouch
//     (a shot that flies for good never is), is claimed: held as a shot that has just landed is held (it hurts no one), what
//     it is kept (its kind, speed, damage).
//   - Catch: the claim the slingshot faces is steered into the pouch from kHomingRange (kHomingGain of the way a frame) and
//     caught there, as is one the game stops at the pouch; any other near her feet, or stopped anywhere else, vanishes into
//     her with a dull white flash. Either waits to be fired back (kPendingMax at most). One that ran out of flight, or ended
//     away from both, is lost. While the shield is solid a shot meeting "the player" meets the pouch at its catch pose.
//   - Return: from the slingshot's muzzle at the nearest monster with life (its lock-on point; the arrival line reversed with
//     none), clear of her, kReturnBoost times as fast, turned along its flight, for kReturnFrames. It is tested as Xiao's
//     pellets are, against the monsters' hurt spheres, then walls, never her. The monster whose sphere it meets takes one hit
//     planted there, reaching its flying radius + kHitMargin, of the weapon's attack times (the dungeon's number + 1) /
//     kTierDivisor (halves rounded to even), in the shot's element when it has one and no ailment, through the monster's
//     defence and resistance as Xiao's own pellets are, and the shot's freeze, poison or slow as the monster's own hit from it
//     would give them; the shot's flight ends in its burst.
// With the shield down every claim is let go to land and the waiting ones dropped. Distances are units, times frames.
inline constexpr float kClaimRadius = 100.0f;
inline constexpr float kCaptureRadius = 7.0f;    // the pouch's catch, and twice the shot's speed
inline constexpr float kAbsorbNear = 8.0f;       // her body's, and three times the shot's speed
inline constexpr float kEngineCatchNear = 20.0f; // a claim the game ended this near the pouch was caught there
inline constexpr float kHomingRange = 40.0f;
inline constexpr float kHomingGain = 0.1338f; // the mod's 0.35 each 50 ms, a frame at a time
inline constexpr int   kPendingMax = 6;
inline constexpr float kReturnBoost = 1.6f;
inline constexpr int   kReturnFrames = 240;
inline constexpr float kClearLead = 4.0f, kPastHer = 10.0f; // how far down the aim it leaves from: clear, or past her when it runs back through her
inline constexpr float kTierDivisor = 14.0f;
inline constexpr float kHitMargin = 4.0f;
inline constexpr float kAimLift = 8.0f; // the aim with no lock-on point: this far over its feet
inline constexpr int   kHitLife = 1;
inline constexpr float kAbsorbFlash = 90.0f, kAbsorbFlashFrames = 15.0f;
inline constexpr char  kHeld = 0x7F; // a claim's hit hold, renewed every frame

// The claims: closing shots taken.
void ClaimShots(const float *her);
// Each claim kept harmless and ended in the pouch (when solid) or in her (her feet at her), waiting to be fired back;
// shield_up false lets every claim go and drops the waiting ones.
void SustainClaims(bool shield_up, bool solid, const float *her, const float *pouch);
// The world bearing the slingshot should face: while firing or with a shot waiting and none coming, the target; else the
// nearest claim (which it then steers into the pouch); else the nearest monster; else her facing. False holds it as it is.
bool ChooseBearing(const float *her, float yaw, bool firing, float *bearing);
// The first waiting shot fired back from muzzle; false when it must wait for a free shot of its kind.
bool FireReturn(const float *her, const float *muzzle);
// The returned shots in flight: each kept from landing its own hit.
void TrackReturns();
int  ClaimCount();
int  PendingCount();
void DropPending();
// The aim point of the nearest monster with life to from (flat distance); false with none.
bool NearestAim(const float *from, float *aim);
// Every claim, waiting and flying shot forgotten (a claim's hold runs out on its own).
void ReflectReset();

// Hook: a shot effect's collision test (CSHOT_EFFECT::Step); a returned shot's is its own and lands its hit. Whether a
// returned shot's test is under way (its player point is moved out of reach).
SHOT_COLLISION_RESULT ReflectCollision(CSHOT_EFFECT &effect, int slot, float *hit_position, float *position);
bool                  ReturnTesting();

} // namespace expanded::angel_gear
