#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "audio/hdbank.hpp"

TEST(BankSlices, AStretchOfASampleSoundsAtItsNote) {
    audio::HdSplit   split;
    audio::HdSample  sample;
    sample.base_note = 39;
    audio::VagSample source;
    source.pcm.assign(1000, 1000);
    audio::BankSlice slice;
    slice.layer = {&split, &sample, &source, 100, 0};
    slice.note = 39;
    slice.from = 2.0f; // samples 200 …
    slice.to = 5.0f;   // … to 500
    slice.fade = 1.0f; // the last 100 fading out

    auto bank = audio::Bank::CreateSlices(std::span<const audio::BankSlice>(&slice, 1));
    ASSERT_NE(bank, nullptr);
    std::array<audio::Layer, 8> layers;
    ASSERT_EQ(bank->Resolve(0, audio::SliceNote(0), 0x7F, layers), 1);
    const std::vector<std::int16_t> &pcm = layers[0].vag->pcm;
    ASSERT_EQ(pcm.size(), 300u);
    EXPECT_EQ(pcm[0], 1000);
    EXPECT_EQ(pcm[199], 1000);
    EXPECT_LT(pcm[299], 20);
    EXPECT_EQ(layers[0].rate, 100);
    // Its pitch at its note is the source's at its own: the base note moved with it.
    EXPECT_EQ(layers[0].sample->base_note, audio::SliceNote(0));
    // A semitone either side still sounds it.
    EXPECT_EQ(bank->Resolve(0, audio::SliceNote(0) + 1, 0x7F, layers), 1);
}
