#include "expanded/weapons/toan/serpent_sword.hpp"

#include <libvu0.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/data/stb.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_actions.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/snake_backup.hpp"
#include "expanded/weapons/ungaga/mirage/heat_haze.hpp"
#include "frame.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "mglib.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "savedata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int   kStopAtEnd = 2, kFromFirstFrame = 4; // CCharacter::motion_flags
constexpr float kOpaque = 128.0f;                    // CCharacter::ambient_offset[3]
constexpr float kTau = 6.2831853f;

struct Snake {
    CCharacter *model = nullptr;
    bool        up = false;
    int         fade = 0; // steps faded in, to kSnakeFadeSteps
    bool        appearing = false;
    int         left = 0;    // steps left of the appearance; the idle after it
    int         idle = 0;    // steps of the idle
    int         mouth = 0;   // steps into the mouth's cycle
    float       gape = 0.0f; // how far this cycle opens it (radians)
    bool        near[kFloorMonsters] = {};
    float       upright[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };          // turns the neck's pose at the appearance's end to its rest
    float neck[3] = {};            // where the neck ends the appearance
    float jaw_axis[3] = {0, 0, 1}; // the jaw's hinge, in its parent's space
    float jaw_open = 0.0f;         // how far open the appearance leaves it (radians)
    float jaw_scale[3] = {1, 1, 1};
    float angles[3] = {}; // the model's turn (the last solved, the next guess)
};

Snake snake;

// The glow disc on the snake's face: the steps it has grown (to kSnakeGlowSteps) and where the face last was. It outlives
// the head's fade, so it is kept apart from it.
struct Disc {
    int   steps = 0;
    float at[3] = {};
};

Disc disc;

[[maybe_unused]] const bool kRegistered = (RegisterOwnSound(kSnakeHiss), true);

// A monster near the player: its retreat left to its script (the AI loop's trigger for it and the routine's limits
// raised for its step, put back after), or, for a kind whose script has none to trigger, taken from its script.
struct Retreat {
    bool  native = false;     // its own AI retreats (the move hook leaves it be)
    bool  retreating = false; // in its retreat routine
    float best = 0.0f;        // the farthest it has got in it
    int   stuck = 0;          // steps since it last got farther
    bool  held = false;       // a kind without one: taken from its script, staggered and standing idle
    int   stagger = 0;        // steps of its stagger left before it idles

    // Values in its script raised for its step, as its script has them.
    struct Raised {
        int *at;
        int  was;
    };

    Raised raised[64] = {};
    int    raised_count = 0;
};

Retreat retreats[kFloorMonsters];

