#include "expanded/weapons/xiao/firing_legs.hpp"
#include "mathconst.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string_view>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/motion_layer.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/xiao/moves_while_firing.hpp"
#include "gamepad.hpp"
#include "gameutil.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// The chest and everything on it keep the shot; the chest and the joint under it share the waist's turn.
constexpr const char *kSeam = "jnt2_1";
constexpr const char *kSeamNext = "eff2";
// The upper body holds the shot's pose in the world while the hips sway under it.
constexpr float kTorsoTurn = 0.0f;
constexpr float kFadeStep = 1.0f / static_cast<float>(kLegFadeFrames);

// MoveChara's bands: within kAhead of her facing she goes forward, past kBehind back, between them aside.
constexpr float kAhead = 1.2f;
constexpr float kBehind = 2.6f;

struct Legs {
    bool        on = false;
    CCharacter *chara = nullptr;
    CFrame     *root = nullptr;
    Mot_List   *tracks = nullptr;
    int         key = -1;
    float       rate = 0.0f;
    bool        handing = false;
    MotionLayer layer;
};

Legs legs;

const MOTION_INFO *KeyInfo(CCharacter &chara, int key, int *set_no = nullptr) {
    int             index = -1;
    tagMOTION_TYPE *motion = chara.GetMotionParam(key, &index, nullptr, nullptr, set_no);
    return motion != nullptr && motion->motion_info != nullptr && index >= 0 ? &motion->motion_info[index] : nullptr;
}

// time stepped on by rate, wrapping at the key's end as SetMotionEX does.
float Advance(const MOTION_INFO &info, float time, float rate) {
    time += rate;
    if (static_cast<unsigned int>(time) >= info.end) {
        time -= static_cast<float>(info.end) - static_cast<float>(info.start);
    }
    return time;
}

// Where a move into key starts, from a step of was at time: the run and the walk where the step was in its own (as
// CCharacter::Step hands them over, NextMotionTime_GET_EX and 2 frames on), anything else at its first frame.
float MoveStart(const MOTION_INFO &was, const MOTION_INFO &info, int key, float time) {
    if (key != kMoveRun && key != kMoveWalk) {
        return static_cast<float>(info.start);
    }
    const float progress = (time - static_cast<float>(was.start)) / static_cast<float>(was.end - was.start);
    const int   start = static_cast<int>(info.start), end = static_cast<int>(info.end);
    const int   at = std::clamp(static_cast<int>(static_cast<float>(start) + progress * static_cast<float>(end - start)), start, end);
    return static_cast<float>(at + 2);
}

void Log(std::string_view step) {
    LogEvent("firing_legs", std::format("\"step\":\"{}\",\"key\":{},\"time\":{:.2f},\"weight\":{:.2f}", step, legs.key, legs.layer.time, legs.layer.weight));
}

void Lift(bool restore) {
    if (legs.chara != nullptr) {
        LiftMotion(&legs.chara->motion_type, restore);
        LiftMotion(&legs.chara->shadow_motion_type, restore);
    }
    legs = {};
}

// Whether a step of a clock from one frame to another passes a sound of set_no's at frame, by CCharacter::Step's rule.
bool Passes(int frame, int motion_no, int set_no, float from, float to) {
    const auto at = static_cast<float>(frame);
    return std::fabs(to - from) < 1.0f && at >= 0.0f && motion_no == set_no && at >= from && at < to;
}

