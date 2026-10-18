#pragma once

#include <nlohmann/json_fwd.hpp>

#include <string_view>

#include "expanded/core/options.hpp"

namespace expanded {

// config.json's expanded.<key>: a choice's name, or true or false. Returns false for a value the toggle does not take.
bool SetOptionValue(OptionValues &values, std::string_view key, const nlohmann::ordered_json &value);

// Writes every toggle into config.json's expanded object: its choice's name, or whether it is on.
void SaveOptionValues(nlohmann::ordered_json &config, const OptionValues &values);

} // namespace expanded