// clang-format off
constexpr SnakeRetreat kRetreats[] = {
    {Monster::MASTER_JACKET, "e01a", 0, 2},
    {Monster::SKELETON_SOLDIER, "e03a", 0, 2},
    {Monster::DASHER, "e06a", 0, 2},
    {Monster::WEREWOLF, "e07a", 0, 2},
    {Monster::FLI_FLI, "e08a", 0, 2},
    {Monster::HORNET, "e09a", 0, 2},
    {Monster::HALLOWEEN, "e10a", 0, 13},
    {Monster::SUNDAY, "e14a", 0, 2},
    {Monster::MONDAY, "e15a", 0, 2},
    {Monster::TUESDAY, "e16a", 0, 2},
    {Monster::WEDNESDAY, "e17a", 0, 2},
    {Monster::THURSDAY, "e18a", 0, 2},
    {Monster::FRIDAY, "e19a", 0, 2},
    {Monster::SATURDAY, "e20a", 0, 2},
    {Monster::WITCH_HELLZA, "e21a", 0, 2},
    {Monster::WITCH_ILLZA, "e22a", 0, 2},
    {Monster::GUNNY, "e23a", 0, 2},
    {Monster::GYON, "e24a", 0x3E70, 2},
    {Monster::PIRATES_CHARIOT, "e25a", 0x2084, 2},
    {Monster::AUNTIE_MEDU, "e26a", 0, 4},
    {Monster::CAPTAIN, "e27a", 0, 2},
    {Monster::CORCEA, "e28a", 0, 2},
    {Monster::GOLEM, "e30a", 0, 2},
    {Monster::MR_BLARE, "e31a", 0, 2},
    {Monster::DUNE, "e32a", 0, 2},
    {Monster::TITAN, "e33a", 0, 2},
    {Monster::KING_MIMIC_DBC, "e34a", 0, 2},
    {Monster::MIMIC_DBC, "e35a", 0, 2},
    {Monster::KING_MIMIC_SMT, "e36a", 0, 2},
    {Monster::MIMIC_SMT, "e37a", 0, 2},
    {Monster::KING_MIMIC_MS, "e38a", 0, 2},
    {Monster::MIMIC_MS, "e39a", 0, 2},
    {Monster::ARTHUR, "e40a", 0, 2},
    {Monster::GHOST, "e42a", 0, 2},
    {Monster::ALEXANDER, "e43a", 0, 2},
    {Monster::HEART, "e44a", 0x241C, 2},
    {Monster::CLUB, "e45a", 0, 2},
    {Monster::DIAMOND, "e46a", 0, 2},
    {Monster::SPADE, "e47a", 0, 2},
    {Monster::JOKER, "e48a", 0, 2},
    {Monster::BOMBER_HEAD, "e49a", 0, 2},
    {Monster::MUMMY, "e50a", 0, 2},
    {Monster::LICH, "e51a", 0, 2},
    {Monster::CURSE_DANCER, "e52a", 0, 2},
    {Monster::LIVING_ARMOR, "e55a", 0, 2},
    {Monster::WHITE_FANG, "e56a", 0, 2},
    {Monster::PHANTOM, "e58a", 0, 2},
    {Monster::DRAGON, "e59a", 0, 2},
    {Monster::HELL_POCKLE, "e62a", 0, 2},
    {Monster::RASH_DASHER, "e63a", 0, 2},
    {Monster::STEEL_GIANT, "e64a", 0, 2},
    {Monster::BLIZZARD, "e65a", 0, 2},
    {Monster::BILLY, "e69a", 0, 2},
    {Monster::VULCAN, "e70a", 0, 2},
    {Monster::CRABBY_HERMIT, "e71a", 0, 2},
    {Monster::SPACE_GYON, "e72a", 0, 2},
    {Monster::BLUE_DRAGON, "e73a", 0, 2},
    {Monster::BLACK_DRAGON, "e74a", 0, 2},
    {Monster::MASK_OF_PRAJNA, "e75a", 0, 2},
    {Monster::CRESCENT_BARON, "e76a", 0, 2},
    {Monster::ROCKANOFF, "e77a", 0, 2},
    {Monster::KING_MIMIC_WOF, "e78a", 0, 2},
    {Monster::MIMIC_WOF, "e79a", 0, 2},
    {Monster::KING_MIMIC_SW, "e80a", 0, 2},
    {Monster::MIMIC_SW, "e81a", 0, 2},
    {Monster::KING_MIMIC_GOT, "e82a", 0, 2},
    {Monster::MIMIC_GOT, "e83a", 0, 2},
    {Monster::SAM, "e86a", 0, 2},
    {Monster::GOL, "e90a", 0, 2},
    {Monster::SIL, "e91a", 0, 2},
    {Monster::YAMMICH, "e101a", 0, 2},
    {Monster::STATUE_DOG, "e103a", 0, 2},
    {Monster::OPAR, "e104a", 0, 2},
    {Monster::HALEY_HOLEY, "e105a", 0, 2},
    {Monster::KING_PRICKLY, "e106a", 0, 2},
    {Monster::GEMRON_FIRE, "e111a", 0, 2},
    {Monster::WHITE_FANG_ENHANCED, "e125a", 0, 2},
    {Monster::ARTHUR_ENHANCED, "e126a", 0, 2},
    {Monster::SIL_ENHANCED, "e127a", 0, 2},
    {Monster::HALLOWEEN_ENHANCED, "e128a", 0, 13},
    {Monster::MASTER_JACKET_ENHANCED, "e129a", 0, 2},
    {Monster::VULCAN_ENHANCED, "e130a", 0, 2},
    {Monster::MUMMY_ENHANCED, "e131a", 0, 2},
    {Monster::DIAMOND_ENHANCED, "e132a", 0, 2},
    {Monster::GEMRON_ICE, "e112a", 0, 2},
    {Monster::HORN_HEAD, "e119a", 0, 2},
    {Monster::AUNTIE_MEDU_ENHANCED, "e133a", 0, 4},
    {Monster::ROCKANOFF_ENHANCED, "e134a", 0, 2},
    {Monster::YAMMICH_ENHANCED, "e135a", 0, 2},
    {Monster::WITCH_HELLZA_ENHANCED, "e136a", 0, 2},
    {Monster::STEEL_GIANT_ENHANCED, "e137a", 0, 2},
    {Monster::CLUB_ENHANCED, "e138a", 0, 2},
    {Monster::CORCEA_ENHANCED, "e139a", 0, 2},
    {Monster::MIMIC_DS, "e109a", 0, 2},
    {Monster::KING_MIMIC_DS, "e110a", 0, 2},
    {Monster::GEMRON_THUNDER, "e113a", 0, 2},
    {Monster::BISHOP_Q, "e116a", 0, 2},
    {Monster::GOL_ENHANCED, "e141a", 0, 2},
    {Monster::MASK_OF_PRAJNA_ENHANCED, "e142a", 0, 2},
    {Monster::GYON_ENHANCED, "e143a", 0x3F90, 2},
    {Monster::SPADE_ENHANCED, "e144a", 0, 2},
    {Monster::RASH_DASHER_ENHANCED, "e145a", 0, 2},
    {Monster::CAPTAIN_ENHANCED, "e146a", 0, 2},
    {Monster::MIMIC_DS_ENHANCED, "e147a", 0, 2},
    {Monster::KING_MIMIC_DS_ENHANCED, "e148a", 0, 2},
    {Monster::GEMRON_WIND, "e114a", 0, 2},
    {Monster::SILVER_GEAR, "e118a", 0, 2},
    {Monster::ALEXANDER_ENHANCED, "e149a", 0, 2},
    {Monster::HEART_ENHANCED, "e150a", 0x241C, 2},
    {Monster::BOMBER_HEAD_ENHANCED, "e151a", 0, 2},
    {Monster::CRABBY_HERMIT_ENHANCED, "e152a", 0, 2},
    {Monster::PIRATES_CHARIOT_ENHANCED, "e154a", 0x22A4, 2},
    {Monster::SPACE_GYON_ENHANCED, "e155a", 0, 2},
    {Monster::MIMIC_DS_ENHANCED_TWICE, "e156a", 0, 2},
    {Monster::KING_MIMIC_DS_ENHANCED_TWICE, "e157a", 0, 2},
    {Monster::GEMRON_HOLY, "e115a", 0, 2},
    {Monster::GACIOUS_ENHANCED, "e117a", 0, 2},
    {Monster::CRESCENT_BARON_ENHANCED, "e159a", 0, 2},
    {Monster::STATUE_DOG_ENHANCED, "e160a", 0, 2},
    {Monster::JOKER_ENHANCED, "e161a", 0, 2},
    {Monster::LICH_ENHANCED, "e162a", 0, 2},
    {Monster::TITAN_ENHANCED, "e163a", 0, 2},
    {Monster::LIVING_ARMOR_ENHANCED, "e164a", 0, 2},
    {Monster::MIMIC_DS_ENHANCED_THRICE, "e165a", 0, 2},
    {Monster::KING_MIMIC_DS_ENHANCED_THRICE, "e166a", 0, 2},
};
// clang-format on

// A monster in play: its offset from the player across the ground and its distance, false when it is not in play.
bool Offset(CMonstorUnit &unit, int monster, float &dx, float &dz, float &distance) {
    if (monster < 0 || monster >= kFloorMonsters || !MonsterInPlay(unit.monster[monster])) {
        return false;
    }
    float at[4];
    unit.chara[monster][0].GetPosition(at);
    dx = at[0] - CharaMain.pos[0];
    dz = at[2] - CharaMain.pos[2];
    distance = std::sqrt(dx * dx + dz * dz);
    return true;
}

// A monster in play near the player: within the retreat radius.
bool Near(CMonstorUnit &unit, int monster, float &dx, float &dz, float &distance) {
    return Offset(unit, monster, dx, dz, distance) && snake.near[monster];
}

// Marks the monsters near the player; true when one comes near.
bool MarkNear(CMonstorUnit &unit) {
    bool came = false;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        float dx = 0.0f, dz = 0.0f, distance = 0.0f;
        bool  near = Offset(unit, monster, dx, dz, distance) && distance < kSnakeRetreatRadius;
        came = came || (near && !snake.near[monster]);
        snake.near[monster] = near;
    }
    return came;
}

// How far a routine's code is looked through for its end; the AI loop's label; how deep a call is looked into for a shot.
constexpr int kRoutineCells = 4096, kAiLabel = 100, kShotDepth = 2, kPlayerId = -2, kIdleMotion = 0, kWalkMotion = 1;

// A cell pushing the int value, and a function's last cell (its return, the padding after it).
bool Pushes(const vmcode_t &cell, int value) {
    return cell.op == static_cast<int>(stb::kPushConst) && cell.arg1 == static_cast<int>(stb::kInt) && cell.arg2 == value;
}

