#include <gtest/gtest.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/fishing_collision.hpp"
#include "expanded/data/fishing_scripts.hpp"
#include "expanded/data/fishing_spots.hpp"
#include "expanded/data/mes.hpp"
#include "expanded/data/stb.hpp"
#include "expanded/data/town_scene.hpp"
#include "expanded/data/yellow_drops_bank.hpp"
#include "test_support.hpp"

using expanded::Bytes;
using expanded::Get16;
using expanded::Get32;
using expanded::GetF32;
using expanded::Set16;
using expanded::Set32;
namespace fs = std::filesystem;
namespace mes = expanded::mes;
namespace scene = expanded::scene;
namespace stb = expanded::stb;

namespace {

struct Cell {
    std::uint32_t op, a1, a2;
};

// A laid function's cells, from its header's entry to the first word that is no opcode.
std::vector<Cell> Cells(const Bytes &script, std::size_t function) {
    std::size_t       base = stb::CodeBase(script);
    std::vector<Cell> cells;
    for (std::size_t at = base + Get32(script, function); at + stb::kCell <= script.size(); at += stb::kCell) {
        std::uint32_t op = Get32(script, at);
        if (op == 0 || op > 30) {
            break;
        }
        cells.push_back({op, Get32(script, at + 4), Get32(script, at + 8)});
    }
    return cells;
}

// A command a function calls: its number, the cells pushing its arguments, and where the call starts.
struct Call {
    std::uint32_t     command;
    std::vector<Cell> arguments;
    std::size_t       index;
};

std::vector<Call> Calls(const std::vector<Cell> &cells) {
    std::vector<Call> calls;
    for (std::size_t i = 0; i < cells.size(); i++) {
        if (cells[i].op != stb::kExt || cells[i].a1 == 0 || cells[i].a1 > i) {
            continue;
        }
        std::size_t first = i - cells[i].a1;
        if (cells[first].op == stb::kPushConst && cells[first].a1 == stb::kInt) {
            calls.push_back({
                cells[first].a2, {cells.begin() + static_cast<std::ptrdiff_t>(first + 1), cells.begin() + static_cast<std::ptrdiff_t>(i)},
                 first
            });
        }
    }
    return calls;
}

const Call *Find(const std::vector<Call> &calls, std::uint32_t command) {
    for (const Call &call : calls) {
        if (call.command == command) {
            return &call;
        }
    }
    return nullptr;
}

float Float(const Cell &cell) {
    EXPECT_EQ(cell.op, stb::kPushConst);
    EXPECT_EQ(cell.a1, stb::kFloat);
    return std::bit_cast<float>(cell.a2);
}

int Int(const Cell &cell) {
    EXPECT_EQ(cell.op, stb::kPushConst);
    EXPECT_EQ(cell.a1, stb::kInt);
    return static_cast<int>(cell.a2);
}

// A script of code base 0x20 with label 1, its function at 0x20 returning, and its label table after it.
Bytes SmallScript() {
    Bytes script(0x20, 0);
    Set32(script, stb::kCodeBaseAt, 0x20);
    Bytes function(stb::kFunctionHeader, 0);
    Set32(function, 0, stb::kFunctionHeader);
    script.insert(script.end(), function.begin(), function.end());
    stb::AppendCell(script, stb::kPushConst, stb::kInt, 0);
    stb::AppendCell(script, stb::kReturn);
    expanded::PadTo16(script);
    Set32(script, stb::kLabelTableAt, static_cast<std::uint32_t>(script.size()));
    Set32(script, stb::kLabelCountAt, 1);
    expanded::Append32(script, 1);
    expanded::Append32(script, 0x20);
    return script;
}

// A message file of the messages, then zero padding.
Bytes Messages(const std::vector<std::pair<int, mes::Words>> &messages) {
    std::size_t count = messages.size();
    Bytes       out(4 + count * 4, 0);
    Set16(out, 0, static_cast<std::uint16_t>(count));
    std::size_t words = 0;
    for (std::size_t i = 0; i < count; i++) {
        Set16(out, 4 + i * 4, static_cast<std::uint16_t>(messages[i].first));
        Set16(out, 4 + i * 4 + 2, static_cast<std::uint16_t>(count + 1 + words)); // text at byte 2 * (count + offset + 1)
        for (std::uint16_t word : messages[i].second) {
            out.push_back(static_cast<unsigned char>(word));
            out.push_back(static_cast<unsigned char>(word >> 8));
        }
        words += messages[i].second.size();
    }
    out.resize(out.size() + 200, 0);
    return out;
}

Bytes Read(const fs::path &file) {
    std::ifstream stream(file, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

const expanded::FishingSpot &Queens() {
    const expanded::FishingSpot *spot = expanded::FishingSpotOfMap(2);
    EXPECT_NE(spot, nullptr);
    return *spot;
}

// The spots' builds over a copy of the disc at data, in scratch: the failed builds.
int BuildSpots(const fs::path &data, const fs::path &scratch) {
    std::error_code error;
    for (const fs::directory_entry &entry : fs::directory_iterator(data)) {
        if (entry.path().filename() != "expanded") {
            fs::create_symlink(fs::absolute(entry.path()), scratch / entry.path().filename(), error);
        }
    }
    return expanded::RunDataBuilds(scratch, "fishing spot", expanded::FishingSpotBuilds(), nullptr);
}

} // namespace

TEST(ScriptWriter, LaysItsHeaderJumpsAndStrings) {
    stb::Writer w;
    w.UseLocals(3);
    int loop = w.Mark();
    w.PushString("chara/c01d.chr");
    w.JumpFalse(loop);
    w.PushInt(0);
    w.Return();
    constexpr std::size_t kAt = 0x100, kBase = 0x20, kCode = kAt + stb::kFunctionHeader;
    Bytes                 laid = w.Lay(kAt, kBase);
    EXPECT_EQ(Get32(laid, 0), kCode - kBase);                                         // the entry, from the code base
    EXPECT_EQ(Get32(laid, 8), 3u);                                                    // the locals
    EXPECT_EQ(Get32(laid, stb::kFunctionHeader + 8), kCode + 4 * stb::kCell - kBase); // the string, after the cells
    EXPECT_EQ(Get32(laid, stb::kFunctionHeader + stb::kCell + 4), kCode - kBase);     // the jump, to the first cell
    EXPECT_EQ(expanded::NameAt(laid, stb::kFunctionHeader + 4 * stb::kCell, 32), "chara/c01d.chr");
}

TEST(ScriptWriter, AppendsFunctionsUnderLabelsOfTheirOwn) {
    Bytes         script = SmallScript();
    stb::Appender appender(script);
    std::size_t   next = appender.Next();
    stb::Writer   w;
    w.PushInt(0);
    w.Return();
    appender.Add(400, w);
    Bytes out = appender.Build();

    EXPECT_EQ(Get32(out, stb::kLabelCountAt), 2u);
    EXPECT_GT(Get32(out, stb::kLabelTableAt), next); // the table moved after the functions
    ASSERT_TRUE(stb::Label(out, 1).has_value());
    EXPECT_EQ(*stb::Label(out, 1), 0x20u);
    ASSERT_TRUE(stb::Label(out, 400).has_value());
    EXPECT_EQ(*stb::Label(out, 400), next + stb::CodeBase(out));
    std::vector<Cell> cells = Cells(out, *stb::Label(out, 400));
    ASSERT_GE(cells.size(), 2u);
    EXPECT_EQ(cells[0].op, stb::kPushConst);
    EXPECT_EQ(cells[1].op, stb::kReturn);
}

TEST(Messages, AppendedMessagesLeaveTheOthersAsTheyRead) {
    Bytes mes_file = Messages({
        {5,  {0x10, 0x11, mes::kEnd}},
        {40, {0x20, mes::kEnd}      }
    });
    Bytes out = mes::Append(mes_file, {
                                          {20, {0x30, 0x31, 0x32, mes::kEnd}}
    });
    EXPECT_EQ(Get16(out, 0), 3);
    EXPECT_EQ(mes::Extract(out, 5), (mes::Words{0x10, 0x11, mes::kEnd}));
    EXPECT_EQ(mes::Extract(out, 40), (mes::Words{0x20, mes::kEnd}));
    EXPECT_EQ(mes::Extract(out, 20), (mes::Words{0x30, 0x31, 0x32, mes::kEnd}));
    // In id order: 5, 20, 40.
    EXPECT_EQ(Get16(out, 4), 5);
    EXPECT_EQ(Get16(out, 8), 20);
    EXPECT_EQ(Get16(out, 12), 40);
    EXPECT_THROW(mes::Append(out, {
                                      {20, {mes::kEnd}}
    }),
                 std::runtime_error);
}

TEST(TownScene, AScriptEventIsAnAlwaysOnLabelPoint) {
    Bytes event = scene::ScriptEvent({1.0f, 2.0f, 3.0f}, 10.0f, 400);
    ASSERT_EQ(event.size(), 0xC0u);
    EXPECT_EQ(Get32(event, 0x10), 0x12u); // a script event
    EXPECT_EQ(GetF32(event, 0x18), 0.0f); // hours 0 to 24
    EXPECT_EQ(GetF32(event, 0x1C), 24.0f);
    EXPECT_EQ(GetF32(event, 0x44), 2.0f);
    EXPECT_EQ(GetF32(event, 0x60), 10.0f);
    EXPECT_EQ(GetF32(event, 0x70), 400.0f); // the label
}

TEST(TownScene, AnAppendedPartTakesTheDirectorysRoom) {
    Bytes scene_file(0x400, 0);
    Set32(scene_file, 4, 1);
    scene_file[0x10] = 'a';
    Set32(scene_file, 0x20, 0x200); // part a at 0x200
    Set32(scene_file, 0x24, 0x160);
    Set32(scene_file, 0x200 + 4, 0xE0); // its definition
    Bytes header(scene_file.begin() + 0x200, scene_file.begin() + 0x360);
    Bytes model(0x80 + 0x40, 0);
    Set32(model, 0, 0x0053444D);
    Bytes events = scene::ScriptEvent({0, 0, 6}, 10, 400);
    Bytes collision = scene::BoxCollision("sign_a", {-1, 1, 0, 2, -1, 1});
    Bytes out = scene::AppendPart(scene_file, "sign", header, model, events, collision);

    std::vector<scene::SubFile> files = scene::SubFiles(out);
    ASSERT_EQ(files.size(), 2u);
    EXPECT_EQ(files[1].name, "sign");
    EXPECT_EQ(files[1].offset, 0x400u);
    std::size_t part = files[1].offset;
    EXPECT_EQ(expanded::NameAt(out, part + 8, 16), "sign_0.mds");
    EXPECT_EQ(Get32(out, part + 0x48), 0x160u + 0xC0u); // the model after the events
    EXPECT_EQ(Get32(out, part + 0x58), model.size());
    EXPECT_EQ(Get32(out, part + 0xE0 + 0x70), 0x80u); // the events, after the definition's header
    EXPECT_EQ(Get32(out, part + 0xE0 + 0x74), 1u);
    EXPECT_EQ(Get32(out, part + 0x7C), collision.size());
    EXPECT_EQ(Get32(out, part + 0x78) % 16, 0u);
    EXPECT_EQ(Get32(out, part + 0x160 + 0x10), 0x12u);
}

TEST(TownScene, AGroundPlacementGoesAfterItsAnchor) {
    std::string cfg = "\tGROUND\t\"g04\",\t\t//ground\r\n\t\t\"\",\r\n\t\t0,\t0,\t0,\t//position\r\n\t\t0,\t0,\t0\t//rotation\r\n"
                      "\tGROUND\t\"g05\",\r\n";
    std::string out = expanded::ToText(scene::PlaceGround(expanded::ToBytes(cfg), "kanban", "g04", "kanban_a.mds", {250, 70, -64}, 180));
    std::size_t sign = out.find("GROUND\t\"kanban\"");
    ASSERT_NE(sign, std::string::npos);
    EXPECT_LT(out.find("//rotation"), sign);
    EXPECT_LT(sign, out.find("\"g05\""));
    EXPECT_NE(out.find("\t\t250,\t70,\t-64,\t//position\r\n\t\t0,\t180,\t0\t//rotation"), std::string::npos);
    EXPECT_NE(out.find("\"kanban_a.mds\""), std::string::npos);
}

TEST(FishingSpot, QueensIsFishedFromTheCanalsNorthBank) {
    const expanded::FishingSpot &spot = Queens();
    EXPECT_EQ(spot.code, "e03");
    EXPECT_EQ(spot.area, 6);
    EXPECT_EQ(expanded::FishingSpotOfArea(6), &spot);
    EXPECT_EQ(expanded::FishingSpotOfArea(2), nullptr); // the game's own spots stay the game's
    EXPECT_EQ(spot.fish, std::vector<int>{expanded::fish::kBobo});
    EXPECT_EQ(spot.rare_one_in, 0);
    // The trigger in front of the sign is where the player stands to fish, over the canal's water.
    EXPECT_EQ(spot.sign[0] + spot.trigger[0], spot.stance[0]);
    EXPECT_EQ(spot.sign[2] - spot.trigger[2], spot.stance[2]); // the sign faces north (yaw 180)
    EXPECT_GT(spot.stance[1], spot.water);
    EXPECT_TRUE(spot.x1 < spot.stance[0] && spot.stance[0] < spot.x2 && spot.z1 < spot.stance[2] && spot.stance[2] < spot.z2);
}

TEST(FishingSpot, TheEntryScriptLoadsTheSpotBehindBlack) {
    const expanded::FishingSpot &spot = Queens();
    Bytes                        script = expanded::AddFishingScripts(SmallScript(), spot);
    ASSERT_TRUE(stb::Label(script, expanded::kFishingSignLabel).has_value());
    std::vector<Call> calls = Calls(Cells(script, *stb::Label(script, expanded::kFishingSignLabel)));

    const Call *load = Find(calls, 998);
    ASSERT_NE(load, nullptr);
    ASSERT_EQ(load->arguments.size(), 7u);
    EXPECT_EQ(Int(load->arguments[0]), spot.area);
    EXPECT_EQ(Float(load->arguments[1]), spot.x1);
    EXPECT_EQ(Float(load->arguments[2]), spot.z1);
    EXPECT_EQ(Float(load->arguments[3]), spot.x2);
    EXPECT_EQ(Float(load->arguments[4]), spot.z2);
    EXPECT_EQ(Float(load->arguments[5]), spot.water);
    EXPECT_EQ(Float(load->arguments[6]), spot.ground);
    const Call *place = Find(calls, 137);
    ASSERT_NE(place, nullptr);
    EXPECT_EQ(Float(place->arguments[1]), spot.stance[0]);
    EXPECT_EQ(Float(place->arguments[3]), spot.stance[2]);
    const Call *turn = Find(calls, 138);
    ASSERT_NE(turn, nullptr);
    EXPECT_EQ(Float(turn->arguments[2]), spot.facing);

    // The fade, then the villagers dropped (_INITIALIZE before _CLEAR_VILLAGER_BUFF), then the fishing Toan and the spot.
    const Call *fade = Find(calls, 501), *initialize = Find(calls, 8), *clear = Find(calls, 38), *model = Find(calls, 999);
    ASSERT_TRUE(fade && initialize && clear && model);
    EXPECT_LT(fade->index, initialize->index);
    EXPECT_LT(initialize->index, clear->index);
    EXPECT_LT(clear->index, model->index);
    EXPECT_LT(model->index, load->index);
    // The menu first, after the "!" and Cross.
    EXPECT_NE(Find(calls, 10), nullptr);
}

TEST(FishingSpot, TheQuitAndBaitScriptsAreTheLabelsFishingAsksFor) {
    Bytes script = expanded::AddFishingScripts(SmallScript(), Queens());
    for (std::uint32_t label : {expanded::kFishingMenuLabel, expanded::kFishingQuitLabel, expanded::kFishingBaitLabel}) {
        ASSERT_TRUE(stb::Label(script, label).has_value()) << label;
    }
    std::vector<Call> quit = Calls(Cells(script, *stb::Label(script, expanded::kFishingQuitLabel)));
    EXPECT_NE(Find(quit, 995), nullptr); // _EXIT_FISHING
    EXPECT_NE(Find(quit, 57), nullptr);  // _LOAD_VILLAGER
    const Call *keep = Find(quit, 3);    // Continue: _SET_RETURN_CODE(11)
    ASSERT_NE(keep, nullptr);
    EXPECT_EQ(Int(keep->arguments[0]), 11);
    std::vector<Call> bait = Calls(Cells(script, *stb::Label(script, expanded::kFishingBaitLabel)));
    EXPECT_NE(Find(bait, 25), nullptr);  // _GOTO_CHANGE_ESA
    EXPECT_NE(Find(bait, 997), nullptr); // back to fishing
    EXPECT_THROW(expanded::AddFishingScripts(script, Queens()), std::runtime_error);
}

TEST(FishingSpot, ACollisionFileHoldsItsTriangles) {
    std::vector<scene::Triangle> triangles = {
        {{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}}};
    Bytes file = expanded::FishingCollisionFile(2, triangles);
    EXPECT_EQ(expanded::ToText(std::span<const unsigned char>(file).first(4)), "DCFC");
    EXPECT_EQ(Get32(file, 8), 2u);
    EXPECT_EQ(expanded::ReadFishingCollision(file), triangles);
}

// DC_TEST_DATA, an extracted data directory: Queens built with the sign, its trigger, its scripts and its text.
TEST(FishingSpot, QueensIsBuiltFromTheDisc) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    expanded::test::ScratchDir folder("fishing_spot");
    const fs::path            &scratch = folder.path();
    ASSERT_EQ(BuildSpots(data, scratch), 0);
    const fs::path rebuilt = scratch / expanded::kRebuiltDiscFolder / "gedit/e03";

    Bytes                       scene_file = Read(rebuilt / "scene.scn");
    std::vector<scene::SubFile> files = scene::SubFiles(scene_file);
    ASSERT_EQ(files.back().name, "kanban");
    std::size_t part = files.back().offset, definition = part + Get32(scene_file, part + 4);
    ASSERT_EQ(Get32(scene_file, definition + 0x74), 1u);
    std::size_t event = definition + Get32(scene_file, definition + 0x70);
    EXPECT_EQ(Get32(scene_file, event + 0x10), 0x12u);
    EXPECT_EQ(GetF32(scene_file, event + 0x48), 6.0f);
    EXPECT_EQ(GetF32(scene_file, event + 0x60), expanded::kFishingSignRadius);
    EXPECT_EQ(GetF32(scene_file, event + 0x70), static_cast<float>(expanded::kFishingSignLabel));
    EXPECT_GT(Get32(scene_file, part + 0x7C), 0u); // solid

    std::string cfg = expanded::ToText(Read(rebuilt / "mapinfo.cfg"));
    EXPECT_NE(cfg.find("GROUND\t\"kanban\""), std::string::npos);
    EXPECT_NE(cfg.find("\t\t250,\t70,\t-64,\t//position\r\n\t\t0,\t180,\t0\t//rotation"), std::string::npos);

    Bytes script = Read(rebuilt / "event.stb");
    for (std::uint32_t label : {expanded::kFishingMenuLabel, static_cast<std::uint32_t>(expanded::kFishingSignLabel), expanded::kFishingQuitLabel,
                                expanded::kFishingBaitLabel}) {
        EXPECT_TRUE(stb::Label(script, label).has_value()) << label;
    }

    // Norune's menus and catch text, in each language both towns have.
    for (int language = 1; language <= 6; language++) {
        fs::path events = rebuilt / std::format("e03_{}.mes", language), talk = rebuilt / std::format("e03talk_{}.mes", language);
        if (!fs::exists(events)) {
            continue;
        }
        Bytes norune_events = Read(fs::path(data) / std::format("gedit/e01/e01_{}.mes", language));
        Bytes norune_talk = Read(fs::path(data) / std::format("gedit/e01/e01talk_{}.mes", language));
        for (int id : {expanded::kFishingEntryMessage, expanded::kFishingNoRodMessage, expanded::kFishingQuitMessage}) {
            EXPECT_EQ(mes::Extract(Read(events), id), mes::Extract(norune_events, id)) << language << " " << id;
        }
        EXPECT_EQ(mes::Extract(Read(talk), expanded::kFishingCatchMessage), mes::Extract(norune_talk, expanded::kFishingCatchMessage)) << language;
    }

    // The bridges' and pipes' triangles and the authored walls.
    std::vector<scene::Triangle> collision = expanded::ReadFishingCollision(Read(scratch / expanded::FishingCollisionPath(Queens())));
    EXPECT_EQ(collision.size(), 713u);
    EXPECT_EQ(Get32(Read(scratch / expanded::kFishingSignPictures), 4), 1u); // the sign's one picture
}

TEST(TownScene, AReplacedPartIsAppendedAndItsEntryMoved) {
    Bytes scene_file(0x48, 0);
    Set32(scene_file, 4, 1);
    scene_file[0x10] = 'a';
    Set32(scene_file, 0x20, 0x40); // part a at 0x40, 8 bytes
    Set32(scene_file, 0x24, 8);
    const Bytes part = {1, 2, 3};
    Bytes       out = scene::ReplaceSub(scene_file, "a", part);
    ASSERT_EQ(out.size(), 0x50u + 3u);
    EXPECT_EQ(Get32(out, 0x20), 0x50u); // after the old bytes, 16-aligned
    EXPECT_EQ(Get32(out, 0x24), 3u);
    EXPECT_EQ(out[0x52], 3);
    EXPECT_THROW(scene::ReplaceSub(scene_file, "b", part), std::runtime_error);
}

TEST(FishingSpot, YellowDropsIsFishedFromItsWestBank) {
    const expanded::FishingSpot *spot = expanded::FishingSpotOfMap(23);
    ASSERT_NE(spot, nullptr);
    EXPECT_EQ(spot->code, "s13");
    EXPECT_EQ(expanded::FishingSpotOfArea(7), spot);
    EXPECT_EQ(spot->rare_one_in, 0);
    EXPECT_EQ(spot->water, expanded::kYellowDropsWater + 1.0f); // a little over the raised water, as the game's spots
    // The trigger is the sign's foot; the player stands just west of it, over the water, inside the rectangle.
    EXPECT_EQ(spot->trigger, (std::array<float, 3>{0, 0, 0}));
    EXPECT_LT(std::abs(spot->stance[0] - static_cast<float>(spot->sign[0])), expanded::kFishingSignRadius);
    EXPECT_GT(spot->stance[1], spot->water);
    EXPECT_TRUE(spot->x1 < spot->stance[0] && spot->stance[0] < spot->x2 && spot->z1 < spot->stance[2] && spot->stance[2] < spot->z2);
}

TEST(FishingSpot, YellowDropsWaterRisesToTheBanksNotch) {
    const std::string cfg = "WATER_SURFACE\r\n\t\t\t-320, 0, -320,\r\n\t\t\t320, 0, 320,\r\n";
    std::string       raised = expanded::ToText(expanded::YellowDropsWater(expanded::ToBytes(cfg)));
    EXPECT_EQ(raised, "WATER_SURFACE\r\n\t\t\t-320, 4.25, -320,\r\n\t\t\t320, 4.25, 320,\r\n");
    EXPECT_THROW(expanded::YellowDropsWater(expanded::ToBytes("WATER_SURFACE\r\n")), std::runtime_error);
}

TEST(FishingSpot, YellowDropsBankWallsFollowTheBulge) {
    std::vector<scene::Triangle> walls = expanded::YellowDropsBankWalls();
    std::vector<scene::Triangle> pillars = expanded::YellowDropsPillarBases();
    ASSERT_EQ(walls.size(), 18u + pillars.size());               // nine stretches of bank, two triangles each
    EXPECT_EQ(walls[0][0], (scene::Vec3{-485.0, 16.0, -197.0})); // from the shore's corner, where the section starts
    EXPECT_EQ(walls[0][2][1], -24.0);                            // down to under the fish
    // The station at z 102 moves west by a sine over the section (z -145 to 274), 70 at its middle.
    EXPECT_EQ(walls[10][0][2], 102.0);
    EXPECT_NEAR(walls[10][0][0], -413.0 - 70.0 * std::sin(std::numbers::pi * 247.0 / 419.0), 1e-9);
    EXPECT_EQ(walls[18][0], pillars[0][0]);
}

TEST(FishingSpot, YellowDropsIsBuiltFromTheDisc) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    expanded::test::ScratchDir folder("fishing_spot");
    const fs::path            &scratch = folder.path();
    ASSERT_EQ(BuildSpots(data, scratch), 0);
    const fs::path rebuilt = scratch / expanded::kRebuiltDiscFolder / "gedit/s13";

