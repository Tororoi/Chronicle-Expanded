#include "expanded/status/blind.hpp"

#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_actions.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/status/poison.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

// Most kinds guard with motions 5 (raise), 6 (hold) and 7 (lower), and stagger with 8; these are the rest.
constexpr BlindClips kGuardClips = {6, 7, 8};

struct KindClips {
    Monster    kind;
    BlindClips clips;
};

// clang-format off
constexpr KindClips kOtherClips[] = {
    {Monster::STATUE,                   {0,  -1, -1}},
    {Monster::HORNET,                   {0,  -1, 8 }},
    {Monster::CANNIBAL_PLANT,           {0,  -1, 1 }},
    {Monster::EARTH_DIGGER,             {0,  -1, 8 }},
    {Monster::PIRATES_CHARIOT,          {0,  -1, 8 }},
    {Monster::MR_BLARE,                 {6,  -1, -1}},
    {Monster::DUNE,                     {6,  7,  -1}},
    {Monster::MOON_BUG,                 {0,  -1, 2 }},
    {Monster::PHANTOM,                  {0,  -1, 8 }},
    {Monster::CAVE_BAT,                 {0,  -1, 8 }},
    {Monster::EVIL_BAT,                 {0,  -1, 8 }},
    {Monster::MOON_DIGGER,              {0,  -1, 8 }},
    {Monster::DARK_FLOWER,              {0,  -1, 1 }},
    {Monster::CURSED_ROSE,              {0,  -1, 1 }},
    {Monster::BILLY,                    {6,  -1, -1}},
    {Monster::VULCAN,                   {6,  7,  -1}},
    {Monster::ROCKANOFF,                {0,  -1, 8 }},
    {Monster::SAM,                      {6,  -1, -1}},
    {Monster::WINE_KEG,                 {0,  -1, -1}},
    {Monster::HALEY_HOLEY,              {0,  -1, 8 }},
    {Monster::VULCAN_ENHANCED,          {6,  7,  -1}},
    {Monster::ROCKANOFF_ENHANCED,       {0,  -1, 8 }},
    {Monster::CAVE_BAT_ENHANCED,        {0,  -1, 8 }},
    {Monster::CURSED_ROSE_ENHANCED,     {0,  -1, 1 }},
    {Monster::PIRATES_CHARIOT_ENHANCED, {0,  -1, 8 }},
    {Monster::EVIL_BAT_ENHANCED,        {0,  -1, 8 }},
};
// clang-format on

// Kinds whose own steps the blinding must not take over: the gemrons Expanded drives itself.
constexpr Monster kLeftAlone[] = {Monster::BOMB_GEMRON, Monster::CRYSTAL_GEMRON, Monster::ATLA_GEMRON};

enum class Phase {
    kNone,
    kHold,
    kStagger,
    kLower,
};

struct Held {
    Phase      phase = Phase::kNone;
    BlindClips clips = {};
    int        lower_at = 0; // frames left at which it lowers its guard
    int        stagger = 0;  // frames of stagger left
};

Held          held[kFloorMonsters];
CMonstorUnit *blind_unit = nullptr;
int           frames_left = 0;
int           blind_in = 0; // frames until a waiting blinding

// The frames a motion takes at its own speed.
int MotionFrames(const CMonstorUnit &unit, int monster, int motion) {
    const MOTION_INFO &info = unit.chara[monster][0].motion_type.motion_info[motion];
    return info.speed > 0.0f ? (int) ((float) (info.end - info.start) / info.speed) : 0;
}

void Play(CMonstorUnit &unit, int monster, int motion, int flags) {
    if (motion >= 0) {
        PlayMonsterMotion(unit, monster, motion, flags);
    }
}

void Raise(CMonstorUnit &unit, int monster) {
    Held &state = held[monster];
    state.phase = Phase::kHold;
    Play(unit, monster, state.clips.hold, kMotionLoop);
}

void Release(int monster) {
    if (blind_unit != nullptr && held[monster].phase != Phase::kNone) {
        blind_unit->script_state[monster] = 0;
    }
    held[monster] = {};
}

} // namespace

bool BlindClipsOf(int model_no, BlindClips *clips) {
    if (model_no < 0 || model_no > No(Monster::BLACK_KNIGHT_MOUNT)) {
        return false;
    }
    for (Monster kind : kLeftAlone) {
        if (model_no == No(kind)) {
            return false;
        }
    }
    *clips = kGuardClips;
    for (const KindClips &other : kOtherClips) {
        if (model_no == No(other.kind)) {
            *clips = other.clips;
        }
    }
    return true;
}

void Blind(CMonstorUnit &unit, int frames) {
    if (blind_unit != &unit) {
        BlindEnd();
    }
    blind_unit = &unit;
    frames_left = frames;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        Held &state = held[monster];
        if (!MonsterInPlay(unit.monster[monster])) {
            continue;
        }
        const MONSTOR       &live = unit.monster[monster];
        const MONSTOR_MODEL &model = unit.model[live.base_model];
        BlindClips           clips;
        if (model.kind == MONSTER_KIND_NO_LOCK_ON || !BlindClipsOf(ModelKind(live.base_model), &clips)) {
            continue;
        }
        if (state.phase == Phase::kNone) {
            state.clips = clips;
            unit.script_state[monster] = 0;
        }
        int lower = clips.lower >= 0 ? MotionFrames(unit, monster, clips.lower) + kBlindIdleBeat : 0;
        state.lower_at = lower;
        if (state.phase != Phase::kStagger) {
            Raise(unit, monster);
        }
    }
}

void BlindEnd() {
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        Release(monster);
    }
    blind_unit = nullptr;
    frames_left = 0;
}

bool Blinding() {
    return frames_left > 0;
}

int BlindFramesLeft() {
    return frames_left;
}

bool Blinded(int monster) {
    return monster >= 0 && monster < kFloorMonsters && held[monster].phase != Phase::kNone;
}

void BlindAfter(int frames) {
    blind_in = frames;
}

void BlindTick() {
    if (blind_in > 0 && --blind_in == 0 && NowMonstorUnit != nullptr) {
        Blind(*NowMonstorUnit);
    }
    if (MonstersStepping() && frames_left > 0 && --frames_left == 0) {
        BlindEnd();
    }
}

void BlindReset() {
    blind_in = 0;
    BlindEnd();
}

bool BlindStep(CMonstorUnit &unit, int monster) {
    Held &state = held[monster];
    if (state.phase == Phase::kNone || &unit != blind_unit) {
        return false;
    }
    MONSTOR &live = unit.monster[monster];
    if (live.state == -1 || live.hp <= 0) {
        // Its death script runs as ever.
        held[monster] = {};
        return false;
    }
    if (unit.script_state[monster] != 0) {
        // A hit started its reaction script: the stagger in its place.
        unit.script_state[monster] = 0;
        state.phase = Phase::kStagger;
        state.stagger = kBlindStaggerFrames;
        Play(unit, monster, state.clips.stagger, kMotionHoldLast | kMotionCut);
    }
    live.movement_speed = 0.0f;
    live.turn_speed = 0.0f;
    switch (state.phase) {
        case Phase::kStagger:
            if (--state.stagger <= 0) {
                Raise(unit, monster);
            }
            break;
        case Phase::kHold:
            if (frames_left <= state.lower_at) {
                state.phase = Phase::kLower;
                Play(unit, monster, state.clips.lower, kMotionHoldLast);
            }
            break;
        default:
            break;
    }
    return true;
}

bool BlindGuardBlocks(int monster) {
    return !Blinded(monster);
}

} // namespace expanded
