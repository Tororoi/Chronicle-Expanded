#include "expanded/core/options_json.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace expanded {

bool SetOptionValue(OptionValues &values, std::string_view key, const nlohmann::ordered_json &value) {
    if (value.is_string()) {
        return SetOptionChoice(values, key, value.get<std::string>());
    }
    return value.is_boolean() && SetOption(values, key, value.get<bool>());
}

void SaveOptionValues(nlohmann::ordered_json &config, const OptionValues &values) {
    for (const Toggle &toggle : Toggles()) {
        if (toggle.choices != nullptr) {
            config["expanded"][toggle.key] = OptionChoiceName(toggle.key, OptionChoice(values, toggle.key));
        } else {
            config["expanded"][toggle.key] = OptionOn(values, toggle.key);
        }
    }
}

} // namespace expanded
