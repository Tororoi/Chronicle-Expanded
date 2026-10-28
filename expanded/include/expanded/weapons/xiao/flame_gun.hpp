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

// Hook: BattleActionPlay_Jinn, each frame of her shot (XiaoShotPlaying), aimed whether in first person.
void FlameGunShot(int aimed);

// Hook: the top of a dungeon frame, menus too (DungeonFrameBegins): the roar let go once the flame is no longer fed.
void FlameGunFrame();

// Hook: CSHOT_FIREBAR::Step, the member a puff's hit is: Xiao while she is out, else owner (the game's, Osmond).
int FlameOwner(int owner);

} // namespace expanded
