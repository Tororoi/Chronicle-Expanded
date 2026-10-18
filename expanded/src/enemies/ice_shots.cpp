#include "expanded/enemies/ice_shots.hpp"

#include <cmath>

#include "bt_shot_effect.hpp"
#include "dun/gameloop.hpp"
#include "mathutil.hpp"
#include "shot_effect.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr float kArrowSpeed = 1.2f;
constexpr float kArrowTurn = 0.12f;
constexpr float kArrowCone = 0.5f;
constexpr float kChest = 10.1f;
constexpr float kShatterSpeed = 0.5f;
constexpr int   kFreezeSounds[] = {1772, 1777};
constexpr int   kShatterSound = 1776;

} // namespace

BT_SHOT_EFFECT IcePrisonShot() {
    // clang-format off
    return {"info", 1, 0, {}, {-100.0f, -100.0f, -100.0f, -100.0f}, 1800, 0, 0, 2, 1, {0, 0, 1, 1}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
    // clang-format on
}

void PlayerChest(float *chest) {
    sceVu0CopyVector(chest, CharaMain.pos);
    chest[1] += kChest;
    chest[3] = 1.0f;
}

void PlayerFeet(float *feet) {
    sceVu0CopyVector(feet, CharaMain.pos);
    feet[3] = 1.0f;
}

bool PlayerFrozen() {
    return UserStatus != nullptr && (UserStatus->ailments[UserStatus->cur_chara] & AILMENT_FREEZE) != 0;
}

bool PlayerAfflicted() {
    return UserStatus != nullptr && UserStatus->ailment_frames[UserStatus->cur_chara] > 0;
}

void SteerIceArrow(CSHOT_EFFECT &effect, int slot) {
    float position[4];
    float chest[4];
    effect.chara[slot].GetPosition(position);
    PlayerChest(chest);
    float heading[4];
    float wanted[4] = {chest[0] - position[0], chest[1] - position[1], chest[2] - position[2], 1.0f};
    sceVu0Normalize(heading, effect.velocity[slot]);
    sceVu0Normalize(wanted, wanted);
    if (sceVu0InnerProduct(heading, wanted) >= kArrowCone) {
        float step[4] = {wanted[0] - heading[0], wanted[1] - heading[1], wanted[2] - heading[2], 1.0f};
        float gap = std::sqrt(step[0] * step[0] + step[1] * step[1] + step[2] * step[2]);
        if (gap <= kArrowTurn) {
            sceVu0CopyVector(heading, wanted);
        } else {
            for (int i = 0; i < 3; i++) {
                heading[i] += step[i] * (kArrowTurn / gap);
            }
            sceVu0Normalize(heading, heading);
        }
    }
    sceVu0ScaleVectorXYZ(effect.velocity[slot], heading, kArrowSpeed);
    sceVu0FMATRIX facing;
    LookAtMatrixZ(facing, effect.velocity[slot]);
    effect.chara[slot].frame->SetTransMatrix(facing);
    effect.chara[slot].SetPosition(position);
}

void EnterShotPhase(CSHOT_EFFECT &effect, int slot, int phase) {
    effect.phase[slot] = static_cast<s16>(phase);
    int motion = effect.effect_data->motion[phase];
    if (motion == -1) {
        effect.active[slot] = 0;
        return;
    }
    effect.chara[slot].motion_type.state.time = static_cast<float>(effect.chara[slot].motion_type.motion_info[motion].start);
    effect.chara[slot].SetMotion(motion, 6);
    effect.velocity[slot][0] = effect.velocity[slot][1] = effect.velocity[slot][2] = 0.0f;
}

bool HoldIcePrison(CSHOT_EFFECT &effect, int slot) {
    if (effect.active[slot] == 0 || effect.phase[slot] != 1) {
        return false;
    }
    const MOTION_INFO &formed = effect.chara[slot].motion_type.motion_info[effect.effect_data->motion[1]];
    effect.chara[slot].motion_type.state.time = static_cast<float>(formed.end) - 0.01f;
    effect.chara[slot].SetMotionSpeed(0.0f);
    return true;
}

void PlayIceFreeze() {
    for (int sound : kFreezeSounds) {
        SndSePlay(sound, -1, 0);
    }
}

void ShatterIcePrison(CSHOT_EFFECT &effect, int slot) {
    SndSePlay(kShatterSound, -1, 0);
    EnterShotPhase(effect, slot, 2);
    effect.chara[slot].SetMotionSpeed(kShatterSpeed);
}

} // namespace expanded
