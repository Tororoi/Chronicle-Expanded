#pragma once

#include <string_view>

// Seas the original game draws with its ground. East Harbor's sea (s0903, the town's third ground model) is drawn in the
// ground pass there, before the fish, so it could not lie over them as the other waters do; Expanded draws it with the
// water instead, after the fish and before the water surface, as every other fishing water is drawn.

namespace expanded {

// Hook: the town's fixed parts as they are placed (`name` as the map's info has it, with its folder): the pass a part
// is drawn in (its category), the water's (0x15) for a sea drawn with the ground in the original game, else `pass`.
int TownPartPass(int map_no, std::string_view name, int pass);

} // namespace expanded
