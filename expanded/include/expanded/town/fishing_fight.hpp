#pragma once

namespace dc::game {
class CCameraFollow;
class CCharacter;
class CCPoly;
class CFish;
struct FishInfo;
} // namespace dc::game

namespace expanded {

// Fishing fights: a struck fish first swims straight away from the angler, then runs to the left or right of where it
// was hooked, as he sees it (square to the way from him to it: the screen's left and right, as the camera turns to look
// along the line once it is struck), thrashing as it goes as the game's fight has it, switching sides now and then. A gauge lies on the water at the
// float, an arc facing the camera: the way to pull, against the way the fish swims as the screen shows it, is a mark
// on it, and the left stick's left and right move a block along it (the stick at rest, its middle). The block on the
// mark fills the tension bar at the top of the screen, a near miss holds it, anything else drains it (as fast as a
// pull fills it until the bar first rises past where it started, then kDrain times as fast), and the stick pulled
// back makes the bar go kPullBoost times as fast either way; a full bar lands the fish the game's way, an empty one
// lets it get away ("Bait ran out.": the bait went as it was struck). A bigger fish switches sooner, swims faster and
// takes longer to land, a Garayan most. Off (the Fishing Fights option, config.json's
// expanded.fishing_fights, on by default), a struck fish is landed as in the original game; debug mode always pulls, as
// it always strikes.
inline constexpr const char *kFishingFights = "fishing_fights";

bool FishingFightsOn();

// The bar's frames of pulling to fill from empty for the weakest fish, how much faster anything else drains it once
// the bar has risen past kStartBar (until then a miss drains it only as fast as a pull fills it), and how much faster
// it goes either way with the stick pulled back past kPullBack (as read).
inline constexpr int   kFillFrames = 240;
inline constexpr float kDrain = 2.0f, kPullBoost = 1.5f, kPullBack = 0.3f;

// The bar as a fight starts; the frames it holds as a fight starts (the fish swimming straight away) and after each
// switch, for the stick to follow.
inline constexpr float kStartBar = 0.3f;
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

// The gauge's places are angles along its arc (radians from its middle, the screen's right positive), kCone either
// side. The way to pull is against the way the fish swims as the camera sees it: a fish swimming straight away from
// the camera marks the middle, one swimming to the screen's right marks the left, held at the arc's ends. The mark
// follows kTargetEase of the turn a frame, so the fish's thrash steadies a little, and no more than kTargetTurn (3
// degrees) a frame, so a switch sweeps the mark across the gauge rather than jumping it.
inline constexpr float kTargetEase = 0.2f, kTargetTurn = 0.05235988f;

// The block pulls within kPullTolerance (20 degrees: its green middle) of the mark and holds within kHoldTolerance (30
// degrees: its yellow ends), the bar standing still.
inline constexpr float kPullTolerance = 0.34906585f, kHoldTolerance = 0.52359878f;

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

// The gauge: an arc flat on the water, its middle kGaugeRadius out from the float toward the camera and kGaugeBand
// wide, spanning kCone either side; it follows the float kGaugeEase of the way a frame, so it holds still while the
// float bobs. The mark for the way to pull reaches kMarkOverhang past the band's edges.
inline constexpr float kGaugeRadius = 7.0f, kGaugeBand = 2.5f, kGaugeEase = 0.1f, kMarkOverhang = 0.75f;

// The block's place on the gauge for a stick read x across (as FightStickRead reads it: right positive): the arc's
// ends at full tilt.
float FightStickAlong(float x);

// The mark's place on the gauge for a fish swimming heading (a yaw, as atan2(x, z)) with the camera looking
// camera_yaw: the way it swims as the screen shows it, turned round.
float FightTargetAlong(float camera_yaw, float heading);

// target followed toward along a frame.
float FightEaseTarget(float target, float along);

// Whether a stick read x across pulls toward the mark at target, and whether it holds (a pull holds too).
bool FightPulls(float x, float target);
bool FightHolds(float x, float target);

// Whether a stick read y down is pulled back.
bool FightPullsBack(float y);

// A point of the gauge (x, y, z): radius out from centre across the water, along (radians) from its middle, which
// points middle (a yaw: the way back toward the camera); at centre's height.
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
// stick read only while a fight goes on (stick_x and stick_y: as read), which keeps the angler turned as he stands
// and, for a fish that got away, sends it off (BattleFish cleared) with its close-up model's read dropped; CFish::Step
// on a fish in FISH_ACTION_BATTLE, which steers the fought fish on its run and returns the pace its turn and speed
// steps take (kFightPace for the fought fish, 1 for any other), and once it has moved from start (after its collision
// against count polys), which draws the fought fish back to the line's reach, sliding along a bank in the way, and
// failing that ends its step no farther from the rod than it started; and the town's 2D drawing, the bar and the gauge
// while fighting.
void      FishingFightStarted(dc::game::CFish &fish);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing, float stick_x, float stick_y);
float     FishFightSwim(dc::game::CFish &fish);
void      FishFightHold(dc::game::CFish &fish, const float *start, dc::game::CCPoly *polys, int count);
void      DrawFishingFight();

// After FishingBattleFish starts a fight: the fishing camera turns (over its own steps, not at once) to look straight
// along the way from the angler to the fish, so the gauge's left and right are his.
void FishingFightTurnCamera(dc::game::CCameraFollow &camera);

// The fight's bar (0 to 1), the side its fish runs to and the mark's place on the gauge (as eased), while one goes on
// (side 0 and place 0 for none).
float FishingFightBar();
int   FishingFightSide();
float FishingFightTarget();

} // namespace expanded
