#include "expanded/enemies/boss_casts.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <optional>
#include <string>

#include "dun/gameloop.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_actions.hpp"
#include "expanded/enemies/roster.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kMembers = 2;

// A cast's needs when anyone can hurt its boss.
constexpr int kAnyone = -1;

// A member that follows the other: dormant, it waits by it (raised by rise), and its script's hiding place, when it has
// one, is taken there too.
struct Follower {
    Monster                             kind;
    float                               rise;
    std::optional<std::array<float, 3>> hide = std::nullopt;
};

// A member a roster may bring alone: it starts with the mailbox's g0 at phase, waiting in motion, as the rest of the
// cast would leave it.
struct Alone {
    Monster kind;
    int     phase;
    int     motion;
};

struct Cast {
    Monster                 kinds[kMembers]; // in the order its scripts number them
    const char             *names[kMembers];
    int                     needs = kAnyone; // the party member who can hurt the boss (CHARA_*)
    std::optional<Follower> follower = std::nullopt;
    std::optional<Alone>    alone = std::nullopt;
};

const Cast kCasts[] = {
    // The Ice Queen's barrier stands at her chest.
    {.kinds = {Monster::ICE_QUEEN, Monster::ICE_BARRIER},
     .names = {"Ice Queen", "Ice Barrier"},
     .needs = CHARA_RUBY,
     .follower = Follower{.kind = Monster::ICE_BARRIER, .rise = 10.1f}},
    // King's Curse's coffin hides 500 under the king until his smoke is blown away (in retail at 0, 0, -500, out of
    // the arena).
    {.kinds = {Monster::KINGS_CURSE_COFFIN, Monster::KINGS_CURSE},
     .names = {"King's Curse's coffin", "King's Curse"},
     .needs = CHARA_UNGAGA,
     .follower = Follower{.kind = Monster::KINGS_CURSE_COFFIN, .rise = -500.0f, .hide = std::array{0.0f, 0.0f, -500.0f}}},
    // The Black Knight rides his mount, and without it starts as the mount leaves him: dismounted (g0 1), standing
    // (motion 15).
    {.kinds = {Monster::BLACK_KNIGHT_MOUNT, Monster::BLACK_KNIGHT},
     .names = {"Black Knight's mount", "Black Knight"},
     .alone = Alone{.kind = Monster::BLACK_KNIGHT, .phase = 1, .motion = 15}},
};

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

// The cast and member a monster plays on this floor.
struct Role {
    int cast;
    int member;
};

std::optional<Role> RoleOf(int monster) {
    if (monster < 0) {
        return std::nullopt;
    }
    for (int cast = 0; cast < kCastCount; cast++) {
        for (int member = 0; member < kMembers; member++) {
            if (playing[cast].monster[member] == monster) {
                return Role{.cast = cast, .member = member};
            }
        }
    }
    return std::nullopt;
}

// Whether the party has the member who can hurt the cast's boss.
bool PartyCanFight(const Cast &cast) {
    return cast.needs == kAnyone || (UserStatus != nullptr && UserStatus->party_size > cast.needs);
}

// Whether a position is the follower's script's hiding place.
bool Hiding(const Follower &follower, const float *position) {
    return follower.hide && position[0] == (*follower.hide)[0] && position[1] == (*follower.hide)[1] &&
           position[2] == (*follower.hide)[2];
}

bool Live(const CMonstorUnit &unit, int monster) {
    return monster >= 0 && monster < kFloorMonsters && unit.monster[monster].state != -1;
}

// Where a cast's follower waits: by its partner, raised by its rise; false when the cast has none or the partner is
// gone.
bool WaitingPlace(CMonstorUnit &unit, int cast, float *place) {
    const Cast &spec = kCasts[cast];
    if (!spec.follower) {
        return false;
    }
    int followed = playing[cast].monster[1 - Member(spec, spec.follower->kind)];
    if (!Live(unit, followed)) {
        return false;
    }
    unit.chara[followed][0].GetPosition(place);
    place[1] += spec.follower->rise;
    return true;
}

