#include "expanded/enemies/bosses.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <utility>

#include "dun/gameloop.hpp"
#include "expanded/data/boss_model.hpp"
#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/ice_queen.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"
#include "platform/paths.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kMonsters = 16;

// The bosses' death labels (120): invincible a while.
constexpr int kDeathInvincible = 100;

// Utan's death (死亡), which his label 120 holds on its last frame.
constexpr int kUtanDeath = 13;

// The Ice Queen's death and its loop.
constexpr int kIceQueenDeath = 11;
constexpr int kIceQueenDeathLoop = 12;

// King's Curse's coffin's death and its loop.
constexpr int kCoffinDeath = 5;
constexpr int kCoffinDeathLoop = 6;

// The Black Knight's death and its loop.
constexpr int kBlackKnightDeath = 7;
constexpr int kBlackKnightDeathLoop = 8;

// The arena crowd's cheer, which Joe's script plays when he is hit.
constexpr int kCrowdCheer = 1798;

enum class Phase {
    Fighting,
    Falling,
    Fading,
    Dead,
};

struct Death {
    Phase phase = Phase::Fighting;
    int   loop = -1;
};

// The motion a beaten boss falls with, and the one it loops while fading (-1 holds the fall's last frame).
std::pair<int, int> DeathMotions(int kind, const CCharacter &character) {
    if (kind == No(Monster::MASTER_UTAN)) {
        return {kUtanDeath, -1};
    }
    if (kind == No(Monster::ICE_QUEEN)) {
        return {kIceQueenDeath, kIceQueenDeathLoop};
    }
    if (kind == No(Monster::KINGS_CURSE_COFFIN)) {
        return {kCoffinDeath, kCoffinDeathLoop};
    }
    if (kind == No(Monster::BLACK_KNIGHT)) {
        return {kBlackKnightDeath, kBlackKnightDeathLoop};
    }
    // Joe: the built fall when the pack has it, else the disc's death.
    if (MotionCount(character) > kJoeDeathLoopKey) {
        return {kJoeFallKey, kJoeDeathLoopKey};
    }
    return {kJoeDiscDeath, -1};
}

bool  out_of_place[kMonsters] = {};
Death death[kMonsters];
bool  logged_missing;

void OnSpawn(CMonstorUnit &unit, int monster) {
    int kind = ModelKind(unit.monster[monster].base_model);
    out_of_place[monster] = LoadedRoster() != nullptr && kind >= 0 && BossSupported(kind);
    death[monster] = {};
}

[[maybe_unused]] const bool kListening = (AddEnemySpawnListener(OnSpawn), true);

constexpr int kLoop = 0;
constexpr int kStopOnLastFrame = 2;

// _SET_MOTION on the monster and its attachments, with the motion's speed and flags.
void PlayMotion(CMonstorUnit &unit, int monster, int motion, int flags) {
    float speed = unit.chara[monster][0].motion_type.motion_info[motion].speed;
    unit.monster[monster].requested_motion = static_cast<s16>(motion);
    unit.monster[monster].requested_motion_flags = static_cast<s16>(flags);
    unit.monster[monster].requested_motion_speed = speed;
    for (int i = 0; i <= unit.monster[monster].attachment_count; i++) {
        unit.chara[monster][i].SetMotion(motion, flags);
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

bool BossSupported(int model_no) {
    return model_no == No(Monster::MINOTAUR_JOE) || model_no == No(Monster::MASTER_UTAN) ||
           model_no == No(Monster::ICE_QUEEN) || model_no == No(Monster::KINGS_CURSE_COFFIN) ||
           model_no == No(Monster::BLACK_KNIGHT);
}

bool BossOutOfPlace(int monster) {
    return monster >= 0 && monster < kMonsters && out_of_place[monster];
}

int MotionCount(const CCharacter &character) {
    const MOTION_INFO *info = character.motion_type.motion_info;
    int                count = 0;
    while (info != nullptr && info[count].start != -1) {
        count++;
    }
    return count;
}

void BossPackFile(char *filename, int model_no) {
    if (model_no != No(Monster::MINOTAUR_JOE)) {
        return;
    }
    std::error_code error;
    if (std::filesystem::is_regular_file(PathsDataRoot() / kJoeModelFile, error)) {
        std::strcpy(filename, kJoeModelFile);
    } else if (!logged_missing) {
        logged_missing = true;
        std::printf("bosses: the data lacks %s; Minotaur Joe falls with his disc death (dcdata expanded <data> adds "
                    "the new one)\n",
                    kJoeModelFile);
    }
}

bool BossKeepsPlace(const CMonstorUnit &unit, int monster) {
    return BossOutOfPlace(monster) && (unit.monster[monster].state == 1 || unit.monster[monster].hp <= 0);
}

bool BossPlace(CMonstorUnit &unit, int monster, float *position) {
    if (BossCastMember(monster)) {
        return BossCastPlace(unit, monster, position) || (BossOutOfPlace(monster) && unit.monster[monster].hp <= 0);
    }
    return BossKeepsPlace(unit, monster);
}

bool BossSmokes(int monster, int model_slot) {
    if (UserStatus->cur_georama == 3 && UserStatus->cur_floor == 17 && monster == 1) {
        return true;
    }
    return LoadedRoster() != nullptr && ModelKind(model_slot) == No(Monster::KINGS_CURSE);
}

bool BossEventHeld(int monster) {
    return BossOutOfPlace(monster);
}

bool BossSoundHeld(int monster, int sound) {
    return BossOutOfPlace(monster) && sound == kCrowdCheer;
}

bool BossDeathStep(CMonstorUnit &unit, int monster) {
    MONSTOR &live = unit.monster[monster];
    if (!BossOutOfPlace(monster) || live.hp > 0) {
        return false;
    }
    Death &state = death[monster];
    switch (state.phase) {
        case Phase::Fighting: {
            // Label 120's start, then the fall.
            live.movement_speed = 0.0f;
            live.turn_speed = 0.0f;
            live.invincible_timer = kDeathInvincible;
            int kind = ModelKind(live.base_model);
            BossCastDefeated(unit, monster);
            if (kind == No(Monster::ICE_QUEEN)) {
                IceQueenDefeated();
            }
            auto [fall, loop] = DeathMotions(kind, unit.chara[monster][0]);
            state.loop = loop;
            PlayMotion(unit, monster, fall, kStopOnLastFrame);
            state.phase = Phase::Falling;
            break;
        }
        case Phase::Falling:
            if (OnLastFrame(unit.chara[monster][0])) {
                if (state.loop >= 0) {
                    PlayMotion(unit, monster, state.loop, kLoop);
                }
                live.palette_alpha_step = 128.0f / kBossFadeFrames;
                live.palette_delay = 0;
                state.phase = Phase::Fading;
            }
            break;
        case Phase::Fading:
            if (live.palette_alpha <= 0.0f) {
                live.state = -1;
                unit.alive_count--;
                state.phase = Phase::Dead;
            }
            break;
        case Phase::Dead:
            break;
    }
    return true;
}

} // namespace expanded
