#include "expanded/data/script_damage.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <vector>

#include "expanded/data/data_build.hpp"
#include "expanded/data/stb.hpp"

namespace expanded {

namespace {

namespace fs = std::filesystem;

// The script's layout and cells as stb.hpp has them; an int local (stb::kIntLocal) is a function's argument.
constexpr int kMaxDepth = 8;

struct Value {
    enum Kind { Unknown, Constant, Argument } kind = Unknown;
    std::int32_t value = 0;
};

class Script {
public:
    explicit Script(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

    // A word, 0 past the end.
    std::uint32_t U32(std::size_t at) const { return at + 4 > bytes_.size() ? 0 : Get32(bytes_, at); }

    std::size_t Size() const { return bytes_.size(); }

    // A function's cells, from its entry to the return followed by an empty cell.
    std::vector<std::size_t> Cells(std::size_t base, std::size_t header) const {
        std::vector<std::size_t> cells;
        for (std::size_t at = base + U32(header); at + stb::kCell <= Size(); at += stb::kCell) {
            cells.push_back(at);
            if (U32(at) == stb::kReturn && (at + 2 * stb::kCell > Size() || U32(at + stb::kCell) == 0)) {
                break;
            }
        }
        return cells;
    }

private:
    std::span<const std::uint8_t> bytes_;
};

struct Use {
    bool        melee = false;
    Value       damage;
    std::size_t function = 0;
};

} // namespace

std::optional<ScriptDamage> ReadScriptDamage(std::span<const std::uint8_t> bytes) {
    Script script(bytes);
    if (bytes.size() < stb::kLabelCountAt + 4) {
        return std::nullopt;
    }
    std::size_t base = script.U32(stb::kCodeBaseAt), table = script.U32(stb::kLabelTableAt), count = script.U32(stb::kLabelCountAt);
    if (base >= bytes.size() || table + count * stb::kLabelEntry > bytes.size()) {
        return std::nullopt;
    }

    // Every function the labels reach through calls.
    std::set<std::size_t>    functions;
    std::vector<std::size_t> todo;
    for (std::size_t i = 0; i < count; i++) {
        std::size_t header = script.U32(table + i * stb::kLabelEntry + 4);
        if (functions.insert(header).second) {
            todo.push_back(header);
        }
    }
    while (!todo.empty()) {
        std::size_t header = todo.back();
        todo.pop_back();
        for (std::size_t at : script.Cells(base, header)) {
            std::uint32_t op = script.U32(at);
            std::size_t   target = base + script.U32(at + 8);
            if ((op == stb::kCall || op == stb::kEnd) && target < bytes.size() && functions.insert(target).second) {
                todo.push_back(target);
            }
        }
    }

    // Each function's stack folded: the damage argument of each command, and each call's site and caller.
    ScriptDamage                                                     damage;
    std::vector<Use>                                                 uses;
    std::map<std::size_t, std::vector<std::pair<std::size_t, std::size_t>>> calls; // callee: {site, caller}
    for (std::size_t header : functions) {
        std::vector<Value> stack;
        auto               pop = [&](std::size_t n) { stack.resize(stack.size() - std::min(n, stack.size())); };
        for (std::size_t at : script.Cells(base, header)) {
            std::uint32_t op = script.U32(at), a1 = script.U32(at + 4), a2 = script.U32(at + 8);
            switch (op) {
                case stb::kLoad:
                    stack.push_back(a2 == stb::kIntLocal ? Value{Value::Argument, static_cast<std::int32_t>(a1)} : Value{});
                    break;
                case stb::kLoadAddress:
                    stack.push_back({});
                    break;
                case stb::kPushConst:
                    stack.push_back(a1 != stb::kFloat && a1 != stb::kString ? Value{Value::Constant, static_cast<std::int32_t>(a2)} : Value{});
                    break;
                case stb::kPop:
                case stb::kReturn:
                case stb::kJumpFalse:
                case stb::kJumpTrue:
                    pop(1);
                    break;
                case stb::kExt: {
                    std::size_t        n = std::min<std::size_t>(a1, stack.size());
                    std::vector<Value> args(stack.end() - static_cast<std::ptrdiff_t>(n), stack.end());
                    pop(n);
                    if (!args.empty() && args[0].kind == Value::Constant) {
                        std::int32_t command = args[0].value;
                        if (command == stb::kSetDmgPara && args.size() >= 2) {
                            uses.push_back({true, args[1], header});
                        } else if ((command == stb::kSetShot || command == stb::kSetShot2) && a1 == 6 && args.size() == 6) {
                            uses.push_back({false, args[5], header});
                        } else if (command == stb::kSetShot || command == stb::kSetShot2) {
                            damage.default_shot[command == stb::kSetShot ? 0 : 1] = true;
                        }
                    }
                    stack.push_back({});
                    break;
                }
                case stb::kCall:
                case stb::kEnd:
                    calls[base + a2].push_back({at, header});
                    stack.push_back({});
                    break;
                default:
                    if (op == stb::kStore || stb::IsBinaryOperator(op)) {
                        pop(2);
                        stack.push_back({});
                    } else if (stb::IsUnaryOperator(op)) {
                        pop(1);
                        stack.push_back({});
                    }
                    break;
            }
        }
    }

    // A function's argument: the constant each caller passes, through its own arguments too.
    auto resolve = [&](auto &&self, Value value, std::size_t function, int depth, std::set<std::pair<std::size_t, int>> seen,
                       std::vector<std::int32_t> &out) -> void {
        if (value.kind == Value::Constant) {
            out.push_back(value.value);
            return;
        }
        if (value.kind != Value::Argument || depth > kMaxDepth || !seen.insert({function, value.value}).second) {
            return;
        }
        for (auto [site, caller] : calls[function]) {
            std::vector<std::size_t> args;
            for (std::size_t at = site; at >= base + stb::kCell;) {
                at -= stb::kCell;
                std::uint32_t op = script.U32(at);
                if (op != stb::kLoad && op != stb::kLoadAddress && op != stb::kPushConst) {
                    break;
                }
                args.insert(args.begin(), at);
            }
            if (value.value < 0 || static_cast<std::size_t>(value.value) >= args.size()) {
                continue;
            }
            std::size_t   at = args[value.value];
            std::uint32_t op = script.U32(at), a1 = script.U32(at + 4), a2 = script.U32(at + 8);
            if (op == stb::kPushConst && a1 != stb::kFloat && a1 != stb::kString) {
                out.push_back(static_cast<std::int32_t>(a2));
            } else if (op == stb::kLoad && a2 == stb::kIntLocal) {
                self(self, Value{Value::Argument, static_cast<std::int32_t>(a1)}, caller, depth + 1, seen, out);
            }
        }
    };
    for (const Use &use : uses) {
        std::vector<std::int32_t> values;
        resolve(resolve, use.damage, use.function, 0, {}, values);
        for (std::int32_t value : values) {
            int &best = use.melee ? damage.melee : damage.shot;
            best = std::max(best, static_cast<int>(value));
        }
    }
    return damage;
}

std::string FormatScriptDamage(const std::map<std::string, ScriptDamage> &scripts) {
    std::string text = "# script melee shot default-shots (1: _SET_SHOT, 2: _SET_SHOT2), from dcdata expanded\n";
    for (const auto &[name, damage] : scripts) {
        text += std::format("{} {} {} {}\n", name, damage.melee, damage.shot,
                            (damage.default_shot[0] ? 1 : 0) | (damage.default_shot[1] ? 2 : 0));
    }
    return text;
}

std::map<std::string, ScriptDamage, std::less<>> ParseScriptDamage(std::string_view text) {
    std::map<std::string, ScriptDamage, std::less<>> scripts;
    std::istringstream                               lines{std::string(text)};
    std::string                                      line;
    while (std::getline(lines, line)) {
        std::istringstream fields(line);
        std::string        name;
        ScriptDamage       damage;
        int                defaults = 0;
        if (line.starts_with('#') || !(fields >> name >> damage.melee >> damage.shot >> defaults)) {
            continue;
        }
        damage.default_shot[0] = (defaults & 1) != 0;
        damage.default_shot[1] = (defaults & 2) != 0;
        scripts[name] = damage;
    }
    return scripts;
}

int BuildScriptDamage(const fs::path &data, std::FILE *log) {
    auto say = [&](const std::string &line) {
        if (log) {
            std::fputs(line.c_str(), log);
        }
    };
    std::error_code error;
    fs::path        folder = data / "dun" / "monstor";
    if (!fs::is_directory(folder, error)) {
        say("script damage: skipped, the data has no dun/monstor\n");
        return 0;
    }
    std::map<std::string, ScriptDamage> scripts;
    for (const fs::directory_entry &entry : fs::directory_iterator(folder, error)) {
        if (entry.path().extension() != ".stb") {
            continue;
        }
        std::ifstream             stream(entry.path(), std::ios::binary);
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        if (std::optional<ScriptDamage> damage = ReadScriptDamage(bytes)) {
            scripts[entry.path().stem().string()] = *damage;
        }
    }
    std::string text = FormatScriptDamage(scripts);
    bool        unchanged = false;
    try {
        unchanged = WriteIfChanged(data / kScriptDamageFile, Bytes(text.begin(), text.end()));
    } catch (const std::exception &failure) {
        say(std::format("script damage: {}\n", failure.what()));
        return 1;
    }
    say(std::format("script damage: {} ({} scripts{})\n", kScriptDamageFile, scripts.size(), unchanged ? ", unchanged" : ""));
    return 0;
}

} // namespace expanded