void OnFloor() {
    playing = {};
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
        if (spec.alone && spec.kinds[member] == spec.alone->kind && !roster->Contains(spec.kinds[1 - member])) {
            playing[cast].mailbox[0] = spec.alone->phase;
            unit.chara[monster][0].SetMotion(spec.alone->motion, 0);
        }
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), AddEnemySpawnListener(OnSpawn), true);

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
        bool kept = PartyCanFight(cast);
        if (cast.alone && roster.Contains(cast.alone->kind)) {
            bool rest = false;
            for (Monster kind : cast.kinds) {
                rest |= kind != cast.alone->kind && roster.Contains(kind);
            }
            if (!rest) {
                // The member that may come alone does.
                if (kept) {
                    roster.SetOnce(cast.alone->kind, true);
                } else {
                    roster.Remove(cast.alone->kind);
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

std::optional<BossCastInfo> BossCastOf(Monster kind) {
    for (const Cast &cast : kCasts) {
        int member = Member(cast, kind);
        if (member >= 0) {
            return BossCastInfo{
                .kinds = {cast.kinds[0], cast.kinds[1]},
                .names = {cast.names[0], cast.names[1]},
                .member = member,
                .needs = cast.needs,
                .alone = cast.alone && cast.alone->kind == kind,
            };
        }
    }
    return std::nullopt;
}

bool BossCastAllowed(Monster kind) {
    for (const Cast &cast : kCasts) {
        if (Member(cast, kind) >= 0) {
            return PartyCanFight(cast);
        }
    }
    return true;
}

bool BossCastMember(int monster) {
    return RoleOf(monster).has_value();
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
    std::optional<Role> role = RoleOf(monster);
    return role ? playing[role->cast].mailbox : globals;
}

int BossCastMonsterRef(int monster, int target) {
    std::optional<Role> role = RoleOf(monster);
    if (!role || target < 0 || target >= kMembers) {
        return target;
    }
    // A partner that did not come is the member itself.
    int partner = playing[role->cast].monster[target];
    return partner >= 0 ? partner : monster;
}

bool BossCastPlace(CMonstorUnit &unit, int monster, float *position) {
    std::optional<Role> role = RoleOf(monster);
    if (!role) {
        return false;
    }
    const Cast &spec = kCasts[role->cast];
    if (spec.follower && spec.kinds[role->member] == spec.follower->kind && Hiding(*spec.follower, position)) {
        float place[4];
        if (WaitingPlace(unit, role->cast, place)) {
            position[0] = place[0];
            position[1] = place[1];
            position[2] = place[2];
            return false;
        }
    }
    return unit.monster[monster].state == 1;
}

bool BossCastCountsKill(int monster) {
    std::optional<Role> role = RoleOf(monster);
    return !role || BossSupported(No(kCasts[role->cast].kinds[role->member]));
}

void BossCastStep(CMonstorUnit &unit) {
    for (int cast = 0; cast < kCastCount; cast++) {
        const Cast &spec = kCasts[cast];
        if (!spec.follower) {
            continue;
        }
        int   follower = playing[cast].monster[Member(spec, spec.follower->kind)];
        float place[4];
        if (Live(unit, follower) && unit.monster[follower].state == 1 && unit.monster[follower].hp > 0 &&
            WaitingPlace(unit, cast, place)) {
            unit.chara[follower][0].SetPosition(place[0], place[1], place[2]);
        }
    }
}

void BossCastReferLater(int monster, int target, const char *frame) {
    std::optional<Role> role = RoleOf(monster);
    const Roster       *roster = LoadedRoster();
    if (!role || target < 0 || target >= kMembers || roster == nullptr || !roster->Contains(kCasts[role->cast].kinds[target])) {
        return;
    }
    playing[role->cast].later[role->member] = target;
    playing[role->cast].later_frame[role->member] = frame;
}

void BossCastAttached(CMonstorUnit &unit, int monster, int partner) {
    std::optional<Role> role = RoleOf(monster);
    if (!role) {
        return;
    }
    playing[role->cast].riding[role->member] = partner;
    if (partner >= 0) {
        // Onto the anchor: what the member held was its spot on the floor.
        unit.chara[monster][0].SetPosition(0.0f, 0.0f, 0.0f);
    }
}

void BossCastWakePosition(CMonstorUnit &unit, int monster, float *position) {
    std::optional<Role> role = RoleOf(monster);
    if (!role || !Live(unit, playing[role->cast].riding[role->member])) {
        return;
    }
    unit.chara[playing[role->cast].riding[role->member]][0].GetPosition(position);
}

void BossCastDefeated(CMonstorUnit &unit, int monster) {
    std::optional<Role> role = RoleOf(monster);
    if (!role) {
        return;
    }
    for (int other : playing[role->cast].monster) {
        if (other != monster && Live(unit, other)) {
            RemoveMonster(unit, other);
        }
    }
}

} // namespace expanded
