#include "expanded/debug/event_log.hpp"

#include <cstdio>
#include <format>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "gameloop.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

std::FILE *file = nullptr;

// What the last step read of the member out, to log changes.
struct Seen {
    int   chara = -1;
    int   life = -1;
    int   weapon = -1;
    float whp = -1.0f;
};

Seen seen;

} // namespace

bool SetEventLog(const char *path, std::string *why) {
    if (file != nullptr) {
        std::fclose(file);
        file = nullptr;
    }
    seen = {};
    if (path == nullptr || *path == '\0') {
        return true;
    }
    file = std::fopen(path, "w");
    if (file == nullptr && why != nullptr) {
        *why = "cannot be opened";
    }
    return file != nullptr;
}

bool EventLogOn() {
    return file != nullptr;
}

void LogEvent(std::string_view event, std::string_view fields) {
    if (file == nullptr) {
        return;
    }
    std::string line = std::format("{{\"frame\":{},\"event\":\"{}\"", GameFrameCount(), event);
    if (!fields.empty()) {
        line += ',';
        line += fields;
    }
    line += "}\n";
    std::fputs(line.c_str(), file);
    std::fflush(file); // a crash keeps what came before it
}

void EventLogFloorLoaded() {
    if (file != nullptr && UserStatus != nullptr) {
        LogEvent("floor", std::format("\"dungeon\":{},\"floor\":{}", selectMapNo, static_cast<int>(UserStatus->cur_floor)));
    }
}

void EventLogDamaged(const CMonstorUnit &unit, int monster, int amount) {
    if (file != nullptr) {
        const MONSTOR &target = unit.monster[monster];
        LogEvent("damage", std::format("\"monster\":{},\"kind\":{},\"amount\":{},\"life\":{}", monster,
                                       ModelKind(target.base_model), amount, target.hp));
    }
}

void EventLogKilled(const CMonstorUnit &unit, int monster) {
    if (file != nullptr) {
        LogEvent("kill", std::format("\"monster\":{},\"kind\":{}", monster, ModelKind(unit.monster[monster].base_model)));
    }
}

void EventLogSound(const OwnSound &sound) {
    if (file != nullptr) {
        LogEvent("sound", std::format("\"program\":{},\"note\":{}", sound.program, sound.note));
    }
}

void EventLogStep() {
    if (file == nullptr || UserStatus == nullptr) {
        return;
    }
    int chara = UserStatus->cur_chara;
    if (chara < 0 || chara > 5) {
        return;
    }
    if (chara != seen.chara) {
        LogEvent("character", std::format("\"chara\":{}", chara));
        seen = {.chara = chara};
    }
    int life = UserStatus->hp[chara];
    if (seen.life >= 0 && life != seen.life) {
        LogEvent("life", std::format("\"chara\":{},\"from\":{},\"to\":{}", chara, seen.life, life));
    }
    seen.life = life;
    int                slot = UserStatus->equipped_weapon_slot[chara];
    const WEAPON_HAVE &weapon = UserStatus->chara_weapons[chara][slot];
    if (weapon.item_no == seen.weapon && weapon.durability_f != seen.whp) {
        LogEvent("whp", std::format("\"chara\":{},\"weapon\":{},\"from\":{:.2f},\"to\":{:.2f}", chara, weapon.item_no,
                                    seen.whp, weapon.durability_f));
    }
    seen.weapon = weapon.item_no;
    seen.whp = weapon.durability_f;
}

} // namespace expanded
