#include "expanded/data/gemron_parts.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <set>
#include <stdexcept>

#include "expanded/data/cfg_text.hpp"
#include "expanded/data/gemron_models.hpp"

namespace expanded {

namespace {

constexpr int         kDeathKey = 11, kDeathLoopKey = 12;
constexpr const char *kGemDrop[] = {kGemronBigGem, kGemronSmallGems[0], kGemronSmallGems[1], "tama__appz", "tamas01__appz", "tamas02__appz"};

} // namespace

// A track's value at a frame: its first or last key outside its range, else the pair around it blended.
std::array<double, 4> TrackAt(const MotTrack &track, double frame) {
    const auto &keys = track.keys;
    if (frame <= keys.front().Frame()) {
        return Wide(keys.front().Values());
    }
    if (frame >= keys.back().Frame()) {
        return Wide(keys.back().Values());
    }
    std::size_t b = 0;
    while (keys[b].Frame() <= frame) {
        b++;
    }
    const MotKeyframe   &ka = keys[b - 1], &kb = keys[b];
    double               t = (frame - ka.Frame()) / (double(kb.Frame()) - ka.Frame());
    std::array<float, 4> va = ka.Values(), vb = kb.Values();
    if (track.w2 == mot::kRotation) {
        return SlerpKeys(va, vb, t);
    }
    std::array<double, 4> v;
    for (int i = 0; i < 4; i++) {
        v[std::size_t(i)] = va[std::size_t(i)] + (vb[std::size_t(i)] - double(va[std::size_t(i)])) * t;
    }
    return v;
}

MotTrack TrackLike(const MotTrack &reference, std::uint32_t node, std::uint32_t w1, std::uint32_t channel) {
    MotTrack track;
    track.w0 = node;
    track.w1 = w1;
    track.w2 = channel;
    track.w3 = reference.w3;
    track.w6 = reference.w6;
    track.w7 = reference.w7;
    return track;
}

Bytes GraftRecord(const Bytes &raw, int index, int parent, const std::optional<Mat3> &r, const std::optional<Vec3> &t, double scale) {
    Bytes rec = raw;
    Set32(rec, mds::kIndex, std::uint32_t(index));
    Set32(rec, mds::kParent, std::uint32_t(parent));
    if (r) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                SetF32(rec, mds::kRotation + std::size_t(i * 16 + j * 4), static_cast<float>((*r)[std::size_t(i)][std::size_t(j)]));
            }
            SetF32(rec, mds::kRotation + std::size_t(i * 16 + 12), 0.0f);
        }
    }
    Vec3 p = t ? *t : Vec3{GetF32(rec, mds::kPosition) * scale, GetF32(rec, mds::kPosition + 4) * scale, GetF32(rec, mds::kPosition + 8) * scale};
    for (int c = 0; c < 3; c++) {
        SetF32(rec, mds::kPosition + std::size_t(c) * 4, static_cast<float>(p[std::size_t(c)]));
    }
    return rec;
}

Bytes ScaledMesh(std::span<const unsigned char> mds, std::size_t offset, double s) {
    MdtMesh mesh = MdtMesh::Parse(MdtAt(mds, offset));
    for (auto &p : mesh.positions) {
        p = {p[0] * s, p[1] * s, p[2] * s, p[3]};
    }
    return mesh.Build();
}

