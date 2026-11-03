#include <gtest/gtest.h>

#include <bit>
#include <cstring>
#include <vector>

#include "expanded/data/stb.hpp"
#include "expanded/weapons/toan/snake_backup.hpp"
#include "runscript.hpp"

namespace {

// A program whose AI loop (label 100, its record at kLabel) yields and jumps back to its top.
constexpr int kCode = 0x60, kProg = 0x20, kLabel = 0x10, kLoop = 0x40, kLocals = 3;

std::vector<unsigned char> Program() {
    std::vector<unsigned char> program(kCode + kLoop + 5 * sizeof(vmcode_t), 0);
    RS_PROG_HEADER             header{};
    header.code = kCode;
    header.prog = kProg;
    header.prog_num = 1;
    std::memcpy(program.data(), &header, sizeof(header));
    const RS_PROGDATA label = {100, kCode + kLabel};
    std::memcpy(program.data() + kProg, &label, sizeof(label));
    const int record[4] = {kLoop, 0, kLocals, 0};
    std::memcpy(program.data() + kCode + kLabel, record, sizeof(record));
    const vmcode_t loop[] = {
        {23, 0,     0},
        {16, kLoop, 0},
        {3,  1,     0},
        {15, 0,     0}
    };
    std::memcpy(program.data() + kCode + kLoop, loop, sizeof(loop));
    return program;
}

const vmcode_t &CellAt(const std::vector<unsigned char> &program, int offset) {
    return *reinterpret_cast<const vmcode_t *>(program.data() + kCode + offset);
}

int RecordWord(const std::vector<unsigned char> &program, int record, int word) {
    int value = 0;
    std::memcpy(&value, program.data() + kCode + record + word * 4, sizeof(value));
    return value;
}

TEST(SnakeBackup, TheChariotsBackupIsAddedAndItsLoopCallsIt) {
    std::vector<unsigned char> program = Program(), out;
    expanded::SnakeBackup      backup;
    ASSERT_TRUE(expanded::AddSnakeBackup(program, 13, out, backup));
    // The original program is kept as it was, but for the loop's jump and its frame.
    EXPECT_EQ(std::memcmp(out.data(), program.data(), kCode + kLabel), 0);
    EXPECT_EQ(RecordWord(out, kLabel, 2), kLocals + 1);
    // The loop's jump back goes through the stub, which jumps on to the loop's top.
    int stub = CellAt(out, kLoop + static_cast<int>(sizeof(vmcode_t))).arg1;
    EXPECT_GT(stub, kLoop);
    EXPECT_EQ(CellAt(out, stub + 9 * static_cast<int>(sizeof(vmcode_t))).op, 16);
    EXPECT_EQ(CellAt(out, stub + 9 * static_cast<int>(sizeof(vmcode_t))).arg1, kLoop);
    // The stub: the player's distance into the new frame slot, within the trigger (0: never), then the backup.
    EXPECT_EQ(CellAt(out, stub + static_cast<int>(sizeof(vmcode_t))).arg1, kLocals);
    EXPECT_EQ(backup.trigger, stub + 4 * static_cast<int>(sizeof(vmcode_t)));
    EXPECT_FLOAT_EQ(std::bit_cast<float>(CellAt(out, backup.trigger).arg2), 0.0f);
    EXPECT_EQ(CellAt(out, stub + 7 * static_cast<int>(sizeof(vmcode_t))).op, 19);
    EXPECT_EQ(CellAt(out, stub + 7 * static_cast<int>(sizeof(vmcode_t))).arg2, backup.record);
    // The backup: no arguments, its code past the program's end, its hop the kind's motion, its jumps and calls in the
    // copy.
    EXPECT_EQ(RecordWord(out, backup.record, 3), 0);
    int at = RecordWord(out, backup.record, 0);
    EXPECT_GE(kCode + at, static_cast<int>(program.size()));
    int motions = 0, calls = 0;
    for (int i = 0; i < 129; i++) {
        const vmcode_t &cell = CellAt(out, at + i * static_cast<int>(sizeof(vmcode_t)));
        const vmcode_t &before = CellAt(out, at + (i - 1) * static_cast<int>(sizeof(vmcode_t)));
        if (i > 0 && before.op == 3 && before.arg2 == static_cast<int>(expanded::stb::kSetMotion)) {
            EXPECT_EQ(cell.arg2, 13);
            motions++;
        }
        if (cell.op == 16 || cell.op == 17) {
            EXPECT_GE(cell.arg1, at);
        }
        if (cell.op == 19) {
            EXPECT_EQ(RecordWord(out, cell.arg2, 0) > at, true);
            calls++;
        }
    }
    EXPECT_EQ(motions, 3);
    EXPECT_EQ(calls, 6);
}

TEST(SnakeBackup, ALoopWithoutAJumpBackTakesNone) {
    std::vector<unsigned char> program = Program(), out;
    vmcode_t                   yield = {23, 0, 0};
    std::memcpy(program.data() + kCode + kLoop + sizeof(vmcode_t), &yield, sizeof(yield));
    expanded::SnakeBackup backup;
    EXPECT_FALSE(expanded::AddSnakeBackup(program, 2, out, backup));
}

} // namespace
