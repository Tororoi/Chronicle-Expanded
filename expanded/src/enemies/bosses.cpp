#include "expanded/enemies/bosses.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>

#include "expanded/data/boss_model.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"
#include "platform/paths.hpp"
#include "snd.hpp"

namespace expanded {

namespace {

constexpr int kMonsters = 16;

// Joe's death label (120): invincible a while, his barrel-swing sound cut.
constexpr int kDeathInvincible = 100;
constexpr int kDeathSound = 1798;

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
    return model_no == No(Monster::MINOTAUR_JOE);
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

bool BossEventHeld(int monster) {
    return BossOutOfPlace(monster);
}

bool BossDeathStep(CMonstorUnit &unit, int monster) {
    MONSTOR &live = unit.monster[monster];
    if (!BossOutOfPlace(monster) || live.hp > 0) {
        return false;
    }
    Death &state = death[monster];
    switch (state.phase) {
        case Phase::Fighting: {
            // Label 120's start, then the fall: the built one when the pack has it, else the disc's death.
            live.movement_speed = 0.0f;
            live.turn_speed = 0.0f;
            live.invincible_timer = kDeathInvincible;
            SndSeStop(kDeathSound, 0);
            bool built = MotionCount(unit.chara[monster][0]) > kJoeDeathLoopKey;
            state.loop = built ? kJoeDeathLoopKey : -1;
            PlayMotion(unit, monster, built ? kJoeFallKey : kJoeDiscDeath, kStopOnLastFrame);
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
