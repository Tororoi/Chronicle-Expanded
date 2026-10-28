#pragma once

#include <numbers>

namespace dc::game {
class CRandomItem;
} // namespace dc::game

namespace expanded {

// The Star Breaker's shooting stars (star_breaker.hpp sends them). A star is the event sparkle (event_sparkle.hpp)
// falling onto walkable floor in front of the player, bouncing once onward the way it fell, and coming to rest there as a
// dropped item of the game's own (CRandomItem) holding an empty SynthSphere: the game's pickup takes it (its radius and
// line, a full attachment bag leaving it lying, its corner-map mark), and the floor's end takes it as it takes any drop.
// Only the sparkle shows it, twinkling as over a town's event point: the drop's bag is not drawn (StarDropHidden).

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

// The bounce: off the floor it keeps kStarBounceAcross of its speed along the floor and kStarBounceUp of its speed down,
// turned up, and comes down again under the gravity of the game's drop hop (CRandomItem: 8 high over 30 frames, as a
// parabola), where it rests. Where that would leave walkable floor or meet a wall, it falls from the other side, and
// failing that it carries kStarBounceShorter of the way, then none (straight up and down).
inline constexpr float kStarBounceAcross = 0.5f;
inline constexpr float kStarBounceUp = 1.0f / 3.0f;
inline constexpr float kItemHopHeight = 8.0f;
inline constexpr float kItemHopFrames = 30.0f;
inline constexpr float kStarGravity = 8.0f * kItemHopHeight / (kItemHopFrames * kItemHopFrames);
inline constexpr float kStarBounceShorter = 0.5f;

// How it looks: the sparkle kStarHeadSize across with kStarTrail fading copies behind it, each where it was kStarTrailGap
// back along its fall, each sitting on its path; at rest the trail runs on into the head where the bounce left it, and once
// it has, the head shines: its size swells and eases by kStarShineDepth of itself once every kStarShineFrames.
inline constexpr float kStarHeadSize = 12.0f;
inline constexpr float kStarShineDepth = 0.15f;
inline constexpr float kStarShineFrames = 60.0f;
inline constexpr int   kStarTrail = 8;
inline constexpr float kStarTrailGap = 2.0f;

// The line shown as a star comes down.
inline constexpr int kStarMessageFrames = 240;

// A star's way down: from start along the 60-degree line to touch, then the bounce to rest. side is the way it travels
// across the player's view as he faced when it was sent (1 toward +x for a player facing +z, his left; -1 his right),
// bounce the share of the full bounce's length it carries (1, kStarBounceShorter or 0).
struct StarPath {
    float start[3] = {};
    float touch[3] = {};
    float rest[3] = {};
    int   side = 1;
    float bounce = 1.0f;

    // Frames from start to touch, and from touch to rest.
    float FallFrames() const;
    float BounceFrames() const;

    // Where it is frames after it started: on the line, in the bounce, or at rest.
    void At(float frames, float *at) const;
};

// The full bounce's length along the floor.
float StarBounceLength();

// Sends a star: it falls once a spot is found.
void SendShootingStar();

// One draw of a landing spot for a player standing at player (x, y, z) facing yaw: true with spot (x, the floor's height,
// z) when it is walkable floor. sideways and ahead are the draw, each 0-1 across its range.
bool ShootingStarSpot(const float *player, float yaw, float ahead, float sideways, float *spot);

// The same draw, with the star's way down when it lands there coming from side (1 or -1, tried first): true with path
// when the spot is walkable floor and a bounce from it ends on walkable floor.
bool ShootingStarPath(const float *player, float yaw, float ahead, float sideways, int side, StarPath *path);

// The stars, for tests: how many wait for a spot, are in the air (falling or bouncing) and lie on a floor's drops; one in
// the air's way and the frames it has come; a lying one's drop.
int  StarsWaiting();
int  StarsFalling();
int  StarsLying();
bool FallingStar(int star, StarPath *path, float *frames);
bool LyingStar(int star, const CRandomItem **items, int *slot);

// Whether a drop of the floor's is a lying star's, whose bag is not drawn (CRandomItem::Draw).
bool StarDropHidden(const CRandomItem &items, int slot);

// Hooks: a dungeon step, as the floor's drops step (DungeonShotsStep); its additive draw (DungeonShotsDrawn); a dungeon
// visit begins (every star gone).
void ShootingStarsStep();
void ShootingStarsDraw();
void ShootingStarsVisitBegins();

} // namespace expanded
