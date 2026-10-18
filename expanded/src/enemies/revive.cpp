#include "expanded/enemies/revive.hpp"

#include <cstring>
#include <initializer_list>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/core/options.hpp"
#include "expanded/core/random.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "runscript_opcodes.hpp"

namespace expanded {

namespace {

[[maybe_unused]] const bool kRegistered = AddToggle({.key = "harder_enemy_ai",
                                                     .label = "Harder Enemy AI",
                                                     .help = "\"Harder Enemy AI\"\nFallen monsters may\nget back up."});

// Every monster with a real get-up (10); those whose death throws them back collapse with their knockdown (9).
constexpr ReviveSpecies kSpecies[] = {
    {"e01a",  Revival::kScript, 0,  -1,        false}, // Master Jacket
    {"e03a",  Revival::kGetUp,  11, -1,        false}, // Skeleton Soldier
    {"e14a",  Revival::kGetUp,  9,  -1,        false}, // Sunday
    {"e15a",  Revival::kGetUp,  9,  -1,        false}, // Monday
    {"e16a",  Revival::kGetUp,  9,  -1,        false}, // Tuesday
    {"e17a",  Revival::kGetUp,  9,  -1,        false}, // Wednesday
    {"e18a",  Revival::kGetUp,  9,  -1,        false}, // Thursday
    {"e19a",  Revival::kGetUp,  9,  -1,        false}, // Friday
    {"e20a",  Revival::kGetUp,  9,  -1,        false}, // Saturday
    {"e44a",  Revival::kGetUp,  9,  1418 + 11, false}, // Heart
    {"e45a",  Revival::kGetUp,  9,  1437 + 11, false}, // Club
    {"e46a",  Revival::kGetUp,  9,  1456 + 11, false}, // Diamond
    {"e47a",  Revival::kGetUp,  9,  1475 + 11, false}, // Spade
    {"e48a",  Revival::kGetUp,  9,  1494 + 11, false}, // Joker
    {"e50a",  Revival::kScript, 0,  -1,        false}, // Mummy
    {"e52a",  Revival::kGetUp,  9,  -1,        false}, // Curse Dancer
    {"e62a",  Revival::kGetUp,  9,  -1,        false}, // Hell Pockle
    {"e77a",  Revival::kGetUp,  11, -1,        true }, // Rockanoff
    {"e106a", Revival::kGetUp,  9,  -1,        true }, // King Prickly
    {"e117a", Revival::kScript, 0,  -1,        false}, // Gacious (Enhanced)
    {"e118a", Revival::kScript, 0,  -1,        false}, // Silver Gear
    {"e119a", Revival::kScript, 0,  -1,        false}, // Horn Head
    {"e124a", Revival::kScript, 0,  -1,        false}, // Gacious
    {"e129a", Revival::kScript, 0,  -1,        false}, // Master Jacket (Enhanced)
    {"e131a", Revival::kScript, 0,  -1,        false}, // Mummy (Enhanced)
    {"e132a", Revival::kGetUp,  9,  1456 + 11, false}, // Diamond (Enhanced)
    {"e134a", Revival::kGetUp,  11, -1,        true }, // Rockanoff (Enhanced)
    {"e138a", Revival::kGetUp,  9,  1437 + 11, false}, // Club (Enhanced)
    {"e144a", Revival::kGetUp,  9,  1475 + 11, false}, // Spade (Enhanced)
    {"e150a", Revival::kGetUp,  9,  1418 + 11, false}, // Heart (Enhanced)
    {"e161a", Revival::kGetUp,  9,  1494 + 11, false}, // Joker (Enhanced)
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

Reviving reviving[16];

// The roll a kill decided for a death script's revival: -1 none, 0 stays down, 1 revives.
int forced_roll[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

std::vector<StaysDownRule> &Rules() {
    static std::vector<StaysDownRule> rules;
    return rules;
}

// A percent roll as the game's scripts make it.
int Percent() {
    return (int) (100.0f * GameFraction());
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
    Command(_SET_MOTION, {Int(motion), Float(-1.0f), Int(2 | 4)});
}

// Whether monster_no's motion has reached the margin before its end.
bool MotionEnding(CMonstorUnit &unit, int monster_no) {
    CCharacter        &character = unit.chara[monster_no][0];
    const MOTION_INFO &info = character.motion_type.motion_info[character.motion_no];
    float              time = character.motion_type.state.time;

    return time >= (float) info.start && time <= (float) info.end && time >= (float) info.end - kReviveMotionEndMargin;
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

    CMonstorUnit *now = NowMonstorUnit;
    NowMonstorUnit = &unit;
    ReviveStart(unit, monster_no, *species);
    NowMonstorUnit = now;
    return MONSTER_DMG_NONE;
}

bool ReviveStep(CMonstorUnit &unit, int monster_no) {
    Reviving &state = reviving[monster_no];

    forced_roll[monster_no] = -1;

    if (state.phase == Phase::kNone) {
        return false;
    }

    CMonstorUnit *now = NowMonstorUnit;
    NowMonstorUnit = &unit;

    switch (state.phase) {
        case Phase::kCollapse:
            // A step first, for the motion to start.
            if (++state.steps > 1 && MotionEnding(unit, monster_no)) {
                state = {Phase::kLie, kReviveLieSteps + (int) ((float) kReviveLieSpread * GameFraction())};
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

    NowMonstorUnit = now;
    return true;
}

int ReviveRoll(int bound, int roll) {
    if (NowMonstorUnit == nullptr || bound != 100) {
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
    for (int i = 0; i < 16; i++) {
        reviving[i] = {};
        forced_roll[i] = -1;
    }
}

} // namespace expanded
