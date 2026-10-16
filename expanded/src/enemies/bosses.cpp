#include "expanded/enemies/bosses.hpp"

#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

constexpr int kMonsters = 16;

struct Death {
    int  frames = 0;
    int  motion = -1;
    bool collapsing = false;
    bool fading = false;
};

bool  out_of_place[kMonsters] = {};
Death death[kMonsters];

void OnSpawn(CMonstorUnit &unit, int monster) {
    int kind = ModelKind(unit.monster[monster].base_model);
    out_of_place[monster] = LoadedRoster() != nullptr && kind >= 0 && MonstorTable[kind].kind == MONSTER_KIND_NO_LOCK_ON;
    death[monster] = {};
}

[[maybe_unused]] const bool kListening = (AddEnemySpawnListener(OnSpawn), true);

constexpr int kStopOnLastFrame = 2;

// _SET_MOTION on the monster and its attachments, stopping on the motion's last frame.
void PlayOnce(CMonstorUnit &unit, int monster, int motion) {
    float speed = unit.chara[monster][0].motion_type.motion_info[motion].speed;
    unit.monster[monster].requested_motion = static_cast<s16>(motion);
    unit.monster[monster].requested_motion_flags = kStopOnLastFrame;
    unit.monster[monster].requested_motion_speed = speed;
    for (int i = 0; i <= unit.monster[monster].attachment_count; i++) {
        unit.chara[monster][i].SetMotion(motion, kStopOnLastFrame);
        unit.chara[monster][i].SetMotionSpeed(speed);
    }
}

// Whether the motion stands on its last frame (not still on the one before it).
bool OnLastFrame(const CCharacter &character) {
    const MOTION_INFO &info = character.motion_type.motion_info[character.motion_no];
    float              time = character.motion_type.state.time;
    return time >= info.end - 1.0f && time <= info.end;
}

} // namespace

bool BossOutOfPlace(int monster) {
    return monster >= 0 && monster < kMonsters && out_of_place[monster];
}

int BossCollapseMotion(int model_no) {
    switch (model_no) {
        case No(Monster::DRAN):
            return 6;
        case No(Monster::MASTER_UTAN):
            return 13;
        case No(Monster::ICE_QUEEN):
            return 11;
        case No(Monster::KINGS_CURSE_COFFIN):
            return 5;
        case No(Monster::MINOTAUR_JOE):
            return 9;
        default:
            return -1;
    }
}

bool BossKeepsPlace(const CMonstorUnit &unit, int monster) {
    return BossOutOfPlace(monster) && (unit.monster[monster].state == 1 || unit.monster[monster].hp <= 0);
}

bool BossEventHeld(int monster) {
    return BossOutOfPlace(monster);
}

bool BossDeathStep(CMonstorUnit &unit, int monster) {
    MONSTOR &live = unit.monster[monster];
    if (!BossOutOfPlace(monster) || live.hp > 0) {
        return false;
    }
    Death &state = death[monster];
    state.frames++;
    if (!state.collapsing) {
        if (unit.script_state[monster] != 0 && state.frames < kBossScriptFrames) {
            return false;
        }
        state.collapsing = true;
        live.movement_speed = 0.0f;
        live.turn_speed = 0.0f;
        state.motion = BossCollapseMotion(ModelKind(live.base_model));
        if (state.motion >= 0) {
            PlayOnce(unit, monster, state.motion);
        }
    }
    if (!state.fading && (state.motion < 0 || OnLastFrame(unit.chara[monster][0]))) {
        state.fading = true;
        live.palette_alpha_step = 128.0f / kBossFadeFrames;
        live.palette_delay = 0;
    }
    if (state.fading && live.palette_alpha <= 0.0f) {
        live.state = -1;
        unit.alive_count--;
    }
    return true;
}

} // namespace expanded
