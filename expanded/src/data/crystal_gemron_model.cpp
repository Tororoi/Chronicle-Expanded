#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

#include "expanded/data/gemron_models.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

namespace {

constexpr const char *kMds = "e115a.mds";
constexpr const char *kMot = "e115a.mot";
constexpr const char *kShadowMot = "e115s.mot";
constexpr const char *kBbp = "e115a.bbp";
constexpr const char *kImg = "e115a01.img";
constexpr const char *kSheet = "e115a01";
constexpr const char *kSheet2 = "e115a02";
constexpr const char *kBallMds = "e209ball_b.mds";
constexpr const char *kBallMot = "e209ball_b.mot";

// The ball's shell in its own model sits 1.0 over its root. It hangs from each gem bone; only the big one keeps its
// glints, beams and rings (null14).
constexpr double      kBallRadius = 1.0;
constexpr const char *kGems[] = {"tama1__m", "tamas00__m", "tamas03__m"};
constexpr const char *kEffectGem = "tama1__m";
constexpr const char *kGemDrop[] = {"tama1__m", "tamas00__m", "tamas03__m", "tama__appz", "tamas01__appz", "tamas02__appz"};
constexpr const char *kPiecesRoot = "null9";
constexpr const char *kEffectsRoot = "null14";
constexpr const char *kShardsRoot = "null10";

// The death clip: the vanilla death (105-125, KEY 11 at 0.35) copied to 200, the Gemron then holding its last pose
// while the ball's shatter (its own frames 1-21, at 0.1 in the event) runs from the copy of frame 121, stretched to keep
// its speed under the death's; its effects 1.5 times that, its pieces twice.
constexpr int    kDeathKey = 11, kDeathLoopKey = 12;
constexpr int    kClipFrom = 105, kClipTo = 125;
constexpr int    kShatterFrom = 121;
constexpr double kDeathSpeed = 0.35, kBallSpeed = 0.1;
constexpr int    kDeathStart = 200;
constexpr double kPieceSpeedup = 2.0, kEffectSpeedup = 1.5;
const double     kStretch = kDeathSpeed / kBallSpeed;
constexpr int    kShatterStart = kDeathStart + kShatterFrom - kClipFrom;
const int        kShatterEnd = kShatterStart + static_cast<int>(std::nearbyint(20 * kStretch / kEffectSpeedup));
const int        kDeathEnd = kShatterEnd;
// The engine plays a track only between two of its keys, so every track runs from 0 to one past the last frame any key
// plays.
const int               kTimelineEnd = kDeathEnd + 1;
constexpr std::uint32_t kMaterialAlpha = 40;
constexpr std::uint32_t kSubtreeVisible = 0x33; // < 1: the frame and its subtree out of the draw
constexpr float         kHidden = 0.001f;
constexpr const char   *kBallScroll = "TEX_SCROLL_DATA \"ball_b01\",0,32,128,32,\"ball_b01\",0,0,0.5,0.5,0,1\t//\x8b\xca"; // 玉

// The eyes: the head's triangles whose texture centroid lies in this box move to a frame of their own.
constexpr const char *kEyeFrame = "eyes";
constexpr const char *kEyeHead = "obj2_2";
constexpr double      kEyeU0 = 0.21, kEyeU1 = 0.35, kEyeV0 = 0.50, kEyeV1 = 0.61;

constexpr Vec3 kForward = {0.0, 0.0, 1.0}, kUp = {0.0, 1.0, 0.0};

std::array<float, 4> Values(const MotKeyframe &key) {
    return {key.Value(0), key.Value(1), key.Value(2), key.Value(3)};
}

MotKeyframe Key(std::uint32_t frame, const std::array<double, 4> &v) {
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
std::array<double, 4> At(const MotTrack &track, double frame) {
    const auto &keys = track.keys;
    if (frame <= keys.front().Frame()) {
        return Wide(Values(keys.front()));
    }
    if (frame >= keys.back().Frame()) {
        return Wide(Values(keys.back()));
    }
    std::size_t b = 0;
    while (keys[b].Frame() <= frame) {
        b++;
    }
    const MotKeyframe   &ka = keys[b - 1], &kb = keys[b];
    double               t = (frame - ka.Frame()) / (double(kb.Frame()) - ka.Frame());
    std::array<float, 4> va = Values(ka), vb = Values(kb);
    if (track.w2 == 0) {
        return SlerpKeys(va, vb, t);
    }
    std::array<double, 4> v;
    for (int i = 0; i < 4; i++) {
        v[std::size_t(i)] = va[std::size_t(i)] + (vb[std::size_t(i)] - double(va[std::size_t(i)])) * t;
    }
    return v;
}

Mat3 QuatToMat(const std::array<double, 4> &q) {
    double w = q[0], x = q[1], y = q[2], z = q[3];
    return {
        {
         {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
         {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
         {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)},
         }
    };
}

Mat4 MatMul4(const Mat4 &a, const Mat4 &b) {
    Mat4 out{};
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            out[std::size_t(r)][std::size_t(c)] =
                ExactSum({a[std::size_t(r)][0] * b[0][std::size_t(c)], a[std::size_t(r)][1] * b[1][std::size_t(c)],
                          a[std::size_t(r)][2] * b[2][std::size_t(c)], a[std::size_t(r)][3] * b[3][std::size_t(c)]});
        }
    }
    return out;
}

// Every frame's world matrix at a frame of the motion.
std::vector<Mat4> PoseWorld(const std::vector<MdsFrame> &frames, const MotFile &mot, double frame) {
    std::map<std::uint32_t, const MotTrack *> rotation, translation;
    for (const MotTrack &track : mot.tracks) {
        if (track.w2 == 0) {
            rotation[track.w0] = &track;
        } else if (track.w2 == 2) {
            translation[track.w0] = &track;
        }
    }
    std::vector<Mat4> world;
    for (const MdsFrame &f : frames) {
        Mat4 local = f.local;
        if (auto found = rotation.find(std::uint32_t(f.index)); found != rotation.end()) {
            Mat3 r = QuatToMat(At(*found->second, frame));
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    local[std::size_t(i)][std::size_t(j)] = r[std::size_t(i)][std::size_t(j)];
                }
            }
        }
        if (auto found = translation.find(std::uint32_t(f.index)); found != translation.end()) {
            std::array<double, 4> t = At(*found->second, frame);
            for (int j = 0; j < 3; j++) {
                local[3][std::size_t(j)] = t[std::size_t(j)];
            }
        }
        world.push_back(f.parent < 0 ? local : MatMul4(local, world[std::size_t(f.parent)]));
    }
    return world;
}

