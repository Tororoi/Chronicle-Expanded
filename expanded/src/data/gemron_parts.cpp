#include "expanded/data/gemron_parts.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <set>
#include <stdexcept>

namespace expanded {

namespace {

constexpr int         kDeathKey = 11, kDeathLoopKey = 12;
constexpr const char *kGemDrop[] = {"tama1__m", "tamas00__m", "tamas03__m", "tama__appz", "tamas01__appz", "tamas02__appz"};

} // namespace

std::array<float, 4> KeyValues(const MotKeyframe &key) {
    return {key.Value(0), key.Value(1), key.Value(2), key.Value(3)};
}

MotKeyframe MakeKey(std::uint32_t frame, const std::array<double, 4> &v) {
    MotKeyframe key;
    key.SetFrame(frame);
    for (int i = 0; i < 4; i++) {
        key.SetValue(i, static_cast<float>(v[std::size_t(i)]));
    }
    return key;
}

std::array<double, 4> Wide(const std::array<float, 4> &v) {
    return {v[0], v[1], v[2], v[3]};
}

// A track's value at a frame: its first or last key outside its range, else the pair around it blended.
std::array<double, 4> TrackAt(const MotTrack &track, double frame) {
    const auto &keys = track.keys;
    if (frame <= keys.front().Frame()) {
        return Wide(KeyValues(keys.front()));
    }
    if (frame >= keys.back().Frame()) {
        return Wide(KeyValues(keys.back()));
    }
    std::size_t b = 0;
    while (keys[b].Frame() <= frame) {
        b++;
    }
    const MotKeyframe   &ka = keys[b - 1], &kb = keys[b];
    double               t = (frame - ka.Frame()) / (double(kb.Frame()) - ka.Frame());
    std::array<float, 4> va = KeyValues(ka), vb = KeyValues(kb);
    if (track.w2 == 0) {
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

void SetF32(Bytes &data, std::size_t at, float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    Set32(data, at, bits);
}

Bytes GraftRecord(const Bytes &raw, int index, int parent, const std::optional<Mat3> &r, const std::optional<Vec3> &t, double scale) {
    Bytes rec = raw;
    Set32(rec, 0, std::uint32_t(index));
    Set32(rec, 0x2C, std::uint32_t(parent));
    if (r) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                SetF32(rec, 0x30 + std::size_t(i * 16 + j * 4), static_cast<float>((*r)[std::size_t(i)][std::size_t(j)]));
            }
            SetF32(rec, 0x30 + std::size_t(i * 16 + 12), 0.0f);
        }
    }
    Vec3 p = t ? *t : Vec3{GetF32(rec, 0x60) * scale, GetF32(rec, 0x64) * scale, GetF32(rec, 0x68) * scale};
    for (int c = 0; c < 3; c++) {
        SetF32(rec, 0x60 + std::size_t(c) * 4, static_cast<float>(p[std::size_t(c)]));
    }
    return rec;
}

Bytes PlainRecord(int index, int parent, std::string_view name, const Mat3 &r, const Vec3 &t) {
    Bytes rec(0x70, 0);
    Set32(rec, 0, std::uint32_t(index));
    Set32(rec, 4, 0x70);
    std::copy(name.begin(), name.end(), rec.begin() + 8);
    Set32(rec, 0x2C, std::uint32_t(parent));
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            SetF32(rec, 0x30 + std::size_t(i * 16 + j * 4), static_cast<float>(r[std::size_t(i)][std::size_t(j)]));
        }
    }
    for (int c = 0; c < 3; c++) {
        SetF32(rec, 0x60 + std::size_t(c) * 4, static_cast<float>(t[std::size_t(c)]));
    }
    SetF32(rec, 0x6C, 1.0f);
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
    int                               count = static_cast<int>(Get32(mds, 8));
    std::size_t                       table = Get32(mds, 12);
    std::vector<Bytes>                records;
    std::vector<std::optional<Bytes>> blobs;
    std::set<std::string>             drop(std::begin(kGemDrop), std::end(kGemDrop));
    for (int i = 0; i < count; i++) {
        std::size_t at = table + std::size_t(i) * 0x70;
        Bytes       raw(mds.begin() + std::ptrdiff_t(at), mds.begin() + std::ptrdiff_t(at + 0x70));
        std::string name = NameAt(raw, 8, 0x20);
        std::size_t off = Get32(raw, 0x28);
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
    std::size_t base = 0x10 + records.size() * 0x70;
    Bytes       body;
    for (std::size_t i = 0; i < records.size(); i++) {
        bool has = blobs[i] && !blobs[i]->empty();
        Set32(records[i], 0x28, has ? std::uint32_t(base + body.size()) : 0u);
        if (has) {
            body.insert(body.end(), blobs[i]->begin(), blobs[i]->end());
            while (body.size() % 16 != 0) {
                body.push_back(0);
            }
        }
    }
    Bytes out(mds.begin(), mds.begin() + 8);
    Bytes words(8);
    Set32(words, 0, std::uint32_t(records.size()));
    Set32(words, 4, 0x10);
    out.insert(out.end(), words.begin(), words.end());
    for (const Bytes &record : records) {
        out.insert(out.end(), record.begin(), record.end());
    }
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

void DeathClip(MotFile &mot, int start, int from, int to, int end) {
    for (MotTrack &track : mot.tracks) {
        if (track.keys.back().Frame() >= std::uint32_t(start)) {
            throw std::runtime_error(std::format("track {}/{} already reaches {}", track.w0, track.w2, track.keys.back().Frame()));
        }
        std::vector<MotKeyframe> made;
        for (int n = start; n <= start + to - from; n++) {
            made.push_back(MakeKey(std::uint32_t(n), TrackAt(track, n - start + from)));
        }
        made.push_back(MakeKey(std::uint32_t(end), TrackAt(track, to)));
        made.push_back(MakeKey(std::uint32_t(end + 1), TrackAt(track, to)));
        track.keys.insert(track.keys.end(), made.begin(), made.end());
    }
}

void DeathCfg(ChrPack &pack, int start, int end, const char *scroll) {
    ChrRecord               &record = pack.Require("info.cfg");
    Bytes                    payload = record.Payload();
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
    int  k = -1;
    bool scrolled = scroll == nullptr;
    for (std::string &line : lines) {
        std::string s = line.substr(std::min(line.size(), line.find_first_not_of(" \t")));
        if (s.starts_with("KEY_START")) {
            k = 0;
            continue;
        }
        if (s.starts_with("TEX_ANIME_END") && !scrolled) {
            line += std::string("\nTEX_ANIME 1,1\r\n") + scroll + "\r\nTEX_ANIME_END\r";
            scrolled = true;
        }
        if (s.starts_with("MATERIAL_ANIME")) {
            line = "MATERIAL_ANIME 1\r";
        }
        if (k >= 0 && s.starts_with("KEY") && !s.starts_with("KEY_")) {
            std::size_t comment_at = line.find("//");
            std::string comment = comment_at == std::string::npos ? "\r" : line.substr(comment_at);
            if (k == kDeathKey) {
                line = std::format("KEY\t{},\t{},\t{},\t{}", start, end, "0.35", comment);
            }
            if (k == kDeathLoopKey) {
                line = std::format("KEY\t{},\t{},\t0.0,\t{}", end, end, comment);
            }
            k++;
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
