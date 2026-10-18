#include "expanded/enemies/boss_casts.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <string>

#include "dun/gameloop.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kMonsters = 16;
constexpr int kMembers = 2;

struct Cast {
    Monster kinds[kMembers]; // in the order its scripts number them
    int     needs;           // the party member who can hurt the boss, or -1 for anyone
    // The member that follows the other (when one does): dormant, it waits by it (raised by rise), and its
    // script's hiding place (when it has one) is taken there too.
    bool    follows;
    Monster follower;
    float   rise;
    bool    hides;
    float   hide[3];
    // A member a roster may bring alone (when one may): it starts with the mailbox's g0 at alone_phase,
    // waiting in alone_motion, as the rest of the cast would leave it.
    bool    solo;
    Monster alone;
    int     alone_phase;
    int     alone_motion;
};

// The Ice Queen's barrier stands at her chest; King's Curse's coffin hides 500 under the king until his
// smoke is blown away (in retail at 0, 0, -500, out of the arena). The Black Knight rides his mount, and
// without it starts as the mount leaves him: dismounted (g0 1), standing (motion 15).
// clang-format off
const Cast kCasts[] = {
    {{Monster::ICE_QUEEN, Monster::ICE_BARRIER},                CHARA_RUBY,   true,  Monster::ICE_BARRIER,        10.1f,   false, {},                    false, {},                    0, 0 },
    {{Monster::KINGS_CURSE_COFFIN, Monster::KINGS_CURSE},       CHARA_UNGAGA, true,  Monster::KINGS_CURSE_COFFIN, -500.0f, true,  {0.0f, 0.0f, -500.0f}, false, {},                    0, 0 },
    {{Monster::BLACK_KNIGHT_MOUNT, Monster::BLACK_KNIGHT},      -1,           false, {},                          0.0f,    false, {},                    true,  Monster::BLACK_KNIGHT, 1, 15},
};
// clang-format on

constexpr int kCastCount = static_cast<int>(std::size(kCasts));

struct Playing {
    int monster[kMembers] = {-1, -1};
    int mailbox[kBossMailbox] = {};
    int riding[kMembers] = {-1, -1}; // the monster each member is attached to (_SET_REFERENCE), or -1
    // A member's attachment to a partner still to come: the partner's number and the frame, or -1.
    int         later[kMembers] = {-1, -1};
    std::string later_frame[kMembers];
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
    if (!spec.follows) {
        return false;
    }
    int followed = playing[cast].monster[1 - Member(spec, spec.follower)];
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
        const Cast &spec = kCasts[cast];
        playing[cast].monster[member] = monster;
        if (member == 0) {
            std::fill(std::begin(playing[cast].mailbox), std::end(playing[cast].mailbox), 0);
        }
        // A member waiting to ride this one gets on.
        for (int rider = 0; rider < kMembers; rider++) {
            int waiting = playing[cast].monster[rider];
            if (playing[cast].later[rider] != member || waiting < 0) {
                continue;
            }
            playing[cast].later[rider] = -1;
            CFrame *anchor = unit.chara[monster][0].frame->SearchFrame(playing[cast].later_frame[rider].data());
            if (anchor != nullptr) {
                unit.chara[waiting][0].frame->SetReference(anchor);
                BossCastAttached(unit, waiting, monster);
            }
        }
        bool alone = spec.solo && spec.kinds[member] == spec.alone && !roster->Contains(spec.kinds[1 - member]);
        if (alone) {
            playing[cast].mailbox[0] = spec.alone_phase;
            unit.chara[monster][0].SetMotion(spec.alone_motion, 0);
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
        bool kept = cast.needs < 0 || (UserStatus != nullptr && UserStatus->party_size > cast.needs);
        if (cast.solo && roster.Contains(cast.alone)) {
            bool rest = false;
            for (Monster kind : cast.kinds) {
                rest |= kind != cast.alone && roster.Contains(kind);
            }
            if (!rest) {
                // The member that may come alone does.
                if (kept) {
                    roster.SetOnce(cast.alone, true);
                } else {
                    roster.Remove(cast.alone);
                }
                continue;
            }
        }
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

bool BossCastAllowed(Monster kind) {
    for (const Cast &cast : kCasts) {
        if (Member(cast, kind) >= 0) {
            return cast.needs < 0 || (UserStatus != nullptr && UserStatus->party_size > cast.needs);
        }
    }
    return true;
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
    if (!Find(monster, cast, member) || target < 0 || target >= kMembers) {
        return target;
    }
    // A partner that did not come is the member itself.
    return playing[cast].monster[target] >= 0 ? playing[cast].monster[target] : monster;
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
        if (!spec.follows) {
            continue;
        }
        int   follower = playing[cast].monster[Member(spec, spec.follower)];
        float place[4];
        if (Live(unit, follower) && unit.monster[follower].state == 1 && unit.monster[follower].hp > 0 &&
            WaitingPlace(unit, cast, place)) {
            unit.chara[follower][0].SetPosition(place[0], place[1], place[2]);
        }
    }
}

void BossCastReferLater(int monster, int target, const char *frame) {
    int           cast;
    int           member;
    const Roster *roster = LoadedRoster();
    if (!Find(monster, cast, member) || target < 0 || target >= kMembers || roster == nullptr ||
        !roster->Contains(kCasts[cast].kinds[target])) {
        return;
    }
    playing[cast].later[member] = target;
    playing[cast].later_frame[member] = frame;
}

void BossCastAttached(CMonstorUnit &unit, int monster, int partner) {
    int cast;
    int member;
    if (!Find(monster, cast, member)) {
        return;
    }
    playing[cast].riding[member] = partner;
    if (partner >= 0) {
        // Onto the anchor: what the member held was its spot on the floor.
        unit.chara[monster][0].SetPosition(0.0f, 0.0f, 0.0f);
    }
}

void BossCastWakePosition(CMonstorUnit &unit, int monster, float *position) {
    int cast;
    int member;
    if (!Find(monster, cast, member) || !Live(unit, playing[cast].riding[member])) {
        return;
    }
    unit.chara[playing[cast].riding[member]][0].GetPosition(position);
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
