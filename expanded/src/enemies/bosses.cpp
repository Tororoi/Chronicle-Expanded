#include "expanded/enemies/bosses.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <utility>

#include "dun/gameloop.hpp"
#include "expanded/data/boss_model.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/black_knight.hpp"
#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/boss_moves.hpp"
#include "expanded/enemies/boss_table.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/ice_queen.hpp"
#include "expanded/enemies/monster_actions.hpp"
#include "expanded/enemies/roster.hpp"
#include "monstorunit.hpp"
#include "platform/paths.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// The bosses' death labels (120): invincible a while.
constexpr int kDeathInvincible = 100;

// Steps from the start of the Black Knight's death throes to the light coming down on him, as event 508 times it.
constexpr int kKnightLightAfterThroes = 195;

// The arena crowd's cheer, which Joe's script plays when he is hit.
constexpr int kCrowdCheer = 1798;

// King's Curse's own floor (Sun and Moon Temple's, cur_georama being the dungeon), where the king is monster 1.
constexpr int kKingsCurseFloor = 17;
constexpr int kKingsCurseKing = 1;

enum class Phase {
    Fighting,
    Falling,
    Throes, // the Black Knight's built death: head bowed, its loop, the death throes, their loop till the light
    Fading,
    Dead,
};

struct Death {
    Phase phase = Phase::Fighting;
    int   loop = -1;
    int   key = -1;         // Throes: the KEY playing
    int   throes = -1;      // Throes: steps since the death throes began, or -1
    float fell_from = 0.0f; // Falling: the height an airborne boss fell from
};

// A model's built boss pack, and the boss it is for.
struct BuiltPack {
    Monster     boss;
    const char *file;
    const char *death; // what the boss falls with without it
};

const BuiltPack kBuiltPacks[] = {
    {Monster::MINOTAUR_JOE, kJoeModelFile,         "his disc death"},
    {Monster::BLACK_KNIGHT, kBlackKnightModelFile, "his dismount"  },
};

// The motion a beaten boss falls with and the one it loops while fading: its table's, but Joe's built fall when his
// pack has it.
std::pair<int, int> DeathMotions(const BossEntry &boss, const CCharacter &character) {
    if (boss.kind == Monster::MINOTAUR_JOE && MotionCount(character) > kJoeDeathLoopKey) {
        return {kJoeFallKey, kJoeDeathLoopKey};
    }
    return {boss.death, boss.death_loop};
}

// Each monster's entry when it is a supported boss a staged roster brought, else null.
const BossEntry *out_of_place[kFloorMonsters] = {};
Death            death[kFloorMonsters];
bool             logged_missing;

void OnSpawn(CMonstorUnit &unit, int monster) {
    out_of_place[monster] = LoadedRoster() != nullptr ? BossEntryOf(ModelKind(unit.monster[monster].base_model)) : nullptr;
    death[monster] = {};
}

[[maybe_unused]] const bool kListening = (AddEnemySpawnListener(OnSpawn), true);

// Lowers a boss falling in its arena's coordinates from fell_from to the arena's floor, landing at frame landing.
void FallToFloor(CMonstorUnit &unit, int monster, float fell_from, float landing) {
    if (!BossInArenaFrame(monster)) {
        return;
    }
    float floor[3] = {0.0f, 0.0f, 0.0f};
    BossPointIn(unit, monster, floor, BossPoint::Target);
    const CCharacter  &character = unit.chara[monster][0];
    const MOTION_INFO &info = character.motion_type.motion_info[character.motion_no];
    float              k = std::clamp((character.motion_type.state.time - static_cast<float>(info.start)) /
                                          (landing - static_cast<float>(info.start)),
                                      0.0f, 1.0f);
    float              at[4];
    unit.chara[monster][0].GetPosition(at);
    unit.chara[monster][0].SetPosition(at[0], fell_from + (floor[1] - fell_from) * k, at[2]);
}

// Whether the motion stands on its last frame (not still on the one before it).
bool OnLastFrame(const CCharacter &character) {
    const MOTION_INFO &info = character.motion_type.motion_info[character.motion_no];
    float              time = character.motion_type.state.time;
    return time >= info.end - 1.0f && time <= info.end;
}

} // namespace

bool BossSupported(int model_no) {
    return BossEntryOf(model_no) != nullptr;
}

