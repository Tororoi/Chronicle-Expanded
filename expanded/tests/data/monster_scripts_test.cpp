#include <gtest/gtest.h>

#include <bit>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/monster_scripts.hpp"
#include "expanded/data/stb.hpp"

using expanded::Bytes;
using expanded::Get32;
using expanded::GetF32;
namespace stb = expanded::stb;

namespace {

constexpr std::size_t kBase = 0x20;

// A script of the cells given, its code from kBase, with no labels.
struct Script {
    Bytes bytes = Bytes(kBase, 0);

    Script() { expanded::Set32(bytes, 8, kBase); }

    Script &Cell(std::uint32_t op, std::uint32_t a1 = 0, std::uint32_t a2 = 0) {
        expanded::Append32(bytes, op);
        expanded::Append32(bytes, a1);
        expanded::Append32(bytes, a2);
        return *this;
    }

    Script &Int(std::uint32_t value) { return Cell(stb::kPushConst, stb::kInt, value); }

    Script &Float(float value) { return Cell(stb::kPushConst, stb::kFloat, std::bit_cast<std::uint32_t>(value)); }
};

std::uint32_t Op(const Bytes &script, std::size_t cell) {
    return Get32(script, kBase + cell * stb::kCell);
}

// The function a call cell at cell calls: its first cell's offset.
std::size_t Called(const Bytes &script, std::size_t cell) {
    EXPECT_EQ(Op(script, cell), stb::kCall);
    std::size_t header = kBase + Get32(script, kBase + cell * stb::kCell + 8);
    return kBase + Get32(script, header);
}

} // namespace

TEST(MonsterScripts, AMimicsWakeIsAGuardNotInvincibility) {
    Script mimic;
    mimic.Int(244).Float(173.0f).Float(186.0f).Cell(stb::kExt, 3); // its own guard window, in its first label
    mimic.Cell(stb::kReturn);
    mimic.Int(101).Int(100).Cell(stb::kExt, 2); // the wake's invincibility
    mimic.Cell(stb::kReturn);
    Bytes out = expanded::BakeMonsterScript("e109a", mimic.bytes);

    for (std::size_t cell : {5, 6, 7}) {
        EXPECT_EQ(Op(out, cell), stb::kMarker) << cell;
    }
    for (std::size_t cell : {1, 2, 3}) {
        EXPECT_EQ(Op(out, cell), stb::kMarker) << cell;
    }
    // The guard block calls a function: the block as it was, then the wake's window, then a return.
    std::size_t entry = Called(out, 0);
    EXPECT_TRUE(std::equal(mimic.bytes.begin() + kBase, mimic.bytes.begin() + kBase + 4 * stb::kCell, out.begin() + entry));
    std::size_t added = entry + 4 * stb::kCell;
    EXPECT_EQ(Get32(out, added + 8), 244u);
    EXPECT_EQ(GetF32(out, added + 12 + 8), expanded::kMimicWakeGuardStart);
    EXPECT_EQ(GetF32(out, added + 24 + 8), expanded::kMimicWakeGuardEnd);
    EXPECT_EQ(Get32(out, added + 36), stb::kExt);
    EXPECT_EQ(Get32(out, added + 48), stb::kReturn);
    EXPECT_THROW(expanded::BakeMonsterScript("e109a", out), std::runtime_error); // it has the mark
}

TEST(MonsterScripts, AKingWithoutAGuardHangsItsWakeOnItsFirstSphere) {
    Script king;
    king.Int(130).Cell(stb::kPushConst, stb::kString, 0).Float(10.0f).Cell(stb::kExt, 3);
    king.Int(101).Int(100).Cell(stb::kExt, 2);
    Bytes       out = expanded::BakeMonsterScript("e110a", king.bytes);
    std::size_t added = Called(out, 0) + 4 * stb::kCell;
    EXPECT_EQ(Get32(out, added + 8), 244u);
    EXPECT_EQ(GetF32(out, added + 24 + 8), expanded::kKingMimicWakeGuardEnd);
}

TEST(MonsterScripts, OnlyItsScriptsAreRebuilt) {
    Script none;
    none.Int(244).Float(173.0f).Float(186.0f).Cell(stb::kExt, 3);
    EXPECT_THROW(expanded::BakeMonsterScript("e109a", none.bytes), std::runtime_error); // no wake to change
    EXPECT_THROW(expanded::BakeMonsterScript("e115a", none.bytes), std::runtime_error);
    EXPECT_TRUE(expanded::ScriptIsBaked("e109a"));
    EXPECT_TRUE(expanded::ScriptIsBaked("c16a"));
    EXPECT_FALSE(expanded::ScriptIsBaked("e115a"));
    EXPECT_EQ(expanded::BakedScriptFile("e109a"), "expanded/monstor/e109a.stb");
}

TEST(MonsterScripts, BlizzardTakesTitansSpheresInOneCall) {
    Script blizzard;
    blizzard.Int(130).Cell(stb::kPushConst, stb::kString, 0).Float(9.0f).Cell(stb::kExt, 3);
    blizzard.Int(130).Cell(stb::kPushConst, stb::kString, 0).Float(7.0f).Cell(stb::kExt, 3);
    Bytes out = expanded::BakeMonsterScript("e65a", blizzard.bytes);
    for (std::size_t cell : {1, 2, 3, 4, 5, 6, 7}) {
        EXPECT_EQ(Op(out, cell), stb::kMarker) << cell;
    }
    std::size_t                                entry = Called(out, 0);
    std::vector<std::pair<std::string, float>> spheres;
    for (std::size_t at = entry; Get32(out, at) != stb::kReturn; at += 4 * stb::kCell) {
        EXPECT_EQ(Get32(out, at + 8), 130u);
        spheres.emplace_back(expanded::NameAt(out, kBase + Get32(out, at + 12 + 8), 16), GetF32(out, at + 24 + 8));
    }
    std::vector<std::pair<std::string, float>> titan = {
        {"bcol0", 5.0f },
        {"bcol1", 10.6f},
        {"bcol2", 6.0f },
        {"bcol3", 6.0f }
    };
    EXPECT_EQ(spheres, titan);
}

TEST(MonsterScripts, JoesSpheresTakeTheCatsHit) {
    Script joe;
    joe.Int(130).Cell(stb::kPushConst, stb::kString, 0).Float(9.0f).Cell(stb::kExt, 3);
    Bytes       out = expanded::BakeMonsterScript("c16a", joe.bytes);
    std::size_t added = Called(out, 0) + 4 * stb::kCell;
    EXPECT_EQ(Get32(out, added + 8), 134u); // _SET_BODY_COL_PARA(0, percent)
    EXPECT_EQ(Get32(out, added + 20), 0u);
    EXPECT_EQ(Get32(out, added + 32), static_cast<std::uint32_t>(expanded::kSpiritBeastSpherePercent));
    EXPECT_EQ(Get32(out, added + 48 + 20), 1u); // _SET_BODY_COL_PARA(1, kick)
    EXPECT_EQ(Get32(out, added + 48 + 32), static_cast<std::uint32_t>(expanded::kSpiritBeastSphereKick));
}
