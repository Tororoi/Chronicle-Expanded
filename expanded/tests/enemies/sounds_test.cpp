#include <gtest/gtest.h>

#include <vector>

#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/sounds.hpp"
#include "sound.hpp"

namespace {

using expanded::Monster;

} // namespace

TEST(MonsterSounds, AKindsSoundsAreInTheSetsOfTheDungeonsLayingItOut) {
    EXPECT_EQ(expanded::HomeSoundSets(Monster::CAVE_BAT), (std::vector<int>{100}));
    EXPECT_EQ(expanded::HomeSoundSets(Monster::GYON), (std::vector<int>{102}));
    EXPECT_EQ(expanded::HomeSoundSets(Monster::TITAN), (std::vector<int>{104}));
    EXPECT_EQ(expanded::HomeSoundSets(Monster::MIMIC_DBC), (std::vector<int>{100}));
    // Demon Shaft's bands each have their set.
    EXPECT_EQ(expanded::HomeSoundSets(Monster::GEMRON_HOLY), (std::vector<int>{110}));
    EXPECT_EQ(expanded::HomeSoundSets(Monster::MASTER_UTAN), (std::vector<int>{101, 201}));
    // Joe's sounds are only in the set his arena's event loads.
    EXPECT_EQ(expanded::HomeSoundSets(Monster::MINOTAUR_JOE).back(), 204);
    EXPECT_EQ(expanded::HomeSoundSets(Monster::ICE_QUEEN).back(), 202);
}

TEST(MonsterSounds, EachDungeonAndDemonShaftBandHasItsSet) {
    EXPECT_EQ(expanded::DungeonSoundSet(0, 7), 100);
    EXPECT_EQ(expanded::DungeonSoundSet(5, 24), 105);
    EXPECT_EQ(expanded::DungeonSoundSet(6, 0), expanded::kDemonShaftSoundSets);
    EXPECT_EQ(expanded::DungeonSoundSet(6, 99), 110);
}

TEST(MonsterSounds, NoBanksBoundLeavesThePortAlone) {
    EXPECT_EQ(expanded::MonsterSoundBank(MIDI_PORT_UNK_A, 75, 51), -1);
    EXPECT_EQ(expanded::MonsterSoundBank(MIDI_PORT_SE_DEFAULT, 75, 51), -1);
}
