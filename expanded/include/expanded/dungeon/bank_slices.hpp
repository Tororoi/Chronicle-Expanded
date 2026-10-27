#pragma once

#include <memory>
#include <span>

#include "audio/hdbank.hpp"

namespace expanded {

// A stretch of another bank's sample to make a bank of: the layer a key-on of note resolved to, [from, to) of its
// samples in seconds, faded out over its last fade seconds.
struct BankSlice {
    audio::Layer layer;
    int          note = 60;
    float        from = 0.0f;
    float        to = 0.0f;
    float        fade = 0.0f;
};

// The note slice i sounds at in a bank of slices (program 0) as its source's note did; a note either side shifts it a
// semitone, as the source's neighbours did.
inline constexpr int kSliceNoteSpacing = 12;

constexpr int SliceNote(int slice) {
    return kSliceNoteSpacing * (slice + 1);
}

// A bank of stretches of other banks' samples, each on program 0 at SliceNote, keeping its source's rate, volume and
// envelope: Kazarov's semantic bank, its samples handed over as WAVs in memory. Null when a slice has no sample, or
// nothing of one is left.
std::shared_ptr<audio::Bank> CreateBankSlices(std::span<const BankSlice> slices);

} // namespace expanded
