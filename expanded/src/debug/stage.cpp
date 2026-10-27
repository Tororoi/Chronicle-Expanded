#include "expanded/debug/stage.hpp"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>

#include "btactstatus.hpp"
#include "btitem.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "expanded/core/party.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/enemy_roster.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/floor_grid.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "memcard.hpp"
#include "memorycardaccess.hpp"
#include "menu_inventory.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

extern s32        gameTask;
extern CFrameVu1 *CharaFrame;

namespace expanded {

namespace {

constexpr int   kWalkFrom = 10;    // the monster step the walk to a big room starts on
constexpr float kWalkStep = 10.0f; // and how far it goes a step (further flings the player off stale collision)
constexpr int   kBigRoomCells = 9;
constexpr float kAway = 2000.0f; // where monsters not set out are sent

std::optional<Stage> active;

// The stage's progress through a run.
struct Run {
    bool  entered = false; // the dungeon's entrance has taken the stage's floor
    bool  out = false;     // the member is out (or none was asked for)
    int   steps = 0;       // monster steps since
    bool  walking = false;
    float to[2] = {};
    bool  placed = false;
};

Run run;

std::string_view Trim(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r')) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) {
        text.remove_suffix(1);
    }
    return text;
}

template <typename T>
bool Number(std::string_view text, T &out) {
    const char *end = text.data() + text.size();
    auto [at, error] = std::from_chars(text.data(), end, out);
    return error == std::errc{} && at == end;
}

bool Flag(std::string_view text, bool &out) {
    if (text == "yes" || text == "true" || text == "1") {
        out = true;
        return true;
    }
    if (text == "no" || text == "false" || text == "0") {
        out = false;
        return true;
    }
    return false;
}

bool Character(std::string_view text, int &out) {
    constexpr std::string_view kNames[] = {"toan", "xiao", "goro", "ruby", "ungaga", "osmond"};
    for (int c = 0; c < static_cast<int>(std::size(kNames)); c++) {
        if (text == kNames[c]) {
            out = c;
            return true;
        }
    }
    return Number(text, out) && out >= CHARA_TOAN && out <= CHARA_OSMOND;
}

// A count or an item number: 0 or more.
bool Count(std::string_view text, int &out) {
    return Number(text, out) && out >= 0;
}

bool Text(std::string_view text, std::string &out) {
    out = std::string(text);
    return true;
}

bool Within(std::string_view text, int &out, int low, int high) {
    return Number(text, out) && out >= low && out <= high;
}

// A stage file's key, and how its value is read into a stage; false for a bad value.
struct Key {
    std::string_view name;
    bool (*read)(std::string_view value, Stage &stage);
};

constexpr Key kKeys[] = {
    {"roster",      [](std::string_view v, Stage &s) { return Text(v, s.roster); }                          },
    {"slot",        [](std::string_view v, Stage &s) { return Count(v, s.slot) && s.slot >= 1; }            },
    {"dungeon",     [](std::string_view v, Stage &s) { return Within(v, s.dungeon, 0, 6); }                 },
    {"floor",       [](std::string_view v, Stage &s) { return Count(v, s.floor); }                          },
    {"character",   [](std::string_view v, Stage &s) { return Character(v, s.character); }                  },
    {"weapon",      [](std::string_view v, Stage &s) { return Count(v, s.weapon); }                         },
    {"attack",      [](std::string_view v, Stage &s) { return Count(v, s.attack); }                         },
    {"sphere",      [](std::string_view v, Stage &s) { return Count(v, s.sphere); }                         },
    {"seed",        [](std::string_view v, Stage &s) { return Number(v, s.seed) && s.seed >= 0; }           },
    {"place",       [](std::string_view v, Stage &s) { return Count(v, s.place); }                          },
    {"distance",    [](std::string_view v, Stage &s) { return Number(v, s.distance) && s.distance >= 0.0f; }},
    {"spread",      [](std::string_view v, Stage &s) { return Number(v, s.spread); }                        },
    {"angle",       [](std::string_view v, Stage &s) { return Number(v, s.angle); }                         },
    {"place_kind",  [](std::string_view v, Stage &s) { return Count(v, s.place_kind); }                     },
    {"place_after", [](std::string_view v, Stage &s) { return Count(v, s.place_after); }                    },
    {"big_room",    [](std::string_view v, Stage &s) { return Flag(v, s.big_room); }                        },
    {"keep_alive",  [](std::string_view v, Stage &s) { return Flag(v, s.keep_alive); }                      },
};

// One key's value into stage; false for an unknown key or a bad value.
bool Set(std::string_view key, std::string_view value, Stage &stage) {
    for (const Key &known : kKeys) {
        if (key == known.name) {
            return known.read(value, stage);
        }
    }
    return false;
}

// Puts weapon_no in the hand of chara at its maximum attack, or at attack.
void Arm(int chara, int weapon_no, int attack) {
    WEAPON_HAVE *weapon = EquippedWeapon(*UserStatus, chara);
    if (weapon == nullptr) {
        return;
    }
    WepDataListToHaveCopy(weapon_no, weapon);
    weapon->attack = attack >= 0 ? attack : GetWeaponData(weapon_no)->attack_max;
}

void SetPlayerAt(float x, float z) {
    CharaMain.SetPosition(x, CharaMain.pos[1], z);
    if (CharaFrame != nullptr) {
        CharaFrame->SetPosition(x, CharaMain.pos[1], z);
    }
}

// The middle of the nearest room of at least kBigRoomCells cells; false when the floor has none.
bool NearestBigRoom(float *to) {
    if (NowDngMap == nullptr) {
        return false;
    }
    float best = -1.0f;
    for (int r = 0; r < NowDngMap->room_num; r++) {
        const ROOM_INFO &room = NowDngMap->rooms[r];
        if (room.width * room.height < kBigRoomCells) {
            continue;
        }
        float x = 0.0f, z = 0.0f;
        RoomMiddle(room, &x, &z);
        float distance = std::hypot(x - CharaMain.pos[0], z - CharaMain.pos[2]);
        if (best < 0.0f || distance < best) {
            best = distance;
            to[0] = x;
            to[1] = z;
        }
    }
    return best >= 0.0f;
}

void Walk() {
    float dx = run.to[0] - CharaMain.pos[0], dz = run.to[1] - CharaMain.pos[2];
    float distance = std::hypot(dx, dz);
    float k = distance > kWalkStep ? kWalkStep / distance : 1.0f;
    SetPlayerAt(CharaMain.pos[0] + dx * k, CharaMain.pos[2] + dz * k);
    run.walking = distance > kWalkStep;
}

// The stage's monsters set out in front of the member, every other one in play sent away (one in a chest stays).
void Place(const Stage &stage) {
    CMonstorUnit &unit = *NowMonstorUnit;
    unsigned      placed = 0;
    int           n = 0;
    for (int m = 0; m < static_cast<int>(std::size(unit.monster)) && n < stage.place; m++) {
        const MONSTOR &monster = unit.monster[m];
        if (monster.state == -1 || monster.revealed != -1) {
            continue;
        }
        if (stage.place_kind >= 0 && ModelKind(monster.base_model) != stage.place_kind) {
            continue;
        }
        float offset = (static_cast<float>(n) - static_cast<float>(stage.place - 1) * 0.5f) * stage.spread;
        float a = CharaMain.rotation.y + stage.angle + offset;
        unit.chara[m][0].SetPosition(CharaMain.pos[0] + stage.distance * std::sin(a), CharaMain.pos[1],
                                     CharaMain.pos[2] + stage.distance * std::cos(a));
        placed |= 1u << m;
        n++;
    }
    for (int m = 0; m < static_cast<int>(std::size(unit.monster)); m++) {
        if (unit.monster[m].state != -1 && unit.monster[m].revealed == -1 && (placed >> m & 1u) == 0) {
            unit.chara[m][0].SetPosition(CharaMain.pos[0] + kAway, CharaMain.pos[1], CharaMain.pos[2] + kAway);
        }
    }
}

// The floor in play with no action or event running: when a member can be brought out.
bool InPlay() {
    return gameTask == GAME_TASK_PLAY && BtGameModeFlag == BT_GAME_MODE_PLAY && BtEventMode == 0 &&
           BtActStatus.action_on == 0;
}

} // namespace

