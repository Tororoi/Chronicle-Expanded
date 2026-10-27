#include "expanded/data/monster_scripts.hpp"

#include <algorithm>
#include <bit>
#include <format>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "expanded/data/data_build.hpp"
#include "expanded/data/stb.hpp"

namespace expanded {

namespace {

using Arg = std::variant<std::monostate, int, float, std::string>;

struct Sphere {
    const char *name;
    float       radius;
};

struct Param {
    std::uint32_t command;
    Arg           a, b;
};

enum class Bake {
    kReplace,
    kArm,
    kGuardWake
};

struct Spec {
    std::string         script;
    std::string         who;
    Bake                bake;
    std::vector<Sphere> spheres; // kReplace: the new hurt-sphere set
    std::vector<Param>  params;  // kArm: added after each _SET_BODY_COL
    float               guard[2] = {};
};

std::vector<Spec> Specs() {
    std::vector<Spec> specs = {
        {"e65a", "Blizzard → Titan", Bake::kReplace, {{"bcol0", 5.0f}, {"bcol1", 10.6f}, {"bcol2", 6.0f}, {"bcol3", 6.0f}}},
        {"e86a", "Sam → Mr. Blare", Bake::kReplace, {{"bcol0", 8.5f}, {"bcol1", 6.5f}}},
        {"e69a", "Billy → Mr. Blare", Bake::kReplace, {{"bcol0", 8.5f}, {"bcol1", 6.5f}}},
        {"c16a",
         "Minotaur Joe: every hurt sphere cat-hittable",
         Bake::kArm,
         {},
         {{stb::kSetBodyColPara, 0, kSpiritBeastSpherePercent}, {stb::kSetBodyColPara, 1, kSpiritBeastSphereKick}}},
    };
    for (const char *mimic : kMimicScripts) {
        specs.push_back({
            mimic, std::format("Mimic {}: wake guards instead of 100 invincibility frames", mimic), Bake::kGuardWake, {},
                {},
                {kMimicWakeGuardStart, kMimicWakeGuardEnd}
        });
    }
    for (const char *king : kKingMimicScripts) {
        specs.push_back({
            king, std::format("King Mimic {}: wake guards instead of 100 invincibility frames", king), Bake::kGuardWake, {},
                {},
                {kMimicWakeGuardStart, kKingMimicWakeGuardEnd}
        });
    }
    return specs;
}

struct Block {
    std::size_t      pos;
    std::size_t      cells;
    std::vector<Arg> args;
};

// Every command(...) call: the pushes (the command's number, then its arguments) and the EXT that takes them, its
// arguments read as int, float or string (none for a push that is not a constant). Searched on every word, as the
// mod's bake does.
std::vector<Block> CallBlocks(std::span<const unsigned char> script, std::uint32_t command) {
    std::size_t        base = stb::CodeBase(script);
    std::vector<Block> blocks;
    for (std::size_t at = base; at + stb::kCell <= script.size(); at += 4) {
        if (Get32(script, at) != stb::kExt || Get32(script, at + 8) != 0) {
            continue;
        }
        std::uint32_t count = Get32(script, at + 4);
        if (count < 2 || count > 10 || at < count * stb::kCell) {
            continue;
        }
        std::size_t first = at - count * stb::kCell;
        if (first < base || Get32(script, first) != stb::kPushConst || Get32(script, first + 4) != stb::kInt || Get32(script, first + 8) != command) {
            continue;
        }
        Block block{first, count + 1, {}};
        for (std::uint32_t k = 1; k < count; k++) {
            std::size_t   cell = first + k * stb::kCell;
            std::uint32_t op = Get32(script, cell), kind = Get32(script, cell + 4), value = Get32(script, cell + 8);
            if (op == stb::kPushConst && kind == stb::kString && base + value < script.size()) {
                block.args.emplace_back(NameAt(script, base + value, script.size() - base - value));
            } else if (op == stb::kPushConst && kind == stb::kFloat) {
                block.args.emplace_back(std::bit_cast<float>(value));
            } else if (op == stb::kPushConst && kind == stb::kInt) {
                block.args.emplace_back(static_cast<int>(value));
            } else {
                block.args.emplace_back();
            }
        }
        blocks.push_back(std::move(block));
    }
    return blocks;
}

void Push(Bytes &out, const Arg &value) {
    if (const float *f = std::get_if<float>(&value)) {
        stb::AppendCell(out, stb::kPushConst, stb::kFloat, std::bit_cast<std::uint32_t>(*f));
    } else {
        stb::AppendCell(out, stb::kPushConst, stb::kInt, static_cast<std::uint32_t>(std::get<int>(value)));
    }
}

void Params(Bytes &out, const std::vector<Param> &params) {
    for (const Param &param : params) {
        stb::AppendCell(out, stb::kPushConst, stb::kInt, param.command);
        Push(out, param.a);
        Push(out, param.b);
        stb::AppendCell(out, stb::kExt, 3, 0);
    }
}

// A function's header: its entry, from the code base; no locals, no arguments.
void FunctionHeader(Bytes &out, std::size_t base) {
    std::size_t entry = out.size() + stb::kFunctionHeader - base;
    Append32(out, static_cast<std::uint32_t>(entry));
    out.resize(out.size() + stb::kFunctionHeader - 4, 0);
}

void Mark(Bytes &out) {
    out.insert(out.end(), kBakedScriptMark.begin(), kBakedScriptMark.end());
}

// Writes over a block: a call to the function at entry (none: nothing), the rest no-ops.
void Overwrite(Bytes &out, const Block &block, std::optional<std::size_t> function) {
    Bytes       cells;
    std::size_t nops = block.cells;
    if (function) {
        stb::AppendCell(cells, stb::kCall, 0, static_cast<std::uint32_t>(*function));
        nops--;
    }
    for (std::size_t i = 0; i < nops; i++) {
        stb::AppendCell(cells, stb::kMarker);
    }
    std::copy(cells.begin(), cells.end(), out.begin() + static_cast<std::ptrdiff_t>(block.pos));
}

Bytes Copy(std::span<const unsigned char> script, const Block &block) {
    std::span<const unsigned char> cells = Slice(script, block.pos, block.cells * stb::kCell, "a call block");
    return Bytes(cells.begin(), cells.end());
}

bool Is100(const Block &block) {
    if (block.args.size() != 1) {
        return false;
    }
    const int   *i = std::get_if<int>(&block.args[0]);
    const float *f = std::get_if<float>(&block.args[0]);
    return (i != nullptr && *i == 100) || (f != nullptr && *f == 100.0f);
}

Bytes GuardWake(std::span<const unsigned char> script, const Spec &spec, std::size_t base, std::string &done) {
    std::vector<Block> muteki;
    for (Block &block : CallBlocks(script, stb::kStatusSetMuteki)) {
        if (Is100(block)) {
            muteki.push_back(std::move(block));
        }
    }
    if (muteki.empty()) {
        throw std::runtime_error("no _STATUS_SET_MUTEKI(100) block found");
    }
    std::vector<Block> anchors = CallBlocks(script, stb::kSetGuardFrame);
    if (anchors.empty()) {
        anchors = CallBlocks(script, stb::kSetBodyCol);
    }
    if (anchors.empty()) {
        throw std::runtime_error("no init-label block to hang the guard frame on");
    }
    const Block &anchor = anchors[0];
    Bytes        out(script.begin(), script.end());
    PadTo4(out);
    Mark(out);
    PadTo4(out);
    std::size_t function = out.size();
    FunctionHeader(out, base);
    Bytes cells = Copy(script, anchor);
    out.insert(out.end(), cells.begin(), cells.end());
    Params(out, {
                    {stb::kSetGuardFrame, spec.guard[0], spec.guard[1]}
    });
    stb::AppendCell(out, stb::kReturn);
    Overwrite(out, anchor, function - base);
    for (const Block &block : muteki) {
        Overwrite(out, block, std::nullopt);
    }
    done = std::format("{}: {} MUTEKI(100) block(s) NOP'd, guard frames {:g}..{:g} added via the block @0x{:X} → function @+0x{:X}", spec.who, muteki.size(),
                       spec.guard[0], spec.guard[1], anchor.pos, function - base);
    return out;
}

Bytes Replace(std::span<const unsigned char> script, const Spec &spec, std::size_t base, const std::vector<Block> &blocks, std::string &done) {
    Bytes out(script.begin(), script.end());
    PadTo4(out);
    std::map<std::string, std::size_t> names;
    for (const Sphere &sphere : spec.spheres) {
        if (!names.contains(sphere.name)) {
            names[sphere.name] = out.size() - base;
            out.insert(out.end(), sphere.name, sphere.name + std::char_traits<char>::length(sphere.name) + 1);
        }
    }
    Mark(out);
    PadTo4(out);
    std::size_t function = out.size();
    FunctionHeader(out, base);
    for (const Sphere &sphere : spec.spheres) {
        stb::AppendCell(out, stb::kPushConst, stb::kInt, stb::kSetBodyCol);
        stb::AppendCell(out, stb::kPushConst, stb::kString, static_cast<std::uint32_t>(names[sphere.name]));
        stb::AppendCell(out, stb::kPushConst, stb::kFloat, std::bit_cast<std::uint32_t>(sphere.radius));
        stb::AppendCell(out, stb::kExt, 3, 0);
    }
    stb::AppendCell(out, stb::kReturn);
    Overwrite(out, blocks[0], function - base);
    for (std::size_t i = 1; i < blocks.size(); i++) {
        Overwrite(out, blocks[i], std::nullopt);
    }
    done = std::format("{}: {} block(s) → one function @+0x{:X} ({} spheres)", spec.who, blocks.size(), function - base, spec.spheres.size());
    return out;
}

// Each _SET_BODY_COL becomes a call to a function of its own: the block as it was, the params, a return.
Bytes Arm(std::span<const unsigned char> script, const Spec &spec, std::size_t base, const std::vector<Block> &blocks, std::string &done) {
    Bytes out(script.begin(), script.end());
    PadTo4(out);
    Mark(out);
    PadTo4(out);
    std::string functions;
    for (const Block &block : blocks) {
        std::size_t function = out.size();
        FunctionHeader(out, base);
        Bytes cells = Copy(script, block);
        out.insert(out.end(), cells.begin(), cells.end());
        Params(out, spec.params);
        stb::AppendCell(out, stb::kReturn);
        Overwrite(out, block, function - base);
        const std::string *name = block.args.empty() ? nullptr : std::get_if<std::string>(&block.args[0]);
        functions += std::format("{}{}@+0x{:X}", functions.empty() ? "" : ", ", name != nullptr ? *name : "", function - base);
    }
    done = std::format("{}: {} block(s) → {} ({} params each)", spec.who, blocks.size(), functions, spec.params.size());
    return out;
}

} // namespace

bool ScriptIsBaked(std::string_view script) {
    std::vector<Spec> specs = Specs();
    return std::any_of(specs.begin(), specs.end(), [&](const Spec &spec) { return EqualsFolded(spec.script, script); });
}

std::string BakedScriptFile(std::string_view script) {
    return std::format("{}{}.stb", kBakedScriptDir, script);
}

Bytes BakeMonsterScript(std::string_view script, std::span<const unsigned char> stb, std::string *done) {
    std::vector<Spec> specs = Specs();
    auto              spec = std::find_if(specs.begin(), specs.end(), [&](const Spec &s) { return EqualsFolded(s.script, script); });
    if (spec == specs.end()) {
        throw std::runtime_error(std::format("{} is not a script the bake rebuilds", script));
    }
    if (std::search(stb.begin(), stb.end(), kBakedScriptMark.begin(), kBakedScriptMark.end()) != stb.end()) {
        throw std::runtime_error("already rebuilt");
    }
    std::size_t base = stb::CodeBase(stb);
    std::string note;
    Bytes       out;
    if (spec->bake == Bake::kGuardWake) {
        out = GuardWake(stb, *spec, base, note);
    } else {
        std::vector<Block> blocks = CallBlocks(stb, stb::kSetBodyCol);
        if (blocks.empty()) {
            throw std::runtime_error("no _SET_BODY_COL block found");
        }
        out = spec->bake == Bake::kReplace ? Replace(stb, *spec, base, blocks, note) : Arm(stb, *spec, base, blocks, note);
    }
    if (done != nullptr) {
        *done = note;
    }
    return out;
}

std::vector<DataBuild> MonsterScriptBuilds() {
    std::vector<DataBuild> builds;
    for (const Spec &spec : Specs()) {
        builds.push_back(DataBuild::FromSources(spec.who, BakedScriptFile(spec.script), {std::format("dun/monstor/{}.stb", spec.script)},
                                                [script = spec.script](const std::vector<Bytes> &s) { return BakeMonsterScript(script, s[0]); }));
    }
    return builds;
}

} // namespace expanded
