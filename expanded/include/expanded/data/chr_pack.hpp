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

#include "expanded/data/bytes.hpp"

namespace expanded {

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
    // A built pack's trailer: a zero record header.
    static constexpr std::size_t kEndRecord = 0x50;

    std::vector<ChrRecord> records;
    Bytes                  trailer;

    static ChrPack Parse(std::span<const unsigned char> blob);
    // A new pack of records, then the kEndRecord trailer.
    static Bytes Build(std::vector<ChrRecord> records);
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

    // All four values, and set from doubles (each rounded to a float).
    std::array<float, 4> Values() const;
    void                 SetValues(const std::array<double, 4> &values);
    static MotKeyframe   Make(std::uint32_t frame, const std::array<double, 4> &values);
};

// Keys in frame order, those on one frame kept in their order.
void SortByFrame(std::vector<MotKeyframe> &keys);

// .mot track channels (MotTrack::w2): a joint's rotation, scale and translation; a .wgt's skin weights; a cutscene
// camera's position, reference point, roll and field of view; a material's alpha; a frame shown, and its subtree shown
// (each sets the frame's flag word by whether the value is below 1; below 1 the subtree is out of the draw).
namespace mot {
inline constexpr std::uint32_t kRotation = 0, kScale = 1, kTranslation = 2, kWeight = 20;
inline constexpr std::uint32_t kCameraPosition = 0x1E, kCameraReference = 0x1F, kCameraRoll = 0x20, kCameraFov = 0x21;
inline constexpr std::uint32_t kMaterialAlpha = 0x28, kVisible = 0x32, kSubtreeVisible = 0x33;
} // namespace mot

// One .mot track: w0 the joint's .mds frame index, w2 the channel (mot::).
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

// An .mds: its frame count @kCountAt and the frame table's offset @kTableAt (kTable in every pack), the records, then
// the meshes. A record (kRecord bytes): its index, its size @kSize, name @kName (kNameSize bytes), mesh offset @kMesh (0
// for none), parent @kParent, then its local matrix (rotation rows @kRotation, translation @kPosition).
namespace mds {
inline constexpr std::size_t kCountAt = 0x08, kTableAt = 0x0C, kTable = 0x10, kRecord = 0x70;
inline constexpr std::size_t kIndex = 0x00, kSize = 0x04, kName = 0x08, kNameSize = 0x20, kMesh = 0x28, kParent = 0x2C,
                             kRotation = 0x30, kPosition = 0x60;
} // namespace mds

// A .bbp: a kEntry-byte bind matrix a frame, in the .mds's frame order.
namespace bbp {
inline constexpr std::size_t kEntry = 64;
} // namespace bbp

// A .mds payload's frame names in table order, which a track's w0 indexes.
std::vector<std::string> ReadMdsFrames(std::span<const unsigned char> mds);

// Each source frame's index in dest by name, or -1; frame 0, the model root, maps to 0.
std::vector<int> BuildJointRemap(const std::vector<std::string> &src, const std::vector<std::string> &dst);

// Replaces dest's keys in [dest_lo, dest_hi] with the same-named joints' of src in [src_lo, src_hi].
void SpliceByJoint(MotFile &dest, const MotFile &src, const std::vector<std::string> &src_frames,
                   const std::vector<std::string> &dst_frames, std::uint32_t src_lo, std::uint32_t src_hi,
                   std::uint32_t dest_lo, std::uint32_t dest_hi);

} // namespace expanded