// The step's sounds, as CCharacter::Step plays its own clock's.
void PlaySounds(CCharacter &chara, int key, float from, float to) {
    int set_no = 0;
    if (KeyInfo(chara, key, &set_no) == nullptr) {
        return;
    }
    sceVu0FVECTOR position;
    chara.GetWorldPosition(position);
    for (const CHARA_FOOT_SOUND &sound : chara.foot_sound) {
        if (chara.foot_sound_enable == 0 || chara.foot_sound_id < 0 || chara.foot_sound_wait != 0) {
            break;
        }
        if (Passes(sound.frame, sound.motion_no, set_no, from, to)) {
            SndPlayFootSound(chara.foot_sound_id, sound.foot, position);
            chara.foot_sound_wait = 5;
        }
    }
    for (const CHARA_EVENT &event : chara.event) {
        if (chara.event_enable == 0 || !Passes(event.frame, event.motion_no, set_no, from, to)) {
            continue;
        }
        if (static_cast<CharaEventKind>(event.kind) == CharaEventKind::FootSound && chara.foot_sound_enable != 0) {
            SndPlayFootSound(chara.foot_sound_id, event.no, position);
        } else if (static_cast<CharaEventKind>(event.kind) == CharaEventKind::SoundEffect) {
            SndSePlay(event.no, -1, 0);
        }
    }
}

// The legs' frame when the body's cross-fade into their key, starting this step, is done: SetMotionEX adds blend_step a
// step (all of it on a restart) and lands on the key the step its blend reaches 1.
float Arrival(const CCharacter &chara, const MOTION_INFO &info) {
    const float step = (chara.motion_flags & 4) != 0 ? 1.0f : chara.motion_type.state.blend_step;
    float       time = legs.layer.time, blend = 0.0f;
    for (int i = 0; i < 64 && step > 0.0f; i++) {
        blend += step;
        if (!(blend < 1.0f)) {
            break;
        }
        time = Advance(info, time, legs.rate);
    }
    return time;
}

} // namespace

LegMove GameLegMove(float yaw, float move_x, float move_z, float stick, bool locked, float lx, float ly, float run_rate) {
    float turn = yaw - std::atan2(move_x, move_z);
    if (turn < -PI_SHORT) {
        turn += TWO_PI_SHORT;
    }
    if (turn > PI_SHORT) {
        turn -= TWO_PI_SHORT;
    }
    const float strafe = std::min(1.0f, 0.2f + stick);
    if (turn > -kAhead && turn < kAhead) {
        if (locked) {
            return {kStrafeForward, strafe};
        }
        const float x = std::fabs(lx), y = std::fabs(ly);
        if (x + y >= 0.85f) {
            return {kMoveRun, run_rate};
        }
        return {kMoveWalk, std::min(0.7f, 0.8f * (0.1f + std::max(x, y)))};
    }
    if (turn < -kBehind || turn > kBehind) {
        return {kStrafeBack, strafe};
    }
    if (turn > kAhead && turn < kBehind) {
        return {kStrafeRight, strafe};
    }
    if (turn < -kAhead && turn > -kBehind) {
        return {kStrafeLeft, strafe};
    }
    return {kLockOnStance, strafe};
}

std::vector<int> SoundFramesPassed(const CCharacter &chara, int set_no, float from, float to) {
    std::vector<int> frames;
    for (const CHARA_FOOT_SOUND &sound : chara.foot_sound) {
        if (Passes(sound.frame, sound.motion_no, set_no, from, to)) {
            frames.push_back(sound.frame);
        }
    }
    for (const CHARA_EVENT &event : chara.event) {
        if (Passes(event.frame, event.motion_no, set_no, from, to)) {
            frames.push_back(event.frame);
        }
    }
    return frames;
}

