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
#include "expanded/town/fish_kinds.hpp"
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
                                                     .help = "\"Fishing Fights\"\nA struck fish runs;\npoint the stick at\nthe gauge's mark. Off,\nit is always caught.",
                                                     .on = true});

// A fish that got away swims off for this long before it goes back to its own swimming.
constexpr int kGotAwayFrames = 180;

// The rumble while the stick pulls and while it does not; a fish's run ends in a ripple this wide over it.
constexpr int   kPullRumble = 70, kSlackRumble = 120, kRumbleFrames = 4;
constexpr float kSwitchRipple = 10.0f;

// A run's fish slows within kArrive of its end, to no slower than kThrashSpeed; it thrashes kThrash of the game's
// fight thrash (a heading anywhere) either side of its run's heading, inside the cone.
constexpr float kArrive = 3.0f, kThrashSpeed = 0.15f, kThrash = 1.0f / 3.0f;

// The bar, in pixels and field rows: kBarWidth wide, centred at the top of the screen, its track kInset inside.
constexpr int kBarWidth = 240, kBarX = 320 - kBarWidth / 2, kBarY = 12, kBarHeight = 4, kInset = 1;

// The gauge's arc is drawn in kGaugeSteps pieces; the mark is kMarkWidth (radians) either side of the way to pull.
constexpr int   kGaugeSteps = 24;
constexpr float kMarkWidth = 0.04f;

struct Fight {
    CFish        *fish = nullptr; // null: no fight going on
    sceVu0FVECTOR anchor = {};    // where it was hooked
    sceVu0FVECTOR gauge = {};     // the gauge's centre: on the water, following the float
    float         angler[2] = {}; // where the angler stands (x, z)
    float         camera = 0.0f;  // the way the camera looks across the water (a yaw)
    float         right[2] = {};  // the angler's right across the water (x, z), as he looks at the fish: the side 1 runs to
    float         middle = 0.0f;  // the gauge's middle: the way from its centre to the angler
    float         target = 0.0f;  // the way to pull, as eased
    float         stick = 0.0f;   // the way the stick last pointed across the water
    float         push = 0.0f;    // how far it is pushed
    float         strength = 0.0f;
    float         bar = 0.0f;
    int           side = 0;        // -1 the angler's left, 1 the right
    int           run = 0;         // the frames left of the run
    int           grace = 0;       // the frames left that the bar holds
    int           frames = 0;      // the fight's frames
    int           pulled = 0;      // the frames of this run that pulled
    bool          aligned = false; // the stick points the way to pull, pushed or not
    bool          pulls = false;
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

const char *SideName(int side) {
    return side < 0 ? "left" : "right";
}

void StartRun(int side) {
    fight.side = side;
    fight.run = FightRunFrames(fight.strength, GameFraction());
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
void Box(int x, int y, int width, int height, u_char r, u_char g, u_char b) {
    CRect_i_ box;
    box.x = x * 16;
    box.y = y * 16;
    box.width = width * 16;
    box.height = height * 16;
    MGFillBox(box, r, g, b, 0x80);
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

} // namespace

bool FishingFightsOn() {
    return OptionOn(kFishingFights);
}

float FightStrength(int kind, float size, const FishInfo &info) {
    float range = info.max_size - info.min_size;
    float grown = range > 0.0f ? std::clamp((size - info.min_size) / range, 0.0f, 1.0f) : 0.0f;
    return grown + (std::ranges::contains(fish::kGarayans, kind) ? 1.0f : 0.0f);
}

float FightFill(float strength) {
    return 1.0f / (static_cast<float>(kFillFrames) * (1.0f + kFillPerStrength * strength));
}

int FightRunFrames(float strength, float roll) {
    float frames = static_cast<float>(kShortestRun) + std::clamp(roll, 0.0f, 1.0f) * static_cast<float>(kLongestRun - kShortestRun);
    return std::max(1, static_cast<int>(frames * (1.0f - kRunsPerStrength * strength)));
}

float FightRunSpeed(float strength) {
    return kRunSpeed * kFightPace * (1.0f + kSpeedPerStrength * strength);
}

float FightConeYaw(float away, float yaw) {
    float off = std::remainder(yaw - away, 2.0f * PI);
    return away + std::clamp(off, -kCone, kCone);
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

float FightStickAxis(float read) {
    if (read == 0.0f) {
        return 0.0f;
    }
    return std::copysign(std::min(1.0f, kAxisDead + std::fabs(read) * (1.0f - kAxisDead)), read);
}

bool FightPulls(float x, float y, float camera_yaw, float target) {
    const float way = FightStickYaw(camera_yaw, FightStickAxis(x), FightStickAxis(y));
    return std::hypot(x, y) > kPullBack && std::fabs(std::remainder(way - target, 2.0f * PI)) <= kPullTolerance;
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
    fight.strength = FightStrength(fish.fish_kind, fish.size, fish.info);
    fight.bar = kStartBar;
    fight.grace = kStartGrace;
    StartRun(GameFraction() < 0.5f ? -1 : 1);
    if (Chara != nullptr) {
        sceVu0FVECTOR angler;
        Chara->GetPosition(angler);
        fight.angler[0] = angler[0];
        fight.angler[1] = angler[2];
    }
    // The runs go square to the way from the angler to where the fish was hooked (with the camera behind him, the
    // screen's left and right), whichever way the camera turns after.
    const float out_x = fight.anchor[0] - fight.angler[0], out_z = fight.anchor[2] - fight.angler[1];
    const float out_length = std::hypot(out_x, out_z);
    const float out_yaw = out_length > 0.0f ? atan2f(out_x, out_z) : fight.camera;
    fight.right[0] = -std::cos(out_yaw);
    fight.right[1] = std::sin(out_yaw);
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
                                  "\"from_rod\":{:.1f},\"across\":{:.1f},\"side\":\"{}\",\"run\":{}",
                                  fish.fish_kind, static_cast<int>(fish.size * 10.0f), fight.strength, out, line, line * kLineReach,
                                  std::sqrt(dx * dx + dy * dy + dz * dz), std::hypot(dx, dz), SideName(fight.side), fight.run));
}

FightStep FishingFightStep(CCharacter &angler, float *facing) {
    if (fight.fish == nullptr) {
        return FightStep::kRetail;
    }
    return FishingFightStep(angler, facing, GamePad.GetLXf(), GamePad.GetLYf());
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
        StartRun(-fight.side);
        fight.pulled = 0;
        fight.grace = kSwitchGrace;
        sceVu0FVECTOR ripple;
        fight.fish->GetPosition(ripple);
        ripple[1] = FishingGetWaterLevel();
        EffectHamon(&EdEffectGroup, ripple, kSwitchRipple);
        LogFight("switch", std::format("\"side\":\"{}\",\"run\":{},\"pulled\":{},{}", SideName(fight.side), fight.run, pulled, Ways()));
    }

