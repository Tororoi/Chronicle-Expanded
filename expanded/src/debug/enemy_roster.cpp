#include "expanded/debug/enemy_roster.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <format>

#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/species.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "mainselect.hpp"
#include "monstorunit.hpp"
#include "platform/window.hpp"

namespace expanded {

namespace {

constexpr int    kKinds = kSpeciesCount;
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
std::string reason; // why the entry was refused

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

// A refusal's reason under the row, in lines of the menu's 32 columns indented by 2.
std::string WrapReason(std::string_view text) {
    constexpr std::size_t kWidth = 30;
    std::string           out, line;
    while (!text.empty()) {
        std::size_t      space = text.find(' ');
        std::string_view word = text.substr(0, space);
        text = space == std::string_view::npos ? std::string_view() : text.substr(space + 1);
        if (!line.empty() && line.size() + 1 + word.size() > kWidth) {
            out += "  " + line + "\n";
            line.clear();
        }
        line += (line.empty() ? "" : " ") + std::string(word);
    }
    return line.empty() ? out : out + "  " + line + "\n";
}

void StopTyping() {
    typing = false;
    if (WindowHandle() != nullptr) {
        SDL_StopTextInput(WindowHandle());
    }
}

} // namespace

std::string RosterProblem(const Roster &roster) {
    static constexpr const char *kParty[] = {"Toan", "Xiao", "Goro", "Ruby", "Ungaga", "Osmond"};
    int                          size = roster.Size();
    bool                         gated = size > 0;
    for (int i = 0; i < size; i++) {
        int                         kind = roster.model_no[i];
        std::optional<BossCastInfo> cast = BossCastOf(static_cast<Monster>(kind));
        if (!SpeciesBuilt(kind)) {
            return std::format("{} is not built yet: run `dcdata expanded <data>`", kind);
        }
        gated = gated && cast && cast->needs >= 0;
    }
    // A cast keeps together: its other kind comes too, unless this one comes alone.
    int count = size;
    for (int i = 0; i < size; i++) {
        std::optional<BossCastInfo> cast = BossCastOf(static_cast<Monster>(roster.model_no[i]));
        if (!cast || cast->alone) {
            continue;
        }
        Monster partner = cast->kinds[1 - cast->member];
        if (!roster.Contains(partner) && ++count > kRosterSize) {
            return std::format("no room for {} ({}), which comes with {} ({}): drop a kind", No(partner), cast->names[1 - cast->member],
                               roster.model_no[i], cast->names[cast->member]);
        }
    }
    if (gated) {
        std::optional<BossCastInfo> cast = BossCastOf(static_cast<Monster>(roster.model_no[0]));
        return std::format("{} ({}) and {} ({}) come only with {} in the party: add another kind", No(cast->kinds[0]), cast->names[0],
                           No(cast->kinds[1]), cast->names[1], kParty[cast->needs]);
    }
    return {};
}

bool ParseRosterSpec(std::string_view text, Roster &out, std::string *why) {
    auto refuse = [&](std::string message) {
        if (why != nullptr) {
            *why = std::move(message);
        }
        return false;
    };
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
        if (item.empty()) {
            return refuse("a kind is missing between commas");
        }
        if (error != std::errc() || end != item.data() + item.size() || kind < 0 || kind >= kKinds || Species(kind) == nullptr) {
            return refuse(std::format("{} is not a kind: kinds are 0 to {}", item, kKinds - 1));
        }
        if (parsed.Contains(kind)) {
            return refuse(std::format("{} is listed twice", kind));
        }
        if (!parsed.Add(kind, once || Species(kind)->kind == MONSTER_KIND_NO_LOCK_ON)) {
            return refuse(std::format("a roster holds at most {} kinds", kRosterSize));
        }
    }
    out = parsed;
    return true;
}

bool SetDebugRoster(std::string_view text, std::string *why) {
    Roster parsed;
    if (!ParseRosterSpec(text, parsed, why)) {
        return false;
    }
    if (std::string problem = RosterProblem(parsed); !problem.empty()) {
        if (why != nullptr) {
            *why = problem;
        }
        return false;
    }
    spec = Trim(text);
    roster = parsed;
    set_in_visit = RosterVisits();
    std::fprintf(stderr, "debug enemy roster: %s\n", spec.empty() ? "cleared" : spec.c_str());
    return true;
}

const std::string &DebugRosterSpec() {
    return spec;
}

int DevMenuRosterRows() {
    return 1;
}

int DevMenuRosterDraw(char *text, std::size_t room, bool selected) {
    if (room == 0) {
        return 0;
    }
    const char *cursor = selected || typing ? ">" : " ";
    std::string row = typing ? std::format("{}enemy roster {}_{}\n", cursor, entry, rejected ? " ?" : "")
                             : std::format("{}enemy roster {}\n", cursor, spec.empty() ? "-" : spec);
    if (typing && rejected) {
        row += WrapReason(reason);
    }
    int written = std::snprintf(text, room, "%s", row.c_str());
    return std::clamp(written, 0, static_cast<int>(room) - 1);
}

bool DevMenuRosterStep(bool selected) {
    if (typing) {
        if (cancel) {
            StopTyping();
        } else if (commit) {
            rejected = !SetDebugRoster(entry, &reason);
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