bool ParseStage(std::string_view text, Stage &stage, std::string *why) {
    Stage parsed;
    int   line_no = 0;
    while (!text.empty()) {
        size_t           end = text.find('\n');
        std::string_view line = text.substr(0, end);
        text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
        line_no++;
        if (size_t hash = line.find('#'); hash != std::string_view::npos) {
            line = line.substr(0, hash);
        }
        line = Trim(line);
        if (line.empty()) {
            continue;
        }
        size_t equals = line.find('=');
        if (equals == std::string_view::npos) {
            if (why != nullptr) {
                *why = std::format("line {}: no '=' in \"{}\"", line_no, line);
            }
            return false;
        }
        std::string_view key = Trim(line.substr(0, equals));
        std::string_view value = Trim(line.substr(equals + 1));
        if (!Set(key, value, parsed)) {
            if (why != nullptr) {
                *why = std::format("line {}: bad {} \"{}\"", line_no, key, value);
            }
            return false;
        }
    }
    stage = parsed;
    return true;
}

bool SetStage(const char *path, std::string *why) {
    StageReset();
    if (path == nullptr || *path == '\0') {
        active.reset();
        return true;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        if (why != nullptr) {
            *why = "cannot be read";
        }
        return false;
    }
    std::ostringstream text;
    text << file.rdbuf();
    Stage stage;
    if (!ParseStage(text.str(), stage, why)) {
        return false;
    }
    if (std::string refused; !stage.roster.empty() && !SetDebugRoster(stage.roster, &refused)) {
        if (why != nullptr) {
            *why = std::format("roster {}: {}", stage.roster, refused);
        }
        return false;
    }
    if (stage.seed >= 0) {
        SeedModRandom(static_cast<unsigned>(stage.seed));
    }
    active = stage;
    return true;
}

