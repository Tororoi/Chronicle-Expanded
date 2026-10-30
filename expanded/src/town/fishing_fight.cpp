#include "expanded/town/fishing_fight.hpp"

#include <libvu0.h>

#include <algorithm>
#include <cmath>
#include <format>

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

namespace expanded {

namespace {

[[maybe_unused]] const bool kRegistered = AddToggle({.key = kFishingFights,
                                                     .label = "Fishing Fights",
                                                     .help = "\"Fishing Fights\"\nA struck fish runs;\npull the stick back,\naway from it. Off,\nit is always caught.",
                                                     .on = true});

// A fish that got away swims off for this long before it goes back to its own swimming.
constexpr int kGotAwayFrames = 180;

// The rumble while the stick pulls and while it does not; a fish's run ends in a ripple this wide over it.
constexpr int   kPullRumble = 70, kSlackRumble = 120, kRumbleFrames = 4;
constexpr float kSwitchRipple = 10.0f;

// A run's fish slows within kArrive of its end, to no slower than kThrashSpeed; it thrashes kThrash of the game's
// fight thrash (a heading anywhere) either side of its run's heading, inside the cone.
constexpr float kArrive = 3.0f, kThrashSpeed = 0.15f, kThrash = 1.0f / 3.0f;

// The bar, in pixels and field rows: kBarWidth wide, centred low on the screen, its track kInset inside.
constexpr int kBarWidth = 160, kBarX = 320 - kBarWidth / 2, kBarY = 200, kBarHeight = 7, kInset = 2;

struct Fight {
    CFish        *fish = nullptr; // null: no fight going on
    sceVu0FVECTOR anchor = {};    // where it was hooked
    float         angler[2] = {}; // where the angler stands (x, z)
    float         right[2] = {};  // the screen's right across the water (x, z): the side 1 runs to
    float         strength = 0.0f;
    float         bar = 0.0f;
    int           side = 0;   // -1 the angler's left, 1 the right
    int           run = 0;    // the frames left of the run
    int           grace = 0;  // the frames left that the bar holds
    int           frames = 0; // the fight's frames
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

// The screen's right across the water: square to the way the camera looks, or without one, to the way the angler faces.
void LookRight() {
    if (NowCamera != nullptr) {
        sceVu0FVECTOR eye;
        sceVu0FVECTOR look;
        NowCamera->GetPos(eye);
        NowCamera->GetRef(look);
        float ahead_x = look[0] - eye[0], ahead_z = look[2] - eye[2];
        float length = std::hypot(ahead_x, ahead_z);
        if (length > 0.0f) {
            fight.right[0] = -ahead_z / length;
            fight.right[1] = ahead_x / length;
            return;
        }
    }
    float yaw = Chara != nullptr ? Chara->GetRotation()->y : 0.0f;
    fight.right[0] = -std::cos(yaw);
    fight.right[1] = std::sin(yaw);
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
    LogFight("got_away", std::format("\"kind\":{}", fish.fish_kind));
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
    float dy = position[1] - rod[1];
    if (!(std::fabs(dy) < reach)) {
        return false;
    }
    float across = std::sqrt(reach * reach - dy * dy);
    float dx = position[0] - rod[0], dz = position[2] - rod[2];
    float distance = std::hypot(dx, dz);
    if (!(distance > across)) {
        return false;
    }
    position[0] = rod[0] + dx * across / distance;
    position[2] = rod[2] + dz * across / distance;
    return true;
}

bool FightPulls(float x, float y, int side) {
    return y > kPullBack && -static_cast<float>(side) * x > kPushAside;
}

void FishingFightStarted(CFish &fish) {
    fight = {};
    if (!FishingFightsOn()) {
        return;
    }
    fight.fish = &fish;
    fish.GetPosition(fight.anchor);
    LookRight();
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
    const float out = std::hypot(fight.anchor[0] - fight.angler[0], fight.anchor[2] - fight.angler[1]);
    const float line = FightLineLength();
    float       dx = fight.anchor[0] - rod_top[0], dy = fight.anchor[1] - rod_top[1], dz = fight.anchor[2] - rod_top[2];
    LogFight("start", std::format("\"kind\":{},\"size\":{},\"strength\":{:.2f},\"out\":{:.0f},\"line\":{:.1f},\"cap\":{:.1f},"
                                  "\"from_rod\":{:.1f},\"side\":\"{}\",\"run\":{}",
                                  fish.fish_kind, static_cast<int>(fish.size * 10.0f), fight.strength, out, line, line * kLineStretch,
                                  std::sqrt(dx * dx + dy * dy + dz * dz), SideName(fight.side), fight.run));
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
    // The angler turns as while waiting for a bite.
    facing[1] = AngleInterpolate(facing[1], facing[1] - 0.1f * stick_x, 0.01f, INTERPOLATE_STEP);
    angler.SetRotation(facing);
    if (fight.won) {
        return FightStep::kLanded;
    }

    fight.frames++;
    LookRight();
    if (--fight.run <= 0) {
        StartRun(-fight.side);
        fight.grace = kSwitchGrace;
        sceVu0FVECTOR ripple;
        fight.fish->GetPosition(ripple);
        ripple[1] = FishingGetWaterLevel();
        EffectHamon(&EdEffectGroup, ripple, kSwitchRipple);
        LogFight("switch", std::format("\"side\":\"{}\",\"run\":{}", SideName(fight.side), fight.run));
    }

    fight.pulls = DebugMode != 0 || FightPulls(stick_x, stick_y, fight.side);
    if (fight.grace > 0) {
        fight.grace--;
    } else {
        float fill = FightFill(fight.strength);
        fight.bar = std::clamp(fight.bar + (fight.pulls ? fill : -kDrain * fill), 0.0f, 1.0f);
    }
    GamePad.SetVibration(1, fight.pulls ? kPullRumble : kSlackRumble, kRumbleFrames);

    if (fight.bar >= 1.0f) {
        fight.won = true;
        LogFight("landed", std::format("\"kind\":{}", fight.fish->fish_kind));
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
    // The run's end, kRunWidth to the screen's side of where it was hooked: the fish heads for it as far as the cone
    // lets it, so it goes the way the screen shows its side even where it can only slide along the line's arc.
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

void FishFightHold(CFish &fish, CCPoly *polys, int count) {
    if (&fish != fight.fish) {
        return;
    }
    sceVu0FVECTOR position;
    fish.GetPosition(position);
    sceVu0FVECTOR held;
    sceVu0CopyVector(held, position);
    if (!FightLineHold(held, rod_top, FightLineLength() * kLineStretch)) {
        return;
    }
    // Drawn in toward the rod, unless a wall is in the way: then it stays where its collision left it.
    sceVu0FVECTOR hit;
    if (polys == nullptr || CheckHit(polys, count, position, held, hit, 0, 0) < 0) {
        fish.SetPosition(held);
    }
}

void DrawFishingFight() {
    if (fight.fish == nullptr || GameMode != ED_MODE_FISHING || chara_fishing != ED_FISHING_BATTLE) {
        return;
    }
    // As Georama's parts gauge: a dark frame, a grey track, the bar orange while the stick pulls, red while it does not.
    const int track = kBarWidth - 2 * kInset;
    Box(kBarX, kBarY, kBarWidth, kBarHeight, 0x14, 0x14, 0x14);
    Box(kBarX + kInset, kBarY + 1, track, kBarHeight - 2, 0x64, 0x64, 0x64);
    const int filled = static_cast<int>(fight.bar * static_cast<float>(track));
    if (fight.pulls || fight.grace > 0) {
        Box(kBarX + kInset, kBarY + 1, filled, kBarHeight - 2, 0xB4, 0x64, 0x28);
    } else {
        Box(kBarX + kInset, kBarY + 1, filled, kBarHeight - 2, 0xB4, 0x20, 0x18);
    }
}

float FishingFightBar() {
    return fight.fish != nullptr ? fight.bar : 0.0f;
}

int FishingFightSide() {
    return fight.fish != nullptr ? fight.side : 0;
}

} // namespace expanded