void FiringLegsStep(CCharacter &chara, int chara_no) {
    if (legs.on && (chara_no != CHARA_XIAO || &chara != legs.chara || chara.frame != legs.root || chara.motion_type.proc_list != legs.tracks)) {
        Log("gone");
        Lift(false);
    }
    if (chara_no != CHARA_XIAO || chara.frame == nullptr || chara.motion_type.motion_info == nullptr) {
        return;
    }
    if ((chara.motion_flags & 1) != 0 || CCharacter::MotionStopFlag != 0) {
        return;
    }
    const bool    stepping = BtActStatus.can_act != 0 && BtActStatus.stick_strength > 0.0f;
    const bool    walks = stepping && FiringLeavesMove() && CharaMainHandViewFlag == 0;
    MOTION_STATE &state = chara.motion_type.state;
    LegMove       move = {legs.key, legs.rate};
    if (stepping) {
        const MOTION_INFO *run = KeyInfo(chara, kMoveRun);
        float              rotation[4];
        chara.GetRotation(rotation);
        move = GameLegMove(rotation[1], BtActStatus.move_x, BtActStatus.move_z, BtActStatus.stick_strength, lockOnTargetFlag != 0,
                           GamePad.GetLXf(), GamePad.GetLYf(), run != nullptr ? run->speed : 0.0f);
    }

    if (!legs.on) {
        const MOTION_INFO *info = walks ? KeyInfo(chara, move.key) : nullptr;
        if (info == nullptr) {
            return;
        }
        legs.on = true;
        legs.chara = &chara;
        legs.root = chara.frame;
        legs.tracks = chara.motion_type.proc_list;
        legs.key = move.key;
        legs.rate = move.rate;
        legs.layer.seam = kSeam;
        legs.layer.seam_next = kSeamNext;
        legs.layer.torso_turn = kTorsoTurn;
        // A step the body was playing carries on under the shot; from a standstill the legs start the move.
        if (state.playing_no == move.key && state.blending == 0) {
            legs.layer.time = state.time;
            legs.layer.weight = 1.0f;
        } else {
            legs.layer.time = static_cast<float>(info->start);
            legs.layer.weight = kFadeStep;
        }
        LayMotion(&chara.motion_type, &legs.layer);
        LayMotion(&chara.shadow_motion_type, &legs.layer);
        Log(legs.layer.weight >= 1.0f ? "carried" : "on");
        return;
    }

    // The body asks for the legs' own move (the shot over, she moves on): once its cross-fade is done it plays on alone,
    // from the legs' step.
    const bool         handing = chara.motion_no == legs.key;
    const MOTION_INFO *info = KeyInfo(chara, legs.key);
    if (info == nullptr) {
        Lift(true);
        return;
    }
    if (handing && state.playing_no == legs.key && state.blending == 0) {
        state.time = Advance(*info, legs.layer.time, legs.rate);
        Log("handed");
        Lift(true);
        return;
    }

    // A new way to go: the legs cross-fade from the old step, held, into the new move.
    if (walks && move.key != legs.key) {
        const MOTION_INFO *next = KeyInfo(chara, move.key);
        if (next != nullptr) {
            legs.layer.from_time = legs.layer.time;
            legs.layer.from_weight = 0.0f;
            legs.layer.time = MoveStart(*info, *next, move.key, legs.layer.time);
            legs.key = move.key;
            info = next;
            Log("turn");
        }
    }
    if (stepping && move.key == legs.key) {
        legs.rate = move.rate;
    }

    const float from = legs.layer.time;
    legs.layer.time = Advance(*info, from, legs.rate);
    if (legs.layer.from_time >= 0.0f) {
        legs.layer.from_weight = std::min(1.0f, legs.layer.from_weight + kFadeStep);
        if (legs.layer.from_weight >= 1.0f) {
            legs.layer.from_time = -1.0f;
        }
    }
    legs.layer.weight = walks || handing ? std::min(1.0f, legs.layer.weight + kFadeStep) : std::max(0.0f, legs.layer.weight - kFadeStep);
    if (legs.layer.weight <= 0.0f) {
        Log("off");
        Lift(true);
        return;
    }
    if (legs.layer.weight >= 0.5f) {
        PlaySounds(chara, legs.key, from, legs.layer.time);
    }
    legs.layer.carry_key = handing ? legs.key : -1;
    if (handing) {
        legs.layer.carry_frame = Arrival(chara, *info);
        if (!legs.handing) {
            Log("handing");
        }
    }
    legs.handing = handing;
}

const MotionLayer *FiringLegs() {
    return legs.on ? &legs.layer : nullptr;
}

int FiringLegsKey() {
    return legs.on ? legs.key : -1;
}

} // namespace expanded
