#include "expanded/data/toan_grip.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/rig_math.hpp"

namespace expanded {

namespace {

constexpr const char *kToanMds = "c01d.mds", *kToanBbp = "c01d.bbp", *kToanWgt = "c01d.wgt", *kToanCfg = "base.cfg";
constexpr const char *kSkinBuffer = "ALLOC_DBUFF \"skin\"";
constexpr float       kWhole = 100.0f; // a .wgt weight, in percent

const MdsFrame &Require(const std::vector<MdsFrame> &frames, const char *name) {
    const MdsFrame *frame = FindFrame(frames, name);
    if (frame == nullptr) {
        throw std::runtime_error(std::string(kToanMds) + " has no frame " + name);
    }
    return *frame;
}

// The mesh's triangles as position indices (strips with their degenerates dropped).
std::vector<std::array<int, 3>> Triangles(const MdtMesh &m) {
    std::vector<std::array<int, 3>> out;
    auto                            add = [&](const std::vector<int> &a, const std::vector<int> &b, const std::vector<int> &c) {
        if (a[0] != b[0] && b[0] != c[0] && a[0] != c[0]) {
            out.push_back({a[0], b[0], c[0]});
        }
    };
    for (const MdtMesh::Submesh &sub : m.submeshes) {
        const auto &r = sub.records;
        if (sub.prim == 3) {
            for (std::size_t k = 0; k + 2 < r.size(); k += 3) {
                add(r[k], r[k + 1], r[k + 2]);
            }
        } else if (sub.prim == 4) {
            for (std::size_t i = 0; i + 2 < r.size(); i++) {
                add(r[i], r[i + 1], r[i + 2]);
            }
        } else {
            throw std::runtime_error("an MDT primitive that is neither a list nor a strip");
        }
    }
    return out;
}

// A digit's vertices: those it starts from (touching the rest of the hand), and how many steps each lies from them.
struct Digit {
    std::set<int>      vertices;
    std::map<int, int> depth;
    std::set<int>      knuckle; // shared with the palm
};

void Measure(Digit &digit, const std::vector<std::array<int, 3>> &tris) {
    std::map<int, std::set<int>> next;
    std::queue<int>              open;
    for (const auto &t : tris) {
        for (int a : t) {
            for (int b : t) {
                if (a == b || !digit.vertices.contains(a)) {
                    continue;
                }
                if (digit.vertices.contains(b)) {
                    next[a].insert(b);
                } else if (!digit.depth.contains(a)) {
                    digit.depth[a] = 0;
                    open.push(a);
                }
            }
        }
    }
    while (!open.empty()) {
        int v = open.front();
        open.pop();
        for (int n : next[v]) {
            if (!digit.depth.contains(n)) {
                digit.depth[n] = digit.depth[v] + 1;
                open.push(n);
            }
        }
    }
    for (int v : digit.vertices) {
        digit.depth.try_emplace(v, 0);
    }
}

// The hand's digits: its fingers, index finger first, then its thumb.
std::vector<Digit> Digits(const MdtMesh &hand, const std::vector<std::array<int, 3>> &tris) {
    const auto        &p = hand.positions;
    std::vector<Digit> digits;
    std::set<int>      thumb, fingers;
    for (int v = 0; v < static_cast<int>(p.size()); v++) {
        const auto &q = p[std::size_t(v)];
        if (q[0] > kGripThumbFrom && q[2] < kGripThumbSide) {
            thumb.insert(v);
        } else if (q[0] >= kGripFingerCut) {
            fingers.insert(v);
        }
    }
    // The fingers: the pieces of what lies past the cut, joined by their triangles.
    std::map<int, int> group;
    auto               find = [&](int v) {
        while (group[v] != v) {
            v = group[v] = group[group[v]];
        }
        return v;
    };
    for (int v : fingers) {
        group[v] = v;
    }
    for (const auto &t : tris) {
        for (int a : t) {
            for (int b : t) {
                if (fingers.contains(a) && fingers.contains(b)) {
                    group[find(a)] = find(b);
                }
            }
        }
    }
    std::map<int, Digit> pieces;
    for (int v : fingers) {
        pieces[find(v)].vertices.insert(v);
    }
    for (auto &[root, digit] : pieces) {
        digits.push_back(std::move(digit));
    }
    auto least_across = [&](const Digit &d) {
        double z = 1e9;
        for (int v : d.vertices) {
            z = std::min(z, p[std::size_t(v)][2]);
        }
        return z;
    };
    std::ranges::sort(digits, {}, least_across);
    if (digits.size() != kGripDigits - 1) {
        throw std::runtime_error(std::string(kGripHand) + " does not part into four fingers");
    }
    // Each knuckle vertex to the finger nearest it across the hand.
    std::vector<double> across;
    for (const Digit &d : digits) {
        double z = 0.0;
        for (int v : d.vertices) {
            z += p[std::size_t(v)][2] / static_cast<double>(d.vertices.size());
        }
        across.push_back(z);
    }
    for (int v = 0; v < static_cast<int>(p.size()); v++) {
        const auto &q = p[std::size_t(v)];
        if (q[0] < kGripKnuckleFrom || q[0] >= kGripFingerCut || thumb.contains(v)) {
            continue;
        }
        std::size_t nearest = 0;
        for (std::size_t f = 1; f < across.size(); f++) {
            if (std::abs(q[2] - across[f]) < std::abs(q[2] - across[nearest])) {
                nearest = f;
            }
        }
        digits[nearest].knuckle.insert(v);
    }
    digits.push_back({thumb, {}, {}});
    for (Digit &d : digits) {
        Measure(d, tris);
    }
    return digits;
}

Vec3 Mean(const MdtMesh &hand, const Mat4 &local, const std::vector<int> &vertices) {
    Vec3 sum{};
    for (int v : vertices) {
        const auto &q = hand.positions[std::size_t(v)];
        Vec3        at = XformPt(local, {q[0], q[1], q[2]});
        for (int i = 0; i < 3; i++) {
            sum[std::size_t(i)] += at[std::size_t(i)] / static_cast<double>(vertices.size());
        }
    }
    return sum;
}

void AppendFloats(Bytes &out, std::initializer_list<double> values) {
    for (double v : values) {
        float         f = static_cast<float>(v);
        unsigned char b[4];
        std::memcpy(b, &f, 4);
        out.insert(out.end(), b, b + 4);
    }
}

} // namespace

Bytes BuildToanGrip(std::span<const unsigned char> toan) {
    ChrPack         pack = ChrPack::Parse(toan);
    Bytes           mds = pack.Require(kToanMds).Payload();
    auto            frames = ReadMdsSkeleton(mds);
    const MdsFrame &hand_frame = Require(frames, kGripHand), &wrist = Require(frames, kGripWrist);
    if (hand_frame.parent != wrist.index) {
        throw std::runtime_error(std::string(kGripHand) + " is not under " + kGripWrist);
    }
    MdtMesh hand = MdtMesh::Parse(MdtAt(mds, std::size_t(hand_frame.mesh)));
    auto    tris = Triangles(hand);
    auto    digits = Digits(hand, tris);

    // The skin takes a mesh's frame back out with a rigid inverse: the disc's hand frame is scaled along its rows, so the
    // scale goes into the vertices and the frame keeps its turn alone.
    Mat4 hand_local = hand_frame.local;
    for (std::size_t r = 0; r < 3; r++) {
        double length = std::sqrt(hand_local[r][0] * hand_local[r][0] + hand_local[r][1] * hand_local[r][1] + hand_local[r][2] * hand_local[r][2]);
        for (auto &q : hand.positions) {
            q[r] *= length;
        }
        for (std::size_t c = 0; c < 3; c++) {
            hand_local[r][c] /= length;
        }
    }
    mds = ReplaceMeshes(mds, {
                                 {kGripHand, hand.Build()}
    });
    std::size_t record = Get32(mds, mds::kTableAt) + std::size_t(hand_frame.index) * mds::kRecord + mds::kRotation;
    for (std::size_t r = 0; r < 3; r++) {
        for (std::size_t c = 0; c < 3; c++) {
            float f = static_cast<float>(hand_local[r][c]);
            std::memcpy(mds.data() + record + r * 16 + c * 4, &f, 4);
        }
    }

    // Each digit's joints, in the wrist's frame: its first where it meets the hand (the middle of its knuckle), each next
    // at the first ring of vertices of its section.
    const int                                           first = static_cast<int>(frames.size());
    std::vector<std::pair<Bytes, std::optional<Bytes>>> added;
    Bytes                                               bbp = pack.Require(kToanBbp).Payload();
    using Weights = std::vector<std::pair<int, float>>;
    std::vector<Weights> bones(kGripDigits * kGripJointsPerDigit);
    std::map<int, float> palm_share; // the palm's share of each vertex a digit takes
    for (int d = 0; d < kGripDigits; d++) {
        const Digit                  &digit = digits[std::size_t(d)];
        std::vector<std::vector<int>> rings(kGripJointsPerDigit);
        for (const auto &[v, depth] : digit.depth) {
            int section = std::min(depth, kGripJointsPerDigit - 1);
            bones[std::size_t(d * kGripJointsPerDigit + section)].emplace_back(v, kWhole);
            if (depth < kGripJointsPerDigit) {
                rings[std::size_t(depth)].push_back(v);
            }
            palm_share[v] = 0.0f;
        }
        for (int v : digit.knuckle) {
            bones[std::size_t(d * kGripJointsPerDigit)].emplace_back(v, kWhole / 2);
            palm_share[v] = kWhole / 2;
        }
        if (!digit.knuckle.empty()) {
            rings[0].assign(digit.knuckle.begin(), digit.knuckle.end());
        }
        Vec3 parent_at{};
        for (int k = 0; k < kGripJointsPerDigit; k++) {
            Vec3 at = rings[std::size_t(k)].empty() ? parent_at : Mean(hand, hand_local, rings[std::size_t(k)]);
            Vec3 local{at[0] - parent_at[0], at[1] - parent_at[1], at[2] - parent_at[2]};
            int  index = first + d * kGripJointsPerDigit + k;
            added.emplace_back(FrameRecord(index, kGripJoints[d][k], k == 0 ? wrist.index : index - 1, kIdentity, local), std::nullopt);
            AppendFloats(bbp, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, local[0], local[1], local[2], 1});
            parent_at = at;
        }
    }
    pack.Require(kToanMds).ReplacePayload(AppendMdsFrames(mds, added));
    pack.Require(kToanBbp).ReplacePayload(bbp);

