#include <gtest/gtest.h>

#include <vector>

#include "expanded/enemies/sounds.hpp"
#include "sound.hpp"

namespace {

constexpr int kCaveBat = 52;
constexpr int kGyon = 20;
constexpr int kTitan = 28;
constexpr int kGemronHoly = 154;
constexpr int kMasterUtan = 79;

} // namespace

TEST(MonsterSounds, AKindsSoundsAreInTheSetsOfTheDungeonsLayingItOut) {
    EXPECT_EQ(expanded::HomeSoundSets(kCaveBat), (std::vector<int>{100}));
    EXPECT_EQ(expanded::HomeSoundSets(kGyon), (std::vector<int>{102}));
    EXPECT_EQ(expanded::HomeSoundSets(kTitan), (std::vector<int>{104}));
    EXPECT_EQ(expanded::HomeSoundSets(30), (std::vector<int>{100})); // Divine Beast Cave's mimic
    // Demon Shaft's bands each have their set.
    EXPECT_EQ(expanded::HomeSoundSets(kGemronHoly), (std::vector<int>{110}));
    EXPECT_EQ(expanded::HomeSoundSets(kMasterUtan), (std::vector<int>{101}));
}

TEST(MonsterSounds, NoBanksBoundLeavesThePortAlone) {
    EXPECT_EQ(expanded::MonsterSoundBank(MIDI_PORT_UNK_A, 75, 51), -1);
    EXPECT_EQ(expanded::MonsterSoundBank(MIDI_PORT_SE_DEFAULT, 75, 51), -1);
}
