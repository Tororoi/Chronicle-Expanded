#include <gtest/gtest.h>

#include <string>

#include "character.hpp"
#include "dataalloc.hpp"
#include "expanded/dungeon/floor_characters.hpp"

namespace {

int loads = 0;

bool FakeCharacter(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *) {
    loads++;
    chara.Initialize();
    return std::string(path).find("missing") == std::string::npos;
}

class FloorCharacters : public ::testing::Test {
protected:
    void SetUp() override {
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakeCharacter);
        loads = 0;
    }

    void TearDown() override {
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(nullptr);
    }
};

} // namespace

TEST_F(FloorCharacters, ACharacterIsLoadedOnceAFloor) {
    CCharacter *cat = expanded::EnterFloorCharacter("expanded/chara/spirit_cat.chr", "info.cfg");
    ASSERT_NE(cat, nullptr);
    EXPECT_EQ(expanded::EnterFloorCharacter("expanded/chara/spirit_cat.chr", "info.cfg"), cat);
    EXPECT_EQ(loads, 1);

    // The floor's end takes it down; the next floor loads it again, in the same place.
    expanded::FloorCharactersClear();
    EXPECT_EQ(expanded::EnterFloorCharacter("expanded/chara/spirit_cat.chr", "info.cfg"), cat);
    EXPECT_EQ(loads, 2);
}

TEST_F(FloorCharacters, OneThatFailsIsTriedAgainAndTakesNoPlace) {
    EXPECT_EQ(expanded::EnterFloorCharacter("expanded/chara/missing.chr", "info.cfg"), nullptr);
    EXPECT_EQ(expanded::EnterFloorCharacter("expanded/chara/missing.chr", "info.cfg"), nullptr);
    EXPECT_EQ(loads, 2);
    CCharacter *first = expanded::EnterFloorCharacter("expanded/chara/a.chr", "info.cfg");
    CCharacter *second = expanded::EnterFloorCharacter("expanded/chara/b.chr", "info.cfg");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first, second);
    EXPECT_EQ(expanded::EnterFloorCharacter("expanded/chara/a.chr", "info.cfg"), first);
}

TEST_F(FloorCharacters, ACopyIsEnteredApartFromTheCharacter) {
    CCharacter *copy = expanded::EnterFloorCopy("dun/mainchara/c10b.chr", "base.cfg");
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(expanded::EnterFloorCopy("dun/mainchara/c10b.chr", "base.cfg"), copy);
    CCharacter *own = expanded::EnterFloorCharacter("dun/mainchara/c10b.chr", "base.cfg");
    ASSERT_NE(own, nullptr);
    EXPECT_NE(own, copy);
    EXPECT_EQ(loads, 2);
}