bool Ends(const vmcode_t *cell) {
    return cell[0].op == static_cast<int>(stb::kReturn) && cell[1].op == 0;
}

// A function's record in a loaded script's code (as a CALL names it): where its code starts and its argument count.
struct Record {
    std::int32_t addr, name, local, arg;
};

Record RecordAt(const char *code, int record) {
    Record found{};
    std::memcpy(&found, code + record, sizeof(found));
    return found;
}

// Whether the loaded script holds the retreat where the table has it: a function of as many arguments whose code (or a
// function it calls) sets the retreat's motion (_SET_MOTION's command and the motion pushed in turn).
bool HoldsRetreat(const char *code, const SnakeRetreat &retreat) {
    constexpr int kCells = kRoutineCells;
    auto          cells = [&](const Record &function) { return reinterpret_cast<const vmcode_t *>(code + function.addr); };
    auto          sets_motion = [&](const Record &function) {
        const vmcode_t *cell = cells(function);
        for (int i = 0; i < kCells && !Ends(cell); i++, cell++) {
            if (Pushes(cell[0], stb::kSetMotion) && Pushes(cell[1], retreat.motion)) {
                return true;
            }
        }
        return false;
    };
    Record function = RecordAt(code, retreat.record);
    if (function.arg != retreat.argc) {
        return false;
    }
    if (sets_motion(function)) {
        return true;
    }
    const vmcode_t *cell = cells(function);
    for (int i = 0; i < kCells && !Ends(cell); i++, cell++) {
        if (cell->op == static_cast<int>(stb::kCall) && sets_motion(RecordAt(code, cell->arg2))) {
            return true;
        }
    }
    return false;
}

// Whether a monster may retreat now: an ordinary one always, a mimic once out of its chest and done with its wake (the
// "appear" clip its chest's opening plays before its AI starts), bosses never.
bool MayRetreat(const MONSTOR &live, const MONSTOR_MODEL &model) {
    switch (model.kind) {
        case MONSTER_KIND_NORMAL:
            return true;
        case MONSTER_KIND_MIMIC_SMALL:
            return live.revealed == -1 && live.requested_motion != kMimicWake;
        case MONSTER_KIND_MIMIC_LARGE:
            return live.revealed == -1 && live.requested_motion != kKingMimicWake;
        default:
            return false;
    }
}

// What a script's variable measures, by how its code sets it: the distance to the player (_GET_DISTANCE into it alone),
// the distance from a point (a point's three coordinates, then it), a roll (_GET_RAND into it), or a count (itself plus
// one, stored back).
enum class Measure {
    kNone,
    kPlayer,
    kPoint,
    kRoll,
    kCount,
};

Measure MeasureOf(const vmcode_t *cells, int count, int var) {
    for (int i = 0; i < count; i++) {
        const vmcode_t &cell = cells[i];
        if (cell.op == static_cast<int>(stb::kExt) && cell.arg1 >= 2 && i >= cell.arg1 && cells[i - 1].op == static_cast<int>(stb::kLoadAddress) &&
            cells[i - 1].arg1 == var) {
            if (Pushes(cells[i - cell.arg1], stb::kGetDistance)) {
                return cell.arg1 == 2 ? Measure::kPlayer : Measure::kPoint;
            }
            if (Pushes(cells[i - cell.arg1], stb::kGetRand)) {
                return Measure::kRoll;
            }
        }
        if (i + 4 < count && cell.op == static_cast<int>(stb::kLoadAddress) && cell.arg1 == var && cells[i + 1].op == static_cast<int>(stb::kLoad) &&
            cells[i + 1].arg1 == var && Pushes(cells[i + 2], 1) && cells[i + 3].op == static_cast<int>(stb::kAdd) && cells[i + 4].op == static_cast<int>(stb::kStore)) {
            return Measure::kCount;
        }
    }
    return Measure::kNone;
}

// A function's code, its cells to its end.
vmcode_t *CodeOf(char *code, int record, int &count) {
    auto *cells = reinterpret_cast<vmcode_t *>(code + RecordAt(code, record).addr);
    count = 0;
    while (count < kRoutineCells && !Ends(cells + count)) {
        count++;
    }
    return cells;
}

// A condition at cell i: a variable compared with a constant (the cell after), then (past a marker the compiler may put
// there) a jump past what it guards; the jump's cell, or -1.
int ComparesAt(const vmcode_t *cells, int count, int i, std::initializer_list<int> compares) {
    if (i + 3 >= count || cells[i].op != static_cast<int>(stb::kLoad) || cells[i + 1].op != static_cast<int>(stb::kPushConst) ||
        cells[i + 2].op != static_cast<int>(stb::kCompare) ||
        std::find(compares.begin(), compares.end(), cells[i + 2].arg1) == compares.end()) {
        return -1;
    }
    int jump = cells[i + 3].op == static_cast<int>(stb::kMarker) && i + 4 < count ? i + 4 : i + 3;
    return cells[jump].op == static_cast<int>(stb::kJumpFalse) ? jump : -1;
}

// The AI loop (label 100): its code, or null.
vmcode_t *AiLoop(CRunScript &script, int &count) {
    const char *base = reinterpret_cast<const char *>(script.prog);
    const auto *labels = reinterpret_cast<const RS_PROGDATA *>(base + script.prog->prog);
    for (int i = 0; i < script.prog->prog_num; i++) {
        if (labels[i].no == kAiLabel) {
            return CodeOf(script.code, static_cast<int>(base + labels[i].func - script.code), count);
        }
    }
    return nullptr;
}

// The AI loop's trigger for the retreat (its label 100): a call to the routine after a condition on the distance to the
// player, at most (or under) so much (since the call before it). Some loops back off whenever the player is that close
// (a too-close backup); most roll for it too (a condition on a roll there as well, its cooldown between: a dodge). The distance's constant and
// the roll's (null for a backup), a backup's call first; false when the loop never calls the routine so.
bool TriggerOf(CRunScript &script, int record, vmcode_t *&distance, vmcode_t *&roll) {
    int       count = 0;
    vmcode_t *cells = AiLoop(script, count);
    distance = roll = nullptr;
    if (cells == nullptr) {
        return false;
    }
    for (int call = 0; call < count; call++) {
        if (cells[call].op != static_cast<int>(stb::kCall) || cells[call].arg2 != record) {
            continue;
        }
        // The conditions since the call before it (the nearest distance and roll).
        vmcode_t *near = nullptr, *rolled = nullptr;
        for (int c = call - 1; c >= 0 && cells[c].op != static_cast<int>(stb::kCall); c--) {
            if (ComparesAt(cells, count, c, {static_cast<int>(stb::kLessThan), static_cast<int>(stb::kLessOrEqual)}) < 0) {
                continue;
            }
            Measure measure = MeasureOf(cells, count, cells[c].arg1);
            if (measure == Measure::kPlayer && near == nullptr) {
                near = &cells[c + 1];
            } else if (measure == Measure::kRoll && rolled == nullptr) {
                rolled = &cells[c + 1];
            }
        }
        if (near != nullptr && (distance == nullptr || (roll != nullptr && rolled == nullptr))) {
            distance = near;
            roll = rolled;
        }
    }
    return distance != nullptr;
}

