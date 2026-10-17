#include "expanded/enemies/boss_casts.hpp"

#include <algorithm>
#include <array>
#include <iterator>

#include "dun/gameloop.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kMonsters = 16;
constexpr int kMembers = 2;

struct Cast {
    Monster kinds[kMembers]; // in the order its scripts number them
    int     needs;           // the party member who can hurt the boss
    // The member that follows the other: dormant, it waits by it (raised by rise), and its script's hiding
    // place (when it has one) is taken there too.
    Monster follower;
    float   rise;
    bool    hides;
    float   hide[3];
};

// The Ice Queen's barrier stands at her chest; King's Curse's coffin hides 500 under the king until his
// smoke is blown away (in retail at 0, 0, -500, out of the arena).
const Cast kCasts[] = {
    {{Monster::ICE_QUEEN, Monster::ICE_BARRIER},          CHARA_RUBY,   Monster::ICE_BARRIER,        10.1f,   false, {}                   },
    {{Monster::KINGS_CURSE_COFFIN, Monster::KINGS_CURSE}, CHARA_UNGAGA, Monster::KINGS_CURSE_COFFIN, -500.0f, true,  {0.0f, 0.0f, -500.0f}},
};

constexpr int kCastCount = static_cast<int>(std::size(kCasts));

struct Playing {
    int monster[kMembers] = {-1, -1};
    int mailbox[kBossMailbox] = {};
};

std::array<Playing, kCastCount> playing;

int Member(const Cast &cast, Monster kind) {
    for (int i = 0; i < kMembers; i++) {
        if (cast.kinds[i] == kind) {
            return i;
        }
    }
    return -1;
}

// The cast and member a monster plays, or false.
bool Find(int monster, int &cast, int &member) {
    for (cast = 0; cast < kCastCount; cast++) {
        for (member = 0; member < kMembers; member++) {
            if (monster >= 0 && playing[cast].monster[member] == monster) {
                return true;
            }
        }
    }
    return false;
}

bool Live(const CMonstorUnit &unit, int monster) {
    return monster >= 0 && monster < kMonsters && unit.monster[monster].state != -1;
}

// Where the follower of a cast waits: by its partner, or -1 without one.
bool WaitingPlace(CMonstorUnit &unit, int cast, float *place) {
    const Cast &spec = kCasts[cast];
    int         followed = playing[cast].monster[1 - Member(spec, spec.follower)];
    if (!Live(unit, followed)) {
        return false;
    }
    unit.chara[followed][0].GetPosition(place);
    place[1] += spec.rise;
    return true;
}

void OnModel(CMonstorUnit &, int model_slot, int) {
    if (model_slot == 0) {
        playing = {};
    }
}

void OnSpawn(CMonstorUnit &unit, int monster) {
    const Roster *roster = LoadedRoster();
    if (roster == nullptr) {
        return;
    }
    int kind = ModelKind(unit.monster[monster].base_model);
    for (int cast = 0; cast < kCastCount; cast++) {
        int member = Member(kCasts[cast], static_cast<Monster>(kind));
        if (member < 0) {
            continue;
        }
        playing[cast].monster[member] = monster;
        if (member == 0) {
            std::fill(std::begin(playing[cast].mailbox), std::end(playing[cast].mailbox), 0);
        }
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyModelListener(OnModel), AddEnemySpawnListener(OnSpawn), true);

} // namespace

void BossCastRoster(Roster &roster) {
    for (const Cast &cast : kCasts) {
        bool any = false;
        for (Monster kind : cast.kinds) {
            any |= roster.Contains(kind);
        }
        if (!any) {
            continue;
        }
        bool kept = UserStatus != nullptr && UserStatus->party_size > cast.needs;
        for (Monster kind : cast.kinds) {
            kept = kept && (roster.Contains(kind) || roster.Add(kind, true));
        }
        for (Monster kind : cast.kinds) {
            if (kept) {
                roster.SetOnce(kind, true);
            } else {
                roster.Remove(kind);
            }
        }
    }
}

bool BossCastMember(int monster) {
    int cast;
    int member;
    return Find(monster, cast, member);
}

int BossCastMonster(Monster kind) {
    for (int cast = 0; cast < kCastCount; cast++) {
        int member = Member(kCasts[cast], kind);
        if (member >= 0) {
            return playing[cast].monster[member];
        }
    }
    return -1;
}

int *BossCastMailbox(Monster kind) {
    for (int cast = 0; cast < kCastCount; cast++) {
        if (Member(kCasts[cast], kind) >= 0) {
            return playing[cast].mailbox;
        }
    }
    return nullptr;
}

int *BossCastGlobals(int monster, int *globals) {
    int cast;
    int member;
    return Find(monster, cast, member) ? playing[cast].mailbox : globals;
}

int BossCastMonsterRef(int monster, int target) {
    int cast;
    int member;
    if (!Find(monster, cast, member) || target < 0 || target >= kMembers || playing[cast].monster[target] < 0) {
        return target;
    }
    return playing[cast].monster[target];
}

bool BossCastPlace(CMonstorUnit &unit, int monster, float *position) {
    int cast;
    int member;
    if (!Find(monster, cast, member)) {
        return false;
    }
    const Cast &spec = kCasts[cast];
    if (spec.hides && spec.kinds[member] == spec.follower && position[0] == spec.hide[0] && position[1] == spec.hide[1] &&
        position[2] == spec.hide[2]) {
        float place[4];
        if (WaitingPlace(unit, cast, place)) {
            position[0] = place[0];
            position[1] = place[1];
            position[2] = place[2];
            return false;
        }
    }
    return unit.monster[monster].state == 1;
}

bool BossCastCountsKill(int monster) {
    int cast;
    int member;
    return !Find(monster, cast, member) || BossSupported(No(kCasts[cast].kinds[member]));
}

void BossCastStep(CMonstorUnit &unit) {
    for (int cast = 0; cast < kCastCount; cast++) {
        const Cast &spec = kCasts[cast];
        int         follower = playing[cast].monster[Member(spec, spec.follower)];
        float       place[4];
        if (Live(unit, follower) && unit.monster[follower].state == 1 && unit.monster[follower].hp > 0 &&
            WaitingPlace(unit, cast, place)) {
            unit.chara[follower][0].SetPosition(place[0], place[1], place[2]);
        }
    }
}

void BossCastDefeated(CMonstorUnit &unit, int monster) {
    int cast;
    int member;
    if (!Find(monster, cast, member)) {
        return;
    }
    for (int other : playing[cast].monster) {
        if (other != monster && Live(unit, other)) {
            unit.monster[other].state = -1;
            unit.alive_count--;
        }
    }
}

} // namespace expanded
