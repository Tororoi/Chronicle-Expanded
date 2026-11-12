#include "expanded/weapons/xiao/rapid_fire.hpp"

#include <algorithm>
#include <format>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/motion_layer.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "gamepad.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"

namespace expanded {

namespace {

constexpr float kStill = 0.001f; // a draw held where it is (a speed of 0 is the motion's own)

struct Stream {
    bool on = false;    // the hold went on into the shot with X held
    bool fired = false; // the shot in play has let its pellet go
    int  after = 0;     // frames of follow-through since
    int  drawn = 0;     // frames held at full draw
    int  shots = 0;
};

Stream stream;

[[maybe_unused]] const bool kRegistered = (AddEnemyFloorListener([] { stream = {}; }), true);

bool Shooting() {
    return Playing(CHARA_XIAO) && BtActStatus.action_on == 1 &&
           (BtActStatus.action_no == kXiaoDraw || BtActStatus.action_no == kXiaoHold || BtActStatus.action_no == kXiaoShot);
}

// Plays motion_no from its first frame on the next step, cut to rather than blended into, as the game goes into a shot.
void CutTo(int motion_no) {
    BtActStatus.action_no = motion_no;
    BtActStatus.motion_no = motion_no;
    driveNoInterpolate = 1;
}

// A character's draw: its motion set, the key's index in the set and the key's first frame; no set where it has none.
struct Draw {
    tagMOTION_TYPE *motion = nullptr;
    int             key = -1;
    float           start = 0.0f;
};

Draw DrawOf(CCharacter *chara) {
    Draw            draw;
    int             index = -1;
    tagMOTION_TYPE *motion = chara != nullptr ? chara->GetMotionParam(kXiaoDraw, &index, nullptr, nullptr, nullptr) : nullptr;
    if (motion != nullptr && motion->motion_info != nullptr && index >= 0) {
        draw = {.motion = motion, .key = index, .start = static_cast<float>(motion->motion_info[index].start)};
    }
    return draw;
}

// Back into the draw: the game's fade between motions, landing kRapidPullFrom frames in, for her and her slingshot.
void PullBack(CCharacter &chara) {
    BtActStatus.action_step = 0;
    BtActStatus.action_no = kXiaoDraw;
    BtActStatus.motion_no = kXiaoDraw;
    for (CCharacter *model : {&chara, NowWeapon}) {
        Draw draw = DrawOf(model);
        if (draw.motion != nullptr) {
            CarryMotion(draw.motion, draw.key, static_cast<unsigned int>(draw.start) + kRapidPullFrom, kRapidBlendStep);
        }
    }
}

void DrawSpeed(CCharacter &chara, float speed) {
    chara.motion_speed = speed;
    if (NowWeapon != nullptr) {
        NowWeapon->motion_speed = speed;
    }
}

} // namespace

bool RapidFireSphere() {
    return SuperSteveSphere() == ITEM_WEAPON_MACHINE_GUN;
}

bool RapidFiring() {
    return stream.on;
}

int RapidFireShots() {
    return stream.shots;
}

bool RapidFireShot(CCharacter &chara) {
    if (!Shooting() || !RapidFireSphere()) {
        stream = {};
        return false;
    }
    bool held = GamePad.On(PadInput_OK) != 0;

    // Drawn with X held: let go once held kRapidHold frames.
    if (BtActStatus.action_no == kXiaoHold && held) {
        if (stream.drawn < kRapidHold) {
            stream.drawn++;
            return false;
        }
        stream.drawn = 0;
        stream.on = true;
        stream.fired = false;
        BtActStatus.action_step = 1;
        CutTo(kXiaoShot);
        return false;
    }
    if (!stream.on) {
        return false;
    }

    // The pellet gone and its follow-through played with X still held: pull back again.
    if (BtActStatus.action_no == kXiaoShot && stream.fired) {
        if (stream.after < kRapidFollowThrough) {
            stream.after++;
        } else if (held) {
            stream.fired = false;
            PullBack(chara);
        }
        return false;
    }

    if (BtActStatus.action_no != kXiaoDraw) {
        return false;
    }
    if (!held) {
        LogEvent("rapid_fire", std::format("\"step\":\"let_go\",\"shots\":{}", stream.shots));
        stream = {};
        return true;
    }
    Draw draw = DrawOf(&chara);
    if (draw.motion == nullptr) {
        DrawSpeed(chara, kRapidDrawSpeed);
        return false;
    }
    // Still fading in, the draw waits; at full draw she lets go; short of it, the pull lands on it exactly.
    const MOTION_STATE &state = draw.motion->state;
    if (state.blending != 0 || state.playing_no != draw.key) {
        return false;
    }
    float to = draw.start + kRapidPullTo;
    if (state.time >= to - 0.01f) {
        // Held at full draw (the game turns the draw into its hold there), then let go.
        if (stream.drawn < kRapidHold) {
            stream.drawn++;
            DrawSpeed(chara, kStill);
            return false;
        }
        stream.drawn = 0;
        BtActStatus.action_step = 1;
        CutTo(kXiaoShot);
        return false;
    }
    DrawSpeed(chara, std::min(kRapidDrawSpeed, to - state.time));
    return false;
}

void RapidFirePellet(CSHOT &shots, int slot) {
    if (!stream.on) {
        return;
    }
    stream.fired = true;
    stream.after = 0;
    stream.shots++;
    LogEvent("rapid_fire", std::format("\"step\":\"shot\",\"shots\":{},\"damage\":{}", stream.shots, shots.damage[slot]));
}

float RapidFireWear(float wear) {
    return stream.on ? wear * kRapidFireWear : wear;
}

void RapidFireFrame() {
    if (stream.on && !Shooting()) {
        stream = {};
    }
}

} // namespace expanded
