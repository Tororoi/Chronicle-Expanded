#include <gtest/gtest.h>

#include "audio/mixer.hpp"
#include "sound_port.hpp"

TEST(PortVolume, ADuckPlaysQuieterWhileTheGameReadsItsOwnVolume) {
    constexpr int kPort = 3, kSpared = 9;
    SetPortVolume(kPort, 200);
    SetPortVolume(kSpared, 200);
    DuckPorts(0.5f, kSpared);
    EXPECT_EQ(PortVolume(kPort), 200);
    EXPECT_EQ(audio::DefaultMixer().Volume(kPort), 100);
    EXPECT_EQ(audio::DefaultMixer().Volume(kSpared), 200);
    // The game's own changes keep the duck.
    SetPortVolume(kPort, 160);
    EXPECT_EQ(audio::DefaultMixer().Volume(kPort), 80);
    DuckPorts(1.0f, kSpared);
    EXPECT_EQ(audio::DefaultMixer().Volume(kPort), 160);
    SetPortVolume(kPort, 0);
    SetPortVolume(kSpared, 0);
}
