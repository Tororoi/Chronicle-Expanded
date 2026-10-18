#include "expanded/data/chr_pack.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace expanded {

namespace {

[[noreturn]] void Bad(const std::string &what) {
    throw std::runtime_error(what);
}

void Need(std::span<const unsigned char> data, std::size_t offset, std::size_t size, const char *what) {
    if (offset > data.size() || size > data.size() - offset) {
        Bad(std::string(what) + " runs past the end of its data");
    }
}

char Fold(char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

bool EqualsFolded(std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return Fold(x) == Fold(y);
           });
}

std::string_view Basename(std::string_view path) {
    std::size_t slash = path.find_last_of("/\\");
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

void SortByFrame(std::vector<MotKeyframe> &keys) {
    std::stable_sort(keys.begin(), keys.end(),
                     [](const MotKeyframe &a, const MotKeyframe &b) { return a.Frame() < b.Frame(); });
}

} // namespace

std::uint32_t Get32(std::span<const unsigned char> data, std::size_t offset) {
    Need(data, offset, 4, "a word");
    return data[offset] | data[offset + 1] << 8 | data[offset + 2] << 16 | static_cast<std::uint32_t>(data[offset + 3]) << 24;
}

void Set32(std::span<unsigned char> data, std::size_t offset, std::uint32_t value) {
    Need(data, offset, 4, "a word");
    for (int i = 0; i < 4; i++) {
        data[offset + i] = static_cast<unsigned char>(value >> (8 * i));
    }
}

float GetF32(std::span<const unsigned char> data, std::size_t offset) {
    return std::bit_cast<float>(Get32(data, offset));
}

std::span<const unsigned char> Slice(std::span<const unsigned char> data, std::size_t offset, std::size_t size, const char *what) {
    Need(data, offset, size, what);
    return data.subspan(offset, size);
}

std::string NameAt(std::span<const unsigned char> data, std::size_t offset, std::size_t max) {
    Need(data, offset, 0, "a name");
    std::size_t end = offset;
    while (end < data.size() && end - offset < max && data[end] != 0) {
        end++;
    }
    return std::string(reinterpret_cast<const char *>(data.data() + offset), end - offset);
}

Bytes ChrRecord::Payload() const {
    Need(raw, data_off, size, "a record's payload");
    return Bytes(raw.begin() + data_off, raw.begin() + data_off + size);
}

void ChrRecord::ReplacePayload(std::span<const unsigned char> payload) {
    std::size_t padded = (data_off + payload.size() + 15) & ~std::size_t{15};
    Bytes       record(padded);
    std::copy_n(raw.begin(), data_off, record.begin());
    std::ranges::copy(payload, record.begin() + data_off);
    size = static_cast<std::uint32_t>(payload.size());
    stride = static_cast<std::uint32_t>(padded);
    Set32(record, 0x44, size);
    Set32(record, 0x48, stride);
    raw = std::move(record);
}

ChrRecord ChrRecord::Make(std::string_view name, std::span<const unsigned char> payload) {
    constexpr std::uint32_t kDataOff = 0x50, kTag = 0x00140E02;
    ChrRecord               record;
    record.name = std::string(name.substr(0, 0x3F));
    record.data_off = kDataOff;
    record.raw.assign(kDataOff, 0);
    std::copy(record.name.begin(), record.name.end(), record.raw.begin());
    Set32(record.raw, 0x40, kDataOff);
    Set32(record.raw, 0x4C, kTag);
    record.ReplacePayload(payload);
    return record;
}

ChrPack ChrPack::Parse(std::span<const unsigned char> blob) {
    ChrPack     pack;
    std::size_t p = 0;
    while (p < blob.size() && blob[p] != 0) {
        ChrRecord record;
        record.data_off = Get32(blob, p + 0x40);
        record.size = Get32(blob, p + 0x44);
        record.stride = Get32(blob, p + 0x48);
        if (record.stride == 0) {
            break;
        }
        Need(blob, p, record.stride, "a pack record");
        record.name = NameAt(blob, p, 0x40);
        record.raw.assign(blob.begin() + p, blob.begin() + p + record.stride);
        pack.records.push_back(std::move(record));
        p += pack.records.back().stride;
    }
    pack.trailer.assign(blob.begin() + p, blob.end());
    return pack;
}

ChrRecord *ChrPack::Find(std::string_view name) {
    return const_cast<ChrRecord *>(std::as_const(*this).Find(name));
}

const ChrRecord *ChrPack::Find(std::string_view name) const {
    std::string_view base = Basename(name);
    for (const ChrRecord &record : records) {
        if (EqualsFolded(record.name, base)) {
            return &record;
        }
    }
    return nullptr;
}

ChrRecord &ChrPack::Require(std::string_view name) {
    return const_cast<ChrRecord &>(std::as_const(*this).Require(name));
}

const ChrRecord &ChrPack::Require(std::string_view name) const {
    const ChrRecord *record = Find(name);
    if (!record) {
        Bad("no record '" + std::string(name) + "' in the pack");
    }
    return *record;
}

Bytes ChrPack::Rebuild() const {
    Bytes blob;
    for (const ChrRecord &record : records) {
        blob.insert(blob.end(), record.raw.begin(), record.raw.end());
    }
    blob.insert(blob.end(), trailer.begin(), trailer.end());
    return blob;
}

std::uint32_t MotKeyframe::Frame() const {
    return Get32(raw, 0);
}

void MotKeyframe::SetFrame(std::uint32_t frame) {
    Set32(raw, 0, frame);
}

float MotKeyframe::Value(int i) const {
    return GetF32(raw, 0x10 + 4 * i);
}

void MotKeyframe::SetValue(int i, float value) {
    Set32(raw, 0x10 + 4 * i, std::bit_cast<std::uint32_t>(value));
}

