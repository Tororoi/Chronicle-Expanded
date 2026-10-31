#pragma once

namespace dc::game {
class CCharacter;
class CCPoly;
class CFish;
} // namespace dc::game

namespace expanded {

// Fishing fights: a struck fish runs a random way away from the angler, thrashing as it goes as the game's fight has
// it, and now and then switches to a new way, likelier to the other side than the same, and the left stick pulls
// against it: pointed, as the screen shows the water (up
// into it, down toward the camera), against the way the fish swims. A pull fills the tension bar at the top of the
// screen, a near miss holds it, anything else drains it; a full bar lands the fish the game's way, an empty
// one lets it get away ("Bait ran out.": the bait went as it was struck). A gauge lies on the water at the float: the arc of the
// ways a pull can go, the way to pull a block on it and the stick's way marked on it. A bigger fish switches sooner,
// switches sooner and swims faster, by its length whatever its kind. Off (the Fishing Fights option, config.json's
// expanded.fishing_fights, on by default), a struck fish is landed as in the original game; debug mode always pulls, as
// it always strikes.
inline constexpr const char *kFishingFights = "fishing_fights";

bool FishingFightsOn();

// The bar's frames of pulling to fill from empty, and of missing to drain from full, for every fish.
inline constexpr int kFillFrames = 180, kDrainFrames = 300;

// The bar as a fight starts; the frames it holds as a fight starts and after each switch, for the stick to follow.
inline constexpr float kStartBar = 0.3f;
inline constexpr int   kStartGrace = 30, kSwitchGrace = 12;

// The frames between a smallest fish's switches (drawn between the two), and how fast it runs.
inline constexpr int   kShortestRun = 180, kLongestRun = 360;
inline constexpr float kRunSpeed = 0.35f;

// Each run heads a random way inside the cone (below), kOppositeOdds times as likely in the half of it away from the last
// run's as in the same half, and at least kMinSwitch (30 degrees) from the last run's way.
inline constexpr float kOppositeOdds = 2.0f, kMinSwitch = 0.52359878f;

// How much faster than the game's own fight a fought fish turns (its turn step) and swims (its speed step and the run's
// speed).
inline constexpr float kFightTurnPace = 1.0f, kFightPace = 2.0f;

struct FightPace {
    float turn = 1.0f, speed = 1.0f;
};

// The fish swims away from the angler: its heading stays within kCone (60 degrees) either side of the way from him to
// it. The line holds it across the ground to kLineReach times the line's length from the rod tip, whatever their
// heights (twice the line's length, for the fight to have room), so at that reach swimming away slides it along it.
inline constexpr float kCone = 1.0471976f, kLineReach = 2.0f;

// The line's length from the rod tip to the hook, as it is now: its segments' lengths (distp as the spot sets it and the
// cast paid it out, the spot's hang under the float).
float FightLineLength();

// yaw turned into the cone either side of away (the way from the angler to the fish): itself inside it, else its
// nearer edge.
float FightConeYaw(float away, float yaw);

// A run's way (radians from away, inside the cone) after a run that went previous: with half_roll (0 to 1) under
// kOppositeOdds / (kOppositeOdds + 1), in the half of the cone away from previous's (previous 0 counts as the
// positive half), else in the same half, and no nearer previous than kMinSwitch (the other half where the one chosen
// has no room); within_roll (0 to 1) across what is left of the half, from the cone's middle out to its edge.
float FightRunWay(float previous, float half_roll, float within_roll);

// position (x, y, z) held across the ground to reach from rod (x, y, z), whatever their heights; whether it was moved.
bool FightLineHold(float *position, const float *rod, float reach);

// The way to pull: against the way the fish swims (its heading turned round), turned into kCone either side of the way
// from the float to the angler, the gauge's middle: the fish's cone about the way from the angler to it, mirrored at
// the float. The float trails the fish along the line, so the way from the fish to it stays near that middle whatever
// the fish does; its heading sweeps the cone as it runs. The way follows kTargetEase of the turn a frame, so the
// fish's thrash steadies a little, and no more than kTargetTurn (6 degrees) a frame, so a switch sweeps the block
// across the gauge rather than jumping it.
inline constexpr float kTargetEase = 0.2f, kTargetTurn = 0.10471976f;

// A stick pulls when it is pushed past kPullBack (as read) and points, as the screen shows the water, within
// kPullTolerance (15 degrees: the block's green middle) of the way to pull; pushed as far within kHoldTolerance (25
// degrees: the block's yellow ends) it holds, the bar standing still. Its way is kept while it rests.
inline constexpr float kPullBack = 0.3f, kPullTolerance = 0.26179939f, kHoldTolerance = 0.43633231f;

// The fight reads the left stick with its dead zone measured from the centre: within kStickDead of it (as far out as
// the game's own dead band) the stick reads nothing, and past it, it reads its own way, its push stretched over 0 to 1.
// The game's reads (GetLXf, GetLYf) zero each axis on its own, which reads a push within about 23 degrees of an axis
// as on it.
inline constexpr float kStickDead = 49.0f / 127.0f;

struct FightStick {
    float x = 0.0f, y = 0.0f;
};

// A stick standing at (x, y) from its centre (-1 to 1 each, right and down positive), as the fight reads it.
FightStick FightStickRead(float x, float y);

// The gauge: an arc flat on the water, its middle kGaugeRadius out from the float on the angler's side and kGaugeBand
// wide, spanning kCone either side; it follows the float kGaugeEase of the way a frame, so it holds still while the
// float bobs. The mark for the stick's way reaches kMarkOverhang past the band's edges.
inline constexpr float kGaugeRadius = 7.0f, kGaugeBand = 2.5f, kGaugeEase = 0.1f, kMarkOverhang = 0.75f;

// The way (a yaw, as atan2(x, z)) across the water that a stick read at (x, y) (as FightStickRead reads it: right
// and down positive) points with the camera looking camera_yaw: up into the screen, right to the screen's right,
// down toward the camera.
float FightStickYaw(float camera_yaw, float x, float y);

// The way to pull for a fish swimming heading (a yaw): turned round, inside the cone about middle (the way from the
// float to the angler).
float FightTargetYaw(float middle, float heading);

// target followed toward yaw a frame.
float FightEaseTarget(float target, float yaw);

// Whether a stick read at (x, y) (as GetLXf and GetLYf) with the camera looking camera_yaw pulls toward target.
bool FightPulls(float x, float y, float camera_yaw, float target);

// Whether a stick read at (x, y) with the camera looking camera_yaw holds toward target: pushed as far, within
// kHoldTolerance (a pull holds too).
bool FightHolds(float x, float y, float camera_yaw, float target);

// A point of the gauge (x, y, z): radius out from centre across the water, along (radians) from its middle, which
// points middle (a yaw); at centre's height.
void FightGaugePoint(const float *centre, float middle, float along, float radius, float *point);

// A fish's strength by its length, whatever its kind (sizes as CFish::size, a tenth of its centimetres): 0 at
// kSmallFish (30 cm) and under, 1 at kLargeFish (300 cm, the largest fish), 2 at kAriseFish (600 cm, the largest the
// Arise Mardan grows) and over, straight between them. Its runs are 1 - 2/3 of a point as long up to 1 (a third at
// 300 cm: 1 to 2 seconds) and a sixth less for each point past it (a sixth at 600 cm: half a second to a second); it
// swims kSpeedPerStrength faster for each point.
inline constexpr float kSmallFish = 3.0f, kLargeFish = 30.0f, kAriseFish = 60.0f, kSpeedPerStrength = 0.25f;

float FightStrength(float size);

// The bar's fill a frame of pulling and drain a frame of missing; what a strength makes of a fight: a run's frames
// (roll, 0 to 1, between the shortest and longest) and its speed (kRunSpeed at kFightPace).
float FightFill();
float FightDrain();
int   FightRunFrames(float strength, float roll);
float FightRunSpeed(float strength);

// What EdMoveChara's ED_FISHING_BATTLE does this frame: the game's own fight (none started), the fight going on, the
// fish landed (the bar full: the game's landing once the fish's close-up model is read), or got away.
enum class FightStep {
    kRetail,
    kFighting,
    kLanded,
    kGotAway
};

// Hooks: FishingBattleFish having struck fish; EdMoveChara's ED_FISHING_BATTLE with the angler turned facing, the left
// stick read only while a fight goes on (stick_x and stick_y: as read), which keeps the angler turned as he stands and
// leans his rod with the stick (fishing_lean.hpp) and, for a fish that got away, sends it off (BattleFish cleared) with
// its close-up model's read dropped; CFish::Step on a fish in FISH_ACTION_BATTLE, which steers the fought fish on its
// run and returns the pace its turn and speed steps take (kFightTurnPace and kFightPace for the fought fish, 1 for any
// other), and once it has moved from start (after its collision against count polys), which draws the fought fish back
// to the line's reach, sliding along a bank in the way, and failing that ends its step no farther from the rod than it
// started; and the town's 2D drawing, the bar and the gauge while fighting.
void      FishingFightStarted(dc::game::CFish &fish);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing, float stick_x, float stick_y);
FightPace FishFightSwim(dc::game::CFish &fish);
void      FishFightHold(dc::game::CFish &fish, const float *start, dc::game::CCPoly *polys, int count);
void      DrawFishingFight();

// The fight's bar (0 to 1), its fish's run's way (radians from the way out from the angler) and the way to pull (a yaw,
// as eased), while one goes on (0 for none).
float FishingFightBar();
float FishingFightRunWay();
float FishingFightTarget();

} // namespace expanded
