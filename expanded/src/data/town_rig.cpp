#include "expanded/data/town_rig.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

#include "expanded/data/model_parts.hpp"

namespace expanded::town {

namespace {

// A step of the few cfg patterns: a literal, [ \t]+, [ \t]*, "([^"]+)" (kept or skipped).
struct Step {
    enum Kind {
        Literal,
        Blanks,
        MaybeBlanks,
        Quoted,
        SkipQuoted
    } kind;

    std::string_view literal = {};
};

bool Blank(char c) {
    return c == ' ' || c == '\t';
}

// The capture of the leftmost match of steps in text, as the bake's regexes find it.
std::optional<std::string> Search(std::string_view text, std::initializer_list<Step> steps) {
    for (std::size_t start = 0; start < text.size(); start++) {
        std::size_t                p = start;
        std::optional<std::string> capture;
        bool                       ok = true;
        for (const Step &step : steps) {
            switch (step.kind) {
                case Step::Literal:
                    ok = text.substr(p).starts_with(step.literal);
                    p += step.literal.size();
                    break;
                case Step::Blanks:
                case Step::MaybeBlanks: {
                    std::size_t from = p;
                    while (p < text.size() && Blank(text[p])) {
                        p++;
                    }
                    ok = step.kind == Step::MaybeBlanks || p > from;
                    break;
                }
                case Step::Quoted:
                case Step::SkipQuoted: {
                    if (p >= text.size() || text[p] != '"') {
                        ok = false;
                        break;
                    }
                    std::size_t close = text.find('"', p + 1);
                    if (close == std::string_view::npos || close == p + 1) {
                        ok = false;
                        break;
                    }
                    if (step.kind == Step::Quoted) {
                        capture = std::string(text.substr(p + 1, close - p - 1));
                    }
                    p = close + 1;
                    break;
                }
            }
            if (!ok) {
                break;
            }
        }
        if (ok) {
            return capture;
        }
    }
    return std::nullopt;
}

} // namespace

std::size_t IndexOf(std::string_view text, std::string_view what, std::size_t from) {
    std::size_t at = text.find(what, from);
    if (at == std::string_view::npos) {
        Bad(std::format("cfg lacks \"{}\"", what));
    }
    return at;
}

CfgMotions CfgMotionsOf(const ChrPack &pack, std::string_view prefer) {
    std::vector<std::size_t> candidates;
    for (std::size_t i = 0; i < pack.records.size(); i++) {
        if (EndsFolded(pack.records[i].name, ".cfg")) {
            candidates.push_back(i);
        }
    }
    if (!prefer.empty()) {
        std::stable_partition(candidates.begin(), candidates.end(),
                              [&](std::size_t i) { return EqualsFolded(pack.records[i].name, prefer); });
    }
    if (candidates.empty()) {
        Bad("the pack has no cfg");
    }
    CfgMotions result{.cfg = candidates[0]};
    for (std::size_t i : candidates) {
        std::string text = ToText(pack.records[i].Payload());
        if (text.contains("KEY_START") && text.contains("MOTION")) {
            result.cfg = i;
            break;
        }
    }
    std::string text = ToText(pack.records[result.cfg].Payload());
    using enum Step::Kind;
    auto body_mot = Search(text, {
                                     {Literal, "MOTION"},
                                     {Blanks},
                                     {Literal, "0,"},
                                     {Blanks},
                                     {Quoted}
    });
    if (!body_mot) {
        body_mot = Search(text, {
                                    {Literal, "MOTION"},
                                    {Blanks},
                                    {Literal, "0,"},
                                    {MaybeBlanks},
                                    {Quoted}
        });
    }
    auto body_mds = Search(text, {
                                     {Literal, "MODEL"},
                                     {Blanks},
                                     {Quoted}
    });
    if (!body_mot || !body_mds) {
        Bad(std::format("{} names no body model and motion", pack.records[result.cfg].name));
    }
    result.body_mot = *body_mot;
    result.body_mds = *body_mds;
    result.shadow_mot = Search(text, {
                                         {Literal, "SHADOW_MOTION"},
                                         {Blanks},
                                         {Quoted}
    });
    result.shadow_mds = Search(text, {
                                         {Literal, "SHADOW_MODEL"},
                                         {Blanks},
                                         {Quoted}
    });
    return result;
}

std::string CfgBbp(const ChrRecord &cfg) {
    using enum Step::Kind;
    auto bbp = Search(ToText(cfg.Payload()), {
                                                 {Literal, "MOTION"},
                                                 {Blanks},
                                                 {Literal, "0,"},
                                                 {MaybeBlanks},
                                                 {SkipQuoted},
                                                 {Literal, ","},
                                                 {MaybeBlanks},
                                                 {Quoted}
    });
    if (!bbp) {
        Bad(std::format("{} names no body .bbp", cfg.name));
    }
    return *bbp;
}

Vec4 QuatMul(const Vec4 &a, const Vec4 &b) {
    return {
        a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
        a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
        a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
        a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0],
    };
}

