#pragma once

#include <numbers>

#include "expanded/weapons/kick_back.hpp"

namespace dc::game {
class CCharacter;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Babel's Spear "Curse of Babel": with Babel's Spear in Ungaga's hand, or Super Steve carrying its sphere, holding the
// guard pose kBabelGuardFrames raises a giant copy of the spear point-up out of the floor (babels_spear/copy.hpp), with the
// game's charge-complete flash on the wielder. The clock runs from the first frame in the pose while R1 stays down; out
// of the pose (a swing, a hit) it starts again, and R1 let go readies the next hold. The weapon in hand takes on the
// copy's grey as it charges (kBabelTint added to its ambient, a share of it as the charge fills).
//   - It rises under the monster locked on to, else the nearest living monster within kBabelNoLockReach across the
//     ground, at the monster's root (its height the floor the copy stands on); with none, kBabelAhead ahead of the
//     wielder, at his height. It is turned to the wielder's facing, grey (kBabelTint added to its ambient).
//   - From its tip kBabelBuried under the floor it rises over kBabelEmergeFrames by an ease (up 2D/T the first frame, each
//     frame's climb 1/T of that less), following its monster across the floor until the tip has struck. The tip strikes the
//     moment it reaches the underside of the monster's lowest body sphere with the body's edge within kBabelStrikeRadius of
//     the copy's axis: one hit of the weapon's attack on the body's largest sphere (at least kBabelStrikeRadius wide),
//     thrown away from the wielder as the Baselard throws (kHeavyKickBack), worn on the weapon as a swing is. A monster
//     that has died, left the copy or stays above its risen tip is not struck.
//   - Risen, it stands its exact height and solid (solid_column.hpp: its form's radius, up to the top of what stands out),
//     turning kBabelSpin a frame. Every spike step of its turn (the first as it stops rising) it catches each living
//     monster in its form's reach: a hit of its form's share of the weapon's attack (at least 1, rounded half to even) on
//     the body's largest sphere (at least kBabelStrikeRadius wide), thrown away from the copy at half the Baselard's throw
//     (kBabelSpikeKick), wearing no weapon HP.
//   - Every monster on the floor in play but the bosses is confused (status/confuse.hpp) from the summon until the copy
//     has gone, those that come into play while it stands too: as it goes, all of them come to their senses (nothing
//     lengthens a confusion; one already confused before it keeps its own). The confused take on kBabelConfusedTint,
//     added to their ambient.
//   - The Dark Genie's small beam in cyan (babels_spear/beam.hpp), at its form's scale, marks the copy's spot: its rise as
//     the copy rises (the copy out by the beam's frame 13), looped while it stands, and its vanish once kBabelStandFrames
//     are up. From the vanish's frame 60 the copy fades out with it over kBabelFadeFrames and is gone as the vanish ends,
//     kBabelLifeFrames from the summon; the weapon's grey, the confused monsters' tint and their stars fade with it.
//   - No other rises until it has gone. It goes when the wielder changes or puts the weapon away, every confusion with
//     it, and with the floor.
//   - Ungaga's: the spear, kBabelSpear: its six spikes catch a monster whose body comes within kBabelSpikeReach of its
//     column, every 60 degrees of its turn, for a sixth of the attack.
//   - Super Steve's: a statue of Super Steve, kBabelStatue: its two hands (the ends of its fork, kBabelHands in its fork
//     bone's frame) catch a monster whose body comes within kBabelHandReach of either, every half turn, for half the attack.
// Times are game frames.
inline constexpr int   kBabelGuardFrames = 300;
inline constexpr float kBabelNoLockReach = 60.0f;
inline constexpr float kBabelAhead = 15.0f;
inline constexpr float kBabelBuried = 4.0f;
inline constexpr int   kBabelEmergeFrames = 30; // the beam's frames 10 to 13
inline constexpr int   kBabelStandFrames = 1200;
inline constexpr int   kBabelVanishFrames = 220; // the beam's vanish, frames 48 to 70
inline constexpr int   kBabelFadeFrames = 100;   // its frames 60 to 70
inline constexpr int   kBabelLifeFrames = kBabelStandFrames + kBabelVanishFrames;
inline constexpr float kBabelStrikeRadius = 6.0f;
inline constexpr float kBabelSpin = 4.0f * std::numbers::pi_v<float> / 180.0f; // 240 degrees a second
inline constexpr float kBabelSpikeReach = 2.0f;
inline constexpr float kBabelHandReach = 6.0f;
inline constexpr float kBabelHands[2][3] = {
    {-0.04f, 0.03f, -0.96f},
    {-0.04f, 0.03f, 0.96f }
};
inline constexpr float    kBabelTint = 50.0f;
inline constexpr float    kBabelConfusedTint[3] = {12.0f, 50.0f, 84.0f};
inline constexpr KickBack kBabelSpikeKick = {kHeavyKickBack.speed / std::numbers::sqrt2_v<float>, kHeavyKickBack.decay};

// A form's size and shape, model units at its own size: its full size, its tip along the way it rises, how much of it
// stands out of the floor once risen, the column's radius, the beam's scale, the frames of its turn between catches, the
// share of the attack a catch hits for, and whether its hands catch (else the column's reach). Babel's Spear c10w10 runs
// -9.6 to 16.0 along its blade, its shaft about 2 wide at 5 times; Super Steve c04w13 upright runs from its handle's end at
// -1.85 to its fork's tips at 2.49, its feet 0.46 from its centre, all of it out but its feet.
struct BabelForm {
    float scale, tip, exposed, block_radius, beam_scale;
    int   catch_frames;
    float catch_share;
    bool  hands;
};

inline constexpr BabelForm kBabelSpear = {5.0f, 16.0f, 10.0f, 8.0f, 1.0f, 15, 1.0f / 6.0f, false};
inline constexpr BabelForm kBabelStatue = {4.0f, 2.49f, 3.94f, 2.0f, 0.7f, 45, 0.5f, true};

// Whether a copy stands (its fade included), its frames since it was raised, the monster it rose under (-1: none),
// whether it has risen, where it stands (x, ground, z), its turn, how solid it is (0 to 1) and the weapon's share of the
// grey.
bool  CurseOfBabelUp();
int   CurseOfBabelAge();
int   CurseOfBabelTarget();
bool  CurseOfBabelRisen();
void  CurseOfBabelSpot(float *spot);
float CurseOfBabelYaw();
float CurseOfBabelAlpha();
float CurseOfBabelWeaponTint();
// Whether a hit is one of its catches (it wears no weapon HP).
bool CurseOfBabelCatch(const COLLISION_HIT &hit);

// Hooks: a step (each dungeon step); the draw, beside the monsters; a character's light (CCharacter::Draw: the weapon in
// hand's grey and the confused monsters' tint).
void CurseOfBabelStep();
void CurseOfBabelDraw();
void CurseOfBabelLight(const CCharacter &character, float *ambient);

// Taken down (the floor changed), for tests too.
void CurseOfBabelReset();

// Tests stand in for the controller: whether R1 is held (nullptr: the controller's).
using BabelGuardButton = bool (*)();
void SetCurseOfBabelGuardButton(BabelGuardButton held);

} // namespace expanded
