#include <algorithm>
#include <bit>
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include "expanded/data/atla_gemron_recipe.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/data/stb.hpp"

// The Gemrons' scripts, Holy Gemron's e115a.stb rewritten.

namespace expanded {

namespace crystal {

namespace {

constexpr float         kCryFrame = 214.0f;
constexpr std::uint32_t kSoundBase = 2501, kCryOffset = 11;
constexpr int           kShatterSound = 312; // the life sphere breaking, in the Ice Queen's arena set
constexpr float         kVanillaRange = 70.0f, kShotRange = 140.0f;
constexpr float         kVanillaWalkIn = 50.0f, kWalkIn = 100.0f;

// A label's function header, as e115a.stb has it.
std::size_t Label(std::span<const unsigned char> script, std::uint32_t id) {
    if (std::optional<std::size_t> header = stb::Label(script, id)) {
        return *header;
    }
    throw std::runtime_error(std::format("e115a.stb has no label {}", id));
}

} // namespace

} // namespace crystal

Bytes BuildCrystalGemronScript(std::span<const unsigned char> script) {
    using namespace crystal;
    std::size_t base = stb::CodeBase(script);
    // The death wait cries at frame 214, which the vanilla clip never reaches: it sounds the shatter as the ball breaks.
    std::size_t death_call = base + Get32(script, Label(script, 120));
    if (Get32(script, death_call) != stb::kCall) {
        throw std::runtime_error("e115a.stb: label 120 does not start with a CALL");
    }
    std::size_t call = base + Get32(script, base + Get32(script, death_call + 8));
    while (Get32(script, call) != stb::kCall) {
        if (Get32(script, call) == stb::kReturn) {
            throw std::runtime_error("e115a.stb: the death function has no CALL");
        }
        call += 12;
    }
    std::size_t push = call - 12, id = push - 24;
    if (Get32(script, push) != stb::kPushConst || Get32(script, push + 4) != stb::kFloat || GetF32(script, push + 8) != kCryFrame ||
        Get32(script, push - 36) != stb::kPushConst || Get32(script, push - 32) != stb::kInt || Get32(script, push - 28) != kSoundBase ||
        Get32(script, push - 12) != stb::kAdd || Get32(script, id) != stb::kPushConst || Get32(script, id + 4) != stb::kInt || Get32(script, id + 8) != kCryOffset) {
        throw std::runtime_error("e115a.stb: the death wait is not the vanilla cry at frame 214");
    }
    Bytes out(script.begin(), script.end());
    SetF32(out, push + 8, float(kCrystalShatterFrame));
    Set32(out, id + 8, static_cast<std::uint32_t>(kShatterSound - int(kSoundBase)));
    // The AI loop (label 100) shoots within 140 and walks closer beyond it, to 100.
    std::size_t loop = base + Get32(script, Label(script, 100));
    int         ranges = 0, walks = 0;
    for (std::size_t c = loop; c + 12 <= script.size() && Get32(script, c) != stb::kReturn; c += 12) {
        if (Get32(script, c) == stb::kPushConst && Get32(script, c + 4) == stb::kFloat && GetF32(script, c + 8) == kVanillaRange) {
            SetF32(out, c + 8, kShotRange);
            ranges++;
        }
        if (c > loop && Get32(script, c) == stb::kCall && Get32(script, c - 12) == stb::kPushConst && Get32(script, c - 8) == stb::kFloat &&
            GetF32(script, c - 4) == kVanillaWalkIn) {
            SetF32(out, c - 4, kWalkIn);
            walks++;
        }
    }
    if (ranges != 3 || walks != 1) {
        throw std::runtime_error(std::format("e115a.stb: {} ranges and {} walks in the AI loop, expected 3 and 1", ranges, walks));
    }
    return out;
}

namespace atla {

namespace {

constexpr float         kVanillaRange = 70.0f, kShotRange = 50.0f;     // label 100 shoots within it, walks closer beyond
constexpr std::uint32_t kVanillaCooldownFrom = 80, kCooldownFrom = 40; // the shot's cooldown: _GET_RAND(70) + this
constexpr float         kCloseRange = 20.0f;                           // under it: back off or shoot
constexpr std::uint32_t kVanillaBackOff = 40, kBackOffPercent = 60;    // _GET_RAND(100) under it backs off
constexpr float         kAimAhead = 100.0f;                            // the shot's aim, this far along the facing
constexpr float         kAimUp = 7.4f;
// The shot function's locals: the distance, the aim and the facing.
constexpr std::uint32_t kAimDistance = 8, kAimX = 2, kAimY = 3, kAimZ = 4, kAimDirX = 8, kAimDirY = 9, kAimDirZ = 10;
constexpr const char   *kSpawnFrom = "dcol0";

std::uint32_t Bits(float f) {
    return std::bit_cast<std::uint32_t>(f);
}

void Cell(Bytes &b, std::size_t at, std::uint32_t op, std::uint32_t a1, std::uint32_t a2) {
    Set32(b, at, op);
    Set32(b, at + 4, a1);
    Set32(b, at + 8, a2);
}

// The shot's aim, appended past the script's end: v2..v4 its own place (_GET_POSITION(-1)), v8..v10 its facing
// (_GET_DIRECTION(0)), v2 += facing.x x the reach, v4 += facing.z x the reach, v3 += 7.4, then back to the _SET_SHOT.
Bytes AimCells(std::uint32_t set_shot) {
    std::vector<std::array<std::uint32_t, 3>> cells = {
        {3,                 1,        11              },
        {3,                 1,        1               },
        {11,                0,        0               },
        {stb::kLoadAddress, kAimX,    stb::kFloatLocal},
        {stb::kLoadAddress, kAimY,    stb::kFloatLocal},
        {stb::kLoadAddress, kAimZ,    stb::kFloatLocal},
        {stb::kExt,         5,        0               },
        {3,                 1,        31              },
        {3,                 2,        0               },
        {stb::kLoadAddress, kAimDirX, stb::kFloatLocal},
        {stb::kLoadAddress, kAimDirY, stb::kFloatLocal},
        {stb::kLoadAddress, kAimDirZ, stb::kFloatLocal},
        {stb::kExt,         5,        0               },
    };
    for (auto [v, d] : {
             std::pair{kAimX, kAimDirX},
             std::pair{kAimZ, kAimDirZ}
    }) {
        cells.insert(cells.end(), {
                                      {2, v, stb::kFloatLocal},
                                      {1, v, stb::kFloatLocal},
                                      {3, 2, Bits(kAimAhead) },
                                      {1, d, stb::kFloatLocal},
                                      {8, 0, 0               },
                                      {6, 0, 0               },
                                      {5, 0, 0               },
                                      {4, 0, 0               }
        });
    }
    cells.insert(cells.end(), {
                                  {2, kAimY, stb::kFloatLocal},
                                  {1, kAimY, stb::kFloatLocal},
                                  {3, 2,     Bits(kAimUp)    },
                                  {6, 0,     0               },
                                  {5, 0,     0               },
                                  {4, 0,     0               }
    });
    cells.push_back({stb::kJump, set_shot, 0});
    Bytes b(cells.size() * 12);
    for (std::size_t i = 0; i < cells.size(); i++) {
        Cell(b, i * 12, cells[i][0], cells[i][1], cells[i][2]);
    }
    return b;
}

} // namespace

} // namespace atla

// Holy Gemron's script for a short-range wide shot: its one _SET_SHOT fires from the spawn bone, along its facing (the
// fire frame's hold-fire check becomes a jump to aim cells appended past the end, which jump back); its AI loop shoots
// within 50 (walking closer beyond), every 40-109 frames, and close in backs off 60 % of the time, else shoots (the bite
// before).
Bytes BuildAtlaGemronScript(std::span<const unsigned char> script) {
    using namespace atla;
    auto                       u = [&](std::size_t o) { return Get32(script, o); };
    auto                       f = [&](std::size_t o) { return GetF32(script, o); };
    auto                       op = [&](std::size_t c, std::uint32_t v) { return u(c) == v; };
    auto                       push_i = [&](std::size_t c, std::uint32_t v) { return u(c) == stb::kPushConst && u(c + 4) == stb::kInt && u(c + 8) == v; };
    auto                       push_f = [&](std::size_t c) { return u(c) == stb::kPushConst && u(c + 4) == stb::kFloat; };
    auto                       bad = [](const std::string &what) { return std::runtime_error("e115a.stb: " + what); };
    std::size_t                cb = stb::CodeBase(script);
    std::optional<std::size_t> label100 = stb::Label(script, 100);
    if (!label100) {
        throw bad("no label 100");
    }
    Bytes out(script.begin(), script.end());
    // The shot's bone: push 133, then the bone's string.
    std::vector<std::size_t> sites;
    for (std::size_t c = cb; c + 24 <= script.size(); c += 4) {
        if (push_i(c, 133) && u(c + 12) == 3 && u(c + 16) == 3) {
            sites.push_back(c);
        }
    }
    if (sites.size() != 1) {
        throw bad(std::format("{} _SET_SHOT bone strings, expected 1", sites.size()));
    }
    std::size_t bone_at = cb + u(sites[0] + 20);
    if (NameAt(script, bone_at, 8) != kSpawnFrom) {
        throw bad(std::format("the shot fires from '{}', not {}", NameAt(script, bone_at, 8), kSpawnFrom));
    }
    std::copy_n(kAtlaShotSpawn, std::strlen(kAtlaShotSpawn), out.begin() + std::ptrdiff_t(bone_at));
    // The AI loop, up to its RET.
    std::size_t start = cb + u(*label100), end = start;
    while (end + 12 <= script.size() && u(end) != stb::kReturn) {
        end += 12;
    }
    std::vector<std::size_t> ranges;
    long                     cooldown = -1, close = -1, back_off = -1, shot_call = -1;
    for (std::size_t c = start; c < end; c += 12) {
        if (push_f(c) && f(c + 8) == kVanillaRange && op(c + 12, stb::kLess)) {
            ranges.push_back(c);
        }
        if (push_i(c, 180) && push_i(c + 12, 70)) { // _GET_RAND(70, ...) then + 80
            for (std::size_t k = c + 24; k < c + 108 && cooldown < 0; k += 12) {
                if (push_i(k, kVanillaCooldownFrom)) {
                    cooldown = long(k);
                }
            }
        }
        if (push_f(c) && f(c + 8) == kCloseRange && op(c + 12, stb::kLess) && u(c + 16) == 42 && close < 0) {
            close = long(c);
        }
        if (close >= 0 && back_off < 0 && long(c) > close && push_i(c, 180) && push_i(c + 12, 100)) { // _GET_RAND(100, ...) < 40
            for (std::size_t k = c + 24; k < c + 108 && back_off < 0; k += 12) {
                if (push_i(k, kVanillaBackOff)) {
                    back_off = long(k);
                }
            }
        }
        if (shot_call < 0 && c >= 24 && op(c, stb::kCall) && push_f(c - 24) && push_f(c - 12) && op(c + 12, stb::kPop)) {
            shot_call = long(c);
        }
    }
    if (ranges.size() != 3 || cooldown < 0 || close < 0 || back_off < 0 || shot_call < 0) {
        throw bad(std::format("label 100 not as expected ({} ranges, cooldown {:X}, close range {:X}, back-off {:X}, shot call {:X})",
                              ranges.size(), cooldown, close, back_off, shot_call));
    }
    // The close branch: cmp, BR_FALSE to other, CALL back-off, drop, JMP to out; other: 4 cells; out: the closing marker.
    std::size_t bo = std::size_t(back_off), br = bo + 24, other = bo + 72, jmp = bo + 60, outer = other + 48;
    if (!op(br, stb::kJumpFalse) || u(br + 4) != other - cb || !op(bo + 36, stb::kCall) || !op(jmp, stb::kJump) || u(jmp + 4) != other + 36 - cb ||
        !op(outer, stb::kMarker) || !op(other, stb::kMarker) || !op(other + 12, stb::kCall) || !op(other + 24, stb::kPop) || !op(other + 36, stb::kMarker)) {
        throw bad(std::format("label 100's close branch at {:X} not as expected", close));
    }
    // The shot function's fire frame: _GET_DISTANCE(&v8), v8 < 20 holds fire, else v3 += 7.4, then the _SET_SHOT.
    std::size_t sc = std::size_t(shot_call);
    std::size_t shot_func = cb + u(sc + 8), shot_code = cb + u(shot_func);
    long        hold = -1;
    for (std::size_t c = shot_code; c < sites[0]; c += 12) {
        if (push_i(c, 10) && u(c + 12) == stb::kLoadAddress && u(c + 16) == kAimDistance && op(c + 24, stb::kExt) && push_f(c + 48) &&
            f(c + 56) == 20.0f && op(c + 60, stb::kLess)) {
            hold = long(c);
        }
    }
    if (hold < 0 || long(sites[0]) - hold != 12 * 15) {
        throw bad("the shot function's hold-fire check not as expected");
    }
    for (std::size_t c : ranges) {
        Set32(out, c + 8, Bits(kShotRange));
    }
    Set32(out, std::size_t(cooldown) + 8, kCooldownFrom);
    Set32(out, bo + 8, kBackOffPercent);
    std::copy_n(script.begin() + std::ptrdiff_t(sc - 24), 36, out.begin() + std::ptrdiff_t(other));      // push 0.3, push 0.0, CALL the shot
    std::copy_n(script.begin() + std::ptrdiff_t(sc + 12), 12, out.begin() + std::ptrdiff_t(other + 36)); // its drop
    Set32(out, jmp + 4, std::uint32_t(outer - cb));
    PadTo4(out);
    std::size_t aim = out.size();
    Bytes       tail = AimCells(std::uint32_t(sites[0] - cb));
    out.insert(out.end(), tail.begin(), tail.end());
    Cell(out, std::size_t(hold), stb::kJump, std::uint32_t(aim - cb), 0); // to the aim
    return out;
}

} // namespace expanded
