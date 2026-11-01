#pragma once

namespace dc::game {
class CCharacter;
class CCPoly;
class CFish;
} // namespace dc::game

namespace expanded {

// Fishing fights: a struck fish runs a random way away from the angler, thrashing as it goes as the game's fight has
// it, and now and then switches to a new way on the other side, and the left stick pulls
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

// The bar as a fight starts (empty); the frames of the fight's opening, its first run, straight out from the angler (so
// the way to pull is the gauge's middle, where a stick at rest points), while a miss holds the bar rather than draining
// it; and the frames the bar holds after each switch, for the stick to follow.
inline constexpr float kStartBar = 0.0f;
inline constexpr int   kOpenFrames = 60, kSwitchGrace = 12;

// The frames between a smallest fish's switches (drawn between the two), and how fast it runs.
inline constexpr int   kShortestRun = 180, kLongestRun = 360;
inline constexpr float kRunSpeed = 0.35f;

// Each run heads a random way inside the cone (below), in the half of it across from the last run's, and at least
// kMinSwitch (30 degrees) from the last run's way.
inline constexpr float kMinSwitch = 0.52359878f;

// How much faster than the game's own fight a fought fish turns (its turn step: 0.7 of the game's 0.1 radians a frame,
// about 4 degrees) and swims (its speed step and the run's speed).
inline constexpr float kFightTurnPace = 0.7f, kFightPace = 2.0f;

// Struck, the fish first turns straight out from the angler at kOpenTurnPace of the game's turn step (12 degrees a
// frame), and the fight starts once its heading is within kTurned (1 degree) of that: until then the bar and the runs
// wait.
inline constexpr float kOpenTurnPace = 2.0943951f, kTurned = 0.017453293f;

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

// A run's way (radians from away, inside the cone) after a run that went previous: in the half of the cone across
// from previous's (previous 0 counts as the positive half), no nearer previous than kMinSwitch; roll (0 to 1) across
// what is left of the half, from the cone's middle out to its edge.
float FightRunWay(float previous, float roll);

// position (x, y, z) held across the ground to reach from rod (x, y, z), whatever their heights; whether it was moved.
bool FightLineHold(float *position, const float *rod, float reach);

// The way to pull: against the way the fish swims (its heading turned round), turned into kCone either side of the way
// from the float to the angler, the gauge's middle: the fish's cone about the way from the angler to it, mirrored at
// the float. The float trails the fish along the line, so the way from the fish to it stays near that middle whatever
// the fish does; its heading sweeps the cone as it runs. The way follows kTargetEase of the turn a frame, so the
// fish's thrash steadies a little, and no more than kTargetTurn (6 degrees) a frame, so a switch sweeps the block
// across the gauge rather than jumping it.
inline constexpr float kTargetEase = 0.2f, kTargetTurn = 0.10471976f;

// A stick pulls when it points, as the screen shows the water, within kPullTolerance (10 degrees: the block's green
// middle) of the way to pull; within kHoldTolerance (20 degrees: the block's yellow ends) it holds, the bar standing
// still. Pushed no further than kPullBack (as read), it is at rest and points at the gauge's middle.
inline constexpr float kPullBack = 0.3f, kPullTolerance = 0.17453293f, kHoldTolerance = 0.34906585f;

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

// The way a stick read at (x, y) points as the fight takes it: its own way (FightStickYaw) pushed past kPullBack, and
// at rest (no further) the gauge's middle, middle (a yaw).
float FightStickWay(float camera_yaw, float middle, float x, float y);

// Whether a stick read at (x, y) (as FightStickRead reads it) with the camera looking camera_yaw and the gauge's middle
// middle pulls toward target, its way (FightStickWay) within tolerance of it.
bool FightPulls(float x, float y, float camera_yaw, float middle, float target, float tolerance = kPullTolerance);

// Whether it holds toward target: its way within tolerance (a pull holds too).
bool FightHolds(float x, float y, float camera_yaw, float middle, float target, float tolerance = kHoldTolerance);

// A point of the gauge (x, y, z): radius out from centre across the water, along (radians) from its middle, which
// points middle (a yaw); at centre's height.
void FightGaugePoint(const float *centre, float middle, float along, float radius, float *point);

// A fish's strength by its length, whatever its kind (sizes as CFish::size, a tenth of its centimetres): 0 at
// kSmallFish (30 cm) and under, 1 at kLargeFish (300 cm, the largest fish), 2 at kAriseFish (450 cm, the largest the
// Arise Mardan grows) and over, straight between them. Its runs are 1 - 2/3 of a point as long up to 1 (a third at
// 300 cm: 1 to 2 seconds) and a tenth less for each point past it (7/30 at 450 cm: 0.7 to 1.4 seconds); it
// swims kSpeedPerStrength faster for each point.
inline constexpr float kSmallFish = 3.0f, kLargeFish = 30.0f, kAriseFish = 45.0f, kSpeedPerStrength = 0.25f;

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
// stick read only while a fight goes on (stick_x and stick_y: as read), which turns the angler as the stick does while
// waiting for a bite and, for a fish that got away, sends it off (BattleFish cleared) with its close-up model's read
// dropped; CFish::Step on a fish in FISH_ACTION_BATTLE, which steers the fought fish on its run and returns the pace its
// turn and speed steps take (kFightTurnPace and kFightPace for the fought fish, 1 for any other), and once it has moved
// from start (after its collision against count polys), which draws the fought fish back to the line's reach, sliding
// along a bank in the way, and failing that ends its step no farther from the rod than it started; and the town's 2D
// drawing, the bar and the gauge while fighting.
void      FishingFightStarted(dc::game::CFish &fish);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing, float stick_x, float stick_y);
FightPace FishFightSwim(dc::game::CFish &fish);
void      FishFightHold(dc::game::CFish &fish, const float *start, dc::game::CCPoly *polys, int count);
void      DrawFishingFight();

// The fight's bar (0 to 1), its fish's run's way (radians from the way out from the angler) and the way to pull (a
// yaw, as eased), while one goes on (0 for none).
float FishingFightBar();
float FishingFightRunWay();
float FishingFightTarget();

} // namespace expanded