MotFile MotFile::FromRecord(const ChrRecord &record) {
    std::span<const unsigned char> blob = record.raw;
    Need(blob, record.data_off, record.size, "a .mot payload");
    MotFile     mot;
    std::size_t p = record.data_off;
    std::size_t end = record.data_off + record.size;
    while (p < end) {
        MotTrack track;
        track.w0 = Get32(blob, p);
        track.w1 = Get32(blob, p + 4);
        track.w2 = Get32(blob, p + 8);
        track.w3 = Get32(blob, p + 12);
        track.w6 = Get32(blob, p + 24);
        track.w7 = Get32(blob, p + 28);
        std::uint32_t count = Get32(blob, p + 16);
        std::uint32_t next = Get32(blob, p + 20);
        std::size_t   keys = p + MotTrack::kHeaderSize;
        Need(blob, keys, std::size_t{count} * MotKeyframe::kSize, "a .mot track");
        track.keys.resize(count);
        for (std::uint32_t i = 0; i < count; i++) {
            std::memcpy(track.keys[i].raw.data(), blob.data() + keys + i * MotKeyframe::kSize, MotKeyframe::kSize);
        }
        mot.tracks.push_back(std::move(track));
        p = keys + std::size_t{count} * MotKeyframe::kSize;
        if (next == 0) {
            break;
        }
    }
    return mot;
}

Bytes MotFile::BuildPayload() const {
    Bytes out;
    for (std::size_t t = 0; t < tracks.size(); t++) {
        const MotTrack &track = tracks[t];
        std::size_t     at = out.size();
        std::size_t     size = MotTrack::kHeaderSize + track.keys.size() * MotKeyframe::kSize;
        out.resize(at + size);
        std::span<unsigned char> head(out.data() + at, MotTrack::kHeaderSize);
        Set32(head, 0, track.w0);
        Set32(head, 4, track.w1);
        Set32(head, 8, track.w2);
        Set32(head, 12, track.w3);
        Set32(head, 16, static_cast<std::uint32_t>(track.keys.size()));
        Set32(head, 20, t + 1 == tracks.size() ? 0 : static_cast<std::uint32_t>(size));
        Set32(head, 24, track.w6);
        Set32(head, 28, track.w7);
        for (std::size_t i = 0; i < track.keys.size(); i++) {
            std::memcpy(out.data() + at + MotTrack::kHeaderSize + i * MotKeyframe::kSize, track.keys[i].raw.data(),
                        MotKeyframe::kSize);
        }
    }
    return out;
}

MotTrack *MotFile::TrackBy(std::uint32_t joint, std::uint32_t channel) {
    for (MotTrack &track : tracks) {
        if (track.w0 == joint && track.w2 == channel) {
            return &track;
        }
    }
    return nullptr;
}

std::vector<std::string> ReadMdsFrames(std::span<const unsigned char> mds) {
    std::uint32_t            count = Get32(mds, 0x08);
    std::uint32_t            stride = Get32(mds, 0x14);
    std::vector<std::string> names;
    for (std::uint32_t i = 0; i < count; i++) {
        names.push_back(NameAt(mds, 0x18 + std::size_t{i} * stride, 0x20));
    }
    return names;
}

std::vector<int> BuildJointRemap(const std::vector<std::string> &src, const std::vector<std::string> &dst) {
    std::unordered_map<std::string, int> dst_of;
    for (std::size_t i = 0; i < dst.size(); i++) {
        dst_of.emplace(dst[i], static_cast<int>(i));
    }
    std::vector<int> remap(src.size());
    for (std::size_t i = 0; i < src.size(); i++) {
        auto found = dst_of.find(src[i]);
        remap[i] = i == 0 ? 0 : found == dst_of.end() ? -1
                                                      : found->second;
    }
    return remap;
}

void SpliceByJoint(MotFile &dest, const MotFile &src, const std::vector<std::string> &src_frames,
                   const std::vector<std::string> &dst_frames, std::uint32_t src_lo, std::uint32_t src_hi,
                   std::uint32_t dest_lo, std::uint32_t dest_hi) {
    std::vector<int>                                              remap = BuildJointRemap(src_frames, dst_frames);
    std::int64_t                                                  delta = std::int64_t{dest_lo} - src_lo;
    std::map<std::pair<std::uint32_t, std::uint32_t>, MotTrack *> dst_by;
    for (MotTrack &track : dest.tracks) {
        dst_by[{track.w0, track.w2}] = &track;
    }
    for (const MotTrack &track : src.tracks) {
        std::vector<MotKeyframe> window;
        for (const MotKeyframe &key : track.keys) {
            if (src_lo <= key.Frame() && key.Frame() <= src_hi) {
                window.push_back(key);
            }
        }
        if (window.empty()) {
            continue;
        }
        int joint = track.w0 < remap.size() ? remap[track.w0] : -1;
        if (joint < 0) {
            continue;
        }
        auto found = dst_by.find({static_cast<std::uint32_t>(joint), track.w2});
        if (found == dst_by.end()) {
            continue;
        }
        std::vector<MotKeyframe> keys;
        for (const MotKeyframe &key : found->second->keys) {
            if (!(dest_lo <= key.Frame() && key.Frame() <= dest_hi)) {
                keys.push_back(key);
            }
        }
        for (MotKeyframe key : window) {
            std::int64_t frame = key.Frame() + delta;
            if (frame < dest_lo || frame > dest_hi) {
                continue;
            }
            key.SetFrame(static_cast<std::uint32_t>(frame));
            keys.push_back(key);
        }
        SortByFrame(keys);
        found->second->keys = std::move(keys);
    }
}

} // namespace expanded