// Whether the script is in the routine (running it, or a function it called).
bool InRoutine(const CRunScript &script, int record) {
    int addr = RecordAt(script.code, record).addr;
    if (script.func != nullptr && script.func->addr == addr) {
        return true;
    }
    for (const RS_CALLDATA *call = script.call; call != nullptr && call < script.call_sp; call++) {
        if (call->func != nullptr && call->func->addr == addr) {
            return true;
        }
    }
    return false;
}

// Puts the raised values back as its script has them.
void Restore(Retreat &retreat) {
    for (int i = retreat.raised_count; i-- > 0;) {
        *retreat.raised[i].at = retreat.raised[i].was;
    }
    retreat.raised_count = 0;
}

// A word of its script set to value for the step.
void Set(Retreat &retreat, int &word, int value) {
    if (retreat.raised_count >= static_cast<int>(std::size(retreat.raised))) {
        return;
    }
    retreat.raised[retreat.raised_count++] = {&word, word};
    word = value;
}

// A constant (a pushed int or float) raised to to for the step.
void Raise(Retreat &retreat, vmcode_t &constant, float to) {
    Set(retreat, constant.arg2, constant.arg1 == static_cast<int>(stb::kFloat) ? std::bit_cast<int>(to) : static_cast<int>(to));
}

// The routine's own limits for the step: where it stops once the player is so far it goes on to kSnakeRetreatTo; where
// it stops once so far from where it started, or after so many hops, it goes on (to kSnakeRetreatHops hops) while the
// monster is nearer than kSnakeRetreatTo and getting farther, and stops at its next check once it is that far, or has
// got no farther for kSnakeRetreatStuck steps (a wall at its back).
void RaiseLimits(CRunScript &script, Retreat &retreat, int record, float distance) {
    int       count = 0;
    vmcode_t *cells = CodeOf(script.code, record, count);
    if (!retreat.retreating || distance > retreat.best + kSnakeRetreatGain) {
        retreat.best = distance;
        retreat.stuck = 0;
    } else {
        retreat.stuck++;
    }
    retreat.retreating = true;
    bool far = distance >= kSnakeRetreatTo || retreat.stuck >= kSnakeRetreatStuck;
    for (int i = 0; i + 3 < count; i++) {
        if (ComparesAt(cells, count, i, {static_cast<int>(stb::kGreaterThan), static_cast<int>(stb::kGreaterOrEqual)}) < 0) {
            continue;
        }
        switch (MeasureOf(cells, count, cells[i].arg1)) {
            case Measure::kPlayer:
                Raise(retreat, cells[i + 1], kSnakeRetreatTo);
                break;
            case Measure::kPoint:
                Raise(retreat, cells[i + 1], far ? 0.0f : kSnakeRetreatUnlimited);
                break;
            case Measure::kCount:
                Raise(retreat, cells[i + 1], far ? 0.0f : static_cast<float>(kSnakeRetreatHops));
                break;
            default:
                break;
        }
    }
}

// Whether a function walks the monster in until the player is within its first argument (a distance to the player
// compared at most, or under, the argument).
bool WalksIn(char *code, int record) {
    if (RecordAt(code, record).arg < 1) {
        return false;
    }
    int       count = 0;
    vmcode_t *cells = CodeOf(code, record, count);
    for (int i = 0; i + 2 < count; i++) {
        if (cells[i].op == static_cast<int>(stb::kLoad) && cells[i + 1].op == static_cast<int>(stb::kLoad) && cells[i + 1].arg1 == 0 &&
            cells[i + 2].op == static_cast<int>(stb::kCompare) &&
            (cells[i + 2].arg1 == static_cast<int>(stb::kLessThan) || cells[i + 2].arg1 == static_cast<int>(stb::kLessOrEqual)) &&
            MeasureOf(cells, count, cells[i].arg1) == Measure::kPlayer) {
            return true;
        }
    }
    return false;
}

// Whether a function fires a shot: _SET_SHOT's (or _SET_SHOT2's) command pushed in its code, or in a function it calls
// (depth calls deep).
bool Fires(char *code, int record, int depth) {
    int       count = 0;
    vmcode_t *cells = CodeOf(code, record, count);
    for (int i = 0; i < count; i++) {
        if (Pushes(cells[i], stb::kSetShot) || Pushes(cells[i], stb::kSetShot2) ||
            (depth > 0 && cells[i].op == static_cast<int>(stb::kCall) && Fires(code, cells[i].arg2, depth - 1))) {
            return true;
        }
    }
    return false;
}

// Whether a function charges the player: takes the player's position, moves, and plays a motion of its own (an attack's;
// not the idle, the walk, or the backup's motion), in its code or in a function it calls (depth calls deep).
bool Charges(char *code, int record, int backup, int depth) {
    int       count = 0;
    vmcode_t *cells = CodeOf(code, record, count);
    bool      aims = false, moves = false, attacks = false;
    for (int i = 0; i < count; i++) {
        if (depth > 0 && cells[i].op == static_cast<int>(stb::kCall) && Charges(code, cells[i].arg2, backup, depth - 1)) {
            return true;
        }
        aims = aims || (i + 2 < count && Pushes(cells[i], stb::kGetPosition) && Pushes(cells[i + 1], -kPlayerId) &&
                        cells[i + 2].op == static_cast<int>(stb::kNegate));
        moves = moves || Pushes(cells[i], stb::kSetMove);
        if (i + 1 < count && Pushes(cells[i], stb::kSetMotion) && cells[i + 1].op == static_cast<int>(stb::kPushConst) &&
            cells[i + 1].arg1 == static_cast<int>(stb::kInt)) {
            int motion = cells[i + 1].arg2;
            attacks = attacks || (motion != kIdleMotion && motion != kWalkMotion && motion != backup);
        }
    }
    return aims && moves && attacks;
}