    Bytes                       disc = Read(fs::path(data) / "gedit/s13/scene.scn");
    Bytes                       scene_file = Read(rebuilt / "scene.scn");
    std::vector<scene::SubFile> files = scene::SubFiles(scene_file);
    auto                        part = [&](std::string_view name) { return *std::ranges::find(files, name, &scene::SubFile::name); };
    // The sign, then the bank's part after it, larger, the disc's left where it was.
    ASSERT_EQ(files.back().name, "kanban");
    EXPECT_GT(part("s1301").offset, files.back().offset);
    EXPECT_EQ(part("s1301").size, 346176u);
    EXPECT_EQ(part("s1301").offset + part("s1301").size, scene_file.size());
    EXPECT_EQ(GetF32(scene_file, part("s1302").offset + 0x244), expanded::kYellowDropsWater); // the water sheet
    // The camera walls gained the pillars' hulls.
    std::string bank = expanded::ToText(std::span<const unsigned char>(scene_file).subspan(part("s1301").offset, part("s1301").size));
    EXPECT_NE(bank.find("pcam_xs"), std::string::npos);
    EXPECT_NE(bank.find("pcam_xn"), std::string::npos);
    EXPECT_EQ(scene::SubFiles(disc).size() + 1, files.size());

    std::string cfg = expanded::ToText(Read(rebuilt / "mapinfo.cfg"));
    EXPECT_NE(cfg.find("\t\t\t-320, 4.25, -320,"), std::string::npos);
    EXPECT_NE(cfg.find("\t\t-465,\t30,\t40,\t//position\r\n\t\t0,\t90,\t0\t//rotation"), std::string::npos);

    const expanded::FishingSpot *spot = expanded::FishingSpotOfMap(23);
    ASSERT_NE(spot, nullptr);
    EXPECT_EQ(expanded::ReadFishingCollision(Read(scratch / expanded::FishingCollisionPath(*spot))).size(),
              expanded::YellowDropsBankWalls().size());
}
