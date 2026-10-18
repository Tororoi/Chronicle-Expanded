#pragma once

// The .chr model pack and its .mot motions and .mds frame tables, read and rebuilt byte for byte.
// Includes no game header, so dcdata can build with it.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace expanded {

using Bytes = std::vector<unsigned char>;

std::uint32_t Get32(std::span<const unsigned char> data, std::size_t offset);
void          Set32(std::span<unsigned char> data, std::size_t offset, std::uint32_t value);
float         GetF32(std::span<const unsigned char> data, std::size_t offset);

// size bytes at offset, throwing (naming what) when they run past the end of data.
std::span<const unsigned char> Slice(std::span<const unsigned char> data, std::size_t offset, std::size_t size, const char *what);

// The NUL-terminated name at offset, at most max bytes.
std::string NameAt(std::span<const unsigned char> data, std::size_t offset, std::size_t max);

// One file of a pack: name, payload offset @0x40, size @0x44, stride @0x48, then the payload.
struct ChrRecord {
    std::string   name;
    std::uint32_t data_off = 0;
    std::uint32_t size = 0;
    std::uint32_t stride = 0;
    Bytes         raw;

    Bytes Payload() const;
    // Swaps the payload, padding the stride to 16: a following .mds is DMA'd in qwords.
    void ReplacePayload(std::span<const unsigned char> payload);

    // A new record, its payload at 0x50 and the tag every retail record has @0x4C.
    static ChrRecord Make(std::string_view name, std::span<const unsigned char> payload);
};

// A pack: records chained by their strides up to one whose first byte is 0, then the trailer.
struct ChrPack {
    std::vector<ChrRecord> records;
    Bytes                  trailer;

    static ChrPack Parse(std::span<const unsigned char> blob);
    // The record of a path's basename, case folded, as GetPackFile finds it; nullptr if none.
    ChrRecord       *Find(std::string_view name);
    const ChrRecord *Find(std::string_view name) const;
    // As Find, throwing when there is none.
    ChrRecord       &Require(std::string_view name);
    const ChrRecord &Require(std::string_view name) const;
    Bytes            Rebuild() const;
};

// One .mot keyframe: the frame @0, then four floats @0x10 (a rotation's quaternion, w first).
struct MotKeyframe {
    static constexpr std::size_t     kSize = 0x20;
    std::array<unsigned char, kSize> raw{};

    std::uint32_t Frame() const;
    void          SetFrame(std::uint32_t frame);
    float         Value(int i) const;
    void          SetValue(int i, float value);
};

// One .mot track: w0 the joint's .mds frame index, w2 the channel (0 rotation, 2 translation).
struct MotTrack {
    static constexpr std::size_t   kHeaderSize = 0x20;
    static constexpr std::uint32_t kTagW6 = 0x74700000;
    static constexpr std::uint32_t kTagW7 = 0x747BFE95;
    std::uint32_t                  w0 = 0, w1 = 0, w2 = 0, w3 = 0, w6 = 0, w7 = 0;
    std::vector<MotKeyframe>       keys;
};

// A .mot record's tracks. The last track's continuation word is 0.
struct MotFile {
    std::vector<MotTrack> tracks;

    static MotFile FromRecord(const ChrRecord &record);
    Bytes          BuildPayload() const;
    MotTrack      *TrackBy(std::uint32_t joint, std::uint32_t channel);
};

// A .mds payload's frame names in table order, which a track's w0 indexes.
std::vector<std::string> ReadMdsFrames(std::span<const unsigned char> mds);

// Each source frame's index in dest by name, or -1; frame 0, the model root, maps to 0.
std::vector<int> BuildJointRemap(const std::vector<std::string> &src, const std::vector<std::string> &dst);

// Replaces dest's keys in [dest_lo, dest_hi] with the same-named joints' of src in [src_lo, src_hi].
void SpliceByJoint(MotFile &dest, const MotFile &src, const std::vector<std::string> &src_frames,
                   const std::vector<std::string> &dst_frames, std::uint32_t src_lo, std::uint32_t src_hi,
                   std::uint32_t dest_lo, std::uint32_t dest_hi);

} // namespace expanded
