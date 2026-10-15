#include "expanded/options.hpp"

#include <array>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "expanded/options_page.hpp"
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

template <std::size_t Index>
int GetToggle(const Config &config) {
    return OptionOn(config.expanded_options, Toggles()[Index].key) ? 0 : 1;
}

template <std::size_t Index>
void SetToggle(Config &config, int choice) {
    SetOption(config.expanded_options, Toggles()[Index].key, choice == 0);
}

int Two(const Config &) {
    return 2;
}

// The getter and setter of each toggle's row, by its index.
template <std::size_t... Index>
constexpr auto RowFunctions(std::index_sequence<Index...>) {
    return std::array<std::pair<int (*)(const Config &), void (*)(Config &, int)>, sizeof...(Index)>{
        std::pair{GetToggle<Index>, SetToggle<Index>}...};
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
    auto changed = values.changed.find(key);
    return changed != values.changed.end() ? changed->second : toggle->on;
}

bool OptionOn(std::string_view key) {
    return OptionOn(ConfigGet().expanded_options, key);
}

bool SetOption(OptionValues &values, std::string_view key, bool on) {
    const Toggle *toggle = Find(key);
    if (toggle == nullptr) {
        return false;
    }
    if (on == toggle->on) {
        if (auto changed = values.changed.find(key); changed != values.changed.end()) {
            values.changed.erase(changed);
        }
    } else {
        values.changed.insert_or_assign(std::string(key), on);
    }
    return true;
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
                        .count = Two,
                        .get = kRowFunctions[i].first,
                        .set = kRowFunctions[i].second,
                        .names = "On|Off"});
    }
    out.push_back({"Expanded", "\"Expanded\"\nChronicle-Expanded's\nchanges to the game.", rows});
    return out;
}

} // namespace expanded
