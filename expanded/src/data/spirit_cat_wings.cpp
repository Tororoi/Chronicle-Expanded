#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>

#include "expanded/data/spirit_cat_parts.hpp"

namespace expanded::spirit_cat {

using namespace rig;
using rig::Cross;
using rig::Unit;

namespace {

// The wing graft's knobs, as the mod has them.
constexpr const char                       *kWingBones[] = {"r_wing1", "r_wing2", "r_wing3", "r_wing4", "l_wing1", "l_wing2", "l_wing3", "l_wing4"};
constexpr const char                       *kWingAttachNode = "cat_sebone2";
constexpr double                            kWingScaleMul = 0.5, kWingSizeMul = 0.7;
constexpr V3                                kWingAttachBind = {0.0, 3.4, 1.7};
constexpr double                            kWingRaise = 0.2, kWingYaw = 0.0, kWingPitchDeg = 8.0;
constexpr double                            kWingExtendXIn = 0.20, kWingExtendXOut = 0.95;
constexpr double                            kWingTabTiltDeg = 15.0;
constexpr double                            kWingExtendZBack = 1.1, kWingExtendZFront = 2.03, kWingExtendZFrontOut = 1.85;
constexpr double                            kWingExtendLift = 0.1;
const std::vector<std::vector<std::string>> kWingTabPolys = {
    {"Rb", "Ri", "ir"},
    {"Ri", "if", "ir"},
    {"RIM", "if"},
    {"Rg", "Rf", "if", "under"}
};
constexpr double                 kApexPull = 0.30, kApexReach = 0.6;
constexpr double                 kRootFwd = 0.15, kRootZMax = 0.8, kRootXMax = 0.85, kRootXFull = 0.45;
constexpr std::pair<int, double> kWingFoldLift[] = {
    {28, 0.25},
    {24, 0.15},
    {29, 0.10},
    {19, 0.05}
};
constexpr int    kWingTabSubdiv = 2;
constexpr double kWingTabBulge = 0.2;

struct WingClip {
    int    ki, ds, de;
    double dspd;
    bool   loop;
};

constexpr WingClip kWingClips[] = {
    {4, 200, 205, 0.2,  true },
    {7, 295, 300, 0.15, false}
};
constexpr int                       kLandCs = 215, kLandCe = 227, kLandDs = 70, kLandDe = 75, kLandFe = 220;
constexpr std::pair<double, double> kWingLandLag[] = {
    {220.0, 225.0},
    {220.5, 226.0},
    {221.0, 226.5},
    {221.5, 227.0}
};
constexpr int kWingFoldFrame = 15;

struct Fold {
    V3 span, top;
};

constexpr Fold kWingFold[] = {
    {{-0.08, -0.96, -0.27}, {-0.83, 0.38, 0.10}},
    {{-0.05, 0.35, -0.94},  {-1.00, 0.00, 0.00}},
    {{0.03, 0.20, -0.98},   {-0.90, 0.42, 0.00}},
    {{0.10, 0.15, -0.98},   {-0.90, 0.42, 0.00}},
};
constexpr V3                     kWingFoldRootShift = {-0.43, 0.08, 0.0};
constexpr std::pair<int, double> kWingHandDroop[] = {
    {1, -10.0},
    {2, 8.0  },
    {3, -2.0 }
};
constexpr std::pair<double, double> kWingHandDroopFrames = {224, 227};
constexpr std::pair<int, double>    kWingHandRoll[] = {
    {2, 6.0 },
    {3, 10.0}
};
constexpr std::pair<double, double> kWingHandRollFrames = {224, 227};
constexpr std::pair<double, double> kStrokePhi[] = {
    {285.0, 78.0 },
    {287.5, 100.0},
    {288.5, 100.0},
    {292.0, -20.0},
    {294.0, 0.0  }
};
constexpr std::pair<double, double> kStrokeExtend[] = {
    {285.0, 0.0},
    {287.5, 1.0}
};
constexpr double                 kStrokeLag = 0.2, kStrokeSweep = 60.0, kStrokeHingeIn = 0.25;
constexpr int                    kStrokeKi = 7;
constexpr std::pair<int, double> kWingHold[] = {
    {8, 285.0}
};
constexpr std::pair<double, double> kClipPitch7[] = {
    {285.0, 30.0},
    {292.0, 15.0},
    {294.0, 10.0}
};
constexpr int kWingLevelAt = 4;

using Edge = std::pair<int, int>;

Edge MakeEdge(int x, int y) {
    return {std::min(x, y), std::max(x, y)};
}

V3 Add3(const V3 &a, const V3 &b) {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

struct Local {
    M3 r;
    V3 t;
};

// The two bone influences of a set of vertices pooled with weights, as the mod pools a new vertex from its ends.
void Pool(SkinMesh &me, const std::vector<std::pair<int, double>> &ends) {
    std::vector<int>                     order;
    std::map<int, std::pair<double, V3>> acc;
    for (auto [vi, weight] : ends) {
        std::size_t k = std::size_t(vi);
        for (auto [b, w, pl] : {
                 std::tuple{me.b0[k], me.w0[k],       me.p0[k]},
                 std::tuple{me.b1[k], 1.0 - me.w0[k], me.p1[k]}
        }) {
            if (w <= 1e-6) {
                continue;
            }
            if (!acc.contains(b)) {
                acc[b] = {
                    0.0, {0.0, 0.0, 0.0}
                };
                order.push_back(b);
            }
            auto &a = acc[b];
            a.first += weight * w;
            for (std::size_t c = 0; c < 3; c++) {
                a.second[c] += weight * w * pl[c];
            }
        }
    }
    std::vector<std::pair<int, std::pair<double, V3>>> infl;
    for (int b : order) {
        infl.emplace_back(b, acc[b]);
    }
    std::stable_sort(infl.begin(), infl.end(), [](const auto &a, const auto &b) { return -a.second.first < -b.second.first; });
    if (infl.size() > 2) {
        infl.resize(2);
    }
    std::vector<double> ws;
    for (const auto &x : infl) {
        ws.push_back(x.second.first);
    }
    double tot = Sum(ws);
    auto [ba, aa] = infl[0];
    auto [bb, ab] = infl.size() > 1 ? infl[1] : infl[0];
    me.nv++;
    me.b0.push_back(ba);
    me.p0.push_back({aa.second[0] / aa.first, aa.second[1] / aa.first, aa.second[2] / aa.first});
    me.w0.push_back(aa.first / tot);
    me.b1.push_back(bb);
    me.p1.push_back({ab.second[0] / ab.first, ab.second[1] / ab.first, ab.second[2] / ab.first});
}

class Graft {
public:
    Graft(const std::vector<RigNode> &cat_nodes, const Bytes &cat_mds, const ChrPack &cat_pack, const std::vector<RigNode> &dran_nodes,
          std::span<const unsigned char> dran_mds, const ChrPack &dran_pack, const std::string &dran_mot, const Log &log)
        : cat_nodes_(cat_nodes), cat_mds_(cat_mds), dran_nodes_(dran_nodes), log_(log) {
        for (const RigNode &n : dran_nodes) {
            dn_.emplace(n.name, &n);
        }
        for (const RigNode &n : cat_nodes) {
            cn_.emplace(n.name, &n);
        }
        cat_tracks_ = BuildTracks(cat_pack, "cat.mot", static_cast<int>(cat_nodes.size()));
        for (const Track &t : cat_tracks_) {
            ctr_[{t.node, t.chan}] = &t;
        }
        dran_tracks_ = BuildTracks(dran_pack, dran_mot, static_cast<int>(dran_nodes.size()));
        for (const Track &t : dran_tracks_) {
            dtracks_[{t.node, t.chan}] = &t;
        }
        weights_all_ = LoadWeights(dran_pack, "c12a.wgt").at(Dn("obj1").i);
        dran_pack_ = &dran_pack;
        dran_mds_ = dran_mds;
        cat_pack_ = &cat_pack;
    }

    WingData Build();

private:
    const RigNode &Dn(const std::string &name) const { return *dn_.at(name); }

    const RigNode &Cn(const std::string &name) const { return *cn_.at(name); }

    std::vector<M4> CatWorld(double frame) const {
        std::vector<M4> w(cat_nodes_.size());
        for (const RigNode &n : cat_nodes_) {
            auto q_track = ctr_.find({n.i, 0});
            auto t_track = ctr_.find({n.i, 2});
            V4   q = q_track != ctr_.end() ? Sample(*q_track->second, frame) : n.quat;
            V3   t = n.t;
            if (t_track != ctr_.end()) {
                V4 s = Sample(*t_track->second, frame);
                t = {s[0], s[1], s[2]};
            }
            M4 local = MatFromRT(QuatToMat(q), t);
            w[std::size_t(n.i)] = n.parent < 0 ? local : MatMul(local, w[std::size_t(n.parent)]);
        }
        return w;
    }

    Local LocalOf(const M3 &rw, const V3 &td) const {
        V3 rel = {td[0] - center_[0], td[1] - center_[1], td[2] - center_[2]};
        rel = XformPt(droot4_, rel);
        rel = Pitch(RotY(rel, kWingYaw), pr_);
        for (double &c : rel) {
            c = c * s_;
        }
        Local l;
        l.r = Mul3(rw, r_ref_t_);
        for (std::size_t i = 0; i < 3; i++) {
            l.t[i] = anchor_local_[i] + Sum({rel[0] * r_ref_t_[0][i], rel[1] * r_ref_t_[1][i], rel[2] * r_ref_t_[2][i]});
        }
        return l;
    }

    M3 WorldR(const M3 &rd) const { return Mul3(Mul3(Mul3(rd, droot_r_), yaw_r_), pitch_r_); }

    M3 DranLocalR(const std::string &name, std::optional<double> df) const {
        const RigNode &d = Dn(name);
        auto           src = dtracks_.find({d.i, 0});
        M3             rw = (df && src != dtracks_.end()) ? WorldR(QuatToMat(Sample(*src->second, *df))) : WorldR(d.r);
        return LocalOf(rw, d.t).r;
    }

    std::vector<V3> FkLocals(const std::string &sd, const std::vector<M3> &rls, std::optional<V3> root = std::nullopt) const {
        std::vector<V3> t = {root ? *root : pivot_local_.at(sd)};
        const auto     &seg = seg_len_.at(sd);
        for (std::size_t k = 0; k < 3; k++) {
            t.push_back({t[k][0] + seg[k] * rls[k][0][0], t[k][1] + seg[k] * rls[k][0][1], t[k][2] + seg[k] * rls[k][0][2]});
        }
        return t;
    }

    static M3 Basis(V3 span, V3 top) {
        span = Unit(span);
        double d = Sum3(top, span);
        top = Unit({top[0] - d * span[0], top[1] - d * span[1], top[2] - d * span[2]});
        return {span, top, Cross(span, top)};
    }

    const std::vector<std::string> &Chain(const std::string &sd) const { return sd == "r" ? right_ : left_; }

    int Wid(const std::string &name) const { return wid_.at(Dn(name).i); }

    std::map<int, Local>             WingLocals(double f);
    std::pair<std::map<int, M4>, V3> WingPose(double f);

    const std::vector<RigNode>    &cat_nodes_;
    const Bytes                   &cat_mds_;
    const std::vector<RigNode>    &dran_nodes_;
    const Log                     &log_;
    const ChrPack                 *dran_pack_ = nullptr;
    const ChrPack                 *cat_pack_ = nullptr;
    std::span<const unsigned char> dran_mds_;

    std::map<std::string, const RigNode *>       dn_, cn_;
    std::vector<Track>                           cat_tracks_, dran_tracks_;
    std::map<std::pair<int, int>, const Track *> ctr_, dtracks_;
    VertexWeights                                weights_all_;

    std::vector<RigNode>                       nodes_;
    int                                        parent_ = 0, base_ = 0;
    V3                                         center_{}, anchor_local_{};
    double                                     s_ = 0, pr_ = 0;
    M3                                         r_ref_t_{}, droot_r_{}, yaw_r_{}, pitch_r_{};
    M4                                         droot4_{};
    std::map<int, int>                         wid_;
    std::map<std::string, std::vector<double>> seg_len_;
    std::map<std::string, V3>                  pivot_local_, fold_root_;
    std::map<std::string, std::vector<M3>>     fold_rls_;
    std::map<std::string, std::vector<V3>>     fold_tls_;
    const std::vector<std::string>             right_ = {"r_wing1", "r_wing2", "r_wing3", "r_wing4"};
    const std::vector<std::string>             left_ = {"l_wing1", "l_wing2", "l_wing3", "l_wing4"};

    struct Window {
        int  cs, ce, ds, de, cycles;
        bool loop;
        int  ki;
    };

    std::vector<Window>                    windows_;
    std::map<std::string, std::vector<M3>> idle_rots_;
    std::map<std::string, double>          flap_sign_, sweep_sign_;
};

std::map<int, Local> Graft::WingLocals(double f) {
    for (auto [ki, src] : kWingHold) {
        const CatKeyWindow &hold = kCatKeys[ki];
        if (hold.start <= f && f <= hold.end) {
            return WingLocals(src);
        }
    }
    std::map<int, Local> out;
    const V3             fa = {1.0, 0.0, 0.0}, va = {0.0, 1.0, 0.0};
    for (const std::string sd : {"r", "l"}) {
        const auto           &ch = Chain(sd);
        std::optional<double> df;
        int                   wki = -1;
        for (const Window &w : windows_) {
            if (w.cs <= f && f <= w.ce) {
                double u0 = (f - w.cs) / double(w.ce - w.cs);
                df = w.ds + (w.loop ? std::fmod(u0 * w.cycles * (w.de - w.ds), double(w.de - w.ds)) : u0 * (w.de - w.ds));
                wki = w.ki;
            }
        }
        std::optional<V3> root;
        std::vector<M3>   rls;
        if (df) {
            if (wki == kStrokeKi) {
                const Window &w7 = *std::find_if(windows_.begin(), windows_.end(), [&](const Window &w) { return w.ki == wki; });
                double        u = (f - w7.cs) / double(w7.ce - w7.cs);
                if (!idle_rots_.contains(sd)) {
                    auto idle = WingLocals(kWingFoldFrame);
                    for (const std::string &name : ch) {
                        idle_rots_[sd].push_back(idle.at(Wid(name)).r);
                    }
                }
                std::vector<M3> stroke;
                {
                    const V3 &piv = pivot_local_.at(sd);
                    double    hz = kStrokeHingeIn * (piv[2] < 0 ? 1.0 : -1.0);
                    V3        c = {piv[0], piv[1], piv[2] + hz}, arm = {piv[0] - c[0], piv[1] - c[1], piv[2] - c[2]};
                    M3        q = RotAbout(fa, KeyedVal(kStrokePhi, f) * flap_sign_.at(sd));
                    V3        r;
                    for (std::size_t i = 0; i < 3; i++) {
                        r[i] = c[i] + Sum({arm[0] * q[0][i], arm[1] * q[1][i], arm[2] * q[2][i]});
                    }
                    root = r;
                }
                for (std::size_t k = 0; k < 4; k++) {
                    double uk = std::pow(u, 1.0 + double(k) * kStrokeLag), fk = w7.cs + uk * (w7.ce - w7.cs);
                    M3     r = Mul3(DranLocalR(ch[k], double(w7.de)), RotAbout(fa, KeyedVal(kStrokePhi, fk) * flap_sign_.at(sd)));
                    double psi = kStrokeSweep * (u - uk);
                    if (std::fabs(psi) > 1e-6) {
                        r = Mul3(r, RotAbout(va, psi * sweep_sign_.at(sd)));
                    }
                    stroke.push_back(r);
                    if (k == 0) {
                        rls.push_back(r);
                        continue;
                    }
                    double e = KeyedVal(kStrokeExtend, fk);
                    M3     rel_s = Mul3(stroke[k], T3(stroke[k - 1]));
                    M3     rel_i = Mul3(idle_rots_.at(sd)[k], T3(idle_rots_.at(sd)[k - 1]));
                    M3     rel = e >= 1 ? rel_s : e <= 0 ? rel_i
                                                         : QuatToMat(Slerp(MatToQuat(rel_i), MatToQuat(rel_s), e));
                    rls.push_back(Mul3(rel, rls[k - 1]));
                }
            } else {
                for (const std::string &name : ch) {
                    rls.push_back(DranLocalR(name, df));
                }
            }
            if (wki == 7) {
                double deg = f <= kClipPitch7[0].first ? kClipPitch7[0].second : std::end(kClipPitch7)[-1].second;
                for (std::size_t i = 0; i + 1 < std::size(kClipPitch7); i++) {
                    if (kClipPitch7[i].first <= f && f <= kClipPitch7[i + 1].first) {
                        deg = kClipPitch7[i].second + (kClipPitch7[i + 1].second - kClipPitch7[i].second) *
                                                          Smooth((f - kClipPitch7[i].first) / (kClipPitch7[i + 1].first - kClipPitch7[i].first));
                    }
                }
                if (std::fabs(deg) > 1e-6) {
                    double th = Radians(deg), c = std::cos(th), sn = std::sin(th);
                    M3     qz = {
                        {{c, sn, 0.0}, {-sn, c, 0.0}, {0.0, 0.0, 1.0}}
                    };
                    for (M3 &r : rls) {
                        r = Mul3(r, qz);
                    }
                }
            }
        } else if (kLandCs <= f && f <= kLandCe) {
            double dfl = kLandDs + (kLandDe - kLandDs) * std::min(1.0, (f - kLandCs) / double(kLandFe - kLandCs));
            for (std::size_t k = 0; k < 4; k++) {
                M3 rf = DranLocalR(ch[k], dfl), rt = fold_rls_.at(sd)[k];
                auto [a, b] = kWingLandLag[k];
                double t = Smooth((f - a) / (b - a));
                rls.push_back(t <= 0 ? rf : t >= 1 ? rt
                                                   : QuatToMat(Slerp(MatToQuat(rf), MatToQuat(rt), t)));
            }
            auto [a, b] = kWingLandLag[0];
            double    t = Smooth((f - a) / (b - a));
            const V3 &piv = pivot_local_.at(sd), &fr = fold_root_.at(sd);
            root = V3{piv[0] + (fr[0] - piv[0]) * t, piv[1] + (fr[1] - piv[1]) * t, piv[2] + (fr[2] - piv[2]) * t};
        } else {
            rls = fold_rls_.at(sd);
            root = fold_root_.at(sd);
        }
        bool landing = kLandCs <= f && f <= kLandCe;
        if (landing || !df) {
            auto [a, b] = kWingHandDroopFrames;
            double s = (!df && !landing) ? 1.0 : Smooth((f - a) / (b - a));
            if (s > 0) {
                for (auto [k, deg] : kWingHandDroop) {
                    double th = Radians(deg * s), c = std::cos(th), sn = std::sin(th);
                    M3     qz = {
                        {{c, sn, 0.0}, {-sn, c, 0.0}, {0.0, 0.0, 1.0}}
                    };
                    rls[std::size_t(k)] = Mul3(rls[std::size_t(k)], qz);
                }
            }
        }
        if (landing || !df) {
            auto [a, b] = kWingHandRollFrames;
            double s = (!df && !landing) ? 1.0 : Smooth((f - a) / (b - a));
            if (s > 0) {
                double sgn = sd == "r" ? 1.0 : -1.0;
                for (auto [k, deg] : kWingHandRoll) {
                    double th = Radians(deg * s * sgn), c = std::cos(th), sn = std::sin(th);
                    M3     rx = {
                        {{1.0, 0.0, 0.0}, {0.0, c, sn}, {0.0, -sn, c}}
                    };
                    rls[std::size_t(k)] = Mul3(rx, rls[std::size_t(k)]);
                }
            }
        }
        std::vector<V3> tls = FkLocals(sd, rls, root);
        for (std::size_t k = 0; k < 4; k++) {
            out[Wid(ch[k])] = {rls[k], tls[k]};
        }
    }
    return out;
}

std::pair<std::map<int, M4>, V3> Graft::WingPose(double f) {
    M4                pw = CatWorld(f)[std::size_t(parent_)];
    std::map<int, M4> w;
    for (const auto &[id, local] : WingLocals(f)) {
        w[id] = MatMul(MatFromRT(local.r, local.t), pw);
    }
    return {w, XformPt(pw, anchor_local_)};
}

WingData Graft::Build() {
    const V3 r1 = Dn("r_wing1").t, l1 = Dn("l_wing1").t, r4 = Dn("r_wing4").t;
    center_ = {(r1[0] + l1[0]) / 2, (r1[1] + l1[1]) / 2, (r1[2] + l1[2]) / 2};
    double         wing_len = Dist(r1, r4);
    const RigNode &skin = Cn("cat_skin");
    MdtMesh        sm = MdtMesh::Parse(std::span<const unsigned char>(cat_mds_).subspan(std::size_t(skin.mesh_off)));
    double         lo[3] = {INFINITY, INFINITY, INFINITY}, hi[3] = {-INFINITY, -INFINITY, -INFINITY};
    for (const auto &v : sm.positions) {
        V3 p = XformPt(skin.world, {v[0], v[1], v[2]});
        for (std::size_t k = 0; k < 3; k++) {
            lo[k] = std::min(lo[k], p[k]);
            hi[k] = std::max(hi[k], p[k]);
        }
    }
    double ext[3] = {hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]};
    double body_len = std::max(ext[0], ext[2]);
    s_ = body_len / wing_len * kWingScaleMul;
    log_(std::format("wing graft: cat body {:.1f} long x {:.1f} high; Dran wing {:.1f} -> scale {:.3f} x size {}", body_len, ext[1], wing_len, s_, kWingSizeMul));
    nodes_ = cat_nodes_;
    parent_ = Cn(kWingAttachNode).i;
    M4                  root_inv = nodes_[std::size_t(parent_)].inv_world;
    const CatKeyWindow &level = kCatKeys[kWingLevelAt];
    int                 ref_frame = (level.start + level.end) / 2;
    M4                  p_ref = CatWorld(ref_frame)[std::size_t(parent_)];
    M3                  r_ref = RotOf(p_ref);
    V3                  attach = {kWingAttachBind[0], kWingAttachBind[1] + kWingRaise, kWingAttachBind[2]};
    anchor_local_ = XformPt(root_inv, attach);
    droot_r_ = dran_nodes_[0].r;
    pr_ = Radians(kWingPitchDeg);
    yaw_r_ = {
        {{std::cos(kWingYaw), 0.0, std::sin(kWingYaw)}, {0.0, 1.0, 0.0}, {-std::sin(kWingYaw), 0.0, std::cos(kWingYaw)}}
    };
    pitch_r_ = {
        {{1.0, 0.0, 0.0}, {0.0, std::cos(pr_), -std::sin(pr_)}, {0.0, std::sin(pr_), std::cos(pr_)}}
    };
    droot4_ = {
        {{droot_r_[0][0], droot_r_[0][1], droot_r_[0][2], 0.0},
         {droot_r_[1][0], droot_r_[1][1], droot_r_[1][2], 0.0},
         {droot_r_[2][0], droot_r_[2][1], droot_r_[2][2], 0.0},
         {0.0, 0.0, 0.0, 1.0}}
    };
    r_ref_t_ = T3(r_ref);

    const RigNode  &obj1 = Dn("obj1");
    MdtMesh         om = MdtMesh::Parse(dran_mds_.subspan(std::size_t(obj1.mesh_off)));
    std::vector<V3> vw_all;
    for (const auto &v : om.positions) {
        vw_all.push_back(XformPt(obj1.world, {v[0], v[1], v[2]}));
    }
    base_ = static_cast<int>(nodes_.size());
    for (int k = 0; k < 8; k++) {
        const RigNode &d = Dn(kWingBones[k]);
        Local          l = LocalOf(WorldR(d.r), d.t);
        RigNode        n;
        n.i = base_ + k;
        n.name = kWingBones[k];
        n.mesh_off = 0;
        n.parent = parent_;
        n.r = l.r;
        n.t = l.t;
        n.quat = MatToQuat(l.r);
        n.world = MatMul(MatFromRT(l.r, l.t), nodes_[std::size_t(parent_)].world);
        n.inv_world = RigidInv(n.world);
        n.world_pos = {n.world[3][0], n.world[3][1], n.world[3][2]};
        nodes_.push_back(n);
        wid_[d.i] = base_ + k;
    }
    std::map<int, WingData::BindOpen> bind_open;
    for (std::size_t k = std::size_t(base_); k < nodes_.size(); k++) {
        bind_open[nodes_[k].i] = {nodes_[k].r, nodes_[k].t, nodes_[k].world};
    }
    for (const std::string sd : {"r", "l"}) {
        const auto &ch = Chain(sd);
        for (std::size_t i = 0; i < 3; i++) {
            seg_len_[sd].push_back(Dist(Dn(ch[i]).t, Dn(ch[i + 1]).t) * s_ * kWingSizeMul);
        }
        pivot_local_[sd] = nodes_[std::size_t(Wid(ch[0]))].t;
    }
    int loop_start = INT32_MAX;
    for (const WingClip &c : kWingClips) {
        if (c.loop) {
            loop_start = std::min(loop_start, c.ds);
        }
    }
    M3 r_stand = RotOf(CatWorld(kWingFoldFrame)[std::size_t(parent_)]), r_stand_t = T3(r_stand);
    for (const std::string sd : {"r", "l"}) {
        const auto     &ch = Chain(sd);
        double          sx = sd == "l" ? -1.0 : 1.0;
        std::vector<M3> rls;
        for (std::size_t k = 0; k < 4; k++) {
            const std::string &name = ch[k];
            M3                 rf = DranLocalR(name, double(loop_start));
            int                bid = Dn(name).i;
            std::vector<V3>    loc;
            for (int v : weights_all_.order) {
                const auto &infl = weights_all_.of.at(v);
                if (std::any_of(infl.begin(), infl.end(), [&](const auto &bw) { return bw.first == bid && bw.second >= 20; })) {
                    loc.push_back(XformPt(dran_nodes_[std::size_t(bid)].inv_world, vw_all[std::size_t(v)]));
                }
            }
            if (loc.size() < 4) {
                loc.clear();
                for (int v : weights_all_.order) {
                    const auto &infl = weights_all_.of.at(v);
                    if (std::any_of(infl.begin(), infl.end(), [&](const auto &bw) { return dran_nodes_[std::size_t(bw.first)].name.starts_with(name.substr(0, 2)); })) {
                        loc.push_back(XformPt(dran_nodes_[std::size_t(bid)].inv_world, vw_all[std::size_t(v)]));
                    }
                }
            }
            V3 c;
            for (std::size_t i = 0; i < 3; i++) {
                std::vector<double> xs;
                for (const V3 &p : loc) {
                    xs.push_back(p[i]);
                }
                c[i] = Sum(xs) / double(loc.size());
            }
            V3 nrm = PcaNormal(loc, c);
            V3 span_f = rf[0], top_f, top_w;
            for (std::size_t i = 0; i < 3; i++) {
                top_f[i] = Sum({nrm[0] * rf[0][i], nrm[1] * rf[1][i], nrm[2] * rf[2][i]});
            }
            for (std::size_t i = 0; i < 3; i++) {
                top_w[i] = Sum({top_f[0] * r_ref[0][i], top_f[1] * r_ref[1][i], top_f[2] * r_ref[2][i]});
            }
            if (top_w[1] < 0) {
                top_f = {-top_f[0], -top_f[1], -top_f[2]};
            }
            V3 span_t0 = {kWingFold[k].span[0] * sx, kWingFold[k].span[1], kWingFold[k].span[2]};
            V3 top_t0 = {kWingFold[k].top[0] * sx, kWingFold[k].top[1], kWingFold[k].top[2]};
            V3 span_t, top_t;
            for (std::size_t i = 0; i < 3; i++) {
                span_t[i] = Sum({span_t0[0] * r_stand_t[0][i], span_t0[1] * r_stand_t[1][i], span_t0[2] * r_stand_t[2][i]});
                top_t[i] = Sum({top_t0[0] * r_stand_t[0][i], top_t0[1] * r_stand_t[1][i], top_t0[2] * r_stand_t[2][i]});
            }
            M3 bf = Basis(span_f, top_f), bt = Basis(span_t, top_t);
            M3 a = Mul3(T3(bf), bt);
            rls.push_back(Mul3(rf, a));
        }
        V3 shift_w = {kWingFoldRootShift[0] * sx, kWingFoldRootShift[1], kWingFoldRootShift[2]}, shift_l;
        for (std::size_t i = 0; i < 3; i++) {
            shift_l[i] = Sum({shift_w[0] * r_stand_t[0][i], shift_w[1] * r_stand_t[1][i], shift_w[2] * r_stand_t[2][i]});
        }
        fold_root_[sd] = Add3(pivot_local_[sd], shift_l);
        fold_rls_[sd] = rls;
        fold_tls_[sd] = FkLocals(sd, rls, fold_root_[sd]);
        for (std::size_t k = 0; k < 4; k++) {
            RigNode &n = nodes_[std::size_t(Wid(ch[k]))];
            n.r = rls[k];
            n.t = fold_tls_[sd][k];
            n.quat = MatToQuat(rls[k]);
            n.world = MatMul(MatFromRT(n.r, n.t), nodes_[std::size_t(parent_)].world);
            n.inv_world = RigidInv(n.world);
            n.world_pos = {n.world[3][0], n.world[3][1], n.world[3][2]};
        }
        for (std::size_t k = 1; k < 4; k++) {
            RigNode       &n = nodes_[std::size_t(Wid(ch[k]))];
            const RigNode &pn = nodes_[std::size_t(Wid(ch[k - 1]))];
            M4             l = MatMul(n.world, RigidInv(pn.world));
            n.parent = pn.i;
            n.r = RotOf(l);
            n.t = TransOf(l);
            n.quat = MatToQuat(n.r);
            if (!(std::fabs(n.t[0] - seg_len_[sd][k - 1]) < 1e-3 && std::fabs(n.t[1]) < 1e-3 && std::fabs(n.t[2]) < 1e-3)) {
                throw std::runtime_error("wing chain " + ch[k] + ": segment length drift");
            }
        }
    }

    // The wing meshes carved from obj1 by weight.
    std::vector<Tri> tris = MdtTriangles(om);
    auto             side = [&](int vi) -> std::string {
        const auto *infl = weights_all_.Find(vi);
        if (infl == nullptr || infl->empty()) {
            return "";
        }
        auto best = (*infl)[0];
        for (const auto &bw : *infl) {
            if (bw.second > best.second) {
                best = bw;
            }
        }
        std::string nm = best.first < static_cast<int>(dran_nodes_.size()) ? dran_nodes_[std::size_t(best.first)].name : "";
        return nm.starts_with("r_wing") ? "r" : nm.starts_with("l_wing") ? "l"
                                                                         : "";
    };
    WingData data;
    for (auto [sd, root_name] : {
             std::pair<std::string, std::string>{"r", "r_wing1"},
             {"l", "l_wing1"}
    }) {
        WingData::WingSet set;
        for (const Tri &t : tris) {
            if (side(t[0]) == sd && side(t[1]) == sd && side(t[2]) == sd) {
                set.tris.push_back(t);
            }
        }
        std::set<int> seen;
        for (const Tri &t : set.tris) {
            for (int v : t) {
                seen.insert(v);
            }
        }
        set.used.assign(seen.begin(), seen.end());
        for (std::size_t i = 0; i < set.used.size(); i++) {
            set.remap[set.used[i]] = static_cast<int>(i);
        }
        SkinMesh me;
        me.node = Wid(root_name);
        me.skin = true;
        me.nv = static_cast<int>(set.used.size());
        for (const Tri &t : set.tris) {
            me.tris.push_back({set.remap[t[0]], set.remap[t[1]], set.remap[t[2]]});
        }
        for (int vi : set.used) {
            std::vector<std::pair<int, double>> infl;
            if (const auto *found = weights_all_.Find(vi)) {
                for (const auto &bw : *found) {
                    if (wid_.contains(bw.first)) {
                        infl.push_back(bw);
                    }
                }
            }
            std::stable_sort(infl.begin(), infl.end(), [](const auto &a, const auto &b) { return -a.second < -b.second; });
            if (infl.size() > 2) {
                infl.resize(2);
            }
            if (infl.empty()) {
                infl = {
                    {Dn(root_name).i, 1.0}
                };
            }
            auto [ba, wa] = infl[0];
            auto [bb, wb] = infl.size() > 1 ? infl[1] : std::pair{ba, 0.0};
            double tot = wa + wb;
            V3     pa = XformPt(dran_nodes_[std::size_t(ba)].inv_world, vw_all[std::size_t(vi)]);
            V3     pb = XformPt(dran_nodes_[std::size_t(bb)].inv_world, vw_all[std::size_t(vi)]);
            for (double &c : pa) {
                c = c * s_ * kWingSizeMul;
            }
            for (double &c : pb) {
                c = c * s_ * kWingSizeMul;
            }
            me.b0.push_back(wid_.at(ba));
            me.p0.push_back(pa);
            me.b1.push_back(wid_.at(bb));
            me.p1.push_back(pb);
            me.w0.push_back(tot != 0 ? wa / tot : 1.0);
        }
        log_(std::format("  {} wing: {} verts, {} tris", sd, set.used.size(), set.tris.size()));
        data.meshes.push_back(std::move(me));
        data.wing_sets[sd] = std::move(set);
    }
    auto cw = LoadWeights(*cat_pack_, "cat.wgt");
    for (const RigNode &n : cat_nodes_) {
        if (n.mesh_off == 0) {
            continue;
        }
        auto                    per = cw.find(n.i);
        std::optional<SkinMesh> mesh;
        if (per != cw.end() && !per->second.order.empty()) {
            mesh = BuildMeshWeighted(cat_mds_, n, nodes_, per->second);
        } else {
            mesh = BuildMesh(cat_mds_, n, nodes_);
        }
        if (mesh) {
            data.meshes.push_back(std::move(*mesh));
        }
    }

    // The tracks: the wing bones rigid to the spine, Dran's sampled pose inside the driven windows, the authored landing.
    for (const WingClip &clip : kWingClips) {
        const CatKeyWindow &key = kCatKeys[clip.ki];
        double              win_game = (key.end - key.start) / key.speed, loop_game = (clip.de - clip.ds) / clip.dspd;
        int                 cycles = clip.loop ? std::max(1, static_cast<int>(Round(win_game / loop_game, 0))) : 1;
        windows_.push_back({key.start, key.end, clip.ds, clip.de, cycles, clip.loop, clip.ki});
    }
    std::stable_sort(windows_.begin(), windows_.end(), [](const Window &a, const Window &b) { return a.ki < b.ki; });
    {
        const V3      fa = {1.0, 0.0, 0.0}, va = {0.0, 1.0, 0.0};
        const Window &w7 = *std::find_if(windows_.begin(), windows_.end(), [](const Window &w) { return w.ki == kStrokeKi; });
        for (const std::string sd : {"r", "l"}) {
            const auto     &ch = Chain(sd);
            std::vector<M3> g;
            for (const std::string &name : ch) {
                g.push_back(DranLocalR(name, double(w7.de)));
            }
            auto            tip = [&](const std::vector<M3> &rs) { return FkLocals(sd, rs)[3]; };
            std::vector<M3> up_rs, back_rs;
            for (const M3 &r : g) {
                up_rs.push_back(Mul3(r, RotAbout(fa, 80)));
                back_rs.push_back(Mul3(r, RotAbout(va, 30)));
            }
            flap_sign_[sd] = tip(up_rs)[1] < tip(g)[1] ? 1.0 : -1.0;
            sweep_sign_[sd] = tip(back_rs)[0] < tip(g)[0] ? 1.0 : -1.0;
        }
    }
    auto [wref, anchor_ref] = WingPose(ref_frame);
    std::map<Edge, int> full_edges;
    for (const Tri &t : tris) {
        for (Edge e : {MakeEdge(t[0], t[1]), MakeEdge(t[1], t[2]), MakeEdge(t[2], t[0])}) {
            full_edges[e]++;
        }
    }
    std::vector<M4> wcat = CatWorld(ref_frame);
    const SkinMesh *skin_me = nullptr;
    for (const SkinMesh &m : data.meshes) {
        if (m.node == Cn("cat_skin").i) {
            skin_me = &m;
            break;
        }
    }
    auto            cat_world = [&](int b) -> const M4            &{ return wcat[std::size_t(b)]; };
    std::vector<V3> skin_ref;
    for (int i = 0; i < skin_me->nv; i++) {
        skin_ref.push_back(Skinned(*skin_me, i, cat_world));
    }
    auto mref = [&](int b) -> const M4 & {
        auto found = wref.find(b);
        return found != wref.end() ? found->second : wcat[std::size_t(b)];
    };
    for (const std::string sd : {"r", "l"}) {
        SkinMesh                &me = data.meshes[sd == "r" ? 0 : 1];
        const WingData::WingSet &set = data.wing_sets[sd];
        const auto              &wt = set.tris;
        const auto              &used = set.used;
        auto                     remap = [&](int v) { return set.remap.at(v); };
        std::vector<Edge>        wedge_order;
        std::map<Edge, int>      wedges;
        for (const Tri &t : wt) {
            for (Edge e : {MakeEdge(t[0], t[1]), MakeEdge(t[1], t[2]), MakeEdge(t[2], t[0])}) {
                if (!wedges.contains(e)) {
                    wedges[e] = 0;
                    wedge_order.push_back(e);
                }
                wedges[e]++;
            }
        }
        std::vector<Edge> attach_edges;
        for (Edge e : wedge_order) {
            auto full = full_edges.find(e);
            if (wedges[e] == 1 && full != full_edges.end() && full->second >= 2) {
                attach_edges.push_back(e);
            }
        }
        std::map<int, std::vector<int>> adj;
        std::vector<int>                adj_order;
        for (auto [va, vb] : attach_edges) {
            if (!adj.contains(va)) {
                adj[va] = {};
                adj_order.push_back(va);
            }
            adj[va].push_back(vb);
            if (!adj.contains(vb)) {
                adj[vb] = {};
                adj_order.push_back(vb);
            }
            adj[vb].push_back(va);
        }
        std::vector<int> starts = adj_order;
        std::stable_sort(starts.begin(), starts.end(), [&](int a, int b) {
            if (adj[a].size() != adj[b].size()) {
                return adj[a].size() < adj[b].size();
            }
            return a < b;
        });
        std::vector<std::vector<int>> chains;
        std::set<int>                 seen;
        for (int start : starts) {
            if (seen.contains(start)) {
                continue;
            }
            std::vector<int>   chain = {start};
            int                cur = start;
            std::optional<int> prev;
            seen.insert(start);
            while (true) {
                std::vector<int> next;
                for (int v : adj[cur]) {
                    if ((!prev || v != *prev) && !seen.contains(v)) {
                        next.push_back(v);
                    }
                }
                if (next.empty()) {
                    break;
                }
                prev = cur;
                cur = next[0];
                chain.push_back(cur);
                seen.insert(cur);
            }
            const auto &last_adj = adj[chain.back()];
            if (chain.size() > 2 && std::find(last_adj.begin(), last_adj.end(), chain[0]) != last_adj.end()) {
                chain.push_back(chain[0]);
            }
            chains.push_back(chain);
        }
        auto   wpos = [&](int li) { return Skinned(me, li, mref); };
        double sx = sd == "l" ? 1.0 : -1.0;
        int    arm = Cn(sd == "l" ? "cat_arm_hidari" : "cat_arm_migi").i;
        int    sb1 = Cn("cat_sebone1").i, sb2 = Cn("cat_sebone2").i;
        auto   skin_y2 = [&](double x, double z) -> std::optional<double> {
            std::optional<double> best;
            for (const Tri &t : skin_me->tris) {
                const V3 &a = skin_ref[std::size_t(t[0])], &b = skin_ref[std::size_t(t[1])], &c = skin_ref[std::size_t(t[2])];
                double    det = (b[0] - a[0]) * (c[2] - a[2]) - (c[0] - a[0]) * (b[2] - a[2]);
                if (std::fabs(det) < 1e-9) {
                    continue;
                }
                double l1 = ((b[0] - x) * (c[2] - z) - (c[0] - x) * (b[2] - z)) / det;
                double l2 = ((c[0] - x) * (a[2] - z) - (a[0] - x) * (c[2] - z)) / det;
                double l3 = 1 - l1 - l2;
                if (l1 < -1e-6 || l2 < -1e-6 || l3 < -1e-6) {
                    continue;
                }
                double y = l1 * a[1] + l2 * b[1] + l3 * c[1];
                if (!best || y > *best) {
                    best = y;
                }
            }
            return best;
        };
        std::size_t longest = 0;
        for (std::size_t k = 1; k < chains.size(); k++) {
            if (chains[k].size() > chains[longest].size()) {
                longest = k;
            }
        }
        std::vector<int> chain0 = chains[longest];
        if (chain0.size() > 1 && chain0[0] == chain0.back()) {
            chain0.pop_back();
        }
        std::map<int, V3> p;
        for (int vi : chain0) {
            p[vi] = wpos(remap(vi));
        }
        std::size_t bi = 0;
        for (std::size_t k = 1; k < chain0.size(); k++) {
            if (std::fabs(p[chain0[k]][0]) < std::fabs(p[chain0[bi]][0])) {
                bi = k;
            }
        }
        std::vector<int> chain(chain0.begin() + std::ptrdiff_t(bi), chain0.end());
        chain.insert(chain.end(), chain0.begin(), chain0.begin() + std::ptrdiff_t(bi));
        if (chain.size() > 2 && p[chain[1]][1] < p[chain.back()][1]) {
            std::reverse(chain.begin() + 1, chain.end());
        }
        std::size_t fi = 0;
        for (std::size_t k = 1; k < chain.size(); k++) {
            if (p[chain[k]][2] > p[chain[fi]][2]) {
                fi = k;
            }
        }
        std::vector<int>           top(chain.begin() + 1, chain.begin() + 1 + std::ptrdiff_t(fi));
        std::map<std::string, int> pts = {
            {"Rb", remap(chain[0])                                            },
            {"Ri", remap(top[0])                                              },
            {"Rf", remap(top.back())                                          },
            {"Rg", remap(fi + 1 < chain.size() ? chain[fi + 1] : chain.back())}
        };
        int              eb = chain[0], ei = top[0];
        std::vector<int> rt;
        for (const Tri &t : wt) {
            if (std::find(t.begin(), t.end(), eb) != t.end() && std::find(t.begin(), t.end(), ei) != t.end()) {
                for (int v : t) {
                    if (v != eb && v != ei) {
                        rt.push_back(v);
                    }
                }
            }
        }
        pts["Rt"] = !rt.empty() ? remap(rt[0]) : pts["Ri"];
        auto wp_any = [&](int i) { return Skinned(me, i, mref); };
        // The inboard fan's tip pulled outward toward the wing root.
        for (int vi : used) {
            int    li = remap(vi);
            V3     q = wpos(li);
            double x = std::fabs(q[0]);
            if (x >= kApexReach) {
                continue;
            }
            double d = kApexPull * (kApexReach - x) / kApexReach;
            V3     q2 = {q[0] + (sd == "r" ? -d : d), q[1], q[2]};
            me.p0[std::size_t(li)] = XformPt(RigidInv(mref(me.b0[std::size_t(li)])), q2);
            me.p1[std::size_t(li)] = XformPt(RigidInv(mref(me.b1[std::size_t(li)])), q2);
        }
        auto [wfold, unused_anchor] = WingPose(kWingFoldFrame);
        std::vector<M4> wc_fold = CatWorld(kWingFoldFrame);
        auto            mfold = [&](int b) -> const M4            &{
            auto found = wfold.find(b);
            return found != wfold.end() ? found->second : wc_fold[std::size_t(b)];
        };
        // The fan's back edge moved forward so it stops cutting into the folded wing behind it.
        for (int vi : used) {
            int    li = remap(vi);
            V3     q = wpos(li);
            double x = std::fabs(q[0]);
            if (q[2] >= kRootZMax || x >= kRootXMax) {
                continue;
            }
            double f = kRootFwd * std::min(1.0, (kRootXMax - x) / (kRootXMax - kRootXFull));
            for (int slot : {0, 1}) {
                const M4 &m = mfold(slot == 0 ? me.b0[std::size_t(li)] : me.b1[std::size_t(li)]);
                V3        dl = {f * m[0][2], f * m[1][2], f * m[2][2]};
                V3       &pp = slot == 0 ? me.p0[std::size_t(li)] : me.p1[std::size_t(li)];
                pp = {pp[0] + dl[0], pp[1] + dl[1], pp[2] + dl[2]};
            }
        }
        auto fit = [&](std::size_t li, const V3 &offset) {
            int                               b0 = me.b0[li], b1 = me.b1[li];
            double                            w = me.w0[li];
            std::pair<ColumnPose, ColumnPose> ma = {Col(mref(b0)), Col(mref(b1))}, mb = {Col(mfold(b0)), Col(mfold(b1))};
            V3                                target_a = Blend(w, ma.first, ma.second, me.p0[li], me.p1[li]);
            V3                                target_b = Add3(Blend(w, mb.first, mb.second, me.p0[li], me.p1[li]), offset);
            if (auto sol = TwoPoseFit(w, ma, mb, target_a, target_b)) {
                me.p0[li] = sol->first;
                me.p1[li] = sol->second;
            }
        };
        for (auto [li, d] : kWingFoldLift) {
            if (li >= static_cast<int>(used.size())) {
                continue;
            }
            // Lifted in the folded pose only (two-pose fit; the leap stays exact).
            int                               b0 = me.b0[std::size_t(li)], b1 = me.b1[std::size_t(li)];
            double                            w = me.w0[std::size_t(li)];
            std::pair<ColumnPose, ColumnPose> ma = {Col(mref(b0)), Col(mref(b1))}, mb = {Col(mfold(b0)), Col(mfold(b1))};
            V3                                target_a = Blend(w, ma.first, ma.second, me.p0[std::size_t(li)], me.p1[std::size_t(li)]);
            V3                                target_b = Blend(w, mb.first, mb.second, me.p0[std::size_t(li)], me.p1[std::size_t(li)]);
            target_b[1] += d;
            if (auto sol = TwoPoseFit(w, ma, mb, target_a, target_b)) {
                me.p0[std::size_t(li)] = sol->first;
                me.p1[std::size_t(li)] = sol->second;
            }
        }
        // Rm: the midpoint of the base's rear edge Rb-Rt, skinned as the pooled average of its ends.
        pts["Rm"] = me.nv;
        Pool(me, {
                     {pts["Rb"], 0.5},
                     {pts["Rt"], 0.5}
        });
        auto back_pt = [&](double x, double z) {
            double                xx = sx * x;
            std::optional<double> sy = skin_y2(xx, z);
            while (!sy && std::fabs(xx) > 0.05) {
                xx -= sx * 0.05;
                sy = skin_y2(xx, z);
            }
            double syv = sy ? *sy : (anchor_ref[1] - 0.4);
            V3     pos = {xx, syv + kWingExtendLift, z};
            int    spine = z < 0.9 ? sb1 : sb2;
            double w_spine = std::min(std::max((0.6 - std::fabs(xx)) / (0.6 - 0.35), 0.0), 1.0);
            int    i = me.nv;
            me.nv++;
            me.b0.push_back(spine);
            me.p0.push_back(XformPt(RigidInv(wcat[std::size_t(spine)]), pos));
            me.b1.push_back(arm);
            me.p1.push_back(XformPt(RigidInv(wcat[std::size_t(arm)]), pos));
            me.w0.push_back(w_spine);
            return std::pair{i, pos};
        };
        const std::map<std::string, std::pair<double, double>> corners = {
            {"ir", {kWingExtendXIn, kWingExtendZBack}     },
            {"if", {kWingExtendXIn, kWingExtendZFront}    },
            {"of", {kWingExtendXOut, kWingExtendZFrontOut}},
            {"or", {kWingExtendXOut, kWingExtendZBack}    }
        };
        std::vector<std::pair<int, V3>> cpos;
        for (const auto &poly : kWingTabPolys) {
            for (const std::string &nm : poly) {
                auto corner = corners.find(nm);
                if (corner != corners.end() && !pts.contains(nm)) {
                    auto [i, pos] = back_pt(corner->second.first, corner->second.second);
                    pts[nm] = i;
                    cpos.emplace_back(i, pos);
                }
            }
        }
        std::vector<Tri> polys;
        std::vector<int> upper, lower = {remap(chain[0])};
        for (std::size_t k = 0; k <= fi; k++) {
            upper.push_back(remap(chain[k]));
        }
        for (std::size_t k = chain.size(); k-- > fi + 1;) {
            lower.push_back(remap(chain[k]));
        }
        lower.push_back(remap(chain[fi]));
        {
            // Close the base: zip the ring's upper rim to its lower rim, always closing the shorter diagonal.
            std::size_t i = 0, j = 0;
            while (i < upper.size() - 1 || j < lower.size() - 1) {
                bool advance_upper;
                if (i == upper.size() - 1) {
                    advance_upper = false;
                } else if (j == lower.size() - 1) {
                    advance_upper = true;
                } else {
                    V3 a = wp_any(upper[i + 1]), b = wp_any(lower[j]), c = wp_any(upper[i]), d = wp_any(lower[j + 1]);
                    advance_upper = Dist(a, b) <= Dist(c, d);
                }
                if (advance_upper) {
                    if (upper[i + 1] != lower[j]) {
                        polys.push_back({upper[i], upper[i + 1], lower[j]});
                    }
                    i++;
                } else {
                    if (lower[j + 1] != upper[i]) {
                        polys.push_back({upper[i], lower[j + 1], lower[j]});
                    }
                    j++;
                }
            }
        }
        std::vector<std::pair<Tri, bool>> tab_polys;
        for (const auto &poly : kWingTabPolys) {
            if (poly[0] == "RIM") {
                std::vector<int> rim;
                for (int v : top) {
                    rim.push_back(remap(v));
                }
                for (std::size_t k = 0; k + 1 < rim.size(); k++) {
                    tab_polys.push_back({
                        {rim[k], rim[k + 1], pts[poly[1]]},
                        false
                    });
                }
            } else {
                tab_polys.push_back({
                    {pts[poly[0]], pts[poly[1]], pts[poly[2]]},
                    poly.size() > 3
                });
            }
        }
        {
            // The tabs' dihedral: each corner lowered toward the spine from the rim's height beside it.
            double th = Radians(kWingTabTiltDeg), hx = p[top[0]][0];
            double z0 = p[top[0]][2], y0 = p[top[0]][1], z1 = p[top.back()][2], y1 = p[top.back()][1];
            for (auto &[i, pos] : cpos) {
                double t = z1 == z0 ? 0.0 : std::min(std::max((pos[2] - z0) / (z1 - z0), 0.0), 1.0);
                double y_rim = y0 + (y1 - y0) * t - kWingRaise;
                pos[1] = y_rim - std::fabs(pos[0] - hx) * std::tan(th);
                me.p0[std::size_t(i)] = XformPt(RigidInv(wcat[std::size_t(me.b0[std::size_t(i)])]), pos);
                me.p1[std::size_t(i)] = XformPt(RigidInv(wcat[std::size_t(me.b1[std::size_t(i)])]), pos);
            }
        }
        // The top tabs subdivided: each into n^2 triangles, the new points pooled from their edge's ends.
        const int                  n = kWingTabSubdiv;
        int                        rm_idx = pts["Rm"];
        auto                       wing_vert = [&](int i) { return i < static_cast<int>(used.size()) || i == rm_idx; };
        std::vector<int>           mixed;
        std::map<std::string, int> cache;
        auto                       sub_vertex = [&](const std::string &key, const std::vector<std::pair<int, double>> &ends) {
            auto have = cache.find(key);
            if (have != cache.end()) {
                return have->second;
            }
            int i = me.nv;
            Pool(me, ends);
            bool any = false, all = true;
            for (const auto &e : ends) {
                any = any || wing_vert(e.first);
                all = all && wing_vert(e.first);
            }
            if (any && !all) {
                mixed.push_back(i);
            }
            cache[key] = i;
            return i;
        };
        std::vector<Tri> fine;
        for (const auto &[tri, under] : tab_polys) {
            if (under) {
                polys.push_back(tri);
                continue;
            }
            std::map<std::array<int, 3>, int> grid;
            for (int a = 0; a <= n; a++) {
                for (int b = 0; b <= n - a; b++) {
                    int                c = n - a - b;
                    std::array<int, 3> bc = {a, b, c};
                    int                full = int(a == n) + int(b == n) + int(c == n);
                    if (full == 1) {
                        grid[bc] = tri[std::size_t(std::find(bc.begin(), bc.end(), n) - bc.begin())];
                        continue;
                    }
                    std::vector<int> zero;
                    for (int k = 0; k < 3; k++) {
                        if (bc[std::size_t(k)] == 0) {
                            zero.push_back(k);
                        }
                    }
                    std::string                         key;
                    std::vector<std::pair<int, double>> ends;
                    if (zero.size() == 1) {
                        std::vector<int> nz;
                        for (int k = 0; k < 3; k++) {
                            if (k != zero[0]) {
                                nz.push_back(k);
                            }
                        }
                        std::size_t e0 = std::size_t(nz[0]), e1 = std::size_t(nz[1]);
                        std::size_t lo_e = tri[e0] < tri[e1] ? e0 : e1, hi_e = tri[e0] < tri[e1] ? e1 : e0;
                        key = std::format("e|{}|{}|{}", tri[lo_e], tri[hi_e], bc[hi_e]);
                        ends = {
                            {tri[e0], bc[e0] / double(n)},
                            {tri[e1], bc[e1] / double(n)}
                        };
                    } else {
                        key = std::format("t|{},{},{}|{},{},{}", tri[0], tri[1], tri[2], a, b, c);
                        for (std::size_t k = 0; k < 3; k++) {
                            ends.emplace_back(tri[k], bc[k] / double(n));
                        }
                    }
                    grid[bc] = sub_vertex(key, ends);
                }
            }
            for (int a = 0; a < n; a++) {
                for (int b = 0; b < n - a; b++) {
                    int c = n - a - b;
                    fine.push_back({grid[{a + 1, b, c - 1}], grid[{a, b + 1, c - 1}], grid[{a, b, c}]});
                    if (c >= 2) {
                        fine.push_back({grid[{a + 1, b, c - 1}], grid[{a + 1, b + 1, c - 2}], grid[{a, b + 1, c - 1}]});
                    }
                }
            }
        }
        auto mid_of = [&](int a, int b) -> std::optional<int> {
            auto found = cache.find(std::format("e|{}|{}|1", std::min(a, b), std::max(a, b)));
            return found != cache.end() ? std::optional<int>(found->second) : std::nullopt;
        };
        auto conform = [&](const Tri &t) {
            int                               a = t[0], b = t[1], c = t[2];
            std::array<std::optional<int>, 3> m = {mid_of(a, b), mid_of(b, c), mid_of(c, a)};
            int                               k = int(m[0].has_value()) + int(m[1].has_value()) + int(m[2].has_value());
            if (k == 0) {
                return std::vector<Tri>{t};
            }
            if (k == 3) {
                return std::vector<Tri>{
                    {a,     *m[0], *m[2]},
                    {*m[0], b,     *m[1]},
                    {*m[2], *m[1], c    },
                    {*m[0], *m[1], *m[2]}
                };
            }
            while (!m[0] || (k == 2 && !m[1])) {
                std::tie(a, b, c) = std::tuple{b, c, a};
                m = {m[1], m[2], m[0]};
            }
            if (k == 1) {
                return std::vector<Tri>{
                    {a,     *m[0], c},
                    {*m[0], b,     c}
                };
            }
            return std::vector<Tri>{
                {*m[0], b,     *m[1]},
                {a,     *m[0], *m[1]},
                {a,     *m[1], c    }
            };
        };
        auto conform_all = [&](const std::vector<Tri> &list) {
            std::vector<Tri> out;
            for (const Tri &t : list) {
                for (const Tri &u : conform(t)) {
                    out.push_back(u);
                }
            }
            return out;
        };
        me.tris = conform_all(me.tris);
        polys = conform_all(polys);
        if (pts.contains("ir") && pts.contains("if")) {
            std::vector<Tri> floor;
            for (std::size_t k = 0; k + 2 < lower.size(); k++) {
                floor.push_back({pts["ir"], lower[k], lower[k + 1]});
            }
            floor.push_back({pts["ir"], lower[lower.size() - 2], pts["if"]});
            floor = conform_all(floor);
            polys.insert(polys.end(), floor.begin(), floor.end());
        }
        polys.insert(polys.end(), fine.begin(), fine.end());
        if (!mixed.empty()) {
            // The fold rounded: each wing-body midpoint pushed out along the shoulder in the folded pose (exact in the leap).
            V3 nrm = Unit({0.6 * (sd == "l" ? 1.0 : -1.0), 0.8, 0.0});
            for (int i : mixed) {
                fit(std::size_t(i), {kWingTabBulge * nrm[0], kWingTabBulge * nrm[1], kWingTabBulge * nrm[2]});
            }
        }
        me.tris.insert(me.tris.end(), polys.begin(), polys.end());
        log_(std::format("  {} wing tabs: {} tris, {} new verts", sd, polys.size(), cache.size()));
    }

    // The wing tracks: keys over the driven windows, the landing and the hold, bind brackets one frame outside.
    std::vector<std::pair<int, int>> spans;
    for (const Window &w : windows_) {
        spans.emplace_back(w.cs, w.ce);
    }
    spans.emplace_back(kLandCs, kLandCe);
    for (auto [ki, unused] : kWingHold) {
        spans.emplace_back(kCatKeys[ki].start, kCatKeys[ki].end);
    }
    std::set<int> frames_all;
    for (auto [cs, ce] : spans) {
        for (int f = cs; f <= ce; f++) {
            frames_all.insert(f);
        }
    }
    for (auto [cs, ce] : spans) {
        frames_all.insert(cs - 1);
        frames_all.insert(ce + 1);
    }
    std::map<int, std::map<int, Local>> per_frame;
    for (int f : frames_all) {
        per_frame[f] = WingLocals(f);
    }
    for (int f : frames_all) {
        auto &loc = per_frame[f];
        for (const std::string sd : {"r", "l"}) {
            const auto      &ch = Chain(sd);
            std::vector<int> ids;
            std::vector<M4>  ms;
            for (const std::string &name : ch) {
                ids.push_back(Wid(name));
                ms.push_back(MatFromRT(loc[ids.back()].r, loc[ids.back()].t));
            }
            for (std::size_t k = 1; k < 4; k++) {
                M4     l = MatMul(ms[k], RigidInv(ms[k - 1]));
                double dev = std::max(std::max(std::fabs(l[3][0] - seg_len_[sd][k - 1]), std::fabs(l[3][1])), std::fabs(l[3][2]));
                if (!(dev < 1e-3)) {
                    throw std::runtime_error(std::format("wing chain drift at frame {}, {}", f, ch[k]));
                }
                loc[ids[k]] = {RotOf(l), nodes_[std::size_t(ids[k])].t};
            }
        }
    }
    std::vector<Track> tracks = cat_tracks_;
    for (int k = 0; k < 8; k++) {
        int   nid = base_ + k;
        Track tr;
        tr.node = nid;
        tr.chan = 0;
        for (int f : frames_all) {
            tr.frames.push_back(f);
            tr.vals.push_back(MatToQuat(per_frame[f][nid].r));
        }
        tracks.push_back(tr);
        if (nodes_[std::size_t(nid)].parent == parent_) {
            Track tt;
            tt.node = nid;
            tt.chan = 2;
            for (int f : frames_all) {
                const V3 &t = per_frame[f][nid].t;
                tt.frames.push_back(f);
                tt.vals.push_back({t[0], t[1], t[2], 0.0});
            }
            tracks.push_back(tt);
        }
    }
    log_(std::format("  wing tracks: keys at {}..{} ({} frames)", *frames_all.begin(), *frames_all.rbegin(), frames_all.size()));
    data.nodes = nodes_;
    data.tracks = std::move(tracks);
    data.base = base_;
    data.parent = parent_;
    data.om = om;
    data.frames_all.assign(frames_all.begin(), frames_all.end());
    data.ref_frame = ref_frame;
    data.wref = wref;
    data.wcat = wcat;
    data.bind_open = bind_open;
    return data;
}

} // namespace

WingData WingGraft(std::span<const unsigned char> dran, const Bytes &cat_mds, const ChrPack &cat_pack, const Log &log) {
    std::vector<RigNode> cat_nodes = ReadSkeleton(cat_mds);
    ChrPack              dran_pack = ChrPack::Parse(dran);
    const ChrRecord     *cfg = FindCfg(dran_pack);
    if (cfg == nullptr) {
        throw std::runtime_error("Dran's pack has no cfg");
    }
    auto [mds_name, mot_name] = CfgNames(cfg->Payload());
    Bytes                dran_mds = dran_pack.Require(mds_name).Payload();
    std::vector<RigNode> dran_nodes = ReadSkeleton(dran_mds);
    Graft                graft(cat_nodes, cat_mds, cat_pack, dran_nodes, dran_mds, dran_pack, mot_name, log);
    WingData             data = graft.Build();
    data.cat_nodes = cat_nodes;
    data.mds = cat_mds;
    data.pack = cat_pack;
    return data;
}

} // namespace expanded::spirit_cat