// The AI loop's approach for the step: what it does only while the player is more than so far, by a call that fires
// no shot (an approach, not an attack), only beyond kSnakeApproachTo; where it walks the monster in to within so far of
// the player, to kSnakeApproachTo.
void RaiseApproach(CRunScript &script, Retreat &retreat) {
    int       count = 0;
    vmcode_t *cells = AiLoop(script, count);
    if (cells == nullptr) {
        return;
    }
    auto at = [&](int i) { return static_cast<int>(reinterpret_cast<char *>(&cells[i]) - script.code); };
    auto value = [](const vmcode_t &constant) {
        return constant.arg1 == static_cast<int>(stb::kFloat) ? std::bit_cast<float>(constant.arg2) : static_cast<float>(constant.arg2);
    };
    for (int i = 0; i < count; i++) {
        int  beyond = ComparesAt(cells, count, i, {static_cast<int>(stb::kGreaterThan), static_cast<int>(stb::kGreaterOrEqual)});
        bool farther = beyond >= 0;
        int  jump = farther ? beyond : ComparesAt(cells, count, i, {static_cast<int>(stb::kLessThan), static_cast<int>(stb::kLessOrEqual)});
        if (jump >= 0 && MeasureOf(cells, count, cells[i].arg1) == Measure::kPlayer) {
            bool calls = false, fires = false;
            for (int j = jump + 1; j < count && at(j) < cells[jump].arg1; j++) {
                if (cells[j].op == static_cast<int>(stb::kCall)) {
                    calls = true;
                    fires = fires || Fires(script.code, cells[j].arg2, kShotDepth);
                }
            }
            if (calls && !fires && farther && value(cells[i + 1]) < kSnakeApproachTo) {
                Raise(retreat, cells[i + 1], kSnakeApproachTo);
            }
        }
        if (cells[i].op == static_cast<int>(stb::kCall) && WalksIn(script.code, cells[i].arg2)) {
            // Its arguments pushed in turn before the call: the first, the distance, as many cells back.
            int  first = i - RecordAt(script.code, cells[i].arg2).arg;
            bool pushed = first >= 0;
            for (int j = first; pushed && j < i; j++) {
                pushed = cells[j].op == static_cast<int>(stb::kPushConst);
            }
            if (pushed && value(cells[first]) < kSnakeApproachTo) {
                Raise(retreat, cells[first], kSnakeApproachTo);
            }
        }
    }
}

// What the AI loop calls that leads to a shot (fires one, or calls what does, kShotDepth calls deep), or charges the
// player (backup the kind's backup motion), held for the step: each returns at once (0 pushed, then the return, over
// its first two cells), so its attack, and whatever leads up to it (an aim, a turn), never begins; the loop carries on.
void HoldAttacks(CRunScript &script, Retreat &retreat, vmcode_t *cells, int count, int backup) {
    for (int i = 0; i < count; i++) {
        if (cells[i].op != static_cast<int>(stb::kCall) ||
            (!Fires(script.code, cells[i].arg2, kShotDepth) && !Charges(script.code, cells[i].arg2, backup, kShotDepth))) {
            continue;
        }
        int       called = 0;
        vmcode_t *code = CodeOf(script.code, cells[i].arg2, called);
        if (called >= 2 && code[1].op != static_cast<int>(stb::kReturn)) {
            Set(retreat, code[0].op, static_cast<int>(stb::kPushConst));
            Set(retreat, code[0].arg1, static_cast<int>(stb::kInt));
            Set(retreat, code[0].arg2, 0);
            Set(retreat, code[1].op, static_cast<int>(stb::kReturn));
        }
    }
}

// The monster's retreat, when its kind has one, it may retreat now and its loaded script holds it.
const SnakeRetreat *RetreatOf(CMonstorUnit &unit, int monster) {
    const MONSTOR       &live = unit.monster[monster];
    const MONSTOR_MODEL &model = unit.model[live.base_model];
    const SnakeRetreat  *retreat = SnakeRetreatOf(ModelKind(live.base_model));
    CRunScript          &script = unit.interpreter[monster];
    if (retreat == nullptr || !MayRetreat(live, model) || script.prog == nullptr) {
        return nullptr;
    }
    if (retreat->record == 0) {
        // The backup added to its script (snake_backup.hpp), when its script took it.
        return SnakeBackupOf(script.code) != nullptr ? retreat : nullptr;
    }
    return std::strcmp(model.script_name, retreat->script) == 0 && HoldsRetreat(script.code, *retreat) ? retreat : nullptr;
}

// A motion's length in steps, from its key (its frames at its speed).
int Steps(int motion) {
    const MOTION_INFO *info = snake.model->GetMotionInfo(motion);
    if (info == nullptr || info->speed <= 0.0f) {
        return 60;
    }
    return static_cast<int>(std::ceil(static_cast<float>(info->end - info->start) / info->speed));
}

// The appearance from its first frame, held on its last.
void Appear() {
    snake.model->motion_no = kSnakeAppear;
    snake.model->motion_flags |= kStopAtEnd | kFromFirstFrame;
    snake.model->motion_speed = -1.0f;
    snake.appearing = true;
    snake.left = Steps(kSnakeAppear);
    snake.idle = 0;
}

