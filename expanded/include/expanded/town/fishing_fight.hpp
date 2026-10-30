#pragma once

namespace dc::game {
class CCharacter;
class CCPoly;
class CFish;
struct FishInfo;
} // namespace dc::game

namespace expanded {

// Fishing fights: a struck fish runs to the left or right of where it was hooked, as the screen shows it (square to the
// camera's look), thrashing as it goes as the game's fight has it, switching sides now and then, and the left stick
// pulls against it, back and away from its side (a fish running left: down and right). A pull fills the tension bar,
// anything else drains it kDrain times as fast; a full bar lands the fish the game's way, an empty one lets it get away
// ("Bait ran out.": the bait went as it was struck). A bigger fish switches sooner, swims faster and takes longer to
// land, a Garayan most. Off (the Fishing Fights option, config.json's expanded.fishing_fights, on by default), a struck
// fish is landed as in the original game; debug mode always pulls, as it always strikes.
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
// it. The line holds it to kLineStretch times the line's length from the rod tip, so at the line's end it pulls the
// line taut and stretched and swimming away slides it along the line's reach.
inline constexpr float kCone = 1.0471976f, kLineStretch = 1.1f;

// The line's length from the rod tip to the hook, as it is now: its segments' lengths (distp as the cast paid it out,
// the spot's hang under the float).
float FightLineLength();

// yaw turned into the cone either side of away (the way from the angler to the fish): itself inside it, else its
// nearer edge.
float FightConeYaw(float away, float yaw);

// position (x, y, z) held to reach from rod (x, y, z): across the ground no farther than reach leaves at its depth under
// the rod (none, past it); whether it was moved.
bool FightLineHold(float *position, const float *rod, float reach);

// How far a stick counts as pulled back, and as pushed aside.
inline constexpr float kPullBack = 0.3f, kPushAside = 0.3f;

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

// Whether a stick at (x, y) (as GetLXf and GetLYf: right and down positive) pulls against a fish running to side (-1
// the screen's left, 1 the right).
bool FightPulls(float x, float y, int side);

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
// turn and speed steps take (kFightPace for the fought fish, 1 for any other), and once it has moved (after its
// collision against count polys), which holds the fought fish to the line's end unless a wall is in the way; and the
// town's 2D drawing, the bar while fighting.
void      FishingFightStarted(dc::game::CFish &fish);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing);
FightStep FishingFightStep(dc::game::CCharacter &angler, float *facing, float stick_x, float stick_y);
float     FishFightSwim(dc::game::CFish &fish);
void      FishFightHold(dc::game::CFish &fish, dc::game::CCPoly *polys, int count);
void      DrawFishingFight();

// The fight's bar (0 to 1) and the side its fish runs to, while one goes on (side 0 for none).
float FishingFightBar();
int   FishingFightSide();

} // namespace expanded
