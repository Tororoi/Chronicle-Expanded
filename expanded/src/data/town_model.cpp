#include "expanded/data/town_model.hpp"

#include <algorithm>
#include <format>
#include <functional>
#include <map>
#include <string>

#include "expanded/data/cfg_text.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/town_rig.hpp"

namespace expanded {

using namespace town;

void GraftMotionWindow(ChrPack &base, const ChrPack &src, std::string_view src_cfg, int slo, int shi, int dlo, int dhi) {
    GraftWindow(base, CfgMotionsOf(base), src, CfgMotionsOf(src, src_cfg), slo, shi, dlo, dhi, false, {});
}

Bytes RewriteTownKeys(std::span<const unsigned char> cfg, const std::vector<TownSlot> &slots) {
    std::vector<const TownSlot *> ordered;
    for (const TownSlot &slot : slots) {
        ordered.push_back(&slot);
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const TownSlot *a, const TownSlot *b) { return a->idx < b->idx; });
    std::string keys;
    for (const TownSlot *slot : ordered) {
        keys += std::format("KEY\t{},\t{},\t{:.2f}, //{}\r\n", slot->lo, slot->hi, slot->speed, slot->name.substr(0, 12));
    }
    return ReplaceCfgKeys(cfg, keys);
}

Bytes BuildTownModel(const TownAlly &ally, const DiscReader &read) {
    ChrPack base = ChrPack::Parse(read(ally.base));
    if (ally.shadow_inject) {
        InjectShadow(base, ChrPack::Parse(read(ally.shadow_inject->src)), *ally.shadow_inject);
    }
    CfgMotions                          cfg = CfgMotionsOf(base);
    std::map<std::string_view, ChrPack> cache;

    auto source = [&](std::string_view path) -> const ChrPack & {
        auto found = cache.find(path);
        if (found == cache.end()) {
            found = cache.emplace(path, ChrPack::Parse(read(path))).first;
        }
        return found->second;
    };
    int max_frame = 0;
    for (const TownSlot &slot : ally.slots) {
        max_frame = std::max(max_frame, slot.hi);
    }
    const auto &graft = ally.mesh_graft;
    if (graft) {
        std::vector<Span> windows;
        for (const TownSlot &slot : ally.slots) {
            if (slot.src == graft->src) {
                windows.push_back({slot.lo, slot.hi});
            }
        }
        GraftMeshNodes(base, source(graft->src), *graft, windows, max_frame);
    }
    bool              rm = ally.root_motion;
    std::vector<Span> rm_spans;
    if (rm) {
        std::vector<Span> windows;
        for (const TownSlot &slot : ally.slots) {
            if (!slot.src.empty() || slot.hold) {
                windows.push_back({slot.lo, slot.hi});
            }
        }
        rm_spans = FoldSpans(windows, 1, max_frame);
    }
    Model body{cfg.body_mot, cfg.body_mds};
    for (const TownSlot &slot : ally.slots) {
        int dlo = slot.lo, dhi = slot.hi;
        if (!slot.src.empty()) {
            const ChrPack &sp = source(slot.src);
            CfgMotions     scfg = CfgMotionsOf(sp, slot.src_cfg);
            if (graft && slot.src != graft->src) {
                std::vector<std::string> frames = FramesOf(sp, scfg.body_mds);
                for (std::string_view node : graft->nodes) {
                    if (Index(frames, node) >= 0) {
                        Bad(std::format("slot {}: its source carries the grafted node {}", slot.name, node));
                    }
                }
            }
            auto [wlo, whi] = slot.win;
            GraftWindow(base, cfg, sp, scfg, wlo, whi, dlo, dhi, rm, rm_spans);
        }
        // Each post-op runs on the body, then the shadow; pins on the body alone.
        auto both = [&](const std::function<void(std::string_view)> &op) {
            op(cfg.body_mot);
            if (cfg.shadow_mot) {
                op(*cfg.shadow_mot);
            }
        };
        if (slot.hold) {
            both([&](std::string_view mot) { BakeHold(base, mot, *slot.hold, dlo, dhi); });
        }
        if (slot.root_offset) {
            both([&](std::string_view mot) { ApplyRootOffset(base, mot, dlo, dhi, *slot.root_offset); });
        }
        if (slot.reverse) {
            both([&](std::string_view mot) { ReverseWindow(base, mot, dlo, dhi); });
        }
        if (slot.root_freeze) {
            both([&](std::string_view mot) { FreezeRoots(base, mot, *slot.root_freeze, dlo, dhi); });
        }
        if (!slot.pin_nodes.empty()) {
            PinNodes(base, body, slot.pin_nodes, slot.pin_frame, dlo, dhi);
        }
    }
    if (graft && graft->spin) {
        ApplySpin(base, *graft);
    }
    ChrRecord &cfg_record = base.records[cfg.cfg];
    cfg_record.ReplacePayload(RewriteTownKeys(cfg_record.Payload(), ally.slots));
    return base.Rebuild();
}

std::vector<DataBuild> TownModelBuilds() {
    std::vector<DataBuild> builds;
    for (const TownAlly &ally : TownAllies()) {
        auto build = [&ally](const DiscReader &read) { return BuildTownModel(ally, read); };
        builds.push_back(DataBuild{std::string(ally.name), std::string(ally.file), {std::string(ally.base)}, build});
    }
    return builds;
}

} // namespace expanded
