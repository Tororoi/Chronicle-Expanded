#pragma once

namespace dc::game {
class CCloth;
} // namespace dc::game

namespace expanded {

// The Inferno's wind on Goro's cape as its flame rises (inferno/rising_flame.hpp): the floor gives him none, so his
// cloth is blown kCapeWindSpeed back from where he faces and kCapeWindLift up, swaying kCapeWindSway to either side
// once every kCapeWindSwayFrames, gusting between kCapeWindCalm of it and all of it (a random walk, as a town's wind
// gusts), times how far the flame has risen and times the cloth's own share of a wind (its WINDEFFECT). Times are game
// frames.
inline constexpr float kCapeWindSpeed = 1.0f;
inline constexpr float kCapeWindLift = 0.6f;
inline constexpr float kCapeWindSway = 0.5f;
inline constexpr int   kCapeWindSwayFrames = 50;
inline constexpr float kCapeWindCalm = 0.4f;

// One dungeon step: the gust walked and the sway turned.
void CapeWindStep();
// Hook: the wind a cloth takes this step (CCloth::Step), with Goro's cape's added to it.
void InfernoCapeWind(const CCloth &cloth, float *force);
// The wind a full flame blows now, before a cloth's share (tests).
void CapeWindNow(float *force);
void CapeWindReset();

} // namespace expanded