    // The weights: the palm to the wrist (the mesh's parent, whose track starts its skin), each digit's rings to their
    // joints, a joint's track after its parent's.
    MotFile wgt = MotFile::FromRecord(pack.Require(kToanWgt));
    if (wgt.tracks.empty()) {
        throw std::runtime_error(std::string(kToanWgt) + " has no tracks");
    }
    const MotTrack &model = wgt.tracks.front();
    auto            track = [&](int bone, const Weights &vertices) {
        MotTrack t;
        t.w0 = static_cast<std::uint32_t>(hand_frame.index);
        t.w1 = static_cast<std::uint32_t>(bone);
        t.w2 = mot::kWeight;
        t.w3 = model.w3;
        t.w6 = model.w6;
        t.w7 = model.w7;
        for (const auto &[v, weight] : vertices) {
            t.keys.push_back(MotKeyframe::Make(static_cast<std::uint32_t>(v), {weight, 0.0, 0.0, 0.0}));
        }
        wgt.tracks.push_back(std::move(t));
    };
    Weights palm;
    for (int v = 0; v < static_cast<int>(hand.positions.size()); v++) {
        auto  shared = palm_share.find(v);
        float weight = shared == palm_share.end() ? kWhole : shared->second;
        if (weight > 0.0f) {
            palm.emplace_back(v, weight);
        }
    }
    track(wrist.index, palm);
    for (int b = 0; b < kGripDigits * kGripJointsPerDigit; b++) {
        track(first + b, bones[std::size_t(b)]);
    }
    pack.Require(kToanWgt).ReplacePayload(wgt.BuildPayload());

    // Skinned, the hand is drawn from a vertex buffer of its own, as Toan's body is.
    Bytes       cfg = pack.Require(kToanCfg).Payload();
    std::string text(cfg.begin(), cfg.end());
    std::size_t at = text.find(kSkinBuffer);
    if (at == std::string::npos) {
        throw std::runtime_error(std::string(kToanCfg) + " has no " + kSkinBuffer);
    }
    text.insert(at + std::strlen(kSkinBuffer), std::string("\r\nALLOC_DBUFF \"") + kGripHand + "\"");
    pack.Require(kToanCfg).ReplacePayload(Bytes(text.begin(), text.end()));
    return pack.Rebuild();
}

std::vector<DataBuild> ToanGripBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Toan's sword hand", std::string(kRebuiltDiscFolder) + kToanDungeonFile, {kToanDungeonFile},
                               [](Files s) { return BuildToanGrip(s[0]); }),
    };
}

} // namespace expanded