Vec4 MatToQuat(const Mat3 &r) {
    double m00 = r[0][0], m01 = r[0][1], m02 = r[0][2], m10 = r[1][0], m11 = r[1][1], m12 = r[1][2], m20 = r[2][0],
           m21 = r[2][1], m22 = r[2][2];
    double tr = m00 + m11 + m22, s;
    if (tr > 0) {
        s = std::sqrt(tr + 1.0) * 2;
        return {0.25 * s, (m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s};
    }
    if (m00 >= m11 && m00 >= m22) {
        s = std::sqrt(1.0 + m00 - m11 - m22) * 2;
        return {(m21 - m12) / s, 0.25 * s, (m01 + m10) / s, (m02 + m20) / s};
    }
    if (m11 >= m22) {
        s = std::sqrt(1.0 + m11 - m00 - m22) * 2;
        return {(m02 - m20) / s, (m01 + m10) / s, 0.25 * s, (m12 + m21) / s};
    }
    s = std::sqrt(1.0 + m22 - m00 - m11) * 2;
    return {(m10 - m01) / s, (m02 + m20) / s, (m12 + m21) / s, 0.25 * s};
}

Bind MdsBind(std::span<const unsigned char> mds, int node) {
    std::size_t p = mds::kTable + static_cast<std::size_t>(node) * mds::kRecord;
    double      m[16];
    for (int k = 0; k < 16; k++) {
        m[k] = GetF32(mds, p + mds::kRotation + k * 4);
    }
    return {
        static_cast<int>(Get32(mds, p + mds::kParent)),
        {{{m[0], m[1], m[2]}, {m[4], m[5], m[6]}, {m[8], m[9], m[10]}}},
        {m[12], m[13], m[14]}
    };
}

double Dot(const Vec4 &a, const Vec4 &b) {
    double sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

bool InWindow(const MotKeyframe &key, int lo, int hi) {
    return lo <= static_cast<std::int64_t>(key.Frame()) && key.Frame() <= static_cast<std::int64_t>(hi);
}

int Index(const std::vector<std::string> &names, std::string_view name) {
    auto found = std::find(names.begin(), names.end(), name);
    return found == names.end() ? -1 : static_cast<int>(found - names.begin());
}

std::vector<Span> FoldSpans(std::vector<Span> windows, int lo, int hi) {
    std::stable_sort(windows.begin(), windows.end());
    std::vector<Span> spans;
    int               cur = lo;
    for (auto [a, b] : windows) {
        if (cur < a) {
            spans.push_back({cur, a - 1});
        }
        cur = std::max(cur, b + 1);
    }
    if (cur <= hi) {
        spans.push_back({cur, hi});
    }
    return spans;
}

std::vector<int> SpanEdges(const std::vector<Span> &spans) {
    std::vector<int> edges;
    for (auto [lo, hi] : spans) {
        edges.push_back(lo);
        if (hi != lo) {
            edges.push_back(hi);
        }
    }
    return edges;
}

std::optional<Vec4> SampleTrack(const MotTrack &track, int frame) {
    const MotKeyframe *prev = nullptr;
    const MotKeyframe *next = nullptr;
    for (const MotKeyframe &key : track.keys) {
        std::int64_t at = key.Frame();
        if (at <= frame && (!prev || key.Frame() > prev->Frame())) {
            prev = &key;
        }
        if (at >= frame && (!next || key.Frame() < next->Frame())) {
            next = &key;
        }
    }
    if (!prev && !next) {
        return std::nullopt;
    }
    if (!prev) {
        return Wide(next->Values());
    }
    if (!next || next->Frame() == prev->Frame()) {
        return Wide(prev->Values());
    }
    double u = static_cast<double>(frame - static_cast<std::int64_t>(prev->Frame())) /
               static_cast<double>(next->Frame() - prev->Frame());
    Vec4   a = Wide(prev->Values());
    Vec4   b = Wide(next->Values());
    Vec4   v;
    if (track.w2 == mot::kRotation) {
        if (Dot(a, b) < 0) {
            for (double &x : b) {
                x = -x;
            }
        }
        for (int i = 0; i < 4; i++) {
            v[i] = a[i] + (b[i] - a[i]) * u;
        }
        double n = std::sqrt(Dot(v, v));
        if (n == 0) {
            n = 1.0;
        }
        for (double &x : v) {
            x /= n;
        }
        return v;
    }
    for (int i = 0; i < 4; i++) {
        v[i] = a[i] + (b[i] - a[i]) * u;
    }
    return v;
}

void ReplaceMot(ChrPack &pack, std::string_view mot_name, const MotFile &mot) {
    pack.Require(mot_name).ReplacePayload(mot.BuildPayload());
}

MotFile MotOf(const ChrPack &pack, std::string_view name) {
    return MotFile::FromRecord(pack.Require(name));
}

std::vector<std::string> FramesOf(const ChrPack &pack, std::string_view mds) {
    return ReadMdsFrames(pack.Require(mds).Payload());
}

std::string Joined(const std::vector<std::string> &names) {
    std::string out;
    for (const std::string &name : names) {
        out += (out.empty() ? "" : ", ") + name;
    }
    return out;
}

} // namespace expanded::town
