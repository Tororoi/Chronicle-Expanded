#include "expanded/data/town_model.hpp"

// A slot names only the fields it sets.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"

// Each ally's town model: its base and the source of every slot (expanded/docs/town-models.md).

namespace expanded {

namespace {

constexpr std::string_view kXiaoBase = "gedit/e01/chara/c04pcat.chr";
constexpr std::string_view kXiaoCat = "gedit/s86/chara/c04cat.chr";
constexpr std::string_view kXiaoEventCat = "gedit/e01/chara/e04c04cat.chr";
constexpr std::string_view kXiaoShake = "dun/d01/event/e613c04cat.chr";

constexpr std::string_view kGoroBase = "gedit/s01/chara/c06p.chr";
constexpr std::string_view kGoroDungeon = "dun/mainchara/c06b.chr";
constexpr std::string_view kGoroDescent = "gedit/s01/chara/e101.chr";
constexpr std::string_view kGoroClimb = "gedit/s01/chara/e102.chr";

constexpr std::string_view kRubyBase = "gedit/e03/chara/c05a.chr";
constexpr std::string_view kRubyDungeon = "dun/mainchara/c05a.chr";
constexpr std::string_view kRubyPat = "gedit/e03/chara/e223c05a.chr";
constexpr std::string_view kRubyFloat = "gedit/e03/chara/e228c05a.chr";

constexpr std::string_view kUngagaBase = "gedit/e04/chara/e323_2c10a.chr";
constexpr std::string_view kUngagaDungeon = "dun/mainchara/c10b.chr";

constexpr std::string_view kOsmondBase = "gedit/e05/chara/c18p.chr";
constexpr std::string_view kOsmondDive = "gedit/e05/chara/e403c18a.chr";
constexpr std::string_view kOsmondFlight = "gedit/s13/chara/e402c18a.chr";

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
        {
            .name = "Goro",
            .file = "expanded/town/goro.chr",
            .base = kGoroBase,
            .root_motion = true,
            .slots = {
                {.idx = 0, .lo = 10, .hi = 20, .speed = 0.15, .name = "idle"},
                {.idx = 1, .lo = 60, .hi = 80, .speed = 0.40, .name = "run"},
                {.idx = 2, .lo = 30, .hi = 50, .speed = 0.25, .name = "walk"},
                {.idx = 3, .lo = 10, .hi = 20, .speed = 0.15, .name = "door(idle)"},
                {.idx = 4, .lo = 10, .hi = 20, .speed = 0.15, .name = "door2(idle)"},
                {.idx = 5, .lo = 90, .hi = 100, .speed = 0.30, .name = "item", .src = kGoroDungeon, .win = {515, 525}},
                {.idx = 6, .lo = 105, .hi = 115, .speed = 0.30, .name = "item-loop", .src = kGoroDungeon, .win = {530, 540}},
                {.idx = 7, .lo = 120, .hi = 149, .speed = 0.50, .name = "refuse(no)", .src = kGoroDungeon, .win = {421, 450}},
                // The town's descent cutscene's own fall and landing; its pack also holds Toan.
                {.idx = 8, .lo = 155, .hi = 175, .speed = 0.30, .name = "fall", .src = kGoroDescent, .win = {75, 95},
                 .src_cfg = "e101c06a.cfg"},
                {.idx = 9, .lo = 180, .hi = 190, .speed = 0.25, .name = "land(stand)", .src = kGoroDescent, .win = {95, 105},
                 .src_cfg = "e101c06a.cfg"},
                {.idx = 10, .lo = 10, .hi = 20, .speed = 0.15, .name = "door3(idle)"},
                // The ladder climb: two alternating hops, and the landing reversed.
                {.idx = 11, .lo = 195, .hi = 215, .speed = 0.30, .name = "climb-hopA", .src = kGoroClimb, .win = {90, 110},
                 .src_cfg = "e102c06a.cfg"},
                {.idx = 12, .lo = 220, .hi = 240, .speed = 0.30, .name = "climb-hopB", .src = kGoroClimb, .win = {110, 130},
                 .src_cfg = "e102c06a.cfg"},
                {.idx = 13, .lo = 245, .hi = 255, .speed = 0.25, .name = "rev-land", .src = kGoroDescent, .win = {95, 105},
                 .src_cfg = "e101c06a.cfg", .reverse = true},
            },
        },
        {
            .name = "Ruby",
            .file = "expanded/town/ruby.chr",
            .base = kRubyBase,
            .root_motion = true,
            .shadow_inject = TownShadowInject{.src = kRubyPat, .model = "e223c05s.mds", .mot = "e223c05s.mot",
                                              .bbp = "e223c05s.bbp", .wgt = "e223c05s.wgt"},
            .slots = {
                {.idx = 0, .lo = 10, .hi = 20, .speed = 0.15, .name = "idle", .src = kRubyDungeon, .win = {10, 20}},
                {.idx = 1, .lo = 60, .hi = 80, .speed = 0.55, .name = "run", .src = kRubyDungeon, .win = {60, 80}},
                {.idx = 2, .lo = 30, .hi = 50, .speed = 0.25, .name = "walk", .src = kRubyDungeon, .win = {30, 50}},
                {.idx = 3, .lo = 90, .hi = 102, .speed = 0.15, .name = "door(pat)", .src = kRubyPat, .win = {100, 112},
                 .root_offset = std::array{0.0, 0.0, -3.0}},
                {.idx = 4, .lo = 90, .hi = 102, .speed = 0.15, .name = "door2(pat)"},
                {.idx = 5, .lo = 120, .hi = 130, .speed = 0.30, .name = "item", .src = kRubyDungeon, .win = {575, 585}},
                {.idx = 6, .lo = 135, .hi = 145, .speed = 0.30, .name = "item-loop", .src = kRubyDungeon, .win = {590, 600}},
                {.idx = 7, .lo = 150, .hi = 170, .speed = 0.20, .name = "refuse(no)", .src = kRubyDungeon, .win = {605, 625}},
                {.idx = 8, .lo = 175, .hi = 195, .speed = 0.10, .name = "fall(float)", .src = kRubyFloat, .win = {10, 30}},
                {.idx = 9, .lo = 10, .hi = 20, .speed = 0.15, .name = "land(idle)"},
                {.idx = 10, .lo = 90, .hi = 102, .speed = 0.15, .name = "door3(pat)"},
                // The ladder jump, its travel held in place: the cutscene's leap carries her 43 units.
                {.idx = 11, .lo = 200, .hi = 230, .speed = 0.20, .name = "jump", .src = kRubyFloat, .win = {285, 315},
                 .root_freeze = 200},
                {.idx = 12, .lo = 235, .hi = 245, .speed = 0.10, .name = "jump-loop", .src = kRubyFloat, .win = {320, 330},
                 .root_freeze = 175},
            },
        },
        {
            .name = "Ungaga",
            .file = "expanded/town/ungaga.chr",
            .base = kUngagaBase,
            .root_motion = true,
            .slots = {
                {.idx = 0, .lo = 10, .hi = 20, .speed = 0.10, .name = "idle"},
                {.idx = 1, .lo = 60, .hi = 80, .speed = 0.55, .name = "run", .src = kUngagaDungeon, .win = {60, 80}},
                {.idx = 2, .lo = 30, .hi = 50, .speed = 0.27, .name = "walk"},
                // The talk's hand-to-chest gesture, pulled back off the door.
                {.idx = 3, .lo = 115, .hi = 131, .speed = 0.20, .name = "door(chest)", .root_offset = std::array{0.0, 0.0, -2.5}},
                {.idx = 4, .lo = 115, .hi = 131, .speed = 0.20, .name = "door2"},
                {.idx = 5, .lo = 240, .hi = 250, .speed = 0.30, .name = "item", .src = kUngagaDungeon, .win = {630, 640}},
                {.idx = 6, .lo = 255, .hi = 265, .speed = 0.30, .name = "item-loop", .src = kUngagaDungeon, .win = {643, 653}},
                // The ladder refusal, the staff held in the hand at its idle grip.
                {.idx = 7, .lo = 270, .hi = 310, .speed = 0.22, .name = "refuse(NG)", .src = kUngagaDungeon, .win = {490, 530},
                 .pin_nodes = {"kon_1", "r_handa"}, .pin_frame = 15},
                // A still falling pose: one frame at speed 0.
                {.idx = 8, .lo = 315, .hi = 315, .speed = 0.0, .name = "fall(296)", .src = kUngagaDungeon, .win = {296, 296}},
                {.idx = 9, .lo = 357, .hi = 358, .speed = 0.20, .name = "land(rev)", .src = kUngagaDungeon, .win = {295, 296},
                 .reverse = true},
                {.idx = 10, .lo = 115, .hi = 131, .speed = 0.20, .name = "door3"},
            },
        },
        {
            .name = "Osmond",
            .file = "expanded/town/osmond.chr",
            .base = kOsmondBase,
            // The flight model's propeller, which the base lacks, its hub spun through the flight slots.
            .mesh_graft = TownMeshGraft{.src = kOsmondFlight, .pose_frame = 15,
                                        .nodes = {"bone4", "bone5", "bone6", "tukene1", "obj41", "obj42", "obj43", "obj38",
                                                  "obj39", "obj40", "obj35", "obj36", "obj37"},
                                        .spin = TownSpin{.node = "tukene1", .axis = {0, 1, 0}, .rate = 72.0,
                                                         .handoff_advance = 36.0, .ramp_up = {240, 255}, .loop = {260, 270},
                                                         .ramp_down = {275, 290}}},
            .slots = {
                {.idx = 0, .lo = 10, .hi = 20, .speed = 0.10, .name = "idle"},
                // The walk in a window of its own, sped up.
                {.idx = 1, .lo = 130, .hi = 150, .speed = 0.75, .name = "run(sped)", .src = kOsmondBase, .win = {30, 50}},
                {.idx = 2, .lo = 30, .hi = 50, .speed = 0.20, .name = "walk"},
                {.idx = 3, .lo = 60, .hi = 80, .speed = 0.20, .name = "door(talk)"},
                {.idx = 4, .lo = 60, .hi = 80, .speed = 0.30, .name = "door2(talk)"},
                // The talk's hand-out pose, held.
                {.idx = 5, .lo = 90, .hi = 92, .speed = 0.10, .name = "item(hold69)", .hold = 69},
                {.idx = 6, .lo = 90, .hi = 92, .speed = 0.10, .name = "item-loop"},
                {.idx = 7, .lo = 10, .hi = 20, .speed = 0.10, .name = "damage(skip)"},
                {.idx = 8, .lo = 100, .hi = 108, .speed = 0.30, .name = "fall", .src = kOsmondDive, .win = {210, 218}},
                {.idx = 9, .lo = 110, .hi = 125, .speed = 0.30, .name = "land", .src = kOsmondDive, .win = {220, 235}},
                {.idx = 10, .lo = 60, .hi = 80, .speed = 0.30, .name = "door3(dbl)"},
                // The ladders: down, a dive; up, the propeller unfolds and flies, then folds away.
                {.idx = 11, .lo = 155, .hi = 175, .speed = 0.30, .name = "jump-down", .src = kOsmondDive, .win = {185, 205}},
                {.idx = 12, .lo = 180, .hi = 235, .speed = 0.25, .name = "propeller", .src = kOsmondFlight, .win = {280, 335}},
                {.idx = 13, .lo = 240, .hi = 255, .speed = 0.25, .name = "start-fly", .src = kOsmondFlight, .win = {345, 360}},
                {.idx = 14, .lo = 260, .hi = 270, .speed = 0.20, .name = "fly-loop", .src = kOsmondFlight, .win = {360, 370}},
                {.idx = 15, .lo = 275, .hi = 290, .speed = 0.25, .name = "rev-startfly", .src = kOsmondFlight, .win = {345, 360},
                 .reverse = true},
                {.idx = 16, .lo = 295, .hi = 350, .speed = 0.30, .name = "rev-propeller(stow)", .src = kOsmondFlight,
                 .win = {280, 335}, .reverse = true},
            },
        },
    };
    // clang-format on
    return allies;
}

} // namespace expanded
