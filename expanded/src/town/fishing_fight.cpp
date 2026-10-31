#include "expanded/town/fishing_fight.hpp"

#include <libvu0.h>

#include <algorithm>
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
#include "expanded/core/options.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/town/fishing.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "gamepad.hpp"
#include "mainselect.hpp"
#include "mathutil.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "render/draw2d.hpp"

namespace expanded {

namespace {

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
    float         way = 0.0f;     // the run's way: radians from the way out from the angler to the fish, inside the cone
    int           run = 0;        // the frames left of the run
    int           run_frames = 0; // and all its frames
    float         next = 0.0f;    // the next run's way, picked as this one starts
    int           grace = 0;      // the frames left that the bar holds
    int           frames = 0;     // the fight's frames
    int           pulled = 0;     // the frames of this run that pulled
    float         red = 0.0f;     // how far the mark has turned red, 0 to 1
    bool          pulls = false;
    bool          holds = false; // pulls, or near enough that the bar stands still
    bool          won = false;
};

Fight fight;

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

// A run starts the way picked for it, and picks the next run's.
void StartRun(float way) {
    fight.way = way;
    fight.run = fight.run_frames = FightRunFrames(fight.strength, GameFraction());
    fight.next = FightRunWay(way, GameFraction(), GameFraction());
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

// A triangle on the gauge: the band's full width at along a, narrowing to a point at the band's middle at b.
void Wedge(std::vector<gfx::Vertex2D> &quads, float a, float b, float inner, float outer, const u_char *rgba) {
    const int   steps = std::max(1, static_cast<int>(std::ceil(static_cast<float>(kGaugeSteps) * std::fabs(b - a) / (2.0f * kCone))));
    const float middle = (inner + outer) / 2.0f, half = (outer - inner) / 2.0f;
    auto        vertex = [&](float along, float radius) {
        sceVu0FVECTOR point;
        FightGaugePoint(fight.gauge, fight.middle, along, radius, point);
        int screen[4] = {};
        MGRotTransPers2D(screen, point, 0);
        return draw2d::Vertex(static_cast<float>(screen[0]), static_cast<float>(screen[1]), 0.0f, 0.0f, 0.0f, rgba[0], rgba[1], rgba[2],
                              rgba[3]);
    };
    for (int i = 0; i < steps; i++) {
        const float t0 = static_cast<float>(i) / static_cast<float>(steps), t1 = static_cast<float>(i + 1) / static_cast<float>(steps);
        const float from = a + (b - a) * t0, to = a + (b - a) * t1;
        quads.push_back(vertex(from, middle - half * (1.0f - t0)));
        quads.push_back(vertex(from, middle + half * (1.0f - t0)));
        quads.push_back(vertex(to, middle + half * (1.0f - t1)));
        quads.push_back(vertex(to, middle - half * (1.0f - t1)));
    }
}

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

int FightRunFrames(float strength, float roll) {
    const float frames = static_cast<float>(kShortestRun) + std::clamp(roll, 0.0f, 1.0f) * static_cast<float>(kLongestRun - kShortestRun);
    const float scale = strength <= 1.0f ? 1.0f - 2.0f * strength / 3.0f : 1.0f / 3.0f - (strength - 1.0f) / 6.0f;
    return std::max(1, static_cast<int>(std::lround(frames * scale)));
}

float FightRunSpeed(float strength) {
    return kRunSpeed * kFightPace * (1.0f + kSpeedPerStrength * strength);
}

float FightConeYaw(float away, float yaw) {
    float off = std::remainder(yaw - away, 2.0f * PI);
    return away + std::clamp(off, -kCone, kCone);
}

float FightRunWay(float previous, float half_roll, float within_roll) {
    const bool opposite = half_roll < kOppositeOdds / (kOppositeOdds + 1.0f);
    float      half = (previous >= 0.0f) != opposite ? 1.0f : -1.0f;
    // Out from the middle along the half, what lies kMinSwitch or more from previous: up to its near side and on from
    // its far side.
    auto room = [&](float side, float &near, float &far) {
        const float from = side * previous;
        near = std::clamp(from - kMinSwitch, 0.0f, kCone);
        far = std::clamp(from + kMinSwitch, 0.0f, kCone);
        return near + (kCone - far);
    };
    float near = 0.0f, far = 0.0f;
    if (room(half, near, far) <= 0.0f) {
        half = -half;
        room(half, near, far);
    }
    const float out = std::clamp(within_roll, 0.0f, 1.0f) * (near + kCone - far);
    // On the near side, unless it is empty (then the far side) or the far side is (then the near side's end).
    return half * ((out < near || far >= kCone) ? out : far + (out - near));
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

float FightPullTolerance(int frames) {
    const float open = std::clamp(static_cast<float>(frames) / static_cast<float>(kOpenFrames), 0.0f, 1.0f);
    return 2.0f * kCone + (kPullTolerance - 2.0f * kCone) * open;
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
    fight.bar = kStartBar;
    StartRun((2.0f * GameFraction() - 1.0f) * kCone);
    if (Chara != nullptr) {
        sceVu0FVECTOR angler;
        Chara->GetPosition(angler);
        fight.angler[0] = angler[0];
        fight.angler[1] = angler[2];
    }
    // The gauge starts on the float, the way to pull straight back to the angler.
    FishLineGetUki(fight.gauge);
    fight.gauge[1] = FishingGetWaterLevel();
    fight.middle = atan2f(fight.angler[0] - fight.gauge[0], fight.angler[1] - fight.gauge[2]);
    fight.target = fight.middle;
    fight.stick = fight.middle;
    const float out = std::hypot(fight.anchor[0] - fight.angler[0], fight.anchor[2] - fight.angler[1]);
    const float line = FightLineLength();
    float       dx = fight.anchor[0] - rod_top[0], dy = fight.anchor[1] - rod_top[1], dz = fight.anchor[2] - rod_top[2];
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

    fight.frames++;
    Follow();
    if (--fight.run <= 0) {
        const int pulled = fight.pulled;
        StartRun(fight.next);
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
    const float pull = FightPullTolerance(fight.frames);
    fight.pulls = DebugMode != 0 || FightPulls(stick_x, stick_y, fight.camera, fight.middle, fight.target, pull);
    fight.holds = fight.pulls ||
                  FightHolds(stick_x, stick_y, fight.camera, fight.middle, fight.target, pull + kHoldTolerance - kPullTolerance);
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
    } else if (!fight.holds && !opening) {
        fight.bar = std::max(0.0f, fight.bar - FightDrain());
        drained = true;
    }
    GamePad.SetVibration(1, fight.holds ? kPullRumble : kSlackRumble, kRumbleFrames);

    if (fight.bar >= 1.0f) {
        fight.won = true;
        LogFight("landed", std::format("\"kind\":{},{}", fight.fish->fish_kind, Ways()));
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
    sceVu0FVECTOR here;
    fish.GetPosition(here);
    const float away = atan2f(here[0] - fight.angler[0], here[2] - fight.angler[1]);
    const float heading = away + fight.way;
    // The game's thrash, a new heading every frame (the battle block's roll, target_yaw), narrowed to kThrash of it either
    // side of the run's heading and kept in the cone: the fish twists and struggles as it goes, never toward the angler.
    const float thrash = std::remainder(fish.target_yaw, 2.0f * PI) * kThrash;
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
    const float pull = FightPullTolerance(fight.frames), hold = pull + kHoldTolerance - kPullTolerance;
    part(block - hold, block - pull, kHold);
    part(block - pull, block + pull, kBlock);
    part(block + pull, block + hold, kHold);
    // Past the opening, the yellow on the side of the next run's way stretches out toward it as a triangle, reaching the
    // place it puts the block (the run's way is the block's place on the gauge) as the run ends.
    if (fight.frames > kOpenFrames && fight.run_frames > 0 && std::fabs(fight.next - block) > hold) {
        const float side = fight.next > block ? 1.0f : -1.0f;
        const float edge = block + side * hold;
        const float done = 1.0f - static_cast<float>(fight.run) / static_cast<float>(fight.run_frames);
        const float tip = std::clamp(edge + (fight.next - edge) * done, -kCone, kCone);
        if ((tip - edge) * side > 0.0f) {
            Wedge(quads, edge, tip, inner, outer, kHold);
        }
    }
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

float FishingFightNextWay() {
    return fight.fish != nullptr ? fight.next : 0.0f;
}

float FishingFightTarget() {
    return fight.fish != nullptr ? fight.target : 0.0f;
}

} // namespace expanded