const Stage *ActiveStage() {
    return active ? &*active : nullptr;
}

bool StageLoadSave() {
    if (!active || active->dungeon < 0) {
        return true;
    }
    McAccess.InitForMC();
    return McAccess.LoadFromMc(active->slot - 1) == 1;
}

int StageEntranceFloor() {
    if (!active || active->floor < 0 || run.entered) {
        return -1;
    }
    run.entered = true;
    return active->floor;
}

void StageStatusReady() {
    if (!active || UserStatus == nullptr) {
        return;
    }
    int chara = active->character >= 0 ? active->character : CHARA_TOAN;
    if (active->weapon >= 0) {
        Arm(chara, active->weapon, active->attack);
    }
    if (active->sphere >= 0) {
        Arm(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, active->attack);
        if (WEAPON_HAVE *steve = EquippedWeapon(*UserStatus, CHARA_XIAO); steve != nullptr) {
            steve->attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
            steve->attach[0].sphere_weapon_no = static_cast<s16>(active->sphere);
        }
    }
}

void StageFrame() {
    if (!active || run.out || UserStatus == nullptr) {
        return;
    }
    if (active->character < 0 || UserStatus->cur_chara == active->character) {
        run.out = true;
        LogEvent("stage", "\"step\":\"out\"");
        return;
    }
    if (!InPlay()) {
        return;
    }
    if (UserStatus->party_size <= active->character) {
        std::fprintf(stderr, "stage: the save's party has no character %d\n", active->character);
        run.out = true;
        return;
    }
    selectChrUnit(active->character, 0);
    run.out = true;
    LogEvent("stage", "\"step\":\"out\"");
}

void StageStep() {
    if (!active || !run.out || UserStatus == nullptr) {
        return;
    }
    if (active->keep_alive) {
        for (int c = 0; c < kPartyMembers; c++) {
            UserStatus->hp[c] = UserStatus->next_hp[c] = UserStatus->max_hp[c];
        }
    }
    run.steps++;
    if (active->big_room && run.steps == kWalkFrom) {
        run.walking = NearestBigRoom(run.to);
    }
    if (run.walking) {
        Walk();
        if (!run.walking) {
            LogEvent("stage", "\"step\":\"walked\"");
        }
    }
    bool due = active->place > 0 && !run.placed && run.steps >= active->place_after && !run.walking;
    if (due && NowMonstorUnit != nullptr) {
        Place(*active);
        run.placed = true;
        LogEvent("stage", "\"step\":\"placed\"");
    }
}

void StageReset() {
    run = {};
}

} // namespace expanded
