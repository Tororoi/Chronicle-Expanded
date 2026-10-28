#pragma once

// A message file (.mes): its count @0, a word @2, count {id, word offset} entries @4 in id order, then the text. A
// message's text starts at byte 2 * (count + word offset + 1) and runs to its 0xFF01 word; the files end in zero
// padding. Includes no game header, so dcdata can build with it.

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "expanded/data/bytes.hpp"

namespace expanded::mes {

using Words = std::vector<std::uint16_t>;

inline constexpr std::uint16_t kEnd = 0xFF01;

// Message id's words to its kEnd, inclusive; throws when the file lacks it.
Words Extract(std::span<const unsigned char> mes, int id);

// The file with messages added after its text, the trailing padding cut to a few zero words first.
Bytes Append(std::span<const unsigned char> mes, const std::vector<std::pair<int, Words>> &messages);

} // namespace expanded::mes