float Ease(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// How far the mouth's cycle has it open, 0..1: easing open, held, easing shut, then shut to the cycle's end.
float MouthCurve(int step) {
    if (step < kSnakeMouthEase) {
        return Ease(static_cast<float>(step) / kSnakeMouthEase);
    }
    step -= kSnakeMouthEase;
    if (step < kSnakeMouthHold) {
        return 1.0f;
    }
    step -= kSnakeMouthHold;
    return 1.0f - Ease(static_cast<float>(step) / kSnakeMouthEase);
}

constexpr int kMouthShutFrom = 2 * kSnakeMouthEase + kSnakeMouthHold;

// The appearance played through, then the idle: the mouth's cycle (the first shutting it from where the appearance left
// it), opened afresh when a monster comes near while it is shut.
void Idle(bool startled) {
    if (snake.appearing) {
        if (--snake.left > 0) {
            return;
        }
        snake.appearing = false;
        snake.mouth = kSnakeMouthEase + kSnakeMouthHold;
        snake.gape = snake.jaw_open;
        return;
    }
    snake.idle++;
    if (++snake.mouth >= kSnakeMouthPeriod || (startled && snake.mouth >= kMouthShutFrom)) {
        snake.mouth = 0;
        snake.gape = kSnakeMouthAngle;
    }
}

float MouthAngle() {
    return snake.appearing ? snake.jaw_open : snake.gape * MouthCurve(snake.mouth);
}

// The jaw turned angle about its hinge from shut (its rest turn), at its scale.
void SetJaw(CFrame *jaw, float angle) {
    const float *a = snake.jaw_axis;
    float        c = std::cos(angle), s = std::sin(angle), k = 1.0f - c;
    float        turn[3][3] = {
        {c + k * a[0] * a[0],        k * a[0] * a[1] + s * a[2], k * a[0] * a[2] - s * a[1]},
        {k * a[1] * a[0] - s * a[2], c + k * a[1] * a[1],        k * a[1] * a[2] + s * a[0]},
        {k * a[2] * a[0] + s * a[1], k * a[2] * a[1] - s * a[0], c + k * a[2] * a[2]       },
    };
    for (int r = 0; r < 3; r++) {
        for (int col = 0; col < 3; col++) {
            jaw->local[r][col] = snake.jaw_scale[r] * turn[r][col];
        }
    }
}

// The jaw's hinge and how far the appearance leaves it open, from its pose there (a turn from shut, its rest).
void ReadJaw(const CFrame &jaw) {
    float turn[3][3];
    for (int r = 0; r < 3; r++) {
        float length = std::sqrt(jaw.local[r][0] * jaw.local[r][0] + jaw.local[r][1] * jaw.local[r][1] + jaw.local[r][2] * jaw.local[r][2]);
        snake.jaw_scale[r] = length > 0.0f ? length : 1.0f;
        for (int c = 0; c < 3; c++) {
            turn[r][c] = jaw.local[r][c] / snake.jaw_scale[r];
        }
    }
    float axis[3] = {turn[1][2] - turn[2][1], turn[2][0] - turn[0][2], turn[0][1] - turn[1][0]};
    float twice_sin = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    float cos = std::clamp((turn[0][0] + turn[1][1] + turn[2][2] - 1.0f) * 0.5f, -1.0f, 1.0f);
    if (twice_sin < 1e-4f) {
        snake.jaw_open = 0.0f;
        return;
    }
    for (int c = 0; c < 3; c++) {
        snake.jaw_axis[c] = axis[c] / twice_sin;
    }
    snake.jaw_open = std::atan2(twice_sin * 0.5f, cos);
}

// The engine's turn of a frame by angles (CFrame's srt: about x, then y, then z).
void Turned(sceVu0FMATRIX out, const float *angles) {
    sceVu0UnitMatrix(out);
    sceVu0RotMatrixX(out, out, angles[0]);
    sceVu0RotMatrixY(out, out, angles[1]);
    sceVu0RotMatrixZ(out, out, angles[2]);
}

// The angles that turn as target does (its 3x3), from guess on (Gauss-Newton on the nine entries).
void AnglesFor(sceVu0FMATRIX target, float *angles) {
    constexpr float kStep = 1e-3f;
    for (int round = 0; round < 12; round++) {
        sceVu0FMATRIX at;
        Turned(at, angles);
        float residual[9], jacobian[9][3];
        for (int k = 0; k < 9; k++) {
            residual[k] = at[k / 3][k % 3] - target[k / 3][k % 3];
        }
        for (int a = 0; a < 3; a++) {
            float moved[3] = {angles[0], angles[1], angles[2]};
            moved[a] += kStep;
            sceVu0FMATRIX near;
            Turned(near, moved);
            for (int k = 0; k < 9; k++) {
                jacobian[k][a] = (near[k / 3][k % 3] - at[k / 3][k % 3]) / kStep;
            }
        }
        float normal[3][3] = {}, rhs[3] = {};
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                for (int k = 0; k < 9; k++) {
                    normal[a][b] += jacobian[k][a] * jacobian[k][b];
                }
            }
            for (int k = 0; k < 9; k++) {
                rhs[a] -= jacobian[k][a] * residual[k];
            }
        }
        float det = normal[0][0] * (normal[1][1] * normal[2][2] - normal[1][2] * normal[2][1]) -
                    normal[0][1] * (normal[1][0] * normal[2][2] - normal[1][2] * normal[2][0]) +
                    normal[0][2] * (normal[1][0] * normal[2][1] - normal[1][1] * normal[2][0]);
        if (std::fabs(det) < 1e-9f) {
            return;
        }
        for (int a = 0; a < 3; a++) {
            float swapped[3][3];
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    swapped[r][c] = c == a ? rhs[r] : normal[r][c];
                }
            }
            float d = swapped[0][0] * (swapped[1][1] * swapped[2][2] - swapped[1][2] * swapped[2][1]) -
                      swapped[0][1] * (swapped[1][0] * swapped[2][2] - swapped[1][2] * swapped[2][0]) +
                      swapped[0][2] * (swapped[1][0] * swapped[2][1] - swapped[1][1] * swapped[2][0]);
            angles[a] += d / det;
        }
    }
}

// Ahead of and above the player, where the guard holds the blade, facing as the player faces; the head held where its
// root is, whatever its motion moves.
void Place() {
    float yaw = PlayerYaw();
    float above[3] = {CharaMain.pos[0] + std::sin(yaw) * kSnakeAhead, CharaMain.pos[1] + kSnakeHeight,
                      CharaMain.pos[2] + std::cos(yaw) * kSnakeAhead};
    // The counter-turn first, then the player's yaw, as the model's own angles.
    sceVu0FMATRIX upright, target;
    sceVu0UnitMatrix(upright);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            upright[r][c] = snake.upright[r][c];
        }
    }
    sceVu0RotMatrixY(target, upright, yaw);
    AnglesFor(target, snake.angles);
    snake.model->SetRotation(snake.angles[0], snake.angles[1], snake.angles[2]);
    // Where the neck ends the appearance goes kSnakeLift over that, the head's middle kSnakeBack ahead of it.
    // The idle's stretch along the facing and bob, from nothing as it starts.
    float idle = static_cast<float>(snake.idle);
    float back = kSnakeBack * kSnakeScale - kSnakeStretch * std::sin(kTau * idle / kSnakeStretchPeriod);
    float bob = kSnakeBob * std::sin(kTau * idle / kSnakeBobPeriod);
    float spot[3] = {above[0] - std::sin(yaw) * back, above[1] + kSnakeLift + bob, above[2] - std::cos(yaw) * back};
    for (int c = 0; c < 3; c++) {
        spot[c] -= kSnakeScale * (snake.neck[0] * target[0][c] + snake.neck[1] * target[1][c] + snake.neck[2] * target[2][c]);
    }
    snake.model->SetPosition(spot[0], spot[1], spot[2]);
    snake.model->SetScale(kSnakeScale, kSnakeScale, kSnakeScale);
}

