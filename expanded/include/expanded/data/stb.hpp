#pragma once

// A script (.stb), a monster's or a town's: its code base @kCodeBaseAt, its label table and count @kLabelTableAt and
// @kLabelCountAt ({id, function header} pairs), its code 12-byte cells {op, a1, a2}. A function header is
// kFunctionHeader bytes: its entry from the code base, its name, its locals and its arguments. Includes no game header,
// so dcdata can build with it.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "expanded/data/bytes.hpp"

namespace expanded::stb {

inline constexpr std::size_t kCodeBaseAt = 0x08, kLabelTableAt = 0x0C, kLabelCountAt = 0x10, kLabelEntry = 8;
inline constexpr std::size_t kCell = 12, kFunctionHeader = 56;

// Opcodes.
inline constexpr std::uint32_t kLoad = 1, kLoadAddress = 2, kPushConst = 3, kPop = 4, kStore = 5, kReturn = 15, kJump = 16,
                               kJumpFalse = 17, kJumpTrue = 18, kCall = 19, kExt = 21, kMarker = 22, kWait = 23, kEnd = 27;

// Operators: a binary one pops two values and pushes one, a unary one pops one and pushes one (10, 12, 13, 29 and 30
// unnamed). A compare's a1 is its test.
inline constexpr std::uint32_t kAdd = 6, kSubtract = 7, kMultiply = 8, kDivide = 9, kNegate = 11, kCompare = 14, kAnd = 24,
                               kOr = 25, kNot = 26;
inline constexpr std::uint32_t kBinaryOperators[] = {kAdd, kSubtract, kMultiply, kDivide, 10, kCompare, kAnd, kOr};
inline constexpr std::uint32_t kUnaryOperators[] = {kNegate, 12, 13, kNot, 29, 30};
inline constexpr std::uint32_t kEqual = 40, kNotEqual = 41, kLessThan = 42, kLessOrEqual = 43, kGreaterThan = 44,
                               kGreaterOrEqual = 45;

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

// A function written cell by cell: its jumps go to marks, and its strings and jumps are laid where it is placed.
class Writer {
public:
    void PushInt(int value);
    void PushFloat(float value);
    void PushString(std::string_view text);
    // A local's value, or its address (an out argument); the float forms tag the local a float, so a command reads its
    // bits as one.
    void PushLocal(int local);
    void PushLocalFloat(int local);
    void PushAddress(int local);
    void PushAddressFloat(int local);
    // A command: entries counts it and its arguments.
    void Ext(int entries);
    // A function record's offset from the code base.
    void Call(std::size_t function);
    void Return();
    void Yield();
    void Pop();
    // Pops a value and an address, stores the value there and pushes it again.
    void Store();
    void Add();
    void Subtract();
    void And();
    // Pops two values and pushes the test between them, the first pushed on its left.
    void Compare(std::uint32_t test);

    int  Mark();
    int  MarkForward();
    void Place(int mark);
    void Jump(int mark);
    void JumpTrue(int mark);
    void JumpFalse(int mark);

    void UseLocals(int count);
    // A function called with count arguments, its first locals.
    void SetArguments(int count);

    int Locals() const { return locals_; }

    // The function at file offset at in a script with code base base: its header, cells and strings.
    Bytes Lay(std::size_t at, std::size_t base) const;

private:
    void Cell(std::uint32_t op, std::uint32_t a1, std::uint32_t a2);
    void JumpCell(std::uint32_t op, int mark);

    Bytes                                            cells_;
    std::vector<std::pair<std::string, std::size_t>> strings_; // text, its push's cell
    std::vector<std::size_t>                         marks_;
    std::vector<std::pair<std::size_t, int>>         jumps_; // jump's cell, its mark
    int                                              locals_ = 0;
    int                                              arguments_ = 0;
};

// A script with functions laid after its end, each under a label of its own, and its label table moved after them.
class Appender {
public:
    explicit Appender(std::span<const unsigned char> script);
    // Where the next function's header goes, from the code base: a call's operand.
    std::size_t Next() const;
    void        Add(std::uint32_t id, const Writer &function);
    Bytes       Build() const;

private:
    Bytes                                              script_;
    std::vector<std::pair<std::uint32_t, std::size_t>> labels_;
};

} // namespace expanded::stb
