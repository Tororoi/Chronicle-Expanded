#pragma once

#include <array>

#include "expanded/enemies/monsters.hpp"

namespace expanded {

// Pocklekul "Pockle Call": with the Pocklekul in Ruby's hand, or Super Steve carrying its sphere in Xiao's, holding the
// guard pose kPockleGuardFrames (the clock runs while R1 stays down in the pose, and starts again once either drops)
// summons a pockle on a quiet floor (no menu, hold or event; else on the first quiet frame of the hold): one of
// kPockleKinds at random, stood kPockleSpawnAhead in front of the summoner in the summon smoke (summon_smoke.hpp,
// kPockleSmokeScale, turned to the summoner's facing), with the game's charge-complete flash on the summoner and a notice
// naming it for kPockleNoticeFrames (once a dungeon line showing then, Steve's say, is gone, or has held it
// kPockleNoticeWait). It is a real pockle on a monster unit of its own (enemies/ally_unit.hpp): its own life, its own
// script and AI, no drops, no ABS.
//   - It appears small and grows in the smoke (kPockleGrowth, a frame each), its colour kPockleTint over the room's light
//     until the smoke's frame kPockleGrownFrame, then fading back to it by the smoke's last frame; the smoke follows it
//     until the smoke is gone.
//   - Its enemy: the monster the lock-on cursor is on (locked or marking) within kPockleLockRange of the summoner, else
//     the one it had while it lives within that, else the living monster nearest the summoner within kPockleNearRange
//     (a monster on the floor, near or not, named and with life left). Its script finds the player at a point
//     kPockleAimDepth inside that monster's hurt sphere nearest it (at the sphere's centre for a smaller one, or once it is
//     that deep), at the monster's height: a big monster's root lies deep in its body.
//   - No enemy: a move of its own (any clip but idle and walk) plays out first, its script finding the player
//     kPockleSettleReach times as far past the summoner as it stands from them (so its next choice is a walk), for
//     kPockleSettleFrames at the most. Then its script waits (resumed each step on a wait, so a hit or its fall still runs
//     its script) and it walks to the point kPockleSideOffset to the summoner's right: farther than kPockleIdleReach, it
//     steps straight there (its walk clip at kPockleWalkRate times its pace, d × kPockleWalkPace held to kPockleMinPace to
//     kPockleMaxPace a frame and no more than a sixth of the way; its run clip while the summoner runs), turning to the
//     point; there, it idles (its idle clip) turned the summoner's way, kPockleIdleLook ahead. Its turns are its unit's own
//     (kPockleTurnRate a frame). An enemy coming starts its script afresh.
//   - Falling, its death plays out as its script has it.
//   - Its fights with the floor's monsters: pocklekul/fights.hpp.
// Guarding again summons nothing while one stands. It goes when it falls, when it stands more than kPockleLeash from the
// summoner, when the weapon leaves the hand (or its sphere Super Steve; the smoke goes too), the summoner changes, an
// event takes the floor, or the floor does. Times are game frames.
inline constexpr int                    kPockleGuardFrames = 60;
inline constexpr std::array<Monster, 8> kPockleKinds = {Monster::SUNDAY, Monster::MONDAY, Monster::TUESDAY, Monster::WEDNESDAY,
                                                        Monster::THURSDAY, Monster::FRIDAY, Monster::SATURDAY, Monster::HELL_POCKLE};
inline constexpr float                  kPockleSpawnAhead = 15.0f;
inline constexpr float                  kPockleSmokeScale = 0.5f;
inline constexpr int                    kPockleNoticeFrames = 150, kPockleNoticeWait = 480;
inline constexpr float                  kPockleTint[3] = {150.0f, 100.0f, 120.0f};
inline constexpr float                  kPockleGrownFrame = 12.0f; // the smoke's
// Its size a frame from the summon: a thirtieth until frame 9, out to a little past its size at 16, its size from 19.
inline constexpr std::array<float, 20> kPockleGrowth = {
    0.033333335f, 0.033333335f, 0.033333335f, 0.033333335f, 0.033333335f, 0.033333335f, 0.033333335f,
    0.033333335f, 0.033333335f, 0.033333335f, 0.16747968f, 0.3284553f, 0.4894309f, 0.65040648f,
    0.81138211f, 0.97235775f, 1.1333333f, 1.0761905f, 1.0190476f, 1.0f};
inline constexpr float kPockleLeash = 300.0f;
inline constexpr float kPockleLockRange = 200.0f, kPockleNearRange = 150.0f;
inline constexpr float kPockleAimDepth = 8.0f;
inline constexpr int   kPockleSettleFrames = 180;
inline constexpr float kPockleSettleReach = 32.0f;
inline constexpr float kPockleSideOffset = 16.0f, kPockleIdleReach = 3.0f, kPockleIdleLook = 10.0f;
inline constexpr float kPockleWalkPace = 0.08f, kPockleMinPace = 0.5f, kPockleMaxPace = 2.4f, kPockleWalkRate = 0.68f;
inline constexpr float kPockleTurnRate = 0.3f;
// Its clips (every pockle's), and the summoner's run.
inline constexpr int kPockleIdleClip = 0, kPockleWalkClip = 1, kPockleRunClip = 16;
inline constexpr int kSummonerRunMotion = 1;

// Whether a pockle stands, and which kind it is (Monster) while one does.
bool    PockleUp();
Monster PockleKind();

// Hooks: the floor's monsters stepped (pause the step's hold): the guard charge, the summon, its enemy and its walk,
// before its unit steps; the shots stepped: its own on the floor's monsters; where a monster's script finds the player
// (_GET_POSITION, _GET_DISTANCE): its enemy's point or its walk's for it, the pockle for a monster it provoked — true
// when one of them sets it; the floor's monsters put back or reloaded: it goes.
void PocklekulStep(int pause);
void PocklekulShotsStep();
bool PocklekulScriptTarget(float *position);
void PocklekulReset();

// Tests stand in for the controller: whether R1 is held (nullptr: the controller's).
using PockleGuardButton = bool (*)();
void SetPocklekulGuardButton(PockleGuardButton held);

} // namespace expanded