// The appearance played through unseen: the turn from the neck's pose at its end to its rest pose (the model's, where
// its skin was made), which the model takes so the appearance ends with the head as the model has it; and where the neck
// ends.
void Calibrate() {
    CFrame *neck = FrameNamed(snake.model->frame, "neck");
    float   rest[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    if (neck != nullptr) {
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                rest[r][c] = neck->local[r][c];
            }
        }
    }
    Appear();
    for (int step = 0; step < snake.left + 2; step++) {
        snake.model->Step();
    }
    if (CFrame *jaw = FrameNamed(snake.model->frame, kSnakeJaw)) {
        ReadJaw(*jaw);
    }
    if (neck != nullptr) {
        std::copy(&neck->local[3][0], &neck->local[3][0] + 3, snake.neck);
        // end · upright = rest, so upright = end⁻¹ · rest (end a turn and a scale: its inverse the transpose over the
        // squared row lengths).
        float inverse[3][3];
        for (int r = 0; r < 3; r++) {
            float length = neck->local[r][0] * neck->local[r][0] + neck->local[r][1] * neck->local[r][1] + neck->local[r][2] * neck->local[r][2];
            for (int c = 0; c < 3; c++) {
                inverse[c][r] = length > 0.0f ? neck->local[r][c] / length : (r == c ? 1.0f : 0.0f);
            }
        }
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                snake.upright[r][c] = inverse[r][0] * rest[0][c] + inverse[r][1] * rest[1][c] + inverse[r][2] * rest[2][c];
            }
        }
    }
    snake.appearing = false;
    snake.left = 0;
}

// Everything but the model and what Calibrate read from it forgotten.
void Forget() {
    Snake kept = snake;
    snake = Snake{};
    snake.model = kept.model;
    std::copy(&kept.upright[0][0], &kept.upright[0][0] + 9, &snake.upright[0][0]);
    std::copy(kept.neck, kept.neck + 3, snake.neck);
    std::copy(kept.jaw_axis, kept.jaw_axis + 3, snake.jaw_axis);
    std::copy(kept.jaw_scale, kept.jaw_scale + 3, snake.jaw_scale);
    snake.jaw_open = kept.jaw_open;
}

} // namespace

bool SerpentSwordActive() {
    return WeaponActive(ITEM_WEAPON_SERPENT_SWORD) && SaveData != nullptr && SaveData->GetGameFlag(kKillerSnakeBeaten) != 0;
}

int SnakeSlayerAmount(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount) {
    // The wielder's own weapon hits: they carry its anti-monster table.
    bool wielders = hit.vs_monster != nullptr && UserStatus != nullptr && hit.owner == UserStatus->cur_chara;
    if (!wielders || !SerpentSwordActive()) {
        return amount;
    }
    int  kind = ModelKind(unit.monster[monster].base_model);
    bool medu = kind == No(Monster::AUNTIE_MEDU) || kind == No(Monster::AUNTIE_MEDU_ENHANCED);
    return medu ? unit.monster[monster].hp : amount;
}

void SnakeCharmerStep() {
    if (!SerpentSwordActive() || BtEventMode != 0) {
        Forget();
        disc = {};
        return;
    }
    snake.up = Guarding();
    disc.steps = snake.up ? std::min(kSnakeGlowSteps, disc.steps + 1) : std::max(0, disc.steps - 1);
    if (!snake.up) {
        snake.fade = std::max(0, snake.fade - 1);
        std::fill(std::begin(snake.near), std::end(snake.near), false);
        if (snake.fade == 0) {
            Forget();
            return;
        }
    }
    if (snake.model == nullptr) {
        if ((snake.model = EnterFloorCharacter(kKillerSnakeFile, kKillerSnakeCfg)) == nullptr) {
            return;
        }
        Calibrate();
    }
    bool startled = false;
    if (snake.up) {
        if (snake.fade == 0) {
            Appear();
            PlayOwnSound(kSnakeHiss, false);
        }
        snake.fade = std::min(kSnakeFadeSteps, snake.fade + 1);
        if (NowMonstorUnit != nullptr) {
            startled = MarkNear(*NowMonstorUnit);
        }
    }
    Idle(startled);
    mirage::HazeStep(kSnakeHazeWave);
    Place();
    snake.model->Step();
    if (CFrame *face = FrameNamed(snake.model->frame, kSnakeFace)) {
        sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, at;
        face->GetWorldPosition(at, origin);
        std::copy(at, at + 3, disc.at);
    }
}

void SnakeCharmerPosed(CFrame *frame) {
    if (snake.model == nullptr || frame != snake.model->frame || snake.appearing || snake.fade == 0) {
        return;
    }
    if (CFrame *jaw = FrameNamed(frame, kSnakeJaw)) {
        SetJaw(jaw, MouthAngle());
    }
}

void SnakeCharmerDraw() {
    if (snake.fade > 0 && snake.model != nullptr && snake.model->frame != nullptr && BtEventMode == 0) {
        // Its depth first, unseen, so the see-through draw after shows only its nearest surface, not its inside.
        MGPortSetColorWrite(false);
        snake.model->ambient_offset[3] = kOpaque;
        DrawFloorCharacter(*snake.model);
        MGPortSetColorWrite(true);
        snake.model->ambient_offset[3] = kOpaque * kSnakeGhost * SnakeCharmerShown();
        DrawFloorCharacter(*snake.model);
    }
}

void SnakeCharmerGlowDraw() {
    if (disc.steps > 0 && BtEventMode == 0) {
        DrawGlowDisc(GlowLook::kSerpent, disc.at, kSnakeGlowDisc * Ease(static_cast<float>(disc.steps) / kSnakeGlowSteps), kSnakeGlowPull);
    }
}

void SnakeCharmerHazeDraw() {
    CFrame *face = snake.fade > 0 && snake.model != nullptr && snake.model->frame != nullptr && BtEventMode == 0
                       ? FrameNamed(snake.model->frame, kSnakeFace)
                       : nullptr;
    if (face != nullptr) {
        sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, at;
        face->GetWorldPosition(at, origin);
        mirage::HazeDraw(at, kSnakeHazeGain * SnakeCharmerShown(), kSnakeHaze);
    }
}

void SnakeCharmerLight(const CCharacter &character, float *ambient) {
    if (snake.fade == 0 || (&character != snake.model && &character != NowWeapon)) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] += kSnakeGlow * SnakeCharmerShown();
    }
}

bool SnakeCharmerUp() {
    return snake.up;
}

float SnakeCharmerShown() {
    return static_cast<float>(snake.fade) / kSnakeFadeSteps;
}

float SnakeCharmerGlow() {
    return static_cast<float>(disc.steps) / kSnakeGlowSteps;
}

float SnakeCharmerMouth() {
    return snake.fade > 0 ? MouthAngle() / kSnakeMouthAngle : 0.0f;
}

const SnakeRetreat *SnakeRetreatOf(int kind) {
    // An Expanded species runs its stand-in's script (rewritten in place or added to past its end): its retreat too.
    int script_kind = SpeciesStandIn(kind);
    for (const SnakeRetreat &retreat : kRetreats) {
        if (script_kind == No(retreat.kind)) {
            return &retreat;
        }
    }
    return nullptr;
}

