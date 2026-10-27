#include "expanded/enemies/revive.hpp"

#include <cstring>
#include <initializer_list>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/core/options.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_actions.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "runscript_opcodes.hpp"

namespace expanded {

namespace {

[[maybe_unused]] const bool kRegistered = AddToggle({.key = "harder_enemy_ai",
                                                     .label = "Harder Enemy AI",
                                                     .help = "\"Harder Enemy AI\"\nFallen monsters may\nget back up."});

// The cards' death sounds, each the twelfth of the card's own.
constexpr int kHeartDeath = 1418 + 11;
constexpr int kClubDeath = 1437 + 11;
constexpr int kDiamondDeath = 1456 + 11;
constexpr int kSpadeDeath = 1475 + 11;
constexpr int kJokerDeath = 1494 + 11;

// Every monster with a real get-up (10); those whose death throws them back collapse with their knockdown (9).
constexpr ReviveSpecies kSpecies[] = {
    {.script = "e01a", .revival = Revival::kScript}, // Master Jacket
    {.script = "e03a", .revival = Revival::kGetUp, .collapse_motion = 11}, // Skeleton Soldier
    {.script = "e14a", .revival = Revival::kGetUp, .collapse_motion = 9}, // Sunday
    {.script = "e15a", .revival = Revival::kGetUp, .collapse_motion = 9}, // Monday
    {.script = "e16a", .revival = Revival::kGetUp, .collapse_motion = 9}, // Tuesday
    {.script = "e17a", .revival = Revival::kGetUp, .collapse_motion = 9}, // Wednesday
    {.script = "e18a", .revival = Revival::kGetUp, .collapse_motion = 9}, // Thursday
    {.script = "e19a", .revival = Revival::kGetUp, .collapse_motion = 9}, // Friday
    {.script = "e20a", .revival = Revival::kGetUp, .collapse_motion = 9}, // Saturday
    {.script = "e44a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kHeartDeath}, // Heart
    {.script = "e45a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kClubDeath}, // Club
    {.script = "e46a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kDiamondDeath}, // Diamond
    {.script = "e47a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kSpadeDeath}, // Spade
    {.script = "e48a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kJokerDeath}, // Joker
    {.script = "e50a", .revival = Revival::kScript}, // Mummy
    {.script = "e52a", .revival = Revival::kGetUp, .collapse_motion = 9}, // Curse Dancer
    {.script = "e62a", .revival = Revival::kGetUp, .collapse_motion = 9}, // Hell Pockle
    {.script = "e77a", .revival = Revival::kGetUp, .collapse_motion = 11, .drops_loop_sound = true}, // Rockanoff
    {.script = "e106a", .revival = Revival::kGetUp, .collapse_motion = 9, .drops_loop_sound = true}, // King Prickly
    {.script = "e117a", .revival = Revival::kScript}, // Gacious (Enhanced)
    {.script = "e118a", .revival = Revival::kScript}, // Silver Gear
    {.script = "e119a", .revival = Revival::kScript}, // Horn Head
    {.script = "e124a", .revival = Revival::kScript}, // Gacious
    {.script = "e129a", .revival = Revival::kScript}, // Master Jacket (Enhanced)
    {.script = "e131a", .revival = Revival::kScript}, // Mummy (Enhanced)
    {.script = "e132a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kDiamondDeath}, // Diamond (Enhanced)
    {.script = "e134a", .revival = Revival::kGetUp, .collapse_motion = 11, .drops_loop_sound = true}, // Rockanoff (Enhanced)
    {.script = "e138a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kClubDeath}, // Club (Enhanced)
    {.script = "e144a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kSpadeDeath}, // Spade (Enhanced)
    {.script = "e150a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kHeartDeath}, // Heart (Enhanced)
    {.script = "e161a", .revival = Revival::kGetUp, .collapse_motion = 9, .death_sound = kJokerDeath}, // Joker (Enhanced)
};

// Where a reviving monster is.
enum class Phase {
    kNone,
    kCollapse,
    kLie,
    kGetUp
};

struct Reviving {
    Phase phase = Phase::kNone;
    int   steps = 0; // steps in the phase; for kLie, steps left
};

Reviving reviving[kFloorMonsters];

// The roll a kill decided for a death script's revival: -1 none, 0 stays down, 1 revives.
int forced_roll[kFloorMonsters] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

std::vector<StaysDownRule> &Rules() {
    static std::vector<StaysDownRule> rules;
    return rules;
}

// NowMonstorUnit pointed at a unit while its monster's script commands run, then put back.
class CurrentUnit {
public:
    explicit CurrentUnit(CMonstorUnit &unit) : saved_(NowMonstorUnit) { NowMonstorUnit = &unit; }

    ~CurrentUnit() { NowMonstorUnit = saved_; }

    CurrentUnit(const CurrentUnit &) = delete;
    CurrentUnit &operator=(const CurrentUnit &) = delete;

private:
    CMonstorUnit *saved_;
};

// A percent roll as the game's scripts make it.
int Percent() {
    return static_cast<int>(100.0f * GameFraction());
}

// Runs one of the monster script commands for the current monster.
int Command(int (*command)(RS_STACKDATA *, int), std::initializer_list<RS_STACKDATA> args) {
    RS_STACKDATA stack[4] = {};
    int          count = 0;

    for (const RS_STACKDATA &arg : args) {
        stack[count++] = arg;
    }

    return command(stack, count);
}

RS_STACKDATA Int(int value) {
    RS_STACKDATA data = {};
    data.type = RS_INT;
    data.i = value;
    return data;
}

RS_STACKDATA Float(float value) {
    RS_STACKDATA data = {};
    data.type = RS_FLOAT;
    data.f = value;
    return data;
}

// Plays a motion once from its start at its own speed, as the death scripts do.
void PlayOnce(int motion) {
    Command(_SET_MOTION, {Int(motion), Float(-1.0f), Int(kMotionHoldLast | kMotionCut)});
}

// Whether monster_no's motion has reached the margin before its end.
bool MotionEnding(CMonstorUnit &unit, int monster_no) {
    CCharacter        &character = unit.chara[monster_no][0];
    const MOTION_INFO &info = character.motion_type.motion_info[character.motion_no];
    float              time = character.motion_type.state.time;

    return time >= static_cast<float>(info.start) && time <= static_cast<float>(info.end) &&
           time >= static_cast<float>(info.end) - kReviveMotionEndMargin;
}

// Starts monster_no's revival in place of its death script, which it opens as the script does.
void ReviveStart(CMonstorUnit &unit, int monster_no, const ReviveSpecies &species) {
    if (species.drops_loop_sound) {
        Command(_DEL_LOOP_SND, {});
    }

    if (species.death_sound != -1) {
        Command(_SET_SND_NOW, {Int(species.death_sound)});
    }

    Command(_SET_MOVE_CANSEL, {});
    Command(_STATUS_SET_MUTEKI, {Int(kReviveInvincibleSteps)});
    PlayOnce(species.collapse_motion);
    reviving[monster_no] = {Phase::kCollapse, 0};
    unit.script_state[monster_no] = 1;
}

const char *ScriptOf(const CMonstorUnit &unit, int monster_no) {
    return unit.model[unit.monster[monster_no].base_model].script_name;
}

} // namespace

bool HarderEnemyAiOn() {
    return OptionOn("harder_enemy_ai");
}

const ReviveSpecies *ReviveSpeciesOf(const char *script) {
    for (const ReviveSpecies &species : kSpecies) {
        if (std::strcmp(species.script, script) == 0) {
            return &species;
        }
    }

    return nullptr;
}

bool AddStaysDownRule(StaysDownRule rule) {
    for (StaysDownRule added : Rules()) {
        if (added == rule) {
            return false;
        }
    }

    if (rule == nullptr) {
        return false;
    }

    Rules().push_back(rule);
    return true;
}

bool StaysDown(const CMonstorUnit &unit, int monster_no) {
    for (StaysDownRule rule : Rules()) {
        if (rule(unit, monster_no)) {
            return true;
        }
    }

    return false;
}

int ReviveCheckDmg(CMonstorUnit &unit, int result) {
    if (AllyUnit(unit)) {
        return result;
    }

    int monster_no = unit.current_monster;

    forced_roll[monster_no] = -1;

    if (result == MONSTER_DMG_NONE) {
        return result;
    }

    // A hit or a kill hands the monster back to its script.
    reviving[monster_no] = {};

    if (result != MONSTER_DMG_KILLED) {
        return result;
    }

    const ReviveSpecies *species = ReviveSpeciesOf(ScriptOf(unit, monster_no));

    if (species == nullptr) {
        return result;
    }

    bool down = StaysDown(unit, monster_no);

    if (species->revival == Revival::kScript) {
        if (down) {
            forced_roll[monster_no] = 0;
        } else if (HarderEnemyAiOn()) {
            forced_roll[monster_no] = Percent() < kNativeReviveChance ? 1 : 0;
        }

        return result;
    }

    if (down || !HarderEnemyAiOn() || !(Percent() < kReviveChance)) {
        return result;
    }

    CurrentUnit current(unit);
    ReviveStart(unit, monster_no, *species);
    return MONSTER_DMG_NONE;
}

bool ReviveStep(CMonstorUnit &unit, int monster_no) {
    Reviving &state = reviving[monster_no];

    forced_roll[monster_no] = -1;

    if (state.phase == Phase::kNone) {
        return false;
    }

    CurrentUnit current(unit);

    switch (state.phase) {
        case Phase::kCollapse:
            // A step first, for the motion to start.
            if (++state.steps > 1 && MotionEnding(unit, monster_no)) {
                state = {Phase::kLie, kReviveLieSteps + static_cast<int>(static_cast<float>(kReviveLieSpread) * GameFraction())};
            }
            break;
        case Phase::kLie:
            if (--state.steps <= 0) {
                Command(_STATUS_SET_LIFE, {Float(kReviveLife)});
                PlayOnce(kGetUpMotion);
                state = {Phase::kGetUp, 0};
            }
            break;
        case Phase::kGetUp:
            if (++state.steps > 1 && MotionEnding(unit, monster_no)) {
                Command(_SET_MOTION, {Int(kIdleMotion)});
                state = {};
                unit.script_state[monster_no] = 0;
            }
            break;
        case Phase::kNone:
            break;
    }

    return true;
}

int ReviveRoll(int bound, int roll) {
    if (NowMonstorUnit == nullptr || bound != 100 || AllyStepping()) {
        return roll;
    }

    int &forced = forced_roll[NowMonstorUnit->current_monster];

    if (forced == -1) {
        return roll;
    }

    // The death scripts revive on a roll under their threshold.
    roll = forced == 1 ? 0 : bound - 1;
    forced = -1;
    return roll;
}

void ReviveClear() {
    if (AllyStepping()) {
        return;
    }

    for (int i = 0; i < kFloorMonsters; i++) {
        reviving[i] = {};
        forced_roll[i] = -1;
    }
}

} // namespace expanded
