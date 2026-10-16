#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "expanded/data/script_damage.hpp"

using namespace expanded;

namespace {

constexpr std::uint32_t kBase = 0x60;

struct Cell {
    std::uint32_t op, a1, a2;
};

void Put(std::vector<std::uint8_t> &bytes, std::size_t at, std::uint32_t value) {
    if (bytes.size() < at + 4) {
        bytes.resize(at + 4);
    }
    for (int i = 0; i < 4; i++) {
        bytes[at + i] = static_cast<std::uint8_t>(value >> (8 * i));
    }
}

Cell Int(std::int32_t value) {
    return {3, 1, static_cast<std::uint32_t>(value)};
}

constexpr Cell kFloat = {3, 2, 0x3F800000};
constexpr Cell kName = {3, 3, 0};

// A script of one label running first, which calls second; the functions' headers follow the code.
std::vector<std::uint8_t> Script(const std::vector<Cell> &first, const std::vector<Cell> &second) {
    std::vector<std::uint8_t> bytes(kBase);
    std::size_t               at = kBase;
    auto                      emit = [&](const std::vector<Cell> &cells) {
        std::size_t entry = at;
        for (const Cell &cell : cells) {
            Put(bytes, at, cell.op);
            Put(bytes, at + 4, cell.a1);
            Put(bytes, at + 8, cell.a2);
            at += 12;
        }
        Put(bytes, at, 15); // the return, then an empty cell
        Put(bytes, at + 12 + 8, 0);
        at += 24;
        return entry;
    };
    std::size_t first_entry = emit(first);
    std::size_t second_entry = emit(second);
    std::size_t first_header = at, second_header = at + 0x38;
    Put(bytes, first_header, static_cast<std::uint32_t>(first_entry - kBase));
    Put(bytes, second_header, static_cast<std::uint32_t>(second_entry - kBase));
    Put(bytes, second_header + 0x34, 0);
    // Patch the call's target now the second header is placed.
    for (std::size_t cell = first_entry; cell < second_entry; cell += 12) {
        if (bytes[cell] == 19) {
            Put(bytes, cell + 8, static_cast<std::uint32_t>(second_header - kBase));
        }
    }
    Put(bytes, 8, kBase);
    Put(bytes, 0xC, 0x14);
    Put(bytes, 0x10, 1);
    Put(bytes, 0x14, 0);
    Put(bytes, 0x18, static_cast<std::uint32_t>(first_header));
    return bytes;
}

} // namespace

TEST(ScriptDamage, ReadsTheStrongestHitAndShotThroughCalls) {
    std::vector<std::uint8_t> script = Script(
        {
            Int(132), Int(40), Int(0x200), Int(2), {21, 4, 0},                // _SET_DMG_PARA(40, 0x200, 2)
            Int(132), Int(55), Int(0), Int(3), kFloat, {21, 5, 0},            // _SET_DMG_PARA(55, 0, 3, 1.0f)
            Int(133), kName, kFloat, kFloat, kFloat, {21, 5, 0},              // _SET_SHOT without damage
            Int(133), kName, kFloat, kFloat, kFloat, kFloat, {21, 6, 0},      // a float damage is no constant
            Int(90), {19, 0, 0},                                              // second(90)
        },
        {
            Int(229), kName, kFloat, kFloat, kFloat, {1, 0, 1}, {21, 6, 0},   // _SET_SHOT2(..., argument 0)
        });

    std::optional<ScriptDamage> damage = ReadScriptDamage(script);
    ASSERT_TRUE(damage.has_value());
    EXPECT_EQ(damage->melee, 55);
    EXPECT_EQ(damage->shot, 90);
    EXPECT_TRUE(damage->default_shot[0]);
    EXPECT_FALSE(damage->default_shot[1]);
}

TEST(ScriptDamage, NoScriptReadsAsNone) {
    EXPECT_FALSE(ReadScriptDamage(std::vector<std::uint8_t>(8)).has_value());
}

TEST(ScriptDamage, TheFileRoundTrips) {
    std::map<std::string, ScriptDamage> scripts = {{"e07a", {68, 0, {false, false}}}, {"e08a", {37, 35, {true, true}}}};
    auto                                parsed = ParseScriptDamage(FormatScriptDamage(scripts));
    ASSERT_EQ(parsed.size(), 2u);
    EXPECT_EQ(parsed["e07a"], scripts["e07a"]);
    EXPECT_EQ(parsed["e08a"], scripts["e08a"]);
}