    // The stick's way across the water, kept while it rests; it pulls pushed far enough within the tolerance.
    fight.push = std::hypot(stick_x, stick_y);
    if (fight.push > 0.0f) {
        fight.stick = FightStickYaw(fight.camera, FightStickAxis(stick_x), FightStickAxis(stick_y));
    }
    fight.aligned = fight.push > 0.0f && std::fabs(std::remainder(fight.stick - fight.target, 2.0f * PI)) <= kPullTolerance;
    fight.pulls = DebugMode != 0 || FightPulls(stick_x, stick_y, fight.camera, fight.target);
    fight.pulled += fight.pulls ? 1 : 0;
    if (fight.grace > 0) {
        fight.grace--;
    } else {
        float fill = FightFill(fight.strength);
        fight.bar = std::clamp(fight.bar + (fight.pulls ? fill : -kDrain * fill), 0.0f, 1.0f);
    }
    GamePad.SetVibration(1, fight.pulls ? kPullRumble : kSlackRumble, kRumbleFrames);

    if (fight.bar >= 1.0f) {
        fight.won = true;
        LogFight("landed", std::format("\"kind\":{},{}", fight.fish->fish_kind, Ways()));
        return FightStep::kLanded;
    }
    if (fight.bar <= 0.0f) {
        GotAway();
        return FightStep::kGotAway;
    }
    return FightStep::kFighting;
}

