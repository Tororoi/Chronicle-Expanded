#pragma once

// A monster script (.stb): word 2 its code base, words 3 and 4 its label table and count ({id, function header}
// pairs, a header's first word its entry from the code base), its code 12-byte cells {op, a1, a2}. Includes no game
// header, so dcdata can build with it.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace expanded::stb {

inline constexpr std::size_t kCell = 12;

// Opcodes.
inline constexpr std::uint32_t kLoad = 1, kLoadAddress = 2, kPushConst = 3, kPop = 4, kStore = 5, kAdd = 6, kLess = 14,
                               kReturn = 15, kJump = 16, kJumpFalse = 17, kJumpTrue = 18, kCall = 19, kExt = 21,
                               kMarker = 22, kEnd = 27;

// A constant's kind (a push's a1), and a load's local kinds (its a2).
inline constexpr std::uint32_t kInt = 1, kFloat = 2, kString = 3;
inline constexpr std::uint32_t kIntLocal = 1, kFloatLocal = 8;

// The code base, and a label's function header; none for a label the script lacks (the first of a repeated one).
std::size_t                CodeBase(std::span<const unsigned char> script);
std::optional<std::size_t> Label(std::span<const unsigned char> script, std::uint32_t id);

} // namespace expanded::stb
