#include "expanded/debug/enemy_roster.hpp"

#include <SDL3/SDL.h>

#include <charconv>
#include <cstdio>

#include "gamepad.hpp"
#include "mainselect.hpp"
#include "monstorunit.hpp"
#include "platform/window.hpp"

namespace expanded {

namespace {

constexpr int    kKinds = 167;
constexpr size_t kMaxSpec = 60;

std::string spec;
Roster      roster;
int         set_in_visit = 0;

// The menu row's entry: the text typed, and what the keys asked for since the last step.
bool        typing = false;
bool        hooked = false;
bool        commit = false;
bool        cancel = false;
bool        rejected = false;
std::string entry;

std::string_view Trim(std::string_view text) {
    while (!text.empty() && text.front() == ' ') {
        text.remove_prefix(1);
    }
    while (!text.empty() && text.back() == ' ') {
        text.remove_suffix(1);
    }
    return text;
}

void StageDebugRoster(int, int floor) {
    if (DebugMode == 0 || spec.empty() || RosterVisits() <= set_in_visit) {
        return;
    }
    StageFloor(floor, {roster, roster});
}

[[maybe_unused]] const bool kStager = (AddFloorStager(StageDebugRoster, true), true);

void OnEvent(const SDL_Event &event) {
    if (!typing) {
        return;
    }
    if (event.type == SDL_EVENT_TEXT_INPUT) {
        for (const char *c = event.text.text; *c != '\0' && entry.size() < kMaxSpec; c++) {
            if ((*c >= '0' && *c <= '9') || *c == ',' || *c == '!' || *c == ' ') {
                entry += *c;
            }
        }
    } else if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_BACKSPACE:
                if (!entry.empty()) {
                    entry.pop_back();
                }
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                commit = true;
                break;
            case SDLK_ESCAPE:
                cancel = true;
                break;
            default:
                break;
        }
    }
}

void StopTyping() {
    typing = false;
    if (WindowHandle() != nullptr) {
        SDL_StopTextInput(WindowHandle());
    }
}

} // namespace

bool ParseRosterSpec(std::string_view text, Roster &out) {
    Roster parsed;
    text = Trim(text);
    while (!text.empty()) {
        size_t           comma = text.find(',');
        std::string_view item = Trim(text.substr(0, comma));
        text = comma == std::string_view::npos ? std::string_view() : text.substr(comma + 1);
        bool once = !item.empty() && item.back() == '!';
        if (once) {
            item = Trim(item.substr(0, item.size() - 1));
        }
        int kind = -1;
        auto [end, error] = std::from_chars(item.data(), item.data() + item.size(), kind);
        if (item.empty() || error != std::errc() || end != item.data() + item.size() || kind < 0 || kind >= kKinds) {
            return false;
        }
        if (!parsed.Add(kind, once || MonstorTable[kind].kind == MONSTER_KIND_NO_LOCK_ON)) {
            return false;
        }
    }
    out = parsed;
    return true;
}

bool SetDebugRoster(std::string_view text) {
    Roster parsed;
    if (!ParseRosterSpec(text, parsed)) {
        return false;
    }
    spec = Trim(text);
    roster = parsed;
    set_in_visit = RosterVisits();
    std::printf("debug enemy roster: %s\n", spec.empty() ? "cleared" : spec.c_str());
    return true;
}

const std::string &DebugRosterSpec() {
    return spec;
}

int DevMenuRosterRows() {
    return 1;
}

int DevMenuRosterDraw(char *text, bool selected) {
    const char *cursor = selected || typing ? ">" : " ";
    if (typing) {
        return std::sprintf(text, "%senemy roster %s_%s\n", cursor, entry.c_str(), rejected ? " ?" : "");
    }
    return std::sprintf(text, "%senemy roster %s\n", cursor, spec.empty() ? "-" : spec.c_str());
}

bool DevMenuRosterStep(bool selected) {
    if (typing) {
        if (cancel) {
            StopTyping();
        } else if (commit) {
            rejected = !SetDebugRoster(entry);
            if (!rejected) {
                StopTyping();
            }
        }
        commit = cancel = false;
        return true;
    }
    if (!selected || !(GamePad.Down(PAD_CIRCLE) || GamePad.Down(PAD_TRIANGLE))) {
        return false;
    }
    if (!hooked) {
        WindowAddEventHook(OnEvent);
        hooked = true;
    }
    typing = true;
    commit = cancel = rejected = false;
    entry = spec;
    if (WindowHandle() != nullptr) {
        SDL_StartTextInput(WindowHandle());
    }
    return true;
}

} // namespace expanded
