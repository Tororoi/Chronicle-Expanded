#include "expanded/town/fishing_fight.hpp"

#include <libvu0.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include "camera.hpp"
#include "character.hpp"
#include "collision.hpp"
#include "dataread.hpp"
#include "ebattle.hpp"
#include "edit.hpp"
#include "editloop.hpp"
#include "editmapscript.hpp"
#include "effectmacro.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/core/options.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/town/fishing_sight.hpp"
#include "expanded/weapons/ruby/fairys_ring.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "gamepad.hpp"
#include "localize/localize.hpp"
#include "mainselect.hpp"
#include "mathutil.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "render/draw2d.hpp"

namespace expanded {

namespace {

[[maybe_unused]] const bool kPerfectCatchRegistered = (SetTalkMessage(kCatchMessage, PerfectCatchText), true);

[[maybe_unused]] const bool kRegistered = AddToggle({.key = kFishingFights,
                                                     .label = "Fishing Fights",
                                                     .help = "\"Fishing Fights\"\nA struck fish runs:\nkeep the stick's mark\nin the green. Off, it\nis always caught.",
                                                     .on = true});

// A fish that got away swims off for this long before it goes back to its own swimming.
constexpr int kGotAwayFrames = 180;

// The rumble while the stick pulls or holds (the line taut, the stronger) and while it does neither; a fish's run ends
// in a ripple this wide over it.
constexpr int   kPullRumble = 120, kSlackRumble = 70, kRumbleFrames = 4;
constexpr float kSwitchRipple = 10.0f;

// A running fish thrashes kThrash of the game's fight thrash (a heading anywhere: up to 45 degrees) either side of its
// run's heading, inside the cone.
constexpr float kThrash = 0.25f;

// The bar, in pixels and field rows: kBarWidth wide, centred at the top of the screen, its track kInset inside.
constexpr int kBarWidth = 240, kBarX = 320 - kBarWidth / 2, kBarY = 12, kBarHeight = 4, kInset = 1;

// The gauge's arc is drawn in kGaugeSteps pieces, a pale band with rims kGaugeRim wide so it shows on dark water and
// light; the mark is kMarkWidth (radians) either side of the stick's way.
constexpr int   kGaugeSteps = 24;
constexpr float kGaugeRim = 0.3f, kMarkWidth = 0.04f;

// The mark turns from white to red while the stick neither pulls nor holds, over kMarkFade frames (a tenth of a second) either
// way, so a frame or two off the mark never flickers it.
constexpr int kMarkFade = 6;

// The sight at the float, the water's surface through its middle, drawn over everything: two thin white rings
// kSightRingDistance out the way to pull, kRingTube thick, the inner as wide as the pull's tolerance there and the outer
// the hold's (so the pointer's tip inside the inner pulls and inside the outer holds), the space between them the
// tension bar (from the left at the water's surface, up and round; kRingStagger below); the pointer from the float the stick's way, drawn
// out kPointerStretch long and kPointerWidth wide, its tip kPointerReach past the rings.
constexpr float kSightRingDistance = 5.0f, kRingTube = 0.075f, kPointerStretch = 1.875f, kPointerWidth = 0.25f, kPointerReach = 0.625f;

// The outer ring stands kRingStagger nearer the float (toward the fish) than the inner, each as wide as its tolerance at
// its own distance, so the bar between them is a cone that shows when the fish runs to the side.
constexpr float kRingStagger = 0.625f;

// The rings' and the bar's pieces: around a ring, round its tube.
constexpr int kRingSteps = 48, kTubeSteps = 8;

struct Fight {
    CFish        *fish = nullptr; // null: no fight going on
    sceVu0FVECTOR anchor = {};    // where it was hooked
    sceVu0FVECTOR gauge = {};     // the gauge's centre: on the water, following the float
    float         angler[2] = {}; // where the angler stands (x, z)
    float         camera = 0.0f;  // the way the camera looks across the water (a yaw)
    float         middle = 0.0f;  // the gauge's middle: the way from its centre to the angler
    float         target = 0.0f;  // the way to pull, as eased
    float         stick = 0.0f;   // the way the stick last pointed across the water
    float         push = 0.0f;    // how far it is pushed
    float         strength = 0.0f;
    float         bar = 0.0f;
    float         way = 0.0f; // the run's way: radians from the way out from the angler to the fish, inside the cone
    int           run = 0;    // the frames left of the run
    int           grace = 0;  // the frames left that the bar holds
    int           frames = 0; // the fight's frames
    int           pulled = 0; // the frames of this run that pulled
    float         red = 0.0f; // how far the mark has turned red, 0 to 1
    bool          pulls = false;
    bool          holds = false; // pulls, or near enough that the bar stands still
    bool          won = false;
    bool          turning = false;                                // struck, still turning straight out: the fight not yet started
    bool          perfect = true;                                 // the stick not yet out of the block
    float         tolerance = kPullTolerance;                     // the pull's, for the fish's size
    float         hold_tolerance = kPullTolerance + kHoldPadding; // the hold's
};

Fight fight;

int g_perfect_points = 0; // the last catch's perfect-catch bonus

void LogFight(std::string_view step, std::string_view fields = {}) {
    if (EventLogOn()) {
        LogEvent("fishing_fight", std::format("\"step\":\"{}\",\"frames\":{},\"bar\":{:.2f}{}{}", step, fight.frames, fight.bar,
                                              fields.empty() ? "" : ",", fields));
    }
}

// The way the camera looks across the water (a yaw, as atan2(x, z)), or without a camera, the way the angler faces.
void Look() {
    if (NowCamera != nullptr) {
        sceVu0FVECTOR eye;
        sceVu0FVECTOR look;
        NowCamera->GetPos(eye);
        NowCamera->GetRef(look);
        float ahead_x = look[0] - eye[0], ahead_z = look[2] - eye[2];
        if (std::hypot(ahead_x, ahead_z) > 0.0f) {
            fight.camera = atan2f(ahead_x, ahead_z);
            return;
        }
    }
    fight.camera = Chara != nullptr ? Chara->GetRotation()->y : 0.0f;
}

// A yaw as the screen shows it, in degrees: 0 up (into the screen), 90 right, 180 down, -90 left.
float ScreenDegrees(float yaw) {
    return std::remainder(fight.camera - yaw, 2.0f * PI) * 180.0f / PI;
}

// The way to pull and the stick's, for the log.
std::string Ways() {
    return std::format("\"target\":{:.0f},\"stick\":{:.0f},\"push\":{:.2f}", ScreenDegrees(fight.target), ScreenDegrees(fight.stick),
                       fight.push);
}

// The gauge follows the float on the water, and the way to pull the fish as it swims.
void Follow() {
    sceVu0FVECTOR bob;
    FishLineGetUki(bob);
    fight.gauge[0] += (bob[0] - fight.gauge[0]) * kGaugeEase;
    fight.gauge[2] += (bob[2] - fight.gauge[2]) * kGaugeEase;
    fight.gauge[1] = FishingGetWaterLevel();
    fight.middle = atan2f(fight.angler[0] - fight.gauge[0], fight.angler[1] - fight.gauge[2]);
    sceVu0FVECTOR heading;
    fight.fish->GetRotation(heading);
    fight.target = FightEaseTarget(fight.target, FightTargetYaw(fight.middle, heading[1]));
}

// A run's way in degrees, for the log.
float WayDegrees() {
    return fight.way * 180.0f / PI;
}

// The fight's first run, straight out from the angler.
bool Opening() {
    return fight.frames <= kOpenFrames && fight.way == 0.0f;
}

// A run starts the way given, for frames (0: as long as the fish's strength has its runs).
void StartRun(float way, int frames = 0) {
    fight.way = way;
    fight.run = frames > 0 ? frames : FightRunFrames(fight.strength, GameFraction(), fight.frames);
}

// The fish lets go of the hook and swims off from it, then swims as any fish; its close-up model is not wanted.
void GotAway() {
    CFish &fish = *fight.fish;
    fish.action = FISH_ACTION_LEAVE;
    fish.action_timer = kGotAwayFrames;
    fish.move_mode = FISH_MOVE_SWIM;
    fish.move_timer = kGotAwayFrames;
    BattleFish = nullptr;
    BreakReadBG();
    EdFishingLostEsaMes();
    LogFight("got_away", std::format("\"kind\":{},{}", fish.fish_kind, Ways()));
    fight = {};
}

// A box at (x, y), in pixels and field rows, as MGFillBox takes them in 1/16ths.
void Box(int x, int y, int width, int height, u_char r, u_char g, u_char b, u_char a = 0x80) {
    CRect_i_ box;
    box.x = x * 16;
    box.y = y * 16;
    box.width = width * 16;
    box.height = height * 16;
    MGFillBox(box, r, g, b, a);
}

// The gauge's band from along a to b (radians from its middle), inner to outer out from its centre, as quads on the
// screen.
void Band(std::vector<gfx::Vertex2D> &quads, float a, float b, float inner, float outer, const u_char *rgba) {
    const int steps = std::max(1, static_cast<int>(std::ceil(static_cast<float>(kGaugeSteps) * (b - a) / (2.0f * kCone))));
    auto      vertex = [&](float along, float radius) {
        sceVu0FVECTOR point;
        FightGaugePoint(fight.gauge, fight.middle, along, radius, point);
        int screen[4] = {};
        MGRotTransPers2D(screen, point, 0);
        return draw2d::Vertex(static_cast<float>(screen[0]), static_cast<float>(screen[1]), 0.0f, 0.0f, 0.0f, rgba[0], rgba[1], rgba[2],
                              rgba[3]);
    };
    for (int i = 0; i < steps; i++) {
        const float from = a + (b - a) * static_cast<float>(i) / static_cast<float>(steps);
        const float to = a + (b - a) * static_cast<float>(i + 1) / static_cast<float>(steps);
        quads.push_back(vertex(from, inner));
        quads.push_back(vertex(from, outer));
        quads.push_back(vertex(to, outer));
        quads.push_back(vertex(to, inner));
    }
}

// The sight (below).
void DrawFightSight();

} // namespace

bool FishingFightsOn() {
    return OptionOn(kFishingFights);
}

float FightStrength(float size) {
    if (size <= kLargeFish) {
        return std::max(0.0f, (size - kSmallFish) / (kLargeFish - kSmallFish));
    }
    return std::min(2.0f, 1.0f + (size - kLargeFish) / (kAriseFish - kLargeFish));
}

float FightFill() {
    return 1.0f / static_cast<float>(kFillFrames);
}

float FightDrain() {
    return 1.0f / static_cast<float>(kDrainFrames);
}

int FightRunFrames(float strength, float roll, int fought) {
    const float frames = static_cast<float>(kShortestRun) + std::clamp(roll, 0.0f, 1.0f) * static_cast<float>(kLongestRun - kShortestRun);
    const float scale = strength <= 1.0f ? 1.0f - 2.0f * strength / 3.0f : 1.0f / 3.0f - (strength - 1.0f) / 10.0f;
    const float tired = 1.0f + static_cast<float>(std::clamp(fought, 0, kTiredLimit)) / static_cast<float>(kTiredFrames);
    return std::max(1, static_cast<int>(std::lround(frames * scale * tired)));
}

float FightRunSpeed(float strength) {
    return kRunSpeed * kFightPace * (1.0f + kSpeedPerStrength * strength);
}

float FightConeYaw(float away, float yaw) {
    float off = std::remainder(yaw - away, 2.0f * PI);
    return away + std::clamp(off, -kCone, kCone);
}

float FightRunWay(float previous, float roll) {
    const float half = previous >= 0.0f ? -1.0f : 1.0f;
    // Out from the middle along it, past what lies within kMinSwitch of previous.
    const float least = std::max(0.0f, kMinSwitch - std::fabs(previous));
    return half * (least + std::clamp(roll, 0.0f, 1.0f) * (kCone - least));
}

float FightLineLength() {
    float length = 0.0f;
    for (int segment = 1; segment < 24; segment++) {
        length += LineSegment(segment);
    }
    return length;
}

bool FightLineHold(float *position, const float *rod, float reach) {
    float dx = position[0] - rod[0], dz = position[2] - rod[2];
    float distance = std::hypot(dx, dz);
    if (!(distance > reach)) {
        return false;
    }
    position[0] = rod[0] + dx * reach / distance;
    position[2] = rod[2] + dz * reach / distance;
    return true;
}

float FightStickYaw(float camera_yaw, float x, float y) {
    // The camera's ahead across the water is (sin, cos) of its yaw, its right (-cos, sin); the stick's up is ahead.
    const float c = std::cos(camera_yaw), s = std::sin(camera_yaw);
    return atan2f(-x * c - y * s, x * s - y * c);
}

float FightTargetYaw(float middle, float heading) {
    return FightConeYaw(middle, heading + PI);
}

float FightEaseTarget(float target, float yaw) {
    const float turn = std::clamp(std::remainder(yaw - target, 2.0f * PI) * kTargetEase, -kTargetTurn, kTargetTurn);
    return std::remainder(target + turn, 2.0f * PI);
}

FightStick FightStickRead(float x, float y) {
    const float push = std::hypot(x, y);
    if (push <= kStickDead) {
        return {};
    }
    const float read = std::min(1.0f, (push - kStickDead) / (1.0f - kStickDead));
    return {.x = x / push * read, .y = y / push * read};
}

namespace {

bool Within(float x, float y, float camera_yaw, float middle, float target, float tolerance) {
    return std::fabs(std::remainder(FightStickWay(camera_yaw, middle, x, y) - target, 2.0f * PI)) <= tolerance;
}

} // namespace

float FightStickWay(float camera_yaw, float middle, float x, float y) {
    return std::hypot(x, y) > kPullBack ? FightStickYaw(camera_yaw, x, y) : middle;
}

bool FightPulls(float x, float y, float camera_yaw, float middle, float target, float tolerance) {
    return Within(x, y, camera_yaw, middle, target, tolerance);
}

bool FightHolds(float x, float y, float camera_yaw, float middle, float target, float tolerance) {
    return Within(x, y, camera_yaw, middle, target, tolerance);
}

float FightPullTolerance(float size) {
    return kPullTolerance + std::max(0.0f, kAriseFish - size) / 3.0f * kPullWidening;
}

float FightHoldTolerance(float size) {
    return FightPullTolerance(size) + kHoldPadding;
}

int PerfectCatchPoints(int size_cm) {
    g_perfect_points = fight.won && fight.perfect ? (size_cm + kPerfectCatchShare - 1) / kPerfectCatchShare : 0;
    if (g_perfect_points > 0) {
        LogFight("perfect", std::format("\"size\":{},\"points\":{}", size_cm, g_perfect_points));
    }
    return g_perfect_points;
}

std::optional<std::string> PerfectCatchText(std::string_view own) {
    const std::size_t points = own.find("{value2}");
    if (g_perfect_points <= 0 || points == std::string_view::npos) {
        return std::nullopt;
    }
    const std::size_t end = std::min(own.find('\n', points), own.size());
    std::string       text(own.substr(0, end));
    text += '\n';
    text += LocalizeText(kPerfectCatchKey, "Perfect Catch  +{value4}");
    text += own.substr(end);
    return text;
}

void FightGaugePoint(const float *centre, float middle, float along, float radius, float *point) {
    point[0] = centre[0] + radius * std::sin(middle + along);
    point[1] = centre[1];
    point[2] = centre[2] + radius * std::cos(middle + along);
    point[3] = 1.0f;
}

void FishingFightStarted(CFish &fish) {
    fight = {};
    if (!FishingFightsOn()) {
        return;
    }
    fight.fish = &fish;
    fish.GetPosition(fight.anchor);
    Look();
    fight.strength = FightStrength(fish.size);
    fight.tolerance = FightPullTolerance(fish.size);
    fight.hold_tolerance = FightHoldTolerance(fish.size);
    fight.bar = kStartBar;
    StartRun(0.0f, kOpenFrames);
    if (Chara != nullptr) {
        sceVu0FVECTOR angler;
        Chara->GetPosition(angler);
        fight.angler[0] = angler[0];
        fight.angler[1] = angler[2];
    }
    // The fish turns straight out from the angler before the fight starts.
    fight.turning = true;
    // The gauge starts on the float, the way to pull straight back to the angler.
    FishLineGetUki(fight.gauge);
    fight.gauge[1] = FishingGetWaterLevel();
    fight.middle = atan2f(fight.angler[0] - fight.gauge[0], fight.angler[1] - fight.gauge[2]);
    fight.target = fight.middle;
    fight.stick = fight.middle;
    LogFight("strike", std::format("\"kind\":{},\"size\":{}", fish.fish_kind, static_cast<int>(fish.size * 10.0f)));
}

// The way straight out from the angler to the fish (a yaw).
float OutYaw(CFish &fish) {
    sceVu0FVECTOR here;
    fish.GetPosition(here);
    return atan2f(here[0] - fight.angler[0], here[2] - fight.angler[1]);
}

// The fight starts: the fish has turned straight out.
void Begin() {
    fight.turning = false;
    CFish        &fish = *fight.fish;
    sceVu0FVECTOR here;
    fish.GetPosition(here);
    const float out = std::hypot(here[0] - fight.angler[0], here[2] - fight.angler[1]);
    const float line = FightLineLength();
    float       dx = here[0] - rod_top[0], dy = here[1] - rod_top[1], dz = here[2] - rod_top[2];
    LogFight("start", std::format("\"kind\":{},\"size\":{},\"strength\":{:.2f},\"out\":{:.0f},\"line\":{:.1f},\"reach\":{:.1f},"
                                  "\"from_rod\":{:.1f},\"across\":{:.1f},\"way\":{:.0f},\"run\":{}",
                                  fish.fish_kind, static_cast<int>(fish.size * 10.0f), fight.strength, out, line, line * kLineReach,
                                  std::sqrt(dx * dx + dy * dy + dz * dz), std::hypot(dx, dz), WayDegrees(), fight.run));
}

FightStep FishingFightStep(CCharacter &angler, float *facing) {
    if (fight.fish == nullptr) {
        return FightStep::kRetail;
    }
    const FightStick stick = FightStickRead(GamePad.GetLXRaw(), GamePad.GetLYRaw());
    return FishingFightStep(angler, facing, stick.x, stick.y);
}

FightStep FishingFightStep(CCharacter &angler, float *facing, float stick_x, float stick_y) {
    if (fight.fish == nullptr) {
        return FightStep::kRetail;
    }
    // The stick reads in the camera's frame as the frame starts; the angler turns as while waiting for a bite.
    Look();
    facing[1] = AngleInterpolate(facing[1], facing[1] - 0.1f * stick_x, 0.01f, INTERPOLATE_STEP);
    angler.SetRotation(facing);
    if (fight.won) {
        return FightStep::kLanded;
    }

    Follow();
    if (fight.turning) {
        sceVu0FVECTOR heading;
        fight.fish->GetRotation(heading);
        if (std::fabs(std::remainder(heading[1] - OutYaw(*fight.fish), 2.0f * PI)) > kTurned) {
            return FightStep::kFighting;
        }
        Begin();
    }
    fight.frames++;
    if (--fight.run <= 0) {
        const int pulled = fight.pulled;
        // The opening's run goes straight out; the first after it goes either side at random.
        const float way = FightRunWay(fight.way, GameFraction());
        StartRun(fight.frames <= kOpenFrames ? (GameFraction() < 0.5f ? 1.0f : -1.0f) * std::fabs(way) : way);
        fight.pulled = 0;
        fight.grace = kSwitchGrace;
        sceVu0FVECTOR ripple;
        fight.fish->GetPosition(ripple);
        ripple[1] = FishingGetWaterLevel();
        EffectHamon(&EdEffectGroup, ripple, kSwitchRipple);
        LogFight("switch", std::format("\"way\":{:.0f},\"run\":{},\"pulled\":{},{}", WayDegrees(), fight.run, pulled, Ways()));
    }

    // The stick's way across the water (at rest, the gauge's middle); it pulls within the tolerance, and holds the bar
    // within the hold's.
    fight.push = std::hypot(stick_x, stick_y);
    fight.stick = FightStickWay(fight.camera, fight.middle, stick_x, stick_y);
    fight.pulls = DebugMode != 0 || FightPulls(stick_x, stick_y, fight.camera, fight.middle, fight.target, fight.tolerance);
    fight.holds = fight.pulls || FightHolds(stick_x, stick_y, fight.camera, fight.middle, fight.target, fight.hold_tolerance);
    fight.pulled += fight.pulls ? 1 : 0;
    const float fade = 1.0f / static_cast<float>(kMarkFade);
    fight.red = std::clamp(fight.red + (fight.holds ? -fade : fade), 0.0f, 1.0f);
    // Through the opening a miss holds the bar, as a near miss does after it.
    const bool opening = fight.frames <= kOpenFrames;
    bool       drained = false;
    if (fight.grace > 0) {
        fight.grace--;
    } else if (fight.pulls) {
        fight.bar = std::min(1.0f, fight.bar + FightFill());
    } else if (!fight.holds) {
        // Out of the block: no perfect catch, and after the opening the bar drains.
        fight.perfect = false;
        if (!opening) {
            fight.bar = std::max(0.0f, fight.bar - FightDrain());
            drained = true;
        }
    }
    GamePad.SetVibration(1, fight.holds ? kPullRumble : kSlackRumble, kRumbleFrames);

    if (fight.bar >= 1.0f) {
        fight.won = true;
        LogFight("landed", std::format("\"kind\":{},\"perfect\":{},{}", fight.fish->fish_kind, fight.perfect, Ways()));
        return FightStep::kLanded;
    }
    if (drained && fight.bar <= 0.0f) {
        GotAway();
        return FightStep::kGotAway;
    }
    return FightStep::kFighting;
}

FightPace FishFightSwim(CFish &fish) {
    if (&fish != fight.fish) {
        return {};
    }
    // The run's way out from the angler, the way from him to the fish turning as it goes.
    const float away = OutYaw(fish);
    if (fight.turning) {
        fish.target_yaw = AngleLimit(away);
        fish.target_speed = FightRunSpeed(fight.strength);
        return {.turn = kOpenTurnPace, .speed = kFightPace};
    }
    const float heading = away + fight.way;
    // The game's thrash, a new heading every frame (the battle block's roll, target_yaw), narrowed to kThrash of it either
    // side of the run's heading and kept in the cone: the fish twists and struggles as it goes, never toward the angler.
    // Through the opening it runs straight.
    const float thrash = Opening() ? 0.0f : std::remainder(fish.target_yaw, 2.0f * PI) * kThrash;
    fish.target_yaw = AngleLimit(FightConeYaw(away, heading + thrash));
    fish.target_speed = FightRunSpeed(fight.strength);
    return {.turn = kFightTurnPace, .speed = kFightPace};
}

void FishFightHold(CFish &fish, const float *start, CCPoly *polys, int count) {
    if (&fish != fight.fish || fish.action != FISH_ACTION_BATTLE) {
        return;
    }
    sceVu0FVECTOR position;
    fish.GetPosition(position);
    sceVu0FVECTOR held;
    sceVu0CopyVector(held, position);
    const float reach = FightLineLength() * kLineReach;
    if (!FightLineHold(held, rod_top, reach)) {
        return;
    }
    // Drawn in toward the rod; against a bank, slid along it as far in as the pull goes.
    sceVu0FVECTOR hit;
    const int     wall = polys == nullptr ? -1 : CheckHit(polys, count, position, held, hit, 0, 0);
    if (wall >= 0) {
        float pull[2] = {held[0] - position[0], held[2] - position[2]};
        float normal[2] = {polys[wall].normal[0], polys[wall].normal[2]};
        float length = std::hypot(normal[0], normal[1]);
        sceVu0CopyVector(held, position);
        if (length > 0.0f) {
            normal[0] /= length;
            normal[1] /= length;
            float into = pull[0] * normal[0] + pull[1] * normal[1];
            held[0] += pull[0] - into * normal[0];
            held[2] += pull[1] - into * normal[1];
            if (CheckHit(polys, count, position, held, hit, 0, 0) >= 0) {
                sceVu0CopyVector(held, position);
            }
        }
    }
    // Still past the line's reach, it ends the step no farther from the rod than it started it.
    auto from_rod = [](const float *at) { return std::hypot(at[0] - rod_top[0], at[2] - rod_top[2]); };
    if (from_rod(held) > reach && from_rod(start) < from_rod(held)) {
        std::copy_n(start, 3, held);
    }
    fish.SetPosition(held);
}

void DrawFishingFight() {
    if (fight.fish == nullptr || GameMode != ED_MODE_FISHING || chara_fishing != ED_FISHING_BATTLE) {
        return;
    }
    // With the sight, the bar is in it and there is no gauge.
    if (LoadedFishingSight() != nullptr) {
        DrawFightSight();
        return;
    }
    // Thin at the top: a dark frame, a grey track and the bar in the dungeon's ABS blue.
    const int track = kBarWidth - 2 * kInset;
    const int filled = static_cast<int>(fight.bar * static_cast<float>(track));
    Box(kBarX, kBarY, kBarWidth, kBarHeight, 0x14, 0x14, 0x14);
    Box(kBarX + kInset, kBarY + 1, track, kBarHeight - 2, 0x40, 0x40, 0x40);
    Box(kBarX + kInset, kBarY + 1, filled, kBarHeight - 2, 0x00, 0xA2, 0xFF);

    // The gauge on the water, drawn over everything: its arc, the block for the way to pull on it (a bright green
    // middle, the pull's tolerance, between yellow ends, the hold's; cut off at the arc's ends) and the mark for the
    // stick's way, white while the stick pulls or holds and red while it does neither.
    sceVu0FVECTOR centre;
    sceVu0CopyVector(centre, fight.gauge);
    int screen[4] = {};
    if (MGRotTransPers2D(screen, centre, 0) == 0) {
        return;
    }
    constexpr u_char kArc[4] = {0xA0, 0xA8, 0xB0, 0x30};
    constexpr u_char kRim[4] = {0xE0, 0xE0, 0xE0, 0x60};
    constexpr u_char kBlock[4] = {0x70, 0xFF, 0x70, 0xC0};
    constexpr u_char kHold[4] = {0xFF, 0xE0, 0x40, 0xC0};
    constexpr u_char kMarkWhite[4] = {0xF0, 0xF0, 0xF0, 0x80};
    constexpr u_char kMarkRed[4] = {0xFF, 0x20, 0x20, 0xC0};
    u_char           mark_shade[4];
    for (int i = 0; i < 4; i++) {
        mark_shade[i] = static_cast<u_char>(std::lround(kMarkWhite[i] + (kMarkRed[i] - kMarkWhite[i]) * fight.red));
    }
    const float                inner = kGaugeRadius - kGaugeBand / 2.0f, outer = kGaugeRadius + kGaugeBand / 2.0f;
    std::vector<gfx::Vertex2D> quads;
    Band(quads, -kCone, kCone, inner, outer, kArc);
    Band(quads, -kCone, kCone, inner, inner + kGaugeRim, kRim);
    Band(quads, -kCone, kCone, outer - kGaugeRim, outer, kRim);
    // The block's middle reaches the arc's ends, the parts past them cut off.
    const float block = std::remainder(FightConeYaw(fight.middle, fight.target) - fight.middle, 2.0f * PI);
    auto        part = [&](float a, float b, const u_char *rgba) {
        a = std::max(a, -kCone);
        b = std::min(b, kCone);
        if (a < b) {
            Band(quads, a, b, inner, outer, rgba);
        }
    };
    const float pull = fight.tolerance, hold = fight.hold_tolerance;
    part(block - hold, block - pull, kHold);
    part(block - pull, block + pull, kBlock);
    part(block + pull, block + hold, kHold);
    // The mark held to the arc's ends when the stick points past them.
    const float mark = std::clamp(std::remainder(fight.stick - fight.middle, 2.0f * PI), -kCone, kCone);
    Band(quads, mark - kMarkWidth, mark + kMarkWidth, inner - kMarkOverhang, outer + kMarkOverhang, mark_shade);
    draw2d::DrawUntextured(gfx::Primitive::Quads, quads, draw2d::SpriteState());
    draw2d::RestoreTestZbuf();
}

float FishingFightBar() {
    return fight.fish != nullptr ? fight.bar : 0.0f;
}

float FishingFightRunWay() {
    return fight.fish != nullptr ? fight.way : 0.0f;
}

float FishingFightTarget() {
    return fight.fish != nullptr ? fight.target : 0.0f;
}

namespace {

using World = std::array<float, 3>;

// A triangle of the sight in the world and its colour.
struct SightTriangle {
    std::array<World, 3>  corners;
    std::array<u_char, 4> rgba;
};

World Plus(const World &a, const World &b, float scale) {
    return {a[0] + b[0] * scale, a[1] + b[1] * scale, a[2] + b[2] * scale};
}

// A quad as two triangles.
void SightQuad(std::vector<SightTriangle> &into, const World &a, const World &b, const World &c, const World &d,
               const std::array<u_char, 4> &rgba) {
    into.push_back({
        {a, b, c},
        rgba
    });
    into.push_back({
        {a, c, d},
        rgba
    });
}

// The pointer's triangles: along yaw from origin, its along scaled by kPointerStretch and shifted by start.
void SightPointer(std::vector<SightTriangle> &into, const std::vector<SightPoint> &part, const float *origin, float yaw, float start,
                  const std::array<u_char, 4> &rgba) {
    const World ahead = {std::sin(yaw), 0.0f, std::cos(yaw)}, side = {std::cos(yaw), 0.0f, -std::sin(yaw)};
    const World from = {origin[0], origin[1], origin[2]};
    for (std::size_t i = 0; i + 2 < part.size(); i += 3) {
        SightTriangle t{.rgba = rgba};
        for (std::size_t c = 0; c < 3; c++) {
            const SightPoint &p = part[i + c];
            t.corners[c] = Plus(Plus(Plus(from, ahead, start + p[0] * kPointerStretch), {0.0f, 1.0f, 0.0f}, p[1] * kPointerWidth), side,
                                p[2] * kPointerWidth);
        }
        into.push_back(t);
    }
}

// A thin white ring radius round centre, square to facing, lit from above and in front of it.
void SightRing(std::vector<SightTriangle> &into, const World &centre, const World &facing, const World &left, float radius) {
    constexpr World up = {0.0f, 1.0f, 0.0f};
    auto            corner = [&](int around, int round, World &normal) {
        const float a = 2.0f * PI * static_cast<float>(around) / kRingSteps, b = 2.0f * PI * static_cast<float>(round) / kTubeSteps;
        const World out = Plus(Plus({0.0f, 0.0f, 0.0f}, left, std::cos(a)), up, std::sin(a));
        normal = Plus(Plus({0.0f, 0.0f, 0.0f}, out, std::cos(b)), facing, std::sin(b));
        return Plus(Plus(centre, out, radius + kRingTube * std::cos(b)), facing, kRingTube * std::sin(b));
    };
    auto shade = [&](const World &normal) {
        const float light = std::max(0.0f, 0.6f * normal[1] + 0.8f * (normal[0] * facing[0] + normal[2] * facing[2]));
        const auto  level = static_cast<u_char>(std::lround(0xA0 + 0x50 * light));
        return std::array<u_char, 4>{level, level, level, 0xFF};
    };
    for (int i = 0; i < kRingSteps; i++) {
        for (int j = 0; j < kTubeSteps; j++) {
            // Each quad shaded by its corners' normals between them.
            World       na, nb, nc, nd;
            const World a = corner(i, j, na), b = corner(i + 1, j, nb), c = corner(i + 1, j + 1, nc), d = corner(i, j + 1, nd);
            const World normal = {(na[0] + nb[0] + nc[0] + nd[0]) / 4.0f, (na[1] + nb[1] + nc[1] + nd[1]) / 4.0f,
                                  (na[2] + nb[2] + nc[2] + nd[2]) / 4.0f};
            SightQuad(into, a, b, c, d, shade(normal));
        }
    }
}

// The band between the rings, from the inner (round inner_centre) to the outer (round outer_centre), both square to the
// way to pull: the bar's share of it from the left at the water's surface, up and round, in the bar's blue; the rest
// its track.
void SightBar(std::vector<SightTriangle> &into, const World &inner_centre, const World &outer_centre, const World &left, float inner,
              float outer, float bar) {
    constexpr World                 up = {0.0f, 1.0f, 0.0f};
    constexpr std::array<u_char, 4> kFill = {0x00, 0xA2, 0xFF, 0xFF};
    constexpr std::array<u_char, 4> kTrack = {0x20, 0x20, 0x20, 0x90};
    auto                            at = [&](const World &centre, float angle, float radius) {
        return Plus(Plus(centre, left, radius * std::cos(angle)), up, radius * std::sin(angle));
    };
    // The angle runs from the left (0) through up (a quarter turn).
    for (int i = 0; i < kRingSteps; i++) {
        const float from = 2.0f * PI * static_cast<float>(i) / kRingSteps, to = 2.0f * PI * static_cast<float>(i + 1) / kRingSteps;
        const float split = std::clamp(bar * 2.0f * PI, from, to);
        if (split > from) {
            SightQuad(into, at(inner_centre, from, inner), at(outer_centre, from, outer), at(outer_centre, split, outer),
                      at(inner_centre, split, inner), kFill);
        }
        if (split < to) {
            SightQuad(into, at(inner_centre, split, inner), at(outer_centre, split, outer), at(outer_centre, to, outer),
                      at(inner_centre, to, inner), kTrack);
        }
    }
}

// Triangles drawn farthest from the camera first, so the pointer passes through the rings.
void DrawSightTriangles(std::vector<SightTriangle> &triangles) {
    auto depth = [](const SightTriangle &t) {
        float sum = 0.0f;
        for (const auto &c : t.corners) {
            sceVu0FVECTOR point;
            sceVu0FVECTOR at = {c[0], c[1], c[2], 1.0f};
            sceVu0ApplyMatrix(point, mgRenderInfo.view_scaled, at);
            sum += point[2];
        }
        return sum;
    };
    std::ranges::sort(triangles, std::greater<>{}, depth);
    std::vector<gfx::Vertex2D> tris;
    for (const SightTriangle &t : triangles) {
        gfx::Vertex2D corners[3];
        bool          shown = true;
        for (std::size_t c = 0; c < 3; c++) {
            sceVu0FVECTOR at = {t.corners[c][0], t.corners[c][1], t.corners[c][2], 1.0f};
            int           screen[4] = {};
            shown = shown && MGRotTransPers2D(screen, at, 0) != 0;
            corners[c] = draw2d::Vertex(static_cast<float>(screen[0]), static_cast<float>(screen[1]), 0.0f, 0.0f, 0.0f, t.rgba[0], t.rgba[1],
                                        t.rgba[2], t.rgba[3]);
        }
        if (shown) {
            tris.insert(tris.end(), corners, corners + 3);
        }
    }
    draw2d::DrawUntextured(gfx::Primitive::Triangles, tris, draw2d::SpriteState());
    draw2d::RestoreTestZbuf();
}

// The sight, drawn after the water (clear of its ripple).
void DrawFightSight() {
    const FishingSight *sight = LoadedFishingSight();
    if (sight == nullptr) {
        return;
    }
    constexpr std::array<u_char, 4> kWhite = {0xF0, 0xF0, 0xF0, 0xE0};
    constexpr std::array<u_char, 4> kRed = {0xFF, 0x20, 0x20, 0xE0};
    std::array<u_char, 4>           pointer{};
    for (std::size_t i = 0; i < 4; i++) {
        pointer[i] = static_cast<u_char>(std::lround(kWhite[i] + (kRed[i] - kWhite[i]) * fight.red));
    }
    const float pull = fight.middle + std::remainder(FightConeYaw(fight.middle, fight.target) - fight.middle, 2.0f * PI);
    const World facing = {std::sin(pull), 0.0f, std::cos(pull)};
    const float origin[3] = {fight.gauge[0], FishingGetWaterLevel(), fight.gauge[2]};
    const World centre = Plus({origin[0], origin[1], origin[2]}, facing, kSightRingDistance);
    // The rings' left as the screen shows them: whichever side of them is further left.
    World left = {std::cos(pull), 0.0f, -std::sin(pull)};
    {
        sceVu0FVECTOR one = {centre[0] + left[0], centre[1], centre[2] + left[2], 1.0f};
        sceVu0FVECTOR two = {centre[0] - left[0], centre[1], centre[2] - left[2], 1.0f};
        int           a[4] = {}, b[4] = {};
        MGRotTransPers2D(a, one, 0);
        MGRotTransPers2D(b, two, 0);
        if (b[0] < a[0]) {
            left = {-left[0], 0.0f, -left[2]};
        }
    }
    const World                outer_centre = Plus(centre, facing, -kRingStagger);
    const float                inner = kSightRingDistance * std::tan(fight.tolerance);
    const float                outer = (kSightRingDistance - kRingStagger) * std::tan(fight.hold_tolerance);
    std::vector<SightTriangle> triangles;
    SightRing(triangles, centre, facing, left, inner);
    SightRing(triangles, outer_centre, facing, left, outer);
    SightBar(triangles, centre, outer_centre, left, inner + kRingTube, outer - kRingTube, fight.bar);
    const float reach = kSightRingDistance + kPointerReach;
    SightPointer(triangles, sight->pointer, origin, fight.stick, reach - kSightPointerLength * kPointerStretch, pointer);
    DrawSightTriangles(triangles);
}

} // namespace

} // namespace expanded