bool BossOutOfPlace(int monster) {
    return monster >= 0 && monster < kFloorMonsters && out_of_place[monster] != nullptr && !AllyStepping();
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
    for (const BuiltPack &pack : kBuiltPacks) {
        if (model_no != No(pack.boss)) {
            continue;
        }
        std::error_code error;
        if (std::filesystem::is_regular_file(PathsDataRoot() / pack.file, error)) {
            std::strcpy(filename, pack.file);
        } else if (!logged_missing) {
            logged_missing = true;
            std::fprintf(stderr, "bosses: the data lacks %s; the boss falls with %s (dcdata expanded <data> adds the new one)\n",
                         pack.file, pack.death);
        }
    }
}

bool BossKeepsPlace(const CMonstorUnit &unit, int monster) {
    return BossOutOfPlace(monster) && (unit.monster[monster].state == 1 || unit.monster[monster].hp <= 0);
}

bool BossPlace(CMonstorUnit &unit, int monster, float *position) {
    if (BossInArenaFrame(monster)) {
        return unit.monster[monster].hp <= 0;
    }
    if (BossCastMember(monster)) {
        return BossCastPlace(unit, monster, position) || (BossOutOfPlace(monster) && unit.monster[monster].hp <= 0);
    }
    return BossKeepsPlace(unit, monster);
}

bool BossSmokes(int monster, int model_slot) {
    if (AllyStepping()) {
        return false;
    }
    if (UserStatus->cur_georama == DUNGEON_SUN_MOON_TEMPLE && UserStatus->cur_floor == kKingsCurseFloor && monster == kKingsCurseKing) {
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
    const BossEntry &boss = *out_of_place[monster];
    Death           &state = death[monster];
    switch (state.phase) {
        case Phase::Fighting: {
            // Label 120's start, then the fall.
            live.movement_speed = 0.0f;
            live.turn_speed = 0.0f;
            live.invincible_timer = kDeathInvincible;
            BossCastDefeated(unit, monster);
            if (boss.kind == Monster::ICE_QUEEN) {
                IceQueenDefeated();
            }
            if (boss.kind == Monster::BLACK_KNIGHT &&
                MotionCount(unit.chara[monster][0]) >= kBlackKnightBowKey + kBlackKnightDeathKeys) {
                state.key = kBlackKnightBowKey;
                PlayMonsterMotion(unit, monster, state.key, kMotionHoldLast);
                state.phase = Phase::Throes;
                break;
            }
            auto [fall, loop] = DeathMotions(boss, unit.chara[monster][0]);
            state.loop = loop;
            float at[4];
            unit.chara[monster][0].GetPosition(at);
            state.fell_from = at[1];
            PlayMonsterMotion(unit, monster, fall, kMotionHoldLast);
            state.phase = Phase::Falling;
            break;
        }
        case Phase::Throes: {
            // Each KEY in turn, the throes' loop until the light comes down and hits him; he fades from its hit.
            int last = kBlackKnightBowKey + kBlackKnightDeathKeys - 1;
            if (state.key < last && OnLastFrame(unit.chara[monster][0])) {
                state.key++;
                PlayMonsterMotion(unit, monster, state.key, state.key == last ? kMotionLoop : kMotionHoldLast);
                if (state.key == last - 1) {
                    state.throes = 0;
                }
            }
            if (state.throes < 0) {
                break;
            }
            bool lit = false;
            if (state.throes == kKnightLightAfterThroes) {
                lit = !BlackKnightLight(unit, monster); // without the light, he fades now
            }
            state.throes++;
            if (lit || (state.throes > kKnightLightAfterThroes && BlackKnightLit())) {
                FadeMonster(unit, monster, 128.0f / kBossFadeFrames);
                state.phase = Phase::Fading;
            }
            break;
        }
        case Phase::Falling:
            if (boss.death_landing > 0.0f) {
                FallToFloor(unit, monster, state.fell_from, boss.death_landing);
            }
            if (OnLastFrame(unit.chara[monster][0])) {
                if (state.loop >= 0) {
                    PlayMonsterMotion(unit, monster, state.loop, kMotionLoop);
                }
                FadeMonster(unit, monster, 128.0f / kBossFadeFrames);
                state.phase = Phase::Fading;
            }
            break;
        case Phase::Fading:
            if (RemoveFadedMonster(unit, monster)) {
                state.phase = Phase::Dead;
            }
            break;
        case Phase::Dead:
            break;
    }
    return true;
}

} // namespace expanded