Vec3 LocalDirection(const Mat4 &w, const Vec3 &d) {
    Vec3 v;
    for (int r = 0; r < 3; r++) {
        v[std::size_t(r)] = ExactSum({w[std::size_t(r)][0] * d[0], w[std::size_t(r)][1] * d[1], w[std::size_t(r)][2] * d[2]});
    }
    return Unit(v);
}

// The ball root's rotation in its gem bone's frame: its +Y up the world, its +Z the Gemron's forward.
Mat3 Upright(const Mat4 &w) {
    Vec3   u = LocalDirection(w, kUp), f = LocalDirection(w, kForward);
    double along = ExactSum({f[0] * u[0], f[1] * u[1], f[2] * u[2]});
    f = Unit({f[0] - along * u[0], f[1] - along * u[1], f[2] - along * u[2]});
    return {Cross(u, f), u, f};
}

void SetF32(Bytes &data, std::size_t at, float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    Set32(data, at, bits);
}

// A ball frame's record under the Gemron: its index and parent; the root's rotation and position given, the rest's
// position scaled.
Bytes Record(const Bytes &raw, int index, int parent, const std::optional<Mat3> &r, const std::optional<Vec3> &t, double scale) {
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

Bytes ScaledMesh(std::span<const unsigned char> mds, std::size_t offset, double s) {
    MdtMesh mesh = MdtMesh::Parse(MdtAt(mds, offset));
    for (auto &p : mesh.positions) {
        p = {p[0] * s, p[1] * s, p[2] * s, p[3]};
    }
    return mesh.Build();
}

// The padding after a display list sized so the blocks after it start 16-aligned.
void AlignDisplayList(MdtMesh &mesh) {
    std::size_t dl = 0x10;
    for (const MdtMesh::Submesh &sub : mesh.submeshes) {
        dl += 0xC + sub.records.size() * (mesh.colours ? 16 : 12);
    }
    mesh.pads["DL"] = Bytes((16 - dl % 16) % 16, 0);
}

// The head without the eye triangles, and the eyes: the head's strips split around them (it draws two-sided), the eyes
// one triangle list per material.
std::pair<Bytes, Bytes> SplitEyes(std::span<const unsigned char> mds, const MdsFrame &head) {
    MdtMesh m = MdtMesh::Parse(MdtAt(mds, std::size_t(head.mesh)));
    auto    eye = [&](const std::vector<int> &a, const std::vector<int> &b, const std::vector<int> &c) {
        if (a[0] == b[0] || b[0] == c[0] || a[0] == c[0]) {
            return false;
        }
        double u = ExactSum({m.uvs[std::size_t(a[2])][0], m.uvs[std::size_t(b[2])][0], m.uvs[std::size_t(c[2])][0]}) / 3;
        double v = ExactSum({m.uvs[std::size_t(a[2])][1], m.uvs[std::size_t(b[2])][1], m.uvs[std::size_t(c[2])][1]}) / 3;
        return kEyeU0 < u && u < kEyeU1 && kEyeV0 < v && v < kEyeV1;
    };
    std::vector<MdtMesh::Submesh> keep, eyes;
    auto                          add_eye = [&](int material, std::initializer_list<std::vector<int>> triangle) {
        auto found = std::find_if(eyes.begin(), eyes.end(), [&](const MdtMesh::Submesh &s) { return s.material == material; });
        if (found == eyes.end()) {
            eyes.push_back({3, material, {}});
            found = eyes.end() - 1;
        }
        found->records.insert(found->records.end(), triangle);
    };
    for (const MdtMesh::Submesh &sub : m.submeshes) {
        const auto &recs = sub.records;
        if (sub.prim == 3) {
            MdtMesh::Submesh kept{3, sub.material, {}};
            for (std::size_t k = 0; k + 2 < recs.size(); k += 3) {
                if (eye(recs[k], recs[k + 1], recs[k + 2])) {
                    add_eye(sub.material, {recs[k], recs[k + 1], recs[k + 2]});
                } else {
                    kept.records.insert(kept.records.end(), {recs[k], recs[k + 1], recs[k + 2]});
                }
            }
            if (!kept.records.empty()) {
                keep.push_back(std::move(kept));
            }
        } else {
            int n = static_cast<int>(recs.size()) - 2, run = -1;
            for (int k = 0; k <= n; k++) {
                bool selected = k < n && eye(recs[std::size_t(k)], recs[std::size_t(k + 1)], recs[std::size_t(k + 2)]);
                if (selected) {
                    if ((k & 1) == 1) {
                        add_eye(sub.material, {recs[std::size_t(k + 1)], recs[std::size_t(k)], recs[std::size_t(k + 2)]});
                    } else {
                        add_eye(sub.material, {recs[std::size_t(k)], recs[std::size_t(k + 1)], recs[std::size_t(k + 2)]});
                    }
                }
                if (k < n && !selected) {
                    if (run < 0) {
                        run = k;
                    }
                    continue;
                }
                if (run >= 0) {
                    keep.push_back({4, sub.material, std::vector<std::vector<int>>(recs.begin() + run, recs.begin() + k + 2)});
                    run = -1;
                }
            }
        }
    }
    MdtMesh head_mesh = MdtMesh::Parse(MdtAt(mds, std::size_t(head.mesh)));
    head_mesh.submeshes = keep;
    m.submeshes = eyes;
    AlignDisplayList(head_mesh);
    AlignDisplayList(m);
    return {head_mesh.Build(), m.Build()};
}

struct Graft {
    std::vector<Bytes>   records;
    std::map<int, Bytes> meshes; // by the record's place in the graft
};

// The .mds with the gems' meshes (and glow shells) dropped, the head's replaced and the grafts' records appended.
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

// The vanilla death copied to 200 on, every track keyed on each frame of it, its last pose held to the clip's end.
void DeathClip(MotFile &mot) {
    for (MotTrack &track : mot.tracks) {
        if (track.keys.back().Frame() >= kDeathStart) {
            throw std::runtime_error(std::format("track {}/{} already reaches {}", track.w0, track.w2, track.keys.back().Frame()));
        }
        std::vector<MotKeyframe> made;
        for (int n = kDeathStart; n <= kDeathStart + kClipTo - kClipFrom; n++) {
            made.push_back(Key(std::uint32_t(n), At(track, n - kDeathStart + kClipFrom)));
        }
        made.push_back(Key(std::uint32_t(kDeathEnd), At(track, kClipTo)));
        made.push_back(Key(std::uint32_t(kTimelineEnd), At(track, kClipTo)));
        track.keys.insert(track.keys.end(), made.begin(), made.end());
    }
}

// The death keys on the new clip, the loop on its last frame, material animation on (the fades), and the ball's
// texture scroll as a second TEX_ANIME group.
void Cfg(ChrPack &pack) {
    ChrRecord               &record = pack.Require("info.cfg");
    Bytes                    payload = record.Payload();
    std::string              text(payload.begin(), payload.end());
    std::vector<std::string> lines;
    for (std::size_t from = 0;;) {
        std::size_t end = text.find('\n', from);
        lines.push_back(text.substr(from, end == std::string::npos ? std::string::npos : end - from));
        if (end == std::string::npos) {
            break;
        }
        from = end + 1;
    }
    int  k = -1;
    bool scrolled = false;
    for (std::string &line : lines) {
        std::string s = line.substr(std::min(line.size(), line.find_first_not_of(" \t")));
        if (s.starts_with("KEY_START")) {
            k = 0;
            continue;
        }
        if (s.starts_with("TEX_ANIME_END") && !scrolled) {
            line += std::string("\nTEX_ANIME 1,1\r\n") + kBallScroll + "\r\nTEX_ANIME_END\r";
            scrolled = true;
        }
        if (s.starts_with("MATERIAL_ANIME")) {
            line = "MATERIAL_ANIME 1\r";
        }
        if (k >= 0 && s.starts_with("KEY") && !s.starts_with("KEY_")) {
            std::size_t comment_at = line.find("//");
            std::string comment = comment_at == std::string::npos ? "\r" : line.substr(comment_at);
            if (k == kDeathKey) {
                line = std::format("KEY\t{},\t{},\t{},\t{}", kDeathStart, kDeathEnd, "0.35", comment);
            }
            if (k == kDeathLoopKey) {
                line = std::format("KEY\t{},\t{},\t0.0,\t{}", kDeathEnd, kDeathEnd, comment);
            }
            k++;
        }
    }
    if (!scrolled) {
        throw std::runtime_error("info.cfg has no TEX_ANIME_END to follow with the ball's scroll");
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

} // namespace

Bytes BuildCrystalGemronModel(std::span<const unsigned char> gemron, std::span<const unsigned char> ball_chr) {
    ChrPack               pack = ChrPack::Parse(gemron);
    ChrPack               ball = ChrPack::Parse(ball_chr);
    Bytes                 mds = pack.Require(kMds).Payload();
    std::vector<MdsFrame> frames = ReadMdsSkeleton(mds);
    Bytes                 bmds = ball.Require(kBallMds).Payload();
    std::vector<MdsFrame> bframes = ReadMdsSkeleton(bmds);
    MotFile               bmot = MotFile::FromRecord(ball.Require(kBallMot));
    MotFile               mot = MotFile::FromRecord(pack.Require(kMot));
    std::vector<Mat4>     broken = PoseWorld(frames, mot, kShatterFrom); // the pose the balls break in
    const MotTrack        reference = mot.tracks.back();
    std::set<int>         pieces, effects;
    for (const MdsFrame &f : bframes) {
        if (f.name == kPiecesRoot || pieces.contains(f.parent)) {
            pieces.insert(f.index);
        }
        if (f.name == kEffectsRoot || effects.contains(f.parent)) {
            effects.insert(f.index);
        }
    }
    auto named = [&](std::string_view name) {
        const MdsFrame *f = FindFrame(bframes, name);
        if (f == nullptr) {
            throw std::runtime_error(std::format("{} has no {}", kBallMds, name));
        }
        return f->index;
    };
    auto track_like = [&](std::uint32_t node, std::uint32_t w1, std::uint32_t w2) {
        MotTrack track;
        track.w0 = node;
        track.w1 = w1;
        track.w2 = w2;
        track.w3 = reference.w3;
        track.w6 = reference.w6;
        track.w7 = reference.w7;
        return track;
    };
    std::vector<Graft>    grafts;
    std::vector<MotTrack> added;
    int                   at = static_cast<int>(frames.size());
    for (const char *gem_name : kGems) {
        const MdsFrame *gem = FindFrame(frames, gem_name);
        if (gem == nullptr) {
            throw std::runtime_error(std::format("{} has no {}", kMds, gem_name));
        }
        MdtMesh gem_mesh = MdtMesh::Parse(MdtAt(mds, std::size_t(gem->mesh)));
        double  most = 0;
        for (const auto &p : gem_mesh.positions) {
            most = std::max({most, std::fabs(p[0]), std::fabs(p[1]), std::fabs(p[2])});
        }
        double           s = most / kBallRadius; // the gem's radius
        Mat3             r = Upright(broken[std::size_t(gem->index)]);
        Vec3             t = {-s * kBallRadius * r[1][0], -s * kBallRadius * r[1][1], -s * kBallRadius * r[1][2]}; // the ball's centre on the gem's
        std::vector<int> keep;
        for (const MdsFrame &f : bframes) {
            if (std::string_view(gem_name) == kEffectGem || !effects.contains(f.index)) {
                keep.push_back(f.index);
            }
        }
        std::map<int, int> placed; // ball frame -> its place in this graft
        for (std::size_t k = 0; k < keep.size(); k++) {
            placed[keep[k]] = int(k);
        }
        Graft graft;
        for (int j : keep) {
            const MdsFrame &f = bframes[std::size_t(j)];
            graft.records.push_back(f.parent < 0 ? Record(f.record, at + placed[j], gem->index, r, t, 1.0)
                                                 : Record(f.record, at + placed[j], at + placed[f.parent], std::nullopt, std::nullopt, s));
            if (f.mesh != 0) {
                graft.meshes[placed[j]] = ScaledMesh(bmds, std::size_t(f.mesh), s);
            }
        }
        grafts.push_back(std::move(graft));
        for (const MotTrack &source : bmot.tracks) {
            auto found = placed.find(int(source.w0));
            if (found == placed.end()) {
                continue;
            }
            double   stretch = kStretch / (pieces.contains(int(source.w0)) ? kPieceSpeedup : kEffectSpeedup);
            MotTrack track = track_like(std::uint32_t(at + found->second), source.w1, source.w2);
            for (const MotKeyframe &key : source.keys) {
                std::array<double, 4> v = Wide(Values(key));
                if (source.w2 == 2) {
                    v = {v[0] * s, v[1] * s, v[2] * s, v[3]};
                }
                track.keys.push_back(Key(std::uint32_t(kShatterStart + std::nearbyint((double(key.Frame()) - 1) * stretch)), v));
            }
            std::array<double, 4> first = Wide(Values(track.keys.front())), last = Wide(Values(track.keys.back()));
            // A material-alpha key writes the species' one material: its fades are keyed from the shatter on only, so only
            // a dying Gemron writes it. Every other track is per Gemron: at rest from 0.
            if (source.w2 != kMaterialAlpha) {
                track.keys.insert(track.keys.begin(), Key(0, first));
            }
            if (track.keys.back().Frame() < std::uint32_t(kTimelineEnd)) {
                track.keys.push_back(Key(std::uint32_t(kTimelineEnd), last));
            }
            added.push_back(std::move(track));
        }
        // The effects drawn only through the shatter, the shards only from it on.
        const std::pair<int, double> effect_keys[] = {
            {0,                 0.0},
            {kShatterStart - 1, 0.0},
            {kShatterStart,     1.0},
            {kShatterEnd - 1,   1.0},
            {kShatterEnd,       0.0},
            {kTimelineEnd,      0.0}
        };
        const std::pair<int, double> shard_keys[] = {
            {0,                 0.0},
            {kShatterStart - 1, 0.0},
            {kShatterStart,     1.0},
            {kTimelineEnd,      1.0}
        };
        for (auto [root, keys] : {
                 std::pair{kEffectsRoot, std::span<const std::pair<int, double>>(effect_keys)},
                 std::pair{kShardsRoot,  std::span<const std::pair<int, double>>(shard_keys) }
        }) {
            auto found = placed.find(named(root));
            if (found == placed.end()) {
                continue;
            }
            MotTrack track = track_like(std::uint32_t(at + found->second), 0, kSubtreeVisible);
            for (auto [f, v] : keys) {
                track.keys.push_back(Key(std::uint32_t(f), {v, 0, 0, 0}));
            }
            added.push_back(std::move(track));
        }
        // The glints, beams and rings also scaled to nothing outside the shatter (their camera-facing parents).
        for (int j : keep) {
            if (bframes[std::size_t(j)].name.find("__ba") == std::string::npos) {
                continue;
            }
            MotTrack     track = track_like(std::uint32_t(at + placed[j]), 0, 1);
            const double h = kHidden;
            for (auto [f, v] : {
                     std::pair{0,                 h  },
                     std::pair{kShatterStart - 1, h  },
                     std::pair{kShatterStart,     1.0},
                     std::pair{kShatterEnd - 1,   1.0},
                     std::pair{kShatterEnd,       h  },
                     std::pair{kTimelineEnd,      h  }
            }) {
                track.keys.push_back(Key(std::uint32_t(f), {v, v, v, 0}));
            }
            added.push_back(std::move(track));
        }
        at += static_cast<int>(keep.size());
    }
    // The eyes: their triangles out of the head into a frame of their own, which keeps its bind pose.
    const MdsFrame *head = FindFrame(frames, kEyeHead);
    if (head == nullptr) {
        throw std::runtime_error(std::format("{} has no {}", kMds, kEyeHead));
    }
    auto [head_mdt, eye_mdt] = SplitEyes(mds, *head);
    grafts.push_back({{FrameRecord(at, kEyeFrame, head->index)}, {{0, eye_mdt}}});
    at += 1;
    pack.Require(kMds).ReplacePayload(GraftMds(mds, grafts, {
                                                                {kEyeHead, head_mdt}
    }));
    Bytes bbp = pack.Require(kBbp).Payload();
    bbp.resize(bbp.size() + 64 * (std::size_t(at) - frames.size()), 0); // bind poses (not skinned: zero)
    pack.Require(kBbp).ReplacePayload(bbp);
    DeathClip(mot);
    mot.tracks.insert(mot.tracks.end(), added.begin(), added.end());
    pack.Require(kMot).ReplacePayload(mot.BuildPayload());
    MotFile shadow = MotFile::FromRecord(pack.Require(kShadowMot));
    DeathClip(shadow);
    pack.Require(kShadowMot).ReplacePayload(shadow.BuildPayload());
    // The sheets in the look; the ball's pictures into the bank (e209ex's from a row-major IMG bank, swizzled).
    ImgBank   bank = ImgBank::Parse(pack.Require(kImg).Payload());
    SheetLook look = SheetLook::Parse(kCrystalGemronLook);
    for (auto &[name, block] : bank.entries) {
        if (name == kSheet) {
            block = look.ApplyToSheet(block);
        } else if (name == kSheet2) {
            block = look.ApplyBaseToSheet(block);
        }
    }
    ImgBank pictures = ImgBank::Parse(ball.Require("ball.img").Payload());
    bank.entries.insert(bank.entries.end(), pictures.entries.begin(), pictures.entries.end());
    ImgBank effect_pictures = ImgBank::Parse(ball.Require("e209ex.img").Payload());
    for (const auto &[name, block] : effect_pictures.entries) {
        bank.entries.emplace_back(name, tim8::Swizzled(block));
    }
    pack.Require(kImg).ReplacePayload(bank.Build());
    Cfg(pack);
    return pack.Rebuild();
}

} // namespace expanded

namespace expanded {

namespace {

constexpr std::uint32_t kOpPushConst = 3, kOpAdd = 6, kOpRet = 15, kOpCall = 19;
constexpr std::uint32_t kInt = 1, kFloat = 2;
constexpr float         kCryFrame = 214.0f;
constexpr std::uint32_t kSoundBase = 2501, kCryOffset = 11;
constexpr int           kShatterSound = 312; // the life sphere breaking, in the Ice Queen's arena set
constexpr float         kVanillaRange = 70.0f, kShotRange = 140.0f;
constexpr float         kVanillaWalkIn = 50.0f, kWalkIn = 100.0f;

float Float(std::span<const unsigned char> data, std::size_t at) {
    return GetF32(data, at);
}

void PutFloat(Bytes &data, std::size_t at, float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    Set32(data, at, bits);
}

// The header of a label's function: its first word the code's offset from the code base.
std::size_t Label(std::span<const unsigned char> stb, std::uint32_t id) {
    std::size_t table = Get32(stb, 0xC), count = Get32(stb, 0x10);
    for (std::size_t i = 0; i < count; i++) {
        if (Get32(stb, table + i * 8) == id) {
            return Get32(stb, table + i * 8 + 4);
        }
    }
    throw std::runtime_error(std::format("e115a.stb has no label {}", id));
}

} // namespace

Bytes BuildCrystalGemronScript(std::span<const unsigned char> stb) {
    std::size_t base = Get32(stb, 8);
    // The death wait cries at frame 214, which the vanilla clip never reaches: it sounds the shatter as the ball breaks.
    std::size_t death_call = base + Get32(stb, Label(stb, 120));
    if (Get32(stb, death_call) != kOpCall) {
        throw std::runtime_error("e115a.stb: label 120 does not start with a CALL");
    }
    std::size_t call = base + Get32(stb, base + Get32(stb, death_call + 8));
    while (Get32(stb, call) != kOpCall) {
        if (Get32(stb, call) == kOpRet) {
            throw std::runtime_error("e115a.stb: the death function has no CALL");
        }
        call += 12;
    }
    std::size_t push = call - 12, id = push - 24;
    if (Get32(stb, push) != kOpPushConst || Get32(stb, push + 4) != kFloat || Float(stb, push + 8) != kCryFrame ||
        Get32(stb, push - 36) != kOpPushConst || Get32(stb, push - 32) != kInt || Get32(stb, push - 28) != kSoundBase ||
        Get32(stb, push - 12) != kOpAdd || Get32(stb, id) != kOpPushConst || Get32(stb, id + 4) != kInt || Get32(stb, id + 8) != kCryOffset) {
        throw std::runtime_error("e115a.stb: the death wait is not the vanilla cry at frame 214");
    }
    Bytes out(stb.begin(), stb.end());
    PutFloat(out, push + 8, float(kShatterStart));
    Set32(out, id + 8, static_cast<std::uint32_t>(kShatterSound - int(kSoundBase)));
    // The AI loop (label 100) shoots within 140 and walks closer beyond it, to 100.
    std::size_t loop = base + Get32(stb, Label(stb, 100));
    int         ranges = 0, walks = 0;
    for (std::size_t c = loop; c + 12 <= stb.size() && Get32(stb, c) != kOpRet; c += 12) {
        if (Get32(stb, c) == kOpPushConst && Get32(stb, c + 4) == kFloat && Float(stb, c + 8) == kVanillaRange) {
            PutFloat(out, c + 8, kShotRange);
            ranges++;
        }
        if (c > loop && Get32(stb, c) == kOpCall && Get32(stb, c - 12) == kOpPushConst && Get32(stb, c - 8) == kFloat &&
            Float(stb, c - 4) == kVanillaWalkIn) {
            PutFloat(out, c - 4, kWalkIn);
            walks++;
        }
    }
    if (ranges != 3 || walks != 1) {
        throw std::runtime_error(std::format("e115a.stb: {} ranges and {} walks in the AI loop, expected 3 and 1", ranges, walks));
    }
    return out;
}

} // namespace expanded
