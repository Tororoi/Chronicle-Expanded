#pragma once

// A monster script (.stb): its code base @kCodeBaseAt, its label table and count @kLabelTableAt and @kLabelCountAt
// ({id, function header} pairs), its code 12-byte cells {op, a1, a2}. A function header is kFunctionHeader bytes, its
// first word its entry from the code base. Includes no game header, so dcdata can build with it.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "expanded/data/bytes.hpp"

namespace expanded::stb {

inline constexpr std::size_t kCodeBaseAt = 0x08, kLabelTableAt = 0x0C, kLabelCountAt = 0x10, kLabelEntry = 8;
inline constexpr std::size_t kCell = 12, kFunctionHeader = 56;

// Opcodes.
inline constexpr std::uint32_t kLoad = 1, kLoadAddress = 2, kPushConst = 3, kPop = 4, kStore = 5, kReturn = 15, kJump = 16,
                               kJumpFalse = 17, kJumpTrue = 18, kCall = 19, kExt = 21, kMarker = 22, kEnd = 27;

// Operators: a binary one pops two values and pushes one, a unary one pops one and pushes one (10, 12, 13, 29 and 30
// unnamed). A compare's a1 is its test.
inline constexpr std::uint32_t kAdd = 6, kSubtract = 7, kMultiply = 8, kDivide = 9, kNegate = 11, kCompare = 14, kAnd = 24,
                               kOr = 25, kNot = 26;
inline constexpr std::uint32_t kBinaryOperators[] = {kAdd, kSubtract, kMultiply, kDivide, 10, kCompare, kAnd, kOr};
inline constexpr std::uint32_t kUnaryOperators[] = {kNegate, 12, 13, kNot, 29, 30};
inline constexpr std::uint32_t kLessThan = 42;

inline constexpr bool IsBinaryOperator(std::uint32_t op) {
    return std::ranges::find(kBinaryOperators, op) != std::end(kBinaryOperators);
}

inline constexpr bool IsUnaryOperator(std::uint32_t op) {
    return std::ranges::find(kUnaryOperators, op) != std::end(kUnaryOperators);
}

// A constant's kind (a push's a1), and a load's local kinds (its a2).
inline constexpr std::uint32_t kInt = 1, kFloat = 2, kString = 3;
inline constexpr std::uint32_t kIntLocal = 1, kFloatLocal = 8;

// Commands: an EXT's first argument.
inline constexpr std::uint32_t kGetDistance = 10, kGetPosition = 11, kGetDirection = 31, kStatusSetMuteki = 101, kSetBodyCol = 130,
                               kSetDmgPara = 132, kSetShot = 133, kSetBodyColPara = 134, kGetRand = 180, kSetShot2 = 229,
                               kSetGuardFrame = 244;

// The code base, and a label's function header; none for a label the script lacks (the first of a repeated one).
std::size_t                CodeBase(std::span<const unsigned char> script);
std::optional<std::size_t> Label(std::span<const unsigned char> script, std::uint32_t id);
// A label's function header, throwing (naming the script) when it has none.
std::size_t RequireLabel(std::span<const unsigned char> script, std::uint32_t id, std::string_view name);

// A cell appended, and one written at at.
void AppendCell(Bytes &out, std::uint32_t op, std::uint32_t a1 = 0, std::uint32_t a2 = 0);
void SetCell(std::span<unsigned char> out, std::size_t at, std::uint32_t op, std::uint32_t a1 = 0, std::uint32_t a2 = 0);

} // namespace expanded::stb
