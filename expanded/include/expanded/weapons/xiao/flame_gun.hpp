#pragma once

#include "expanded/dungeon/own_sounds.hpp"

namespace expanded {

// Flame gun: Super Steve carrying the Blessing Gun's or the Skunk's SynthSphere throws Osmond's flame while Xiao holds
// her shot drawn (kXiaoHold); letting go fires her pellet as ever. kFlameIgnition frames into the hold the flame lights
// at her pellet's muzzle (eff30) along her pellet's aim and burns while she holds, as BattleActionPlay_Ozumond_F drives
// his (OzumondFire): each puff her weapon's attack (twice it while the weapon is poisoned) and selected element, its
// hits her weapon's flags and anti-monster values, his gun's wear on her slingshot (kIgnitionWear as it lights,
// kBurnWear each kBurnWearInterval frames it burns), and his flame's roar. The Skunk's sphere throws the Skunk's longer
// flame (skunk.hpp). Its hits are hers (FlameOwner). The hold stays her slingshot action, so she moves and her legs walk
// under it (moves_while_firing.hpp, firing_legs.hpp). Times are game frames.
inline constexpr int   kFlameIgnition = 8;
inline constexpr float kIgnitionWear = 1.5f;
inline constexpr float kBurnWear = 2.2f;
inline constexpr int   kBurnWearInterval = 30;

// The flame burns her action gauge as his burns his: kFlameGaugeDrain a frame from the frame it lights (a full gauge in
// 200 frames), the gauge not refilling while it burns. Her shot spent the gauge as it started, as every shot of hers
// does, so the flame burns what has refilled since. Let go before the gauge is out, it refills at her own rate. Run
// out, the shot ends as his flame's does (the game ends her action, so no pellet), the flame fed that frame left to
// die out, and the gauge refills at his rate once his has run out, kSpentGaugeRate in place of her kXiaoGaugeRate,
// before she can shoot again.
inline constexpr float kFlameGaugeDrain = 0.5f;
inline constexpr float kSpentGaugeRate = 0.5f;

// Osmond's flame picture (c05w_h.img of his pack), which the game enters only with him out; entered into
// kFloorCharacterBlock when no copy of it is in.
inline constexpr const char *kOsmondPack = "dun/mainchara/c18a.chr";
inline constexpr const char *kFlameImage = "c05w_h.img";
inline constexpr const char *kFlamePicture = "c05w_h";

// His flame's roar (his voice set's SE_CHARA_ACTION_5, which no other member's set carries), held while the flame is fed
// and let go kRoarLinger frames after it last was, as SndSeSeqPlayStop holds his; at the level his own sounds at.
inline constexpr OwnSound kFlameRoar = {.path = "sound/voice/voice5.snd", .bank = "ozmd_s.hd", .program = 28, .note = 24, .frames = 0, .volume = 103};
inline constexpr int      kRoarLinger = 5;

// Whether Super Steve in the active character's hand carries a flame gun's sphere.
bool FlameGunSphere();

// Whether the flame burns from her slingshot now, and the frames she has held the shot drawn (for tests).
bool FlameGunLit();
int  FlameGunHeld();

// Hook: BattleActionPlay_Jinn, each frame of her shot (XiaoShotPlaying), aimed whether in first person: true when the
// flame ran her gauge out and her shot ends.
bool FlameGunShot(int aimed);

// Hook: motionDrive's refill of the action gauge (ActionGaugeGain), gain the game's for Xiao.
float FlameGunGaugeGain(float gain);

// Hook: the top of a dungeon frame, menus too (DungeonFrameBegins): the roar let go once the flame is no longer fed, and
// a flame whose shot something else ended (a hit, a change of member) left to die out.
void FlameGunFrame();

// Hook: CSHOT_FIREBAR::Step, the member a puff's hit is: Xiao while she is out, else owner (the game's, Osmond).
int FlameOwner(int owner);

} // namespace expanded
