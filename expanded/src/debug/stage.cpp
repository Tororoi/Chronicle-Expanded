#include "expanded/debug/stage.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>
#include <fstream>
#include <iterator>
#include <numbers>
#include <optional>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "btactstatus.hpp"
#include "btitem.hpp"
#include "camerafollow.hpp"
#include "character.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "ebattle.hpp"
#include "edit.hpp"
#include "editground.hpp"
#include "editloop.hpp"
#include "editloop3.hpp"
#include "expanded/core/party.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/enemy_roster.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/floor_grid.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/town/baits.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "fishing.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "mainselect.hpp"
#include "mathutil.hpp"
#include "memcard.hpp"
#include "memorycardaccess.hpp"
#include "menu_inventory.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "savedata.hpp"
#include "userstatus.hpp"

namespace dc::game {
extern s32        gameTask;
extern CFrameVu1 *CharaFrame;
} // namespace dc::game

namespace expanded {

namespace {

constexpr int   kWalkFrom = 10;    // the monster step the walk to a big room starts on
constexpr float kWalkStep = 10.0f; // and how far it goes a step (further flings the player off stale collision)
constexpr int   kBigRoomCells = 9;
constexpr float kAway = 2000.0f; // where monsters not set out are sent

// How far in front of the member the stage's magic circle is set out.
constexpr float kCircleAhead = 40.0f;

// The sub maps (gedit/s01-s42) by MapNo, and two that are fished.
constexpr int kFirstSubMap = 11;
constexpr int kLastSubMap = 52;
constexpr int kMapBrownboo = 14;
constexpr int kMapEastHarbor = 19;

// The game's clock runs 0-12, two hours a unit from 10:00; EdGetTime's periods are morning 8.5-11.5, day 11.5-2.5,
// evening 2.5-5.5 and night 5.5-8.5. A period by name starts half a unit in.
constexpr float kClockEnd = 12.0f;
constexpr float kMorning = 9.0f;
constexpr float kDay = 0.0f;
constexpr float kEvening = 3.0f;
constexpr float kNight = 6.0f;

// A fishing sign: its program pushes _LOAD_FISHING_DATA's number (an int constant, as the disc codes one).
constexpr int kConstInt = 1;
constexpr int kLoadFishingData = 998;
// The event the square button asks for while fishing: the bait change.
constexpr int kBaitEvent = 0x86;

std::optional<Stage> active;

// The stage's progress through a run.
struct Run {
    bool  entered = false; // the dungeon's entrance has taken the stage's floor
    bool  out = false;     // the member is out (or none was asked for)
    int   steps = 0;       // monster steps since
    bool  walking = false;
    float to[2] = {};
    bool  placed = false;
    bool  circled = false;     // the magic circle set out
    bool  arrived = false;     // the stage's town in play
    bool  at_sign = false;     // the player stood at the fishing sign (or none asked for)
    bool  bait_asked = false;  // the bait change asked for
    bool  bait_picked = false; // and the stage's bait picked in it
    bool  baited = false;      // and on the hook
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

// A map by name or MapNo; the names are the towns' and two fishing maps'.
bool Town(std::string_view text, int &out) {
    constexpr std::pair<std::string_view, int> kNames[] = {
        {"norune",       TOWN_NORUNE      },
        {"matataki",     TOWN_MATATAKI    },
        {"queens",       TOWN_QUEENS      },
        {"muska_lacka",  TOWN_MUSKA_LACKA },
        {"yellow_drops", TOWN_YELLOW_DROPS},
        {"brownboo",     kMapBrownboo     },
        {"east_harbor",  kMapEastHarbor   },
    };
    for (const auto &[name, map_no] : kNames) {
        if (text == name) {
            out = map_no;
            return true;
        }
    }
    return Within(text, out, TOWN_NORUNE, TOWN_COUNT - 1) || Within(text, out, kFirstSubMap, kLastSubMap);
}

// A time of day by its period (EdGetTime's), or the game's clock.
bool Time(std::string_view text, float &out) {
    constexpr std::pair<std::string_view, float> kPeriods[] = {
        {"morning",   kMorning},
        {"day",       kDay    },
        {"afternoon", kDay    },
        {"evening",   kEvening},
        {"dusk",      kEvening},
        {"night",     kNight  },
    };
    for (const auto &[name, time] : kPeriods) {
        if (text == name) {
            out = time;
            return true;
        }
    }
    return Number(text, out) && out >= 0.0f && out < kClockEnd;
}

// A bait the picker offers.
bool Bait(std::string_view text, int &out) {
    constexpr int kBaits[] = {ITEM_EVY, ITEM_MIMI, ITEM_PRICKLY, ITEM_THROBBING_CHERRY, ITEM_GOOEY_PEACH,
                              ITEM_BOMB_NUTS, ITEM_POISONOUS_APPLE, ITEM_MELLOW_BANANA, ITEM_CARROT, ITEM_POTATO_CAKE,
                              ITEM_MINON, ITEM_BATTAN, ITEM_PETITE_FISH};
    return Number(text, out) && (std::ranges::contains(kBaits, out) || ExtraBaitType(out) >= 0);
}

// Item and weapon numbers, comma-separated, as --give takes them.
bool Items(std::string_view text, std::string &out) {
    for (std::string_view rest = text; true;) {
        size_t comma = rest.find(',');
        int    item = 0;
        if (!Number(Trim(rest.substr(0, comma)), item) || item < 1 || item >= ITEM_WEAPON_END) {
            return false;
        }
        if (comma == std::string_view::npos) {
            break;
        }
        rest.remove_prefix(comma + 1);
    }
    out.clear();
    for (char c : text) {
        if (c != ' ' && c != '\t') {
            out += c;
        }
    }
    return true;
}

// A stage file's key, and how its value is read into a stage; false for a bad value.
struct Key {
    std::string_view name;
    bool (*read)(std::string_view value, Stage &stage);
};

constexpr Key kKeys[] = {
    {"roster",       [](std::string_view v, Stage &s) { return Text(v, s.roster); }                          },
    {"slot",         [](std::string_view v, Stage &s) { return Count(v, s.slot) && s.slot >= 1; }            },
    {"dungeon",      [](std::string_view v, Stage &s) { return Within(v, s.dungeon, 0, 6); }                 },
    {"floor",        [](std::string_view v, Stage &s) { return Count(v, s.floor); }                          },
    {"character",    [](std::string_view v, Stage &s) { return Character(v, s.character); }                  },
    {"weapon",       [](std::string_view v, Stage &s) { return Count(v, s.weapon); }                         },
    {"attack",       [](std::string_view v, Stage &s) { return Count(v, s.attack); }                         },
    {"sphere",       [](std::string_view v, Stage &s) { return Count(v, s.sphere); }                         },
    {"seed",         [](std::string_view v, Stage &s) { return Number(v, s.seed) && s.seed >= 0; }           },
    {"place",        [](std::string_view v, Stage &s) { return Count(v, s.place); }                          },
    {"distance",     [](std::string_view v, Stage &s) { return Number(v, s.distance) && s.distance >= 0.0f; }},
    {"spread",       [](std::string_view v, Stage &s) { return Number(v, s.spread); }                        },
    {"angle",        [](std::string_view v, Stage &s) { return Number(v, s.angle); }                         },
    {"place_kind",   [](std::string_view v, Stage &s) { return Count(v, s.place_kind); }                     },
    {"place_after",  [](std::string_view v, Stage &s) { return Count(v, s.place_after); }                    },
    {"big_room",     [](std::string_view v, Stage &s) { return Flag(v, s.big_room); }                        },
    {"keep_alive",   [](std::string_view v, Stage &s) { return Flag(v, s.keep_alive); }                      },
    {"circle",       [](std::string_view v, Stage &s) { return Within(v, s.circle, 0, 9); }                  },
    {"town",         [](std::string_view v, Stage &s) { return Town(v, s.town); }                            },
    {"fishing_spot", [](std::string_view v, Stage &s) { return Count(v, s.fishing_spot); }                   },
    {"bait",         [](std::string_view v, Stage &s) { return Bait(v, s.bait); }                            },
    {"time",         [](std::string_view v, Stage &s) { return Time(v, s.time); }                            },
    {"give",         [](std::string_view v, Stage &s) { return Items(v, s.give); }                           },
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

// The floor's first magic circle, set to the stage's kind and moved in front of the member, waiting to be stepped on.
void SetOutCircle(const Stage &stage) {
    MAP_TRAP_CIRCLE &circle = NowDngMap->trap_circle[0];
    float            a = CharaMain.rotation.y;
    circle.pos[0] = CharaMain.pos[0] + kCircleAhead * std::sin(a);
    circle.pos[1] = CharaMain.pos[1];
    circle.pos[2] = CharaMain.pos[2] + kCircleAhead * std::cos(a);
    circle.pos[3] = 1.0f;
    circle.state = 1;
    circle.kind = stage.circle;
    circle.timer = 0.0f;
}

// The floor in play with no action or event running: when a member can be brought out.
bool InPlay() {
    return gameTask == GAME_TASK_PLAY && BtGameModeFlag == BT_GAME_MODE_PLAY && BtEventMode == 0 &&
           BtActStatus.action_on == 0;
}

// The town's event program for label: whether it reaches _LOAD_FISHING_DATA, as a fishing sign's does.
bool LoadsFishing(int label) {
    const CRunScript &script = EdEventScript;
    if (script.prog == nullptr) {
        return false;
    }
    const char                      *base = reinterpret_cast<const char *>(script.prog);
    const auto                      *entry = reinterpret_cast<const RS_PROGDATA *>(base + script.prog->prog);
    std::vector<const char *>        pending;
    std::unordered_set<const char *> seen;
    for (int i = 0; i < script.prog->prog_num; i++) {
        if (entry[i].no == label) {
            pending.push_back(base + entry[i].func);
        }
    }
    // A function runs from its address to the first word that is no opcode.
    constexpr int kMostInstructions = 20000;
    while (!pending.empty()) {
        const char *record = pending.back();
        pending.pop_back();
        if (!seen.insert(record).second) {
            continue;
        }
        int address = 0;
        std::memcpy(&address, record, sizeof(address));
        if (address < 0) {
            continue;
        }
        const auto *pc = reinterpret_cast<const vmcode_t *>(script.code + address);
        for (int n = 0; n < kMostInstructions && pc->op >= RS_OP_LOAD && pc->op <= RS_OP_COS; n++, pc++) {
            if (pc->op == RS_OP_CALL) {
                pending.push_back(script.code + pc->arg2);
            } else if (pc->op == RS_OP_PUSH_CONST && pc->arg1 == kConstInt && pc->arg2 == kLoadFishingData) {
                return true;
            }
        }
    }
    return false;
}

// Where the town's spot-th fishing sign is, in its event points' order: the event point the player triggers it from.
bool FishingSign(int spot, float *position) {
    const ED_MOVE_CHARA_INFO &info = EdMoveCharaInfo;
    for (int i = 0, found = 0; i < info.point_count; i++) {
        ED_EVENT_POINT &point = info.points[i];
        if (point.enabled == 0 || point.event_type != ED_EVENT_POINT_EVENT || !LoadsFishing(point.side) || found++ < spot) {
            continue;
        }
        sceVu0FVECTOR rotation;
        sceVu0CopyVector(position, point.position);
        CMapObject *object = point.parts_no >= 0 ? pEditGround->GetPartsObject(point.parts_no) : point.map_object;
        if (object != nullptr) {
            GetPosRot(object, position, rotation);
        }
        return true;
    }
    return false;
}

// The player stood on the sign's event point, facing the way they came from the town's entrance, the camera behind.
bool StandAtSign(int spot) {
    sceVu0FVECTOR sign;
    if (!FishingSign(spot, sign)) {
        return false;
    }
    sceVu0FVECTOR position = {sign[0], sign[1], sign[2], 1.0f};
    sceVu0FVECTOR rotation = {0.0f, std::atan2(sign[0] - Chara->pos[0], sign[2] - Chara->pos[2]), 0.0f, 0.0f};
    Chara->SetPosition(position);
    Chara->SetRotation(rotation);
    MainCamera.FollowOn();
    MainCamera.SetAngleSoon(AngleLimit(rotation[1] + std::numbers::pi_v<float>));
    MainCamera.SetFollow(position[0], position[1] + Chara->body_height - 3.0f, position[2]);
    EdInitCameraParam(&MainCamera);
    MainCamera.Step(-1);
    return true;
}

// The bait put in the bag when it lacks one.
void GiveBait(int bait) {
    CDngStatusData &status = *SaveData->GetDngStatus();
    if (status.SearchItemIndexNo(bait) < 0) {
        status.GetItem(bait, 0);
    }
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
    const char *mixed = nullptr;
    if (parsed.town >= 0 && parsed.dungeon >= 0) {
        mixed = "a dungeon and a town";
    } else if (parsed.town < 0 && parsed.fishing_spot >= 0) {
        mixed = "a fishing_spot with no town";
    } else if (parsed.town < 0 && parsed.bait >= 0) {
        mixed = "a bait with no town";
    }
    if (mixed != nullptr) {
        if (why != nullptr) {
            *why = mixed;
        }
        return false;
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
    if (!active || (active->dungeon < 0 && active->town < 0)) {
        return true;
    }
    McAccess.InitForMC();
    if (McAccess.LoadFromMc(active->slot - 1) != 1) {
        return false;
    }
    if (active->time >= 0.0f) {
        SaveData->SetNowTime(active->time);
    }
    return true;
}

int StageEntranceFloor(int otherwise) {
    if (!active || active->floor < 0 || run.entered) {
        return otherwise;
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
    bool circle_due = active->circle >= 0 && !run.circled && run.steps >= active->place_after && !run.walking;
    if (circle_due && NowDngMap != nullptr) {
        SetOutCircle(*active);
        run.circled = true;
        LogEvent("stage", "\"step\":\"circle\"");
    }
}

void StageTownStep() {
    if (!active || active->town < 0 || MapNo != active->town || Chara == nullptr) {
        return;
    }
    if (!run.arrived) {
        run.arrived = true;
        run.at_sign = active->fishing_spot < 0;
        if (active->bait >= 0) {
            GiveBait(active->bait);
        }
        LogEvent("stage", "\"step\":\"town\"");
    }
    if (!run.at_sign) {
        run.at_sign = true;
        if (StandAtSign(active->fishing_spot)) {
            LogEvent("stage", "\"step\":\"at_sign\"");
        } else {
            std::fprintf(stderr, "stage: map %d has no fishing sign %d\n", MapNo, active->fishing_spot);
        }
    }
    ED_MOVE_CHARA_INFO &info = EdMoveCharaInfo;
    bool                standing = info.fishing != 0 && chara_fishing == ED_FISHING_STAND && info.event_no < 0 && info.system_event_no < 0;
    if (active->bait >= 0 && !run.bait_asked && standing) {
        // As the square button asks for it.
        info.event_no = kBaitEvent;
        info.acted = true;
        Chara->SetMotion(0, 0);
        run.bait_asked = true;
    }
    if (run.bait_asked && !run.baited && FishingGetEsaItemNo() == active->bait) {
        run.baited = true;
        LogEvent("stage", std::format("\"step\":\"bait\",\"item\":{}", active->bait));
    }
}

bool StagePickBait(RS_STACKDATA *choice) {
    if (!active || !run.bait_asked || run.bait_picked) {
        return false;
    }
    run.bait_picked = true;
    // As the picker takes it: out of its bag slot.
    ITEM_PACK &pack = SaveData->GetDngStatus()->item_pack;
    for (int i = 0; i < pack.num; i++) {
        if (pack.item[i] == active->bait) {
            pack.item[i] = 0;
            choice->i = active->bait;
            ListBaitForScript(choice, active->bait);
            return true;
        }
    }
    std::fprintf(stderr, "stage: the bag has no bait %d\n", active->bait);
    return false;
}

void StageReset() {
    run = {};
}

} // namespace expanded
