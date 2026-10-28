#include "expanded/debug/event_log.hpp"

#include <cstdio>
#include <format>

#include "app/gameloop.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "ebattle.hpp"
#include "edit.hpp"
#include "expanded/core/party.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "monstorunit.hpp"
#include "savedata.hpp"
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

// What the last town step read of the fishing, to log its changes: the fish is the one that last bit.
struct Angling {
    bool on = false;
    int  state = ED_FISHING_INIT;
    int  bait = -1;
    int  fish = -1;
};

Angling angling;

// The fish doing action, or -1.
int FishDoing(FishAction action) {
    for (int i = 0; Fish != nullptr && i < 6; i++) {
        if (Fish[i].action == action) {
            return i;
        }
    }
    return -1;
}

void LogFish(std::string_view step, int fish) {
    int kind = fish >= 0 && Fish != nullptr ? Fish[fish].fish_kind : -1;
    LogEvent("fishing", std::format("\"step\":\"{}\",\"fish\":{},\"kind\":{}", step, fish, kind));
}

void LogBait(std::string_view step, int bait) {
    LogEvent("fishing", std::format("\"step\":\"{}\",\"bait\":{}", step, bait));
}

// A change of the fishing state: its step (null for none), and whether it is the fish's rather than the line's.
struct Change {
    const char *step = nullptr;
    bool        fish = false;
};

Change AnglingChange(int from, int to) {
    switch (to) {
        case ED_FISHING_STAND:
            return {"ready"};
        case ED_FISHING_CAST_START:
            return {"cast"};
        case ED_FISHING_WAIT_BITE:
            if (from == ED_FISHING_CASTING) {
                return {"float"};
            }
            if (from == ED_FISHING_BITE_CHECK) {
                return {"let_go", true};
            }
            return from == ED_FISHING_HOOKED ? Change{"escaped", true} : Change{};
        case ED_FISHING_REEL_IN:
            return {"reeled"};
        case ED_FISHING_BITE_CHECK:
            return {"bite", true};
        case ED_FISHING_HOOKED:
            return {"hooked", true};
        case ED_FISHING_SINK:
            return {"missed", true};
        case ED_FISHING_BATTLE:
            return {"fight", true};
        case ED_FISHING_LANDED:
            return {"landed", true};
        default:
            return {};
    }
}

} // namespace

bool SetEventLog(const char *path, std::string *why) {
    if (file != nullptr) {
        std::fclose(file);
        file = nullptr;
    }
    seen = {};
    angling = {};
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

void EventLogDamaged(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount) {
    if (file != nullptr) {
        const MONSTOR &target = unit.monster[monster];
        LogEvent("damage", std::format("\"monster\":{},\"kind\":{},\"amount\":{},\"life\":{},\"owner\":{},\"attack\":{}", monster,
                                       ModelKind(target.base_model), amount, target.hp, hit.owner, hit.attack_no));
    }
}

void EventLogKilled(const CMonstorUnit &unit, int monster) {
    if (file != nullptr) {
        const MONSTOR &target = unit.monster[monster];
        LogEvent("kill", std::format("\"monster\":{},\"kind\":{},\"by\":{}", monster, ModelKind(target.base_model), target.last_attacker));
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
    if (chara < 0 || chara >= kPartyMembers) {
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
    const WEAPON_HAVE *equipped = EquippedWeapon(*UserStatus, chara);
    if (equipped == nullptr) {
        return;
    }
    const WEAPON_HAVE &weapon = *equipped;
    if (weapon.item_no == seen.weapon && weapon.durability_f != seen.whp) {
        LogEvent("whp", std::format("\"chara\":{},\"weapon\":{},\"from\":{:.2f},\"to\":{:.2f}", chara, weapon.item_no,
                                    seen.whp, weapon.durability_f));
    }
    seen.weapon = weapon.item_no;
    seen.whp = weapon.durability_f;
}

void EventLogFishingStep() {
    if (file == nullptr) {
        return;
    }
    Angling now = {.on = EdMoveCharaInfo.fishing != 0, .state = chara_fishing, .bait = FishingGetEsaItemNo(), .fish = angling.fish};
    if (now.on && !angling.on) {
        LogBait("start", now.bait);
    }
    if (now.on && angling.on && now.state != angling.state) {
        if (now.state == ED_FISHING_BITE_CHECK) {
            now.fish = FishDoing(FISH_ACTION_EAT_FOOD);
        } else if (now.state == ED_FISHING_HOOKED) {
            now.fish = FishDoing(FISH_ACTION_BITE_HOOK);
        }
        if (Change change = AnglingChange(angling.state, now.state); change.step != nullptr && change.fish) {
            LogFish(change.step, now.fish);
        } else if (change.step != nullptr) {
            LogBait(change.step, now.bait);
        }
    }
    bool lost = now.state == ED_FISHING_WAIT_BITE || now.state == ED_FISHING_SINK;
    if (now.on && angling.on && lost && angling.bait >= 0 && now.bait < 0) {
        LogBait("bait_lost", angling.bait);
    }
    if (now.on && angling.on && now.bait >= 0 && now.bait != angling.bait) {
        LogBait("bait", now.bait);
    }
    if (!now.on && angling.on) {
        LogEvent("fishing", "\"step\":\"quit\"");
    }
    angling = now;
}

void EventLogCaught(int kind, int size, int points) {
    if (file != nullptr) {
        LogEvent("fishing", std::format("\"step\":\"caught\",\"kind\":{},\"size\":{},\"points\":{},\"total\":{}", kind, size, points,
                                        SaveData->GetFishingPoint()));
    }
}

} // namespace expanded