Bytes GraftMds(std::span<const unsigned char> mds, const std::vector<Graft> &grafts, const std::map<std::string, Bytes> &replace) {
    int                               count = static_cast<int>(Get32(mds, mds::kCountAt));
    std::size_t                       table = Get32(mds, mds::kTableAt);
    std::vector<Bytes>                records;
    std::vector<std::optional<Bytes>> blobs;
    std::set<std::string>             drop(std::begin(kGemDrop), std::end(kGemDrop));
    for (int i = 0; i < count; i++) {
        std::size_t at = table + std::size_t(i) * mds::kRecord;
        Bytes       raw(mds.begin() + std::ptrdiff_t(at), mds.begin() + std::ptrdiff_t(at + mds::kRecord));
        std::string name = NameAt(raw, mds::kName, mds::kNameSize);
        std::size_t off = Get32(raw, mds::kMesh);
        records.push_back(raw);
        if (drop.contains(name) || off == 0) {
            blobs.push_back(std::nullopt);
        } else if (auto found = replace.find(name); found != replace.end()) {
            blobs.push_back(found->second);
        } else {
            blobs.push_back(MdtAt(mds, off));
        }
    }
    for (const Graft &graft : grafts) {
        for (std::size_t j = 0; j < graft.records.size(); j++) {
            records.push_back(graft.records[j]);
            auto found = graft.meshes.find(int(j));
            blobs.push_back(found != graft.meshes.end() ? std::optional<Bytes>(found->second) : std::nullopt);
        }
    }
    return LayOutMds(mds, std::move(records), blobs);
}

void AppendBindPoses(ChrPack &pack, std::string_view bbp_name, std::size_t count) {
    ChrRecord &record = pack.Require(bbp_name);
    Bytes      poses = record.Payload();
    poses.resize(poses.size() + bbp::kEntry * count, 0);
    record.ReplacePayload(poses);
}

void DeathClip(MotFile &mot, int start, int from, int to, int end) {
    for (MotTrack &track : mot.tracks) {
        if (track.keys.back().Frame() >= std::uint32_t(start)) {
            throw std::runtime_error(std::format("track {}/{} already reaches {}", track.w0, track.w2, track.keys.back().Frame()));
        }
        std::vector<MotKeyframe> made;
        for (int n = start; n <= start + to - from; n++) {
            made.push_back(MotKeyframe::Make(std::uint32_t(n), TrackAt(track, n - start + from)));
        }
        made.push_back(MotKeyframe::Make(std::uint32_t(end), TrackAt(track, to)));
        made.push_back(MotKeyframe::Make(std::uint32_t(end + 1), TrackAt(track, to)));
        track.keys.insert(track.keys.end(), made.begin(), made.end());
    }
}

void DeathCfg(ChrPack &pack, int start, int end, const char *scroll) {
    ChrRecord &record = pack.Require("info.cfg");
    Bytes      payload = SetCfgKey(record.Payload(), kDeathKey, std::format("KEY\t{},\t{},\t0.35,\t", start, end));
    payload = SetCfgKey(payload, kDeathLoopKey, std::format("KEY\t{},\t{},\t0.0,\t", end, end));
    std::string              text(payload.begin(), payload.end());
    std::vector<std::string> lines;
    for (std::size_t from = 0;;) {
        std::size_t stop = text.find('\n', from);
        lines.push_back(text.substr(from, stop == std::string::npos ? std::string::npos : stop - from));
        if (stop == std::string::npos) {
            break;
        }
        from = stop + 1;
    }
    bool scrolled = scroll == nullptr;
    for (std::string &line : lines) {
        std::string s = line.substr(std::min(line.size(), line.find_first_not_of(" \t")));
        if (s.starts_with("TEX_ANIME_END") && !scrolled) {
            line += std::string("\nTEX_ANIME 1,1\r\n") + scroll + "\r\nTEX_ANIME_END\r";
            scrolled = true;
        }
        if (s.starts_with("MATERIAL_ANIME")) {
            line = "MATERIAL_ANIME 1\r";
        }
    }
    if (!scrolled) {
        throw std::runtime_error("info.cfg has no TEX_ANIME_END to follow with a scroll");
    }
    std::string joined;
    for (std::size_t i = 0; i < lines.size(); i++) {
        joined += lines[i];
        if (i + 1 < lines.size()) {
            joined += '\n';
        }
    }
    record.ReplacePayload(Bytes(joined.begin(), joined.end()));
}

} // namespace expanded
