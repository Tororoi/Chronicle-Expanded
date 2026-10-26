#pragma once

#include <numbers>

#include "expanded/weapons/kick_back.hpp"

class CMonstorUnit;
struct COLLISION_HIT;

namespace expanded {

// Cactus "Absorb": with the Cactus in Ungaga's hand, or Super Steve carrying its SynthSphere, every hit of the wielder's
// that takes life off a monster restores the wielder's thirst by the life it took over kAbsorbDamagePerWater (100 damage
// = 10 = one drop on the gauge), up to the gauge's most; nothing once the gauge is full. The dry monsters, the rock, metal
// and undead kinds (CactusDryKind), give nothing.
inline constexpr float kAbsorbDamagePerWater = 10.0f;

bool AbsorbActive();
// Whether a monster kind (MonstorTable) gives no water.
bool CactusDryKind(int kind);

// Cactus "Desert Bloom": with the Cactus in Ungaga's hand, or Super Steve carrying its sphere, holding the guard pose
// kBloomGuardFrames (the clock runs from the first frame in the pose while R1 stays down, and starts again once it is let
// go) raises a giant copy of the cactus kBloomAhead ahead of the wielder, on the floor there: from its tip kBloomBuried
// under the floor it rises in a poof of smoke (summon_smoke.hpp, kBloomSmokeScale, turned to the wielder's facing), the
// game's charge-complete flash on the wielder. It comes out at kBloomSpawnShare of its size by the smoke's frame
// kBloomEmergedFrame (an ease, fast off the mark and slow to settle), grows evenly to kBloomPeakShare of it by frame
// kBloomPeakFrame (held for at least a frame) and settles to its size by kBloomRisenFrame, the same part of it out of the
// floor at every size. Risen, it stands solid (solid_column.hpp: its column's radius, up to the top of what stands out)
// and pricks every live monster whose body comes within kBloomHitReach of the column, once every kBloomHitFrames: a hit
// of kBloomHitShare of the weapon's attack (at least 1, rounded half to even) on the body's largest sphere, at least
// kBloomHitRadius wide, throwing it away from the cactus (kBloomKick, half the Baselard's throw) and wearing no weapon
// HP. After kBloomStandFrames it fades over kBloomFadeFrames and is gone; it casts a round shadow (ground_shadow.hpp)
// until it starts to fade. Holding the guard again while one stands moves it: the standing one goes, a new one rises.
// It goes when the wielder changes or puts the weapon away, and with the floor.
//   - Ungaga's: his Cactus, point-up, kBloomCactus.
//   - Super Steve's: Queens' two trees (data/prop_models.hpp), as a wall for a ranged fighter to shoot from behind,
//     kBloomTrees: turned a quarter from her facing, they hurt nothing and cast no shadow.
// Times are game frames.
inline constexpr int      kBloomGuardFrames = 15;
inline constexpr float    kBloomAhead = 10.0f;
inline constexpr float    kBloomBuried = 4.0f;
inline constexpr float    kBloomSmokeScale = 0.5f;
inline constexpr float    kBloomSpawnShare = 0.1f / 3.0f, kBloomPeakShare = 3.4f / 3.0f;
inline constexpr float    kBloomEmergedFrame = 6.5f, kBloomPeakFrame = 10.6f, kBloomRisenFrame = 12.0f; // the smoke's
inline constexpr int      kBloomStandFrames = 600, kBloomFadeFrames = 30;
inline constexpr float    kBloomHitReach = 2.0f;
inline constexpr float    kBloomHitShare = 1.0f / 6.0f;
inline constexpr int      kBloomHitFrames = 15;
inline constexpr float    kBloomHitRadius = 6.0f;
inline constexpr KickBack kBloomKick = {kHeavyKickBack.speed / std::numbers::sqrt2_v<float>, kHeavyKickBack.decay};

// A form's size and shape, model units at its own size: its full size, its top along the way it rises, how much of that
// stands out of the floor once risen, the column's radius at full size, its centre in its own x and z (set on the spot),
// its turn from the wielder's facing and its shadow's radius. The cactus c10w13 runs -9.7 to 11.7 along its blade (the
// tip, dcol0, at 11.68), 2.5 across (7.5 with its arms at 3 times), its body 1.8; the trees are 74.3 tall, all of it out,
// centred on their grass square's centre (-6, -1), their trunks 21.5 apart (about 15 at 0.7 times).
struct BloomShape {
    float scale, top, exposed, block_radius, base_x, base_z, turn, shadow_radius;
};

inline constexpr BloomShape kBloomCactus = {3.0f, 11.7f, 6.8f, 6.0f, 0.0f, 0.0f, 0.0f, 1.8f};
inline constexpr BloomShape kBloomTrees = {0.7f, 74.3f, 74.3f, 9.5f, -6.0f, -1.0f, std::numbers::pi_v<float> / 2.0f, 0.0f};

bool DesertBloomActive();
// Whether a copy stands (fading included), its frames since it was raised, its size now, where it stands (its spot: x,
// ground, z) and how solid it is (0 to 1).
bool  DesertBloomUp();
int   DesertBloomAge();
float DesertBloomScale();
void  DesertBloomSpot(float *spot);
float DesertBloomAlpha();
// Whether a hit is one of the cactus's pricks (it wears no weapon HP).
bool DesertBloomPrick(const COLLISION_HIT &hit);

// Hooks: a monster's life taken by a hit (CheckDmg), amount the damage; a step (once a frame the monsters step
// unpaused); the draw, after the units' shadows.
void CactusMonsterDamaged(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);
void DesertBloomStep();
void DesertBloomDraw();

// Taken down (the floor changed), for tests too.
void DesertBloomReset();

// Tests stand in for the controller: whether R1 is held (nullptr: the controller's).
using GuardButtonHeld = bool (*)();
void SetDesertBloomGuardButton(GuardButtonHeld held);

} // namespace expanded
