#include <algorithm>
#include <format>
#include <functional>
#include <map>
#include <set>
#include <string>

#include "expanded/data/model_parts.hpp"
#include "expanded/data/town_rig.hpp"

namespace expanded::town {

namespace {

// A source rig whose root alone is named differently takes the dest's root name.
std::vector<std::string> AliasRoot(std::vector<std::string> s, const std::vector<std::string> &d) {
    if (!s.empty() && !d.empty() && s[0] != d[0] && Index(d, s[0]) < 0 && Index(s, d[0]) < 0) {
        s[0] = d[0];
    }
    return s;
}

// Makes the dest tracks a grafted window lacks, held at the dest's bind outside the grafts.
void CreateMissingTracks(MotFile &dst, const MotFile &src, const std::vector<std::string> &sframes,
                         const std::vector<std::string> &dframes, int slo, int shi, std::span<const unsigned char> dpl,
                         const std::vector<Span> &spans) {
    std::vector<int>                                  remap = BuildJointRemap(sframes, dframes);
    std::set<std::pair<std::uint32_t, std::uint32_t>> have;
    for (const MotTrack &track : dst.tracks) {
        have.insert({track.w0, track.w2});
    }
    std::vector<int> edges = SpanEdges(spans);
    for (const MotTrack &st : src.tracks) {
        if (std::none_of(st.keys.begin(), st.keys.end(), [&](const MotKeyframe &k) { return InWindow(k, slo, shi); })) {
            continue;
        }
        int dw = st.w0 < remap.size() ? remap[st.w0] : -1;
        if (dw < 0 || have.contains({static_cast<std::uint32_t>(dw), st.w2})) {
            continue;
        }
        Bind     b = MdsBind(dpl, dw);
        Vec4     v = st.w2 == mot::kRotation ? MatToQuat(b.rotation) : Vec4{b.translation[0], b.translation[1], b.translation[2], 0.0};
        MotTrack added = st;
        added.w0 = static_cast<std::uint32_t>(dw);
        added.keys.clear();
        for (int frame : edges) {
            added.keys.push_back(MotKeyframe::Make(frame, v));
        }
        auto at = std::find_if(dst.tracks.begin(), dst.tracks.end(), [&](const MotTrack &t) {
            return t.w0 > static_cast<std::uint32_t>(dw) || (t.w0 == static_cast<std::uint32_t>(dw) && t.w2 > st.w2);
        });
        dst.tracks.insert(at, std::move(added));
        have.insert({static_cast<std::uint32_t>(dw), st.w2});
    }
}

void Graft(ChrPack &dst_pack, const Model &dst_model, const ChrPack &src_pack, const Model &src_model, int slo, int shi,
           int dlo, int dhi, bool root_motion, const std::vector<Span> &spans) {
    MotFile                  dst = MotOf(dst_pack, dst_model.mot);
    MotFile                  src = MotOf(src_pack, src_model.mot);
    Bytes                    dpl = dst_pack.Require(dst_model.mds).Payload();
    std::vector<std::string> dframes = ReadMdsFrames(dpl);
    std::vector<std::string> sframes = FramesOf(src_pack, src_model.mds);
    if (root_motion) {
        sframes = AliasRoot(sframes, dframes);
        CreateMissingTracks(dst, src, sframes, dframes, slo, shi, dpl, spans);
    }
    SpliceByJoint(dst, src, sframes, dframes, slo, shi, dlo, dhi);
    ReplaceMot(dst_pack, dst_model.mot, dst);
}

// Keys every grafted track at the window's edges, so a sparse one stays out of the next clip.
void SealGraft(ChrPack &dst_pack, const Model &dst_model, const ChrPack &src_pack, const Model &src_model, int slo,
               int shi, int dlo, int dhi, bool root_motion) {
    std::vector<std::string> dframes = FramesOf(dst_pack, dst_model.mds);
    std::vector<std::string> sframes = FramesOf(src_pack, src_model.mds);
    if (root_motion) {
        sframes = AliasRoot(sframes, dframes);
    }
    std::vector<int> remap(sframes.size());
    for (std::size_t i = 0; i < sframes.size(); i++) {
        remap[i] = Index(dframes, sframes[i]);
    }
    MotFile                                                       dm = MotOf(dst_pack, dst_model.mot);
    MotFile                                                       sm = MotOf(src_pack, src_model.mot);
    std::map<std::pair<std::uint32_t, std::uint32_t>, MotTrack *> dtracks;
    for (MotTrack &track : dm.tracks) {
        dtracks[{track.w0, track.w2}] = &track;
    }
    bool changed = false;
    for (const MotTrack &st : sm.tracks) {
        if (st.w0 >= remap.size() || remap[st.w0] < 0) {
            continue;
        }
        auto found = dtracks.find({static_cast<std::uint32_t>(remap[st.w0]), st.w2});
        if (found == dtracks.end() || found->second->keys.empty()) {
            continue;
        }
        MotTrack &dt = *found->second;
        for (auto [sf, df] : {
                 Span{slo, dlo},
                 Span{shi, dhi}
        }) {
            auto at_edge = [&](const MotKeyframe &k) { return k.Frame() == static_cast<std::uint32_t>(df); };
            if (std::any_of(dt.keys.begin(), dt.keys.end(), at_edge)) {
                continue;
            }
            std::optional<Vec4> v = SampleTrack(st, sf);
            if (!v) {
                continue;
            }
            MotKeyframe key = dt.keys[0];
            key.SetFrame(df);
            key.SetValues(*v);
            dt.keys.push_back(key);
            SortByFrame(dt.keys);
            changed = true;
        }
    }
    if (changed) {
        ReplaceMot(dst_pack, dst_model.mot, dm);
    }
}

// ------------------------------------------------------------------ window post-ops

// Every track's keys in [dlo, dhi] dropped and its value at frame keyed at both edges.
void HoldTracks(MotFile &mot, int frame, int dlo, int dhi, const std::function<bool(const MotTrack &)> &which,
                const char *what) {
    for (MotTrack &track : mot.tracks) {
        if (!which(track)) {
            continue;
        }
        std::optional<Vec4> v = SampleTrack(track, frame);
        if (!v || track.keys.empty()) {
            continue;
        }
        std::erase_if(track.keys, [&](const MotKeyframe &k) { return InWindow(k, dlo, dhi); });
        if (track.keys.empty()) {
            Bad(std::format("{}: a track was keyed only inside the window", what));
        }
        for (int edge : {dlo, dhi}) {
            MotKeyframe key = track.keys[0];
            key.SetFrame(edge);
            key.SetValues(*v);
            track.keys.push_back(key);
        }
        SortByFrame(track.keys);
    }
}

} // namespace

void GraftWindow(ChrPack &base, const CfgMotions &cfg, const ChrPack &src, const CfgMotions &scfg, int slo, int shi,
                 int dlo, int dhi, bool root_motion, const std::vector<Span> &rm_spans) {
    Model body{cfg.body_mot, cfg.body_mds};
    Model sbody{scfg.body_mot, scfg.body_mds};
    Graft(base, body, src, sbody, slo, shi, dlo, dhi, root_motion, rm_spans);
    SealGraft(base, body, src, sbody, slo, shi, dlo, dhi, root_motion);
    if (cfg.shadow_mot && scfg.shadow_mot) {
        Model shadow{*cfg.shadow_mot, cfg.shadow_mds.value_or("")};
        Model sshadow{*scfg.shadow_mot, scfg.shadow_mds.value_or("")};
        Graft(base, shadow, src, sshadow, slo, shi, dlo, dhi, root_motion, rm_spans);
        SealGraft(base, shadow, src, sshadow, slo, shi, dlo, dhi, root_motion);
    }
}

void BakeHold(ChrPack &pack, std::string_view mot_name, int frame, int dlo, int dhi) {
    MotFile mot = MotOf(pack, mot_name);
    HoldTracks(mot, frame, dlo, dhi, [](const MotTrack &) { return true; }, "hold");
    ReplaceMot(pack, mot_name, mot);
}

void ReverseWindow(ChrPack &pack, std::string_view mot_name, int dlo, int dhi) {
    MotFile mot = MotOf(pack, mot_name);
    for (MotTrack &track : mot.tracks) {
        bool any = false;
        for (MotKeyframe &key : track.keys) {
            if (InWindow(key, dlo, dhi)) {
                key.SetFrame(static_cast<std::uint32_t>(dlo + dhi - static_cast<int>(key.Frame())));
                any = true;
            }
        }
        if (any) {
            SortByFrame(track.keys);
        }
    }
    ReplaceMot(pack, mot_name, mot);
}

void ApplyRootOffset(ChrPack &pack, std::string_view mot_name, int dlo, int dhi, const std::array<double, 3> &offset) {
    MotFile mot = MotOf(pack, mot_name);
    auto    root = std::find_if(mot.tracks.begin(), mot.tracks.end(), [](const MotTrack &t) { return t.w2 == mot::kTranslation; });
    if (root == mot.tracks.end()) {
        return;
    }
    for (MotKeyframe &key : root->keys) {
        if (InWindow(key, dlo, dhi)) {
            Vec4 v = Wide(key.Values());
            for (int i = 0; i < 3; i++) {
                v[i] += offset[i];
            }
            key.SetValues(v);
        }
    }
    ReplaceMot(pack, mot_name, mot);
}

void FreezeRoots(ChrPack &pack, std::string_view mot_name, int frame, int dlo, int dhi) {
    MotFile mot = MotOf(pack, mot_name);
    for (MotTrack &track : mot.tracks) {
        if (track.w2 != 2 ||
            std::none_of(track.keys.begin(), track.keys.end(), [&](const MotKeyframe &k) {
                return InWindow(k, dlo, dhi);
            })) {
            continue;
        }
        std::optional<Vec4> v = SampleTrack(track, frame);
        if (!v) {
            continue;
        }
        for (MotKeyframe &key : track.keys) {
            if (InWindow(key, dlo, dhi)) {
                key.SetValues(*v);
            }
        }
    }
    ReplaceMot(pack, mot_name, mot);
}

void PinNodes(ChrPack &pack, const Model &model, const std::vector<std::string_view> &nodes, int frame, int dlo, int dhi) {
    MotFile                  mot = MotOf(pack, model.mot);
    std::vector<std::string> frames = FramesOf(pack, model.mds);
    HoldTracks(
        mot, frame, dlo, dhi,
        [&](const MotTrack &t) {
            return t.w0 < frames.size() && std::find(nodes.begin(), nodes.end(), frames[t.w0]) != nodes.end();
        },
        "pin");
    ReplaceMot(pack, model.mot, mot);
}

void InjectShadow(ChrPack &base, const ChrPack &donor, const TownShadowInject &inject) {
    for (std::string_view name : {inject.model, inject.bbp, inject.wgt, inject.mot}) {
        const ChrRecord *record = donor.Find(name);
        if (!record) {
            Bad(std::format("shadow inject: the donor lacks {}", name));
        }
        if (base.Find(record->name)) {
            Bad(std::format("shadow inject: the base has {} already", record->name));
        }
        base.records.push_back(*record);
    }
    ChrRecord  &cfg = base.records[CfgMotionsOf(base).cfg];
    std::string text = ToText(cfg.Payload());
    std::size_t va = IndexOf(text, "\r\n", IndexOf(text, "VERTEX_ANIME")) + 2;
    text.insert(va, "SHADOW_VERTEX_ANIME 1\r\n");
    std::size_t eol = IndexOf(text, "\r\n", IndexOf(text, "MODEL ")) + 2;
    text.insert(eol, std::format("SHADOW_MODEL \"{}\"\r\n", inject.model));
    if (std::size_t at = text.find("SHADOW_MOTION"); at != std::string::npos) {
        std::size_t end = text.find_first_of("\r\n", at);
        end = end == std::string::npos ? text.size() : end;
        text.replace(at, end - at, std::format("SHADOW_MOTION \"{}\", \"{}\", \"{}\"", inject.mot, inject.bbp, inject.wgt));
    }
    cfg.ReplacePayload(ToBytes(text));
}

} // namespace expanded::town
