#pragma once

#include <numbers>

namespace dc::game {
class CRandomItem;
} // namespace dc::game

namespace expanded {

// The Star Breaker's shooting stars (star_breaker.hpp sends them). A star is the event sparkle (event_sparkle.hpp)
// falling onto walkable floor in front of the player and coming to lie there as a dropped item of the game's own
// (CRandomItem), an empty SynthSphere: the hop a drop makes as it appears is its one bounce, the game's pickup takes
// it (its radius and line, a full attachment bag leaving it lying), and the floor's end takes it as it takes any drop.
// The sparkle twinkles over it as over a town's event point until it is taken.

// Where it lands: kStarNearest to kStarFarthest in front of the player and at most kStarSideways either side of the
// way he faces, drawn again (up to kStarDraws a step, then again the next step) until it is floor he can walk to: floor
// under it within kStarStep of his ground (FloorUnder), and every cell on the way there walkable for a body of
// kStarClearance (NavigateClear: a part's tile of the floor's grid, floor there, no wall within the clearance).
inline constexpr float kStarNearest = 15.0f;
inline constexpr float kStarFarthest = 150.0f;
inline constexpr float kStarSideways = 50.0f;
inline constexpr int   kStarDraws = 64;
inline constexpr float kStarStep = 30.0f;
inline constexpr float kStarClearance = 6.0f;

// The fall: a straight line at kStarFallAngle to the floor, from kStarFallHeight above the spot and across the player's
// view (from his left or his right), kStarFallSpeed a frame.
inline constexpr float kStarFallAngle = std::numbers::pi_v<float> / 3.0f;
inline constexpr float kStarFallHeight = 80.0f;
inline constexpr float kStarFallSpeed = 3.5f;

// How it looks: the falling sparkle kStarHeadSize across with kStarTrail fading copies kStarTrailGap apart behind it;
// lying, the town's twinkle kStarTwinkleSize times as large, kStarTwinkleLift over the drop's middle.
inline constexpr float kStarHeadSize = 12.0f;
inline constexpr int   kStarTrail = 8;
inline constexpr float kStarTrailGap = 2.0f;
inline constexpr float kStarTwinkleSize = 3.0f;
inline constexpr float kStarTwinkleLift = 2.0f;

// The line shown as a star comes down.
inline constexpr int kStarMessageFrames = 240;

// Sends a star: it falls once a spot is found.
void SendShootingStar();

// One draw of a landing spot for a player standing at player (x, y, z) facing yaw: true with spot (x, the floor's height,
// z) when it is walkable floor. sideways and ahead are the draw, each 0-1 across its range.
bool ShootingStarSpot(const float *player, float yaw, float ahead, float sideways, float *spot);

// The stars, for tests: how many wait for a spot, fall and lie on a floor's drops; a falling one's place and the way it
// falls; a lying one's drop.
int  StarsWaiting();
int  StarsFalling();
int  StarsLying();
bool FallingStar(int star, float *at, float *direction);
bool LyingStar(int star, const CRandomItem **items, int *slot);

// Hooks: a dungeon step, as the floor's drops step (DungeonShotsStep); its additive draw (DungeonShotsDrawn); a dungeon
// visit begins (every star gone).
void ShootingStarsStep();
void ShootingStarsDraw();
void ShootingStarsVisitBegins();

} // namespace expanded
