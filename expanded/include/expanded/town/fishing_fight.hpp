#pragma once

namespace dc::game {
class CCharacter;
class CCPoly;
class CFish;
struct FishInfo;
} // namespace dc::game

namespace expanded {

// Fishing fights: a struck fish runs to the left or right of where it was hooked, as the angler sees it (square to the
// way from him to it: the screen's left and right with the camera behind him), thrashing as it goes as the game's fight
// has it, switching sides now and then, and the left stick pulls against it: pointed, as the screen shows the water (up
// into it, down toward the camera), against the way the fish swims. A pull fills the tension bar at the top of the
// screen, a near miss holds it, anything else drains it kDrain times as fast; a full bar lands the fish the game's way, an empty one lets it
// get away ("Bait ran out.": the bait went as it was struck). A gauge lies on the water at the float: the arc of the
// ways a pull can go, the way to pull marked on it and the stick's way a block on it. A bigger fish switches sooner,
// swims faster and takes longer to land, a Garayan most. Off (the Fishing Fights option, config.json's
// expanded.fishing_fights, on by default), a struck fish is landed as in the original game; debug mode always pulls, as
// it always strikes.
inline constexpr const char *kFishingFights = "fishing_fights";

bool FishingFightsOn();

// The bar's frames of pulling to fill from empty for the weakest fish, and how much faster anything else drains it.
inline constexpr int   kFillFrames = 240;
inline constexpr float kDrain = 3.0f;

// The bar as a fight starts; the frames it holds as a fight starts and after each switch, for the stick to follow.
inline constexpr float kStartBar = 1.0f / 3.0f;
inline constexpr int   kStartGrace = 30, kSwitchGrace = 12;

// The frames between a weakest fish's switches (drawn between the two), how far to each side of where it was hooked
// it runs, and how fast.
inline constexpr int   kShortestRun = 60, kLongestRun = 120;
inline constexpr float kRunWidth = 20.0f, kRunSpeed = 0.35f;

// How much faster than the game's own fight a fought fish goes: its heading swings (the turn and speed steps) and it
// swims, the run's speed and its slowest at a run's end.
inline constexpr float kFightPace = 2.0f;

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

// position (x, y, z) held across the ground to reach from rod (x, y, z), whatever their heights; whether it was moved.
bool FightLineHold(float *position, const float *rod, float reach);

// The way to pull: against the way the fish swims (its heading turned round), turned into kCone either side of the way
// from the float to the angler, the gauge's middle: the fish's cone about the way from the angler to it, mirrored at
// the float. The float trails the fish along the line, so the way from the fish to it stays near that middle whatever
// the fish does; its heading sweeps the cone as it runs. The way follows kTargetEase of the turn a frame, so the
// fish's thrash steadies a little, and no more than kTargetTurn (3 degrees) a frame, so a switch sweeps the mark
// across the gauge rather than jumping it.
inline constexpr float kTargetEase = 0.2f, kTargetTurn = 0.05235988f;

// A stick pulls when it is pushed past kPullBack (as read) and points, as the screen shows the water, within
// kPullTolerance (10 degrees: the block's green middle) of the way to pull; pushed as far within kHoldTolerance (20
// degrees: the block's yellow ends) it holds, the bar standing still. Its way is kept while it rests.
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
// float bobs. The mark for the way to pull reaches kMarkOverhang past the band's edges.
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

// A fish's strength, 0 to 2: how far it is from its kind's usual size to its largest (none under it), and 1 more for a
// Garayan. Each point of it makes the bar's fill kFillPerStrength more frames, its runs kRunsPerStrength shorter and its
// speed kSpeedPerStrength faster.
inline constexpr float kFillPerStrength = 0.25f, kRunsPerStrength = 0.15f, kSpeedPerStrength = 0.25f;

float FightStrength(int kind, float size, const dc::game::FishInfo &info);

// What a strength makes of a fight: the bar's fill a frame of pulling, a run's frames (roll, 0 to 1, between the
// shortest and longest) and its speed (kRunSpeed at kFightPace).
float FightFill(float strength);
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
// run and returns the pace its turn and speed steps take (kFightPace for the fought fish, 1 for any other), and once
// it has moved from start (after its collision against count polys), which draws the fought fish back to the line's
// reach, sliding along a bank in the way, and failing that ends its step no farther from the rod than it started; and
// the town's 2D drawing, the bar and the gauge while fighting.
void      FishingFightStarted(dc::game::CFish &fish);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing, float stick_x, float stick_y);
float     FishFightSwim(dc::game::CFish &fish);
void      FishFightHold(dc::game::CFish &fish, const float *start, dc::game::CCPoly *polys, int count);
void      DrawFishingFight();

// The fight's bar (0 to 1), the side its fish runs to and the way to pull (a yaw, as eased), while one goes on (side 0
// and way 0 for none).
float FishingFightBar();
int   FishingFightSide();
float FishingFightTarget();

} // namespace expanded