float FishFightSwim(CFish &fish) {
    if (&fish != fight.fish) {
        return 1.0f;
    }
    // The run's end, kRunWidth to the angler's side of where it was hooked: the fish heads for it as far as the cone lets
    // it, so it goes that way even where it can only slide along the line's arc.
    float         to_x = fight.anchor[0] + static_cast<float>(fight.side) * kRunWidth * fight.right[0];
    float         to_z = fight.anchor[2] + static_cast<float>(fight.side) * kRunWidth * fight.right[1];
    sceVu0FVECTOR here;
    fish.GetPosition(here);
    float dx = to_x - here[0], dz = to_z - here[2];
    float distance = std::sqrt(dx * dx + dz * dz);
    float away = atan2f(here[0] - fight.angler[0], here[2] - fight.angler[1]);
    float heading = FightConeYaw(away, distance > 0.0f ? atan2f(dx, dz) : away);
    // The game's thrash, a new heading every frame (the battle block's roll, target_yaw), narrowed to kThrash of it either
    // side of the run's heading and kept in the cone: the fish twists and struggles as it goes, never toward the angler.
    float thrash = std::remainder(fish.target_yaw, 2.0f * PI) * kThrash;
    fish.target_yaw = AngleLimit(FightConeYaw(away, heading + thrash));
    fish.target_speed = std::max(kThrashSpeed * kFightPace, FightRunSpeed(fight.strength) * std::min(1.0f, distance / kArrive));
    return kFightPace;
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
    // As Georama's parts gauge, thin at the top: a dark frame, a grey track, the bar orange while the stick pulls (and
    // while it holds), red while it does not.
    const int track = kBarWidth - 2 * kInset;
    Box(kBarX, kBarY, kBarWidth, kBarHeight, 0x14, 0x14, 0x14);
    Box(kBarX + kInset, kBarY + 1, track, kBarHeight - 2, 0x64, 0x64, 0x64);
    const int filled = static_cast<int>(fight.bar * static_cast<float>(track));
    if (fight.pulls || fight.grace > 0) {
        Box(kBarX + kInset, kBarY + 1, filled, kBarHeight - 2, 0xB4, 0x64, 0x28);
    } else {
        Box(kBarX + kInset, kBarY + 1, filled, kBarHeight - 2, 0xB4, 0x20, 0x18);
    }

    // The gauge on the water, drawn over everything: its arc, the stick's block on it (glowing while it pulls, plain
    // while it points the right way unpushed, dim otherwise; held inside the arc) and the mark for the way to pull.
    sceVu0FVECTOR centre;
    sceVu0CopyVector(centre, fight.gauge);
    int screen[4] = {};
    if (MGRotTransPers2D(screen, centre, 0) == 0) {
        return;
    }
    constexpr u_char           kArc[4] = {0x18, 0x1C, 0x24, 0x60};
    constexpr u_char           kGlow[4] = {0xFF, 0xB0, 0x40, 0x80};
    constexpr u_char           kPlain[4] = {0xB4, 0x64, 0x28, 0x70};
    constexpr u_char           kDim[4] = {0x60, 0x60, 0x60, 0x40};
    constexpr u_char           kMark[4] = {0xF0, 0xF0, 0xF0, 0x80};
    const float                inner = kGaugeRadius - kGaugeBand / 2.0f, outer = kGaugeRadius + kGaugeBand / 2.0f;
    std::vector<gfx::Vertex2D> quads;
    Band(quads, -kCone, kCone, inner, outer, kArc);
    const float   block = std::clamp(std::remainder(fight.stick - fight.middle, 2.0f * PI) - kPullTolerance, -kCone, kCone - 2.0f * kPullTolerance);
    const u_char *shade = fight.pulls ? kGlow : (fight.aligned ? kPlain : kDim);
    Band(quads, block, block + 2.0f * kPullTolerance, inner, outer, shade);
    const float mark = std::remainder(FightConeYaw(fight.middle, fight.target) - fight.middle, 2.0f * PI);
    Band(quads, mark - kMarkWidth, mark + kMarkWidth, inner - kMarkOverhang, outer + kMarkOverhang, kMark);
    draw2d::DrawUntextured(gfx::Primitive::Quads, quads, draw2d::SpriteState());
    draw2d::RestoreTestZbuf();
}

float FishingFightBar() {
    return fight.fish != nullptr ? fight.bar : 0.0f;
}

int FishingFightSide() {
    return fight.fish != nullptr ? fight.side : 0;
}

float FishingFightTarget() {
    return fight.fish != nullptr ? fight.target : 0.0f;
}

} // namespace expanded
