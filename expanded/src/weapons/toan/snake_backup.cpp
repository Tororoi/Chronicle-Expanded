#include "expanded/weapons/toan/snake_backup.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>

#include "expanded/data/stb.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/weapons/toan/serpent_sword.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"

namespace expanded {

namespace {

// A cell of the Chariot's routines (e25a.stb), its jump's target from its function's start, its call's by the
// function's place in kTemplate.
enum CellKind : std::uint8_t {
    kPlain,
    kJump,
    kCall,
};

struct TemplateCell {
    int      op, arg1, arg2;
    CellKind kind;
};

// backup: 6 locals, 0 arguments.
constexpr TemplateCell kBackupCells[] = {
    {2,  4,    1,          kPlain},
    {3,  1,    0,          kPlain},
    {5,  0,    0,          kPlain},
    {4,  0,    0,          kPlain},
    {3,  1,    11,         kPlain},
    {3,  1,    2,          kPlain},
    {11, 0,    0,          kPlain},
    {2,  1,    8,          kPlain},
    {2,  2,    8,          kPlain},
    {2,  3,    8,          kPlain},
    {21, 5,    0,          kPlain},
    {3,  1,    12,         kPlain},
    {1,  1,    8,          kPlain},
    {1,  2,    8,          kPlain},
    {1,  3,    8,          kPlain},
    {3,  2,    1035489772, kPlain},
    {21, 5,    0,          kPlain},
    {3,  1,    11,         kPlain},
    {3,  1,    1,          kPlain},
    {11, 0,    0,          kPlain},
    {2,  1,    8,          kPlain},
    {2,  2,    8,          kPlain},
    {2,  3,    8,          kPlain},
    {21, 5,    0,          kPlain},
    {22, 0,    0,          kPlain},
    {3,  1,    1,          kPlain},
    {17, 1512, 0,          kJump },
    {23, 0,    0,          kPlain},
    {3,  1,    10,         kPlain},
    {1,  1,    8,          kPlain},
    {1,  2,    8,          kPlain},
    {1,  3,    8,          kPlain},
    {2,  5,    8,          kPlain},
    {21, 5,    0,          kPlain},
    {1,  5,    8,          kPlain},
    {3,  2,    1109393408, kPlain},
    {14, 45,   0,          kPlain},
    {17, 468,  0,          kJump },
    {16, 1512, 0,          kJump },
    {22, 2,    0,          kPlain},
    {3,  1,    10,         kPlain},
    {2,  5,    8,          kPlain},
    {21, 2,    0,          kPlain},
    {1,  5,    8,          kPlain},
    {3,  2,    1109393408, kPlain},
    {14, 45,   0,          kPlain},
    {17, 576,  0,          kJump },
    {16, 1512, 0,          kJump },
    {22, 3,    0,          kPlain},
    {3,  2,    1127481344, kPlain},
    {2,  0,    1,          kPlain},
    {19, 0,    1,          kCall },
    {4,  0,    0,          kPlain},
    {1,  0,    1,          kPlain},
    {17, 756,  0,          kJump },
    {3,  1,    200,        kPlain},
    {3,  1,    2,          kPlain},
    {3,  1,    1,          kPlain},
    {11, 0,    0,          kPlain},
    {3,  1,    4,          kPlain},
    {21, 4,    0,          kPlain},
    {19, 0,    2,          kCall },
    {4,  0,    0,          kPlain},
    {22, 4,    0,          kPlain},
    {1,  0,    1,          kPlain},
    {26, 0,    0,          kPlain},
    {17, 1008, 0,          kJump },
    {3,  2,    1127481344, kPlain},
    {3,  2,    1110704128, kPlain},
    {6,  0,    0,          kPlain},
    {2,  0,    1,          kPlain},
    {19, 0,    1,          kCall },
    {4,  0,    0,          kPlain},
    {1,  0,    1,          kPlain},
    {17, 996,  0,          kJump },
    {3,  1,    200,        kPlain},
    {3,  1,    2,          kPlain},
    {3,  1,    1,          kPlain},
    {11, 0,    0,          kPlain},
    {3,  1,    4,          kPlain},
    {21, 4,    0,          kPlain},
    {19, 0,    2,          kCall },
    {4,  0,    0,          kPlain},
    {22, 6,    0,          kPlain},
    {22, 5,    0,          kPlain},
    {1,  0,    1,          kPlain},
    {26, 0,    0,          kPlain},
    {17, 1260, 0,          kJump },
    {3,  2,    1127481344, kPlain},
    {3,  2,    1110704128, kPlain},
    {7,  0,    0,          kPlain},
    {2,  0,    1,          kPlain},
    {19, 0,    1,          kCall },
    {4,  0,    0,          kPlain},
    {1,  0,    1,          kPlain},
    {17, 1248, 0,          kJump },
    {3,  1,    200,        kPlain},
    {3,  1,    2,          kPlain},
    {3,  1,    1,          kPlain},
    {11, 0,    0,          kPlain},
    {3,  1,    4,          kPlain},
    {21, 4,    0,          kPlain},
    {19, 0,    2,          kCall },
    {4,  0,    0,          kPlain},
    {22, 8,    0,          kPlain},
    {22, 7,    0,          kPlain},
    {3,  1,    34,         kPlain},
    {21, 1,    0,          kPlain},
    {2,  4,    1,          kPlain},
    {1,  4,    1,          kPlain},
    {3,  1,    1,          kPlain},
    {6,  0,    0,          kPlain},
    {5,  0,    0,          kPlain},
    {4,  0,    0,          kPlain},
    {1,  4,    1,          kPlain},
    {3,  1,    4,          kPlain},
    {14, 44,   0,          kPlain},
    {17, 1452, 1,          kJump },
    {1,  5,    8,          kPlain},
    {3,  2,    1099956224, kPlain},
    {14, 42,   0,          kPlain},
    {22, 9,    0,          kPlain},
    {17, 1488, 0,          kJump },
    {16, 1512, 0,          kJump },
    {22, 10,   0,          kPlain},
    {16, 288,  0,          kJump },
    {22, 1,    0,          kPlain},
    {3,  1,    0,          kPlain},
    {15, 0,    0,          kPlain},
};
constexpr int kBackupLocals = 6, kBackupArguments = 0;
// leg: 8 locals, 2 arguments.
constexpr TemplateCell kLegCells[] = {
    {3,  1,   11,         kPlain},
    {3,  1,   2,          kPlain},
    {11, 0,   0,          kPlain},
    {2,  5,   8,          kPlain},
    {2,  6,   8,          kPlain},
    {2,  7,   8,          kPlain},
    {21, 5,   0,          kPlain},
    {3,  1,   30,         kPlain},
    {1,  5,   8,          kPlain},
    {1,  6,   8,          kPlain},
    {1,  7,   8,          kPlain},
    {1,  0,   8,          kPlain},
    {2,  2,   8,          kPlain},
    {2,  3,   8,          kPlain},
    {2,  4,   8,          kPlain},
    {21, 8,   0,          kPlain},
    {3,  1,   11,         kPlain},
    {3,  1,   1,          kPlain},
    {11, 0,   0,          kPlain},
    {2,  5,   8,          kPlain},
    {2,  6,   8,          kPlain},
    {2,  7,   8,          kPlain},
    {21, 5,   0,          kPlain},
    {2,  5,   8,          kPlain},
    {1,  5,   8,          kPlain},
    {1,  2,   8,          kPlain},
    {3,  2,   1101004800, kPlain},
    {8,  0,   0,          kPlain},
    {6,  0,   0,          kPlain},
    {5,  0,   0,          kPlain},
    {4,  0,   0,          kPlain},
    {2,  6,   8,          kPlain},
    {1,  6,   8,          kPlain},
    {1,  3,   8,          kPlain},
    {3,  2,   1101004800, kPlain},
    {8,  0,   0,          kPlain},
    {6,  0,   0,          kPlain},
    {5,  0,   0,          kPlain},
    {4,  0,   0,          kPlain},
    {2,  7,   8,          kPlain},
    {1,  7,   8,          kPlain},
    {1,  4,   8,          kPlain},
    {3,  2,   1101004800, kPlain},
    {8,  0,   0,          kPlain},
    {6,  0,   0,          kPlain},
    {5,  0,   0,          kPlain},
    {4,  0,   0,          kPlain},
    {3,  1,   32,         kPlain},
    {1,  5,   8,          kPlain},
    {1,  6,   8,          kPlain},
    {1,  7,   8,          kPlain},
    {3,  2,   1060320051, kPlain},
    {21, 5,   0,          kPlain},
    {3,  1,   0,          kPlain},
    {2,  1,   4,          kPlain},
    {3,  1,   0,          kPlain},
    {5,  0,   0,          kPlain},
    {4,  0,   0,          kPlain},
    {3,  1,   33,         kPlain},
    {1,  5,   8,          kPlain},
    {1,  6,   8,          kPlain},
    {1,  7,   8,          kPlain},
    {3,  1,   0,          kPlain},
    {2,  1,   4,          kPlain},
    {21, 5,   0,          kPlain},
    {3,  1,   0,          kPlain},
    {1,  1,   4,          kPlain},
    {17, 900, 0,          kJump },
    {3,  1,   32,         kPlain},
    {1,  5,   8,          kPlain},
    {1,  6,   8,          kPlain},
    {1,  7,   8,          kPlain},
    {3,  2,   1060320051, kPlain},
    {21, 5,   0,          kPlain},
    {23, 0,   0,          kPlain},
    {22, 0,   0,          kPlain},
    {3,  1,   0,          kPlain},
    {15, 0,   0,          kPlain},
};
constexpr int kLegLocals = 8, kLegArguments = 2;
// wait: 4 locals, 0 arguments.
constexpr TemplateCell kWaitCells[] = {
    {2,  0,   1,          kPlain},
    {3,  1,   0,          kPlain},
    {5,  0,   0,          kPlain},
    {4,  0,   0,          kPlain},
    {22, 0,   0,          kPlain},
    {1,  0,   1,          kPlain},
    {3,  1,   0,          kPlain},
    {14, 40,  0,          kPlain},
    {17, 324, 0,          kJump },
    {3,  1,   11,         kPlain},
    {3,  1,   2,          kPlain},
    {11, 0,   0,          kPlain},
    {2,  1,   8,          kPlain},
    {2,  2,   8,          kPlain},
    {2,  3,   8,          kPlain},
    {21, 5,   0,          kPlain},
    {3,  1,   12,         kPlain},
    {1,  1,   8,          kPlain},
    {1,  2,   8,          kPlain},
    {1,  3,   8,          kPlain},
    {3,  2,   1034147594, kPlain},
    {21, 5,   0,          kPlain},
    {23, 0,   0,          kPlain},
    {3,  1,   201,        kPlain},
    {2,  0,   1,          kPlain},
    {21, 2,   0,          kPlain},
    {16, 48,  0,          kJump },
    {22, 1,   0,          kPlain},
    {3,  1,   0,          kPlain},
    {15, 0,   0,          kPlain},
};
constexpr int kWaitLocals = 4, kWaitArguments = 0;

struct TemplateFunction {
    std::span<const TemplateCell> cells;
    int                           locals, arguments;
};

// The backup (its too-close backup, 0x20E4), the leg straight back it calls (0x1CF8) and the wait for the hop's end
// (0x2FC), in that order.
constexpr TemplateFunction kTemplate[] = {
    {kBackupCells, kBackupLocals, kBackupArguments},
    {kLegCells,    kLegLocals,    kLegArguments   },
    {kWaitCells,   kWaitLocals,   kWaitArguments  },
};

constexpr int kRecord = 16, kAiLabel = 100, kLoopCells = 8192;

struct Record {
    std::int32_t addr, name, local, arg;
};

std::int32_t Word(std::span<const unsigned char> bytes, std::size_t at) {
    std::int32_t word = 0;
    std::memcpy(&word, bytes.data() + at, sizeof(word));
    return word;
}

void Put(std::vector<unsigned char> &out, std::size_t at, const void *data, std::size_t size) {
    std::memcpy(out.data() + at, data, size);
}

// Each model slot's program with the backup added, and where.
struct Slot {
    std::vector<unsigned char> program;
    SnakeBackup                backup;
    const char                *code = nullptr;
};

std::array<Slot, kModelSlots> slots;

} // namespace

bool AddSnakeBackup(std::span<const unsigned char> program, int motion, std::vector<unsigned char> &out, SnakeBackup &backup) {
    if (program.size() < sizeof(RS_PROG_HEADER)) {
        return false;
    }
    RS_PROG_HEADER header{};
    std::memcpy(&header, program.data(), sizeof(header));
    // Its AI loop: its record, and the loop's jump back to its top (the last jump backward).
    int label = -1;
    for (int i = 0; i < header.prog_num; i++) {
        std::size_t at = static_cast<std::size_t>(header.prog) + static_cast<std::size_t>(i) * sizeof(RS_PROGDATA);
        if (at + sizeof(RS_PROGDATA) <= program.size() && Word(program, at) == kAiLabel) {
            label = Word(program, at + 4) - header.code;
        }
    }
    if (label < 0 || static_cast<std::size_t>(header.code + label) + kRecord > program.size()) {
        return false;
    }
    Record loop{};
    std::memcpy(&loop, program.data() + header.code + label, sizeof(loop));
    int back = -1;
    for (int i = 0; i < kLoopCells; i++) {
        std::size_t at = static_cast<std::size_t>(header.code + loop.addr) + static_cast<std::size_t>(i) * sizeof(vmcode_t);
        if (at + 2 * sizeof(vmcode_t) > program.size()) {
            return false;
        }
        int op = Word(program, at), offset = loop.addr + i * static_cast<int>(sizeof(vmcode_t));
        if (op == static_cast<int>(stb::kJump) && Word(program, at + 4) < offset) {
            back = offset;
        }
        if (op == static_cast<int>(stb::kReturn) && Word(program, at + sizeof(vmcode_t)) == 0) {
            break;
        }
    }
    if (back < 0) {
        return false;
    }
    int top = Word(program, static_cast<std::size_t>(header.code + back) + 4);

    // Past its end (on a 16-byte line): the three records, then each function's cells and a blank cell after them,
    // then the stub.
    std::size_t end = (program.size() + 15) & ~std::size_t{15};
    int         records = static_cast<int>(end) - header.code;
    int         at = records + static_cast<int>(std::size(kTemplate)) * kRecord;
    int         starts[std::size(kTemplate)] = {};
    for (std::size_t f = 0; f < std::size(kTemplate); f++) {
        starts[f] = at;
        at += static_cast<int>((kTemplate[f].cells.size() + 1) * sizeof(vmcode_t));
    }
    int            stub = at;
    const int      local = loop.local;
    const auto     bits = [](float value) { return std::bit_cast<int>(value); };
    const vmcode_t stub_cells[] = {
        {static_cast<int>(stb::kPushConst),   static_cast<int>(stb::kInt),                   static_cast<int>(stb::kGetDistance)},
        {static_cast<int>(stb::kLoadAddress), local,                                         static_cast<int>(stb::kFloatLocal) },
        {static_cast<int>(stb::kExt),         2,                                             0                                  },
        {static_cast<int>(stb::kLoad),        local,                                         static_cast<int>(stb::kFloatLocal) },
        {static_cast<int>(stb::kPushConst),   static_cast<int>(stb::kFloat),                 bits(0.0f)                         }, // the trigger
        {static_cast<int>(stb::kCompare),     static_cast<int>(stb::kLessOrEqual),           0                                  },
        {static_cast<int>(stb::kJumpFalse),   stub + 9 * static_cast<int>(sizeof(vmcode_t)), 0                                  },
        {static_cast<int>(stb::kCall),        0,                                             records                            },
        {static_cast<int>(stb::kPop),         0,                                             0                                  },
        {static_cast<int>(stb::kJump),        top,                                           0                                  },
    };
    out.assign(program.begin(), program.end());
    out.resize(static_cast<std::size_t>(header.code + stub) + sizeof(stub_cells), 0);
    for (std::size_t f = 0; f < std::size(kTemplate); f++) {
        const Record record = {starts[f], 0, kTemplate[f].locals, kTemplate[f].arguments};
        Put(out, static_cast<std::size_t>(header.code + records) + f * kRecord, &record, sizeof(record));
        const auto &cells = kTemplate[f].cells;
        for (std::size_t i = 0; i < cells.size(); i++) {
            vmcode_t cell = {cells[i].op, cells[i].arg1, cells[i].arg2};
            if (cells[i].kind == kJump) {
                cell.arg1 += starts[f];
            } else if (cells[i].kind == kCall) {
                cell.arg2 = records + cells[i].arg2 * kRecord;
            } else if (i > 0 && cells[i - 1].op == static_cast<int>(stb::kPushConst) && cells[i - 1].arg2 == static_cast<int>(stb::kSetMotion) &&
                       cell.op == static_cast<int>(stb::kPushConst)) {
                cell.arg2 = motion; // its hop: the kind's own backup motion
            }
            Put(out, static_cast<std::size_t>(header.code + starts[f]) + i * sizeof(vmcode_t), &cell, sizeof(cell));
        }
    }
    Put(out, static_cast<std::size_t>(header.code + stub), stub_cells, sizeof(stub_cells));
    // The loop's jump back through the stub, and a slot in its frame for the stub's distance.
    Put(out, static_cast<std::size_t>(header.code + back) + 4, &stub, sizeof(stub));
    loop.local++;
    Put(out, static_cast<std::size_t>(header.code + label), &loop, sizeof(loop));
    backup = {records, stub + 4 * static_cast<int>(sizeof(vmcode_t))};
    return true;
}

char *SnakeBackupScript(CMonstorUnit &unit, int slot, int model_no, char *script, int size) {
    if (slot < 0 || slot >= static_cast<int>(slots.size()) || AllyUnit(unit)) {
        return script;
    }
    Slot &added = slots[static_cast<std::size_t>(slot)];
    added = {};
    const SnakeRetreat *retreat = SnakeRetreatOf(model_no);
    if (retreat == nullptr || retreat->record != 0 || size <= 0 ||
        !AddSnakeBackup({reinterpret_cast<const unsigned char *>(script), static_cast<std::size_t>(size)}, retreat->motion, added.program, added.backup)) {
        return script;
    }
    char *program = reinterpret_cast<char *>(added.program.data());
    added.code = program + reinterpret_cast<const RS_PROG_HEADER *>(program)->code;
    return program;
}

const SnakeBackup *SnakeBackupOf(const char *code) {
    for (const Slot &slot : slots) {
        if (slot.code != nullptr && slot.code == code) {
            return &slot.backup;
        }
    }
    return nullptr;
}

} // namespace expanded
