#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "audio/mixer.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/weapons/xiao/divine_beast_title.hpp"
#include "test_support.hpp"

namespace {

// The loudest sample of the mixer's next seconds of output.
float Peak(float seconds) {
    audio::Mixer      &mixer = audio::DefaultMixer();
    int                frames = static_cast<int>(seconds * static_cast<float>(mixer.Rate()));
    std::vector<float> out(2 * static_cast<std::size_t>(frames));
    mixer.Render(out.data(), frames);
    float peak = 0.0f;
    for (float sample : out) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

// DC_TEST_DATA, an extracted data directory: a sound plays at its own volume, the cat's sit sound at 60 of the full 127.
TEST(OwnSounds, ASoundPlaysAtItsVolume) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    expanded::test::DataRootScope root(data);
    expanded::OwnSound            full = expanded::kSpiritBeastSitSound;
    full.volume = 0x7F;
    ASSERT_TRUE(expanded::LoadOwnSound(full));
    ASSERT_TRUE(expanded::PlayOwnSound(full, false));
    float loud = Peak(0.5f);
    ASSERT_GT(loud, 0.0f);
    Peak(3.0f); // played out
    ASSERT_TRUE(expanded::PlayOwnSound(expanded::kSpiritBeastSitSound, false));
    EXPECT_NEAR(Peak(0.5f) / loud, 60.0f / 127.0f, 0.01f);
}

} // namespace
