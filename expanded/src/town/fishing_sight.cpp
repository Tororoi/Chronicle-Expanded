#include "expanded/town/fishing_sight.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>

#include "data/data_read.hpp"
#include "expanded/core/disc_files.hpp"
#include "expanded/data/cat_rig.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/debug/event_log.hpp"

namespace expanded {

namespace {

constexpr const char *kPointerFile = "dun/item/main_data/juelaqua.mds", *kPointerMesh = "juelaqua__m";

} // namespace

std::vector<SightPoint> BuildSightPointer(std::span<const unsigned char> mds) {
    std::vector<SightPoint>     pointer;
    const std::vector<MdsFrame> frames = ReadMdsSkeleton(mds);
    const MdsFrame             *frame = FindFrame(frames, kPointerMesh);
    if (frame == nullptr || frame->mesh == 0) {
        return pointer;
    }
    const MdtMesh     mesh = MdtMesh::Parse(MdtAt(mds, static_cast<std::size_t>(frame->mesh)));
    std::vector<Vec3> points;
    double            low[3] = {1e9, 1e9, 1e9}, high[3] = {-1e9, -1e9, -1e9};
    for (const auto &p : mesh.positions) {
        const Vec3 at = XformPt(frame->world, {p[0], p[1], p[2]});
        points.push_back(at);
        for (int a = 0; a < 3; a++) {
            low[a] = std::min(low[a], at[a]);
            high[a] = std::max(high[a], at[a]);
        }
    }
    // The gem's point is its +y: along from its round end, round its middle.
    for (const rig::Tri &t : rig::MdtTriangles(mesh)) {
        for (int corner : t) {
            const Vec3 &p = points[static_cast<std::size_t>(corner)];
            pointer.push_back({static_cast<float>(p[1] - low[1]), static_cast<float>(p[2] - (low[2] + high[2]) / 2.0),
                               static_cast<float>(p[0] - (low[0] + high[0]) / 2.0)});
        }
    }
    return pointer;
}

const FishingSight *LoadedFishingSight() {
    static std::optional<FishingSight> loaded;
    static bool                        tried = false;
    if (!tried) {
        tried = true;
        std::vector<std::uint8_t> gem;
        if (!DataReadReadFile(DiscFile(kPointerFile), gem)) {
            LogEvent("fishing_sight", std::string("\"error\":\"cannot read ") + kPointerFile + "\"");
        } else {
            try {
                FishingSight sight;
                sight.pointer = BuildSightPointer(gem);
                if (sight.Complete()) {
                    loaded = std::move(sight);
                }
            } catch (const std::exception &e) {
                LogEvent("fishing_sight", std::string("\"error\":\"") + e.what() + "\"");
            }
        }
    }
    return loaded ? &*loaded : nullptr;
}

} // namespace expanded