namespace {

// The steps a monster's motion takes at its own speed.
int MotionSteps(const CMonstorUnit &unit, int monster, int motion) {
    const MOTION_INFO *clips = unit.chara[monster][0].motion_type.motion_info;
    if (clips == nullptr || clips[motion].speed <= 0.0f) {
        return 0;
    }
    return static_cast<int>(std::ceil(static_cast<float>(clips[motion].end - clips[motion].start) / clips[motion].speed));
}

// Whether the monster is between actions, where its own script would choose its next: in a looping motion (a walk, an
// idle), or at the end of a one-shot (an attack, a hit's reaction, played once).
bool AtSeam(const CMonstorUnit &unit, int monster) {
    const CCharacter  &body = unit.chara[monster][0];
    const MOTION_INFO *clips = body.motion_type.motion_info;
    if ((unit.monster[monster].requested_motion_flags & kMotionHoldLast) == 0 || clips == nullptr) {
        return true;
    }
    return body.motion_type.state.time >= static_cast<float>(clips[body.motion_no].end) - 1.0f;
}

// Its stagger (held on its last frame), or straight to its idle for a kind with none, cross-faded in.
void Stagger(CMonstorUnit &unit, int monster, Retreat &retreat, const BlindClips &clips) {
    retreat.stagger = clips.stagger >= 0 ? std::max(1, MotionSteps(unit, monster, clips.stagger)) : 0;
    PlayMonsterMotion(unit, monster, clips.stagger >= 0 ? clips.stagger : kSnakeIdle, clips.stagger >= 0 ? kMotionHoldLast : kMotionLoop);
}

// A kind whose script has no retreat, held instead: staggered, then idle where it stands, as long as it is near.
bool Hold(CMonstorUnit &unit, int monster, Retreat &retreat) {
    const MONSTOR &live = unit.monster[monster];
    BlindClips     clips;
    if (unit.model[live.base_model].kind != MONSTER_KIND_NORMAL || !BlindClipsOf(ModelKind(live.base_model), &clips)) {
        return false;
    }
    if (!retreat.held) {
        retreat.held = true;
        unit.script_state[monster] = 0;
        Stagger(unit, monster, retreat, clips);
    } else if (unit.script_state[monster] != 0) {
        // A hit's reaction: the stagger again in its place.
        unit.script_state[monster] = 0;
        Stagger(unit, monster, retreat, clips);
    } else if (retreat.stagger > 0 && --retreat.stagger == 0) {
        PlayMonsterMotion(unit, monster, kSnakeIdle, kMotionLoop);
    }
    unit.monster[monster].movement_speed = 0.0f;
    unit.monster[monster].turn_speed = 0.0f;
    return true;
}

} // namespace

bool SnakeCharmerMonsterStep(CMonstorUnit &unit, int monster) {
    Retreat            &retreat = retreats[monster];
    float               dx = 0.0f, dz = 0.0f, distance = 0.0f;
    bool                in_play = snake.up && &unit == NowMonstorUnit && unit.monster[monster].hp > 0 && Offset(unit, monster, dx, dz, distance);
    bool                near = in_play && snake.near[monster];
    const SnakeRetreat *own = in_play ? RetreatOf(unit, monster) : nullptr;
    vmcode_t           *trigger = nullptr, *roll = nullptr;
    CRunScript         &script = unit.interpreter[monster];
    int                 record = 0;
    if (own != nullptr && own->record != 0) {
        record = own->record;
        retreat.native = TriggerOf(script, record, trigger, roll);
    } else if (const SnakeBackup *added = own != nullptr ? SnakeBackupOf(script.code) : nullptr) {
        // The backup added to its script: its trigger the stub's constant.
        record = added->record;
        trigger = reinterpret_cast<vmcode_t *>(script.code + added->trigger);
        retreat.native = true;
    } else {
        retreat.native = false;
    }
    if (retreat.native) {
        // Its own AI retreats, as it would: near the player its trigger is certain (the distance it rolls within the
        // reach, the roll always made; its cooldown between), and the routine goes on to the reach's edge.
        if (near) {
            // A dodge rolls within the reach; a backup backs off as the monster comes in to its approach's end.
            Raise(retreat, *trigger, roll != nullptr ? kSnakeRetreatRadius : kSnakeApproachTo);
        }
        if (distance < kSnakeApproachRadius) {
            // Near enough to walk in or attack: its approach kept to kSnakeApproachTo (a walk-in begun out here too), its
            // shots and charges held.
            RaiseApproach(script, retreat);
            int       count = 0;
            vmcode_t *loop = AiLoop(script, count);
            if (loop != nullptr) {
                HoldAttacks(script, retreat, loop, count, own->motion);
            }
        }
        if (InRoutine(script, record) || (near && !retreat.retreating)) {
            RaiseLimits(script, retreat, record, distance);
        }
        retreat.retreating = retreat.retreating && InRoutine(script, record);
        return false;
    }
    if (!near) {
        if (retreat.held) {
            // Back to its script, from the top.
            unit.script_state[monster] = 0;
            retreat = {};
        }
        return false;
    }
    if (!retreat.held && !AtSeam(unit, monster)) {
        // Mid-action: it finishes what it is doing first.
        return false;
    }
    return Hold(unit, monster, retreat);
}

void SnakeCharmerScriptStepped(CMonstorUnit & /*unit*/, int monster) {
    if (monster >= 0 && monster < kFloorMonsters) {
        Restore(retreats[monster]);
    }
}

void SnakeCharmerMonsterMove(CMonstorUnit &unit, int monster) {
    MONSTOR &live = unit.monster[monster];
    float    dx = 0.0f, dz = 0.0f, distance = 0.0f;
    if (!snake.up || retreats[monster].native || retreats[monster].held || live.movement_speed == 0.0f || !Near(unit, monster, dx, dz, distance) ||
        distance < 0.01f) {
        return;
    }
    // A step toward the player loses what takes it nearer: sideways it may go, and away as its script has it.
    dx /= distance, dz /= distance;
    float out = live.movement[0] * dx + live.movement[2] * dz;
    if (out >= 0.0f) {
        return;
    }
    live.movement[0] -= out * dx;
    live.movement[2] -= out * dz;
    float left = std::sqrt(live.movement[0] * live.movement[0] + live.movement[2] * live.movement[2]);
    if (left < 0.01f) {
        live.movement_speed = 0.0f;
        return;
    }
    live.movement[0] /= left;
    live.movement[2] /= left;
    live.movement_speed *= left;
}

void SnakeCharmerReset() {
    snake = Snake{};
    disc = {};
    std::fill(std::begin(retreats), std::end(retreats), Retreat{});
}

} // namespace expanded
