#include "expanded/town_model.hpp"

// A slot names only the fields it sets.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"

// Each ally's town model: its base and the source of every slot (expanded/docs/town-models.md).

namespace expanded {

namespace {

constexpr std::string_view kXiaoBase = "gedit/e01/chara/c04pcat.chr";
constexpr std::string_view kXiaoCat = "gedit/s86/chara/c04cat.chr";
constexpr std::string_view kXiaoEventCat = "gedit/e01/chara/e04c04cat.chr";
constexpr std::string_view kXiaoShake = "dun/d01/event/e613c04cat.chr";

} // namespace

const std::vector<TownAlly> &TownAllies() {
    // clang-format off
    static const std::vector<TownAlly> allies = {
        {
            .name = "Xiao",
            .file = "expanded/town/xiao.chr",
            .base = kXiaoBase,
            .slots = {
                {.idx = 0, .lo = 10, .hi = 20, .speed = 0.10, .name = "idle"},
                {.idx = 1, .lo = 120, .hi = 136, .speed = 0.50, .name = "run", .src = kXiaoCat, .win = {120, 136}},
                {.idx = 2, .lo = 60, .hi = 80, .speed = 0.30, .name = "walk"},
                // The event cat's upright opening, pulled back off the door: the cat is longer than Toan.
                {.idx = 3, .lo = 150, .hi = 159, .speed = 0.20, .name = "door", .src = kXiaoEventCat, .win = {160, 169},
                 .root_offset = std::array{0.0, 0.0, -2.0}},
                {.idx = 4, .lo = 150, .hi = 159, .speed = 0.20, .name = "door2"},
                // A whole looping sit, for both item slots.
                {.idx = 5, .lo = 30, .hi = 40, .speed = 0.10, .name = "sit", .src = kXiaoCat, .win = {30, 40}},
                {.idx = 6, .lo = 30, .hi = 40, .speed = 0.10, .name = "sit-loop"},
                // Damage, which the town never plays: the ladder refusal's head shake.
                {.idx = 7, .lo = 228, .hi = 278, .speed = 0.30, .name = "refuse(shake)", .src = kXiaoShake, .win = {115, 165}},
                {.idx = 8, .lo = 205, .hi = 214, .speed = 0.50, .name = "fall(leap)", .src = kXiaoCat, .win = {205, 214}},
                {.idx = 9, .lo = 215, .hi = 227, .speed = 0.36, .name = "land", .src = kXiaoCat, .win = {215, 227}},
                {.idx = 10, .lo = 150, .hi = 159, .speed = 0.20, .name = "door3(dbl)"},
                // The ladder jump: the ready crouch, the hop up, and the walk reversed.
                {.idx = 11, .lo = 95, .hi = 105, .speed = 0.40, .name = "jump-ready", .src = kXiaoCat, .win = {95, 105}},
                {.idx = 12, .lo = 285, .hi = 294, .speed = 0.60, .name = "float-up", .src = kXiaoEventCat, .win = {160, 169}},
                {.idx = 13, .lo = 300, .hi = 320, .speed = 0.30, .name = "walk-back", .src = kXiaoBase, .win = {60, 80},
                 .reverse = true},
            },
        },
    };
    // clang-format on
    return allies;
}

} // namespace expanded
