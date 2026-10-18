#pragma once

// A model pack's info.cfg: its MODEL and MOTION lines, a KEY_START ... MOTION_END block of motion keys
// ("KEY start, end, speed, // name"), then its EVENT lines. Each edit throws on a cfg without what it edits.
// Includes no game header, so dcdata can build with it.

#include <span>
#include <string_view>

#include "expanded/data/bytes.hpp"

namespace expanded {

// The cfg with lines put before the MOTION_END after its KEY_START, after that MOTION_END's line, or in place of
// the block's keys.
Bytes AppendCfgKeys(std::span<const unsigned char> cfg, std::string_view lines);
Bytes AppendCfgEvents(std::span<const unsigned char> cfg, std::string_view lines);
Bytes ReplaceCfgKeys(std::span<const unsigned char> cfg, std::string_view lines);

// The cfg with its key-th KEY line (0 the first after KEY_START) made line, the old line's comment kept (else
// its \r).
Bytes SetCfgKey(std::span<const unsigned char> cfg, int key, std::string_view line);

} // namespace expanded
