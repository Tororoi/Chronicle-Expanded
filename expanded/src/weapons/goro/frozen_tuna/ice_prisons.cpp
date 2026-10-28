#include "expanded/weapons/goro/frozen_tuna/ice_prisons.hpp"

#include <cstdio>
#include <format>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/goro/frozen_tuna/prison_scales.hpp"
#include "expanded/weapons/player_state.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded::frozen_tuna {

namespace {

constexpr const char *kPrisonCfg = "info.cfg";

// A formed prison is held this short of its forming clip's end, as the Ice Queen's is (HoldIcePrison).
constexpr float kFormedHold = 0.01f;

struct Prisons {
    CCharacter *model = nullptr; // entered on this floor
    bool        failed = false;  // could not be, on this floor
    Prison      monsters[kFloorMonsters];
    Prison      member;
    bool        shattered = false; // a shatter sounded this step
};

Prisons prisons;

// The model goes with the floor's characters.
void OnFloor() {
    prisons = Prisons{};
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

CCharacter *Model() {
    if (prisons.model == nullptr && !prisons.failed) {
        CCharacter *model = EnterFloorCharacter(kIcePrisonFile, kPrisonCfg);
        if (model == nullptr || model->frame == nullptr || model->motion_type.motion_info == nullptr) {
            std::fprintf(stderr, "ice prisons: no prison, %s could not be loaded\n", kIcePrisonFile);
            prisons.failed = true;
            return nullptr;
        }
        prisons.model = model;
    }
    return prisons.model;
}

const MOTION_INFO &Clip(int key) {
    return prisons.model->motion_type.motion_info[key];
}

void Log(const Prison &prison, bool member, const char *stage) {
    int kind = member || NowMonstorUnit == nullptr ? -1 : ModelKind(NowMonstorUnit->monster[prison.body].base_model);
    LogEvent("prison", std::format("\"body\":\"{}\",\"slot\":{},\"kind\":{},\"scale\":{:.2f},\"stage\":\"{}\"", member ? "member" : "monster",
                                   prison.body, kind, prison.scale, stage));
}

bool Encase(Prison &prison, int body, float scale, bool member) {
    if (Model() == nullptr) {
        return false;
    }
    if (prison.stage == PrisonStage::kForming || prison.stage == PrisonStage::kFormed) {
        return true;
    }
    prison = {.stage = PrisonStage::kForming, .time = static_cast<float>(Clip(kIcePrisonForm).start), .scale = scale, .body = body};
    Log(prison, member, "form");
    return true;
}

void Shatter(Prison &prison, bool member) {
    prison.stage = PrisonStage::kShattering;
    prison.time = static_cast<float>(Clip(kIcePrisonShatter).start);
    if (!prisons.shattered) {
        PlayIceShatter();
        prisons.shattered = true;
    }
    Log(prison, member, "shatter");
}

void Step(Prison &prison, bool frozen, bool member) {
    switch (prison.stage) {
        case PrisonStage::kNone:
            return;
        case PrisonStage::kForming: {
            if (!frozen) {
                Shatter(prison, member);
                return;
            }
            float formed = static_cast<float>(Clip(kIcePrisonForm).end) - kFormedHold;
            prison.time += Clip(kIcePrisonForm).speed;
            if (prison.time >= formed) {
                prison.time = formed;
                prison.stage = PrisonStage::kFormed;
            }
            return;
        }
        case PrisonStage::kFormed:
            if (!frozen) {
                Shatter(prison, member);
            }
            return;
        case PrisonStage::kShattering:
            // Gone as the shot effects end their last clip, a frame short of its end.
            prison.time += kIcePrisonShatterSpeed;
            if (prison.time >= static_cast<float>(Clip(kIcePrisonShatter).end) - 1.0f) {
                prison = Prison{};
            }
            return;
    }
}

bool MonsterFrozen(int monster) {
    const MONSTOR *live = FloorMonster(monster);
    return live != nullptr && MonsterLiving(*live) && live->stop_timer > 0;
}

bool MemberFrozen(int chara) {
    return Playing(chara) && (UserStatus->ailments[chara] & AILMENT_FREEZE) != 0;
}

// Where a body's prison stands: its root, and its yaw. False with no body: a member out of play has none in sight.
bool Place(const Prison &prison, bool member, float *at, float *yaw) {
    if (member) {
        if (!Playing(prison.body)) {
            return false;
        }
        PlayerFeet(at);
        *yaw = PlayerYaw();
        return true;
    }
    if (NowMonstorUnit == nullptr) {
        return false;
    }
    CCharacter &body = NowMonstorUnit->chara[prison.body][0];
    body.GetPosition(at);
    *yaw = body.GetRotation()->y;
    return true;
}

void Draw(const Prison &prison, bool member) {
    float at[4], yaw = 0.0f;
    if (prison.stage == PrisonStage::kNone || !Place(prison, member, at, &yaw)) {
        return;
    }
    // Posed at this prison's time and held there, with no cross-fade from the clip the last one was posed in.
    int           key = prison.stage == PrisonStage::kShattering ? kIcePrisonShatter : kIcePrisonForm;
    CCharacter   &model = *prisons.model;
    MOTION_STATE &state = model.motion_type.state;
    state.motion_no = state.playing_no = key;
    state.blending = 0;
    state.time = prison.time;
    model.SetMotion(key, 1);
    model.SetPosition(at[0], at[1], at[2]);
    model.SetRotation(0.0f, yaw, 0.0f);
    model.SetScale(prison.scale, prison.scale, prison.scale);
    model.Step();
    DrawFloorCharacter(model);
}

} // namespace

bool EncaseMonster(int monster) {
    if (monster < 0 || monster >= kFloorMonsters || NowMonstorUnit == nullptr) {
        return false;
    }
    int kind = ModelKind(NowMonstorUnit->monster[monster].base_model);
    return Encase(prisons.monsters[monster], monster, PrisonScale(static_cast<Monster>(kind)), false);
}

bool EncaseMember(int chara) {
    return Playing(chara) && Encase(prisons.member, chara, PrisonScale(static_cast<Character>(chara)), true);
}

const Prison &MonsterPrison(int monster) {
    return prisons.monsters[monster];
}

const Prison &MemberPrison() {
    return prisons.member;
}

void IcePrisonsStep() {
    prisons.shattered = false;
    if (prisons.model == nullptr || !MonstersStepping()) {
        return;
    }
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        Step(prisons.monsters[monster], MonsterFrozen(monster), false);
    }
    Step(prisons.member, MemberFrozen(prisons.member.body), true);
}

void IcePrisonsDraw() {
    if (prisons.model == nullptr) {
        return;
    }
    for (const Prison &prison : prisons.monsters) {
        Draw(prison, false);
    }
    Draw(prisons.member, true);
}

void IcePrisonsClear() {
    for (Prison &prison : prisons.monsters) {
        prison = Prison{};
    }
    prisons.member = Prison{};
}

} // namespace expanded::frozen_tuna
