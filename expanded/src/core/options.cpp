#include "expanded/core/options.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "expanded/core/options_page.hpp"
#include "platform/config.hpp"

namespace expanded {

namespace {

std::vector<Toggle> &Registry() {
    static std::vector<Toggle> toggles;
    return toggles;
}

const Toggle *Find(std::string_view key) {
    for (const Toggle &toggle : Registry()) {
        if (key == toggle.key) {
            return &toggle;
        }
    }
    return nullptr;
}

// A toggle's choices' names.
std::vector<std::string_view> Choices(const Toggle &toggle) {
    std::vector<std::string_view> names;
    if (toggle.choices == nullptr) {
        return names;
    }
    std::string_view rest = toggle.choices;
    while (true) {
        std::size_t bar = rest.find('|');
        names.push_back(rest.substr(0, bar));
        if (bar == std::string_view::npos) {
            return names;
        }
        rest.remove_prefix(bar + 1);
    }
}

bool SameName(std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

// A row's choice is On (0) or Off (1) for a toggle, its own index for one with choices.
template <std::size_t Index>
int GetToggle(const Config &config) {
    const Toggle &toggle = Toggles()[Index];
    if (toggle.choices != nullptr) {
        return OptionChoice(config.expanded_options, toggle.key);
    }
    return OptionOn(config.expanded_options, toggle.key) ? 0 : 1;
}

template <std::size_t Index>
void SetToggle(Config &config, int choice) {
    const Toggle &toggle = Toggles()[Index];
    if (toggle.choices != nullptr) {
        SetOptionChoice(config.expanded_options, toggle.key, choice);
    } else {
        SetOption(config.expanded_options, toggle.key, choice == 0);
    }
}

template <std::size_t Index>
int CountToggle(const Config &) {
    const Toggle &toggle = Toggles()[Index];
    return toggle.choices != nullptr ? static_cast<int>(Choices(toggle).size()) : 2;
}

struct RowFunction {
    int (*count)(const Config &);
    int (*get)(const Config &);
    void (*set)(Config &, int);
};

// The count, getter and setter of each toggle's row, by its index.
template <std::size_t... Index>
constexpr auto RowFunctions(std::index_sequence<Index...>) {
    return std::array<RowFunction, sizeof...(Index)>{
        RowFunction{CountToggle<Index>, GetToggle<Index>, SetToggle<Index>}
        ...
    };
}

constexpr auto kRowFunctions = RowFunctions(std::make_index_sequence<kMaxToggles>());

// Keys as the rows show them: expanded.<toggle key>.
const char *RowKey(const Toggle &toggle) {
    static std::map<std::string, std::string, std::less<>> keys;
    auto [it, added] = keys.try_emplace(toggle.key, std::string("expanded.") + toggle.key);
    return it->second.c_str();
}

} // namespace

bool AddToggle(const Toggle &toggle) {
    if (toggle.key == nullptr || Find(toggle.key) != nullptr || Registry().size() >= kMaxToggles) {
        std::fprintf(stderr, "expanded: toggle %s not added\n", toggle.key != nullptr ? toggle.key : "(null)");
        return false;
    }
    Registry().push_back(toggle);
    return true;
}

std::span<const Toggle> Toggles() {
    return Registry();
}

bool OptionOn(const OptionValues &values, std::string_view key) {
    const Toggle *toggle = Find(key);
    if (toggle == nullptr) {
        return false;
    }
    if (toggle->choices != nullptr) {
        return OptionChoice(values, key) != 0;
    }
    auto changed = values.changed.find(key);
    return changed != values.changed.end() ? changed->second != 0 : toggle->on;
}

bool OptionOn(std::string_view key) {
    return OptionOn(ConfigGet().expanded_options, key);
}

bool SetOption(OptionValues &values, std::string_view key, bool on) {
    const Toggle *toggle = Find(key);
    if (toggle == nullptr || toggle->choices != nullptr) {
        return false;
    }
    if (on == toggle->on) {
        if (auto changed = values.changed.find(key); changed != values.changed.end()) {
            values.changed.erase(changed);
        }
    } else {
        values.changed.insert_or_assign(std::string(key), on ? 1 : 0);
    }
    return true;
}

int OptionChoice(const OptionValues &values, std::string_view key) {
    const Toggle *toggle = Find(key);
    if (toggle == nullptr || toggle->choices == nullptr) {
        return 0;
    }
    auto changed = values.changed.find(key);
    return changed != values.changed.end() ? changed->second : toggle->choice;
}

int OptionChoice(std::string_view key) {
    return OptionChoice(ConfigGet().expanded_options, key);
}

bool SetOptionChoice(OptionValues &values, std::string_view key, int choice) {
    const Toggle *toggle = Find(key);
    if (toggle == nullptr || toggle->choices == nullptr || choice < 0 || choice >= static_cast<int>(Choices(*toggle).size())) {
        return false;
    }
    if (choice == toggle->choice) {
        if (auto changed = values.changed.find(key); changed != values.changed.end()) {
            values.changed.erase(changed);
        }
    } else {
        values.changed.insert_or_assign(std::string(key), choice);
    }
    return true;
}

bool SetOptionChoice(OptionValues &values, std::string_view key, std::string_view choice) {
    const Toggle *toggle = Find(key);
    if (toggle == nullptr) {
        return false;
    }
    std::vector<std::string_view> names = Choices(*toggle);
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (SameName(names[i], choice)) {
            return SetOptionChoice(values, key, static_cast<int>(i));
        }
    }
    return false;
}

std::string OptionChoiceName(std::string_view key, int choice) {
    const Toggle *toggle = Find(key);
    if (toggle == nullptr) {
        return {};
    }
    std::vector<std::string_view> names = Choices(*toggle);
    return choice >= 0 && choice < static_cast<int>(names.size()) ? std::string(names[static_cast<std::size_t>(choice)]) : std::string();
}

std::vector<options::Page> OptionsPages(std::span<const options::Page> pages) {
    std::vector<options::Page> out(pages.begin(), pages.end());
    if (Toggles().empty()) {
        return out;
    }
    static std::vector<options::Row> rows;
    rows.clear();
    for (std::size_t i = 0; i < Toggles().size(); ++i) {
        const Toggle &toggle = Toggles()[i];
        rows.push_back({.key = RowKey(toggle),
                        .label = toggle.label,
                        .help = toggle.help,
                        .count = kRowFunctions[i].count,
                        .get = kRowFunctions[i].get,
                        .set = kRowFunctions[i].set,
                        .names = toggle.choices != nullptr ? toggle.choices : "On|Off"});
    }
    out.push_back({"Expanded", "\"Expanded\"\nChronicle-Expanded's\nchanges to the game.", rows});
    return out;
}

} // namespace expanded
