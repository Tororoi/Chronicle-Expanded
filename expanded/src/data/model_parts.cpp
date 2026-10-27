#include "expanded/data/model_parts.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace expanded {

Mat3 MdsFrame::WorldRotation() const {
    return {
        {{world[0][0], world[0][1], world[0][2]}, {world[1][0], world[1][1], world[1][2]}, {world[2][0], world[2][1], world[2][2]}}
    };
}

Vec3 MdsFrame::WorldPosition() const {
    return {world[3][0], world[3][1], world[3][2]};
}

Vec3 MdsFrame::ToLocal(const Vec3 &world_direction) const {
    // Row vectors (world = local x M): a world direction comes back through M's rotation rows.
    return MatVec(WorldRotation(), world_direction);
}

std::vector<MdsFrame> ReadMdsSkeleton(std::span<const unsigned char> mds) {
    int                   count = static_cast<int>(Get32(mds, mds::kCountAt));
    std::size_t           table = Get32(mds, mds::kTableAt);
    std::vector<MdsFrame> frames(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        std::size_t rec = table + static_cast<std::size_t>(i) * mds::kRecord;
        MdsFrame   &frame = frames[static_cast<std::size_t>(i)];
        frame.index = i;
        frame.name = NameAt(mds, rec + mds::kName, mds::kNameSize);
        frame.mesh = static_cast<int>(Get32(mds, rec + mds::kMesh));
        frame.parent = GetI32(mds, rec + mds::kParent);
        if (frame.parent >= i) {
            throw std::runtime_error("an .mds frame's parent comes after it");
        }
        auto record = Slice(mds, rec, mds::kRecord, "an .mds frame record");
        frame.record.assign(record.begin(), record.end());
        Mat4 local{};
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                local[r][c] = GetF32(mds, rec + mds::kRotation + r * 16 + c * 4);
            }
        }
        for (int c = 0; c < 3; c++) {
            local[3][c] = GetF32(mds, rec + mds::kPosition + c * 4);
        }
        local[3][3] = 1.0;
        frame.local = local;
        frame.world = frame.parent < 0 ? local : MatMulExact(local, frames[static_cast<std::size_t>(frame.parent)].world);
    }
    return frames;
}

const MdsFrame *FindFrame(const std::vector<MdsFrame> &frames, std::string_view name) {
    for (const MdsFrame &frame : frames) {
        if (frame.name == name) {
            return &frame;
        }
    }
    return nullptr;
}

Bytes MdtAt(std::span<const unsigned char> mds, std::size_t offset) {
    auto mesh = Slice(mds, offset, Get32(mds, offset + 8), "an MDT mesh");
    return Bytes(mesh.begin(), mesh.end());
}

Bytes LayOutMds(std::span<const unsigned char> source, std::vector<Bytes> records, const std::vector<std::optional<Bytes>> &meshes) {
    std::size_t base = mds::kTable + records.size() * mds::kRecord;
    Bytes       body;
    for (std::size_t i = 0; i < records.size(); i++) {
        bool has = meshes[i] && !meshes[i]->empty();
        Set32(records[i], mds::kMesh, has ? static_cast<std::uint32_t>(base + body.size()) : 0u);
        if (has) {
            body.insert(body.end(), meshes[i]->begin(), meshes[i]->end());
            PadTo16(body);
        }
    }
    Bytes out(source.begin(), source.begin() + 8);
    Append32(out, static_cast<std::uint32_t>(records.size()));
    Append32(out, mds::kTable);
    for (const Bytes &record : records) {
        out.insert(out.end(), record.begin(), record.end());
    }
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

Bytes ReplaceMeshes(std::span<const unsigned char> mds, const std::map<std::string, std::optional<Bytes>> &replaced,
                    const std::vector<std::pair<Bytes, Bytes>> &appended) {
    int                               count = static_cast<int>(Get32(mds, mds::kCountAt));
    std::size_t                       table = Get32(mds, mds::kTableAt);
    std::vector<Bytes>                records;
    std::vector<std::optional<Bytes>> meshes;
    for (int i = 0; i < count; i++) {
        std::size_t rec = table + static_cast<std::size_t>(i) * mds::kRecord;
        records.emplace_back(mds.begin() + static_cast<std::ptrdiff_t>(rec), mds.begin() + static_cast<std::ptrdiff_t>(rec + mds::kRecord));
        std::string name = NameAt(mds, rec + mds::kName, mds::kNameSize);
        std::size_t off = Get32(mds, rec + mds::kMesh);
        auto        found = replaced.find(name);
        if (found != replaced.end()) {
            meshes.push_back(found->second);
        } else if (off != 0 && off < mds.size()) {
            meshes.push_back(MdtAt(mds, off));
        } else {
            meshes.push_back(std::nullopt);
        }
    }
    for (const auto &[record, mdt] : appended) {
        records.push_back(record);
        meshes.push_back(mdt);
    }
    return LayOutMds(mds, std::move(records), meshes);
}

Bytes AppendMdsFrames(std::span<const unsigned char> mds, const std::vector<std::pair<Bytes, std::optional<Bytes>>> &frames) {
    std::size_t count = Get32(mds, mds::kCountAt), meshes = mds::kTable + count * mds::kRecord;
    std::size_t moved = frames.size() * mds::kRecord, appended_at = mds.size() + moved;
    Bytes       out(mds.begin(), mds.begin() + static_cast<std::ptrdiff_t>(meshes));
    Set32(out, mds::kCountAt, static_cast<std::uint32_t>(count + frames.size()));
    for (std::size_t i = 0; i < count; i++) {
        std::size_t at = mds::kTable + i * mds::kRecord + mds::kMesh;
        if (std::uint32_t mesh = Get32(out, at); mesh != 0) {
            Set32(out, at, static_cast<std::uint32_t>(mesh + moved));
        }
    }
    Bytes appended;
    for (std::size_t k = 0; k < frames.size(); k++) {
        const auto &[source, mdt] = frames[k];
        Bytes record = source;
        Set32(record, mds::kIndex, static_cast<std::uint32_t>(count + k));
        Set32(record, mds::kSize, mds::kRecord);
        Set32(record, mds::kMesh, mdt ? static_cast<std::uint32_t>(appended_at + appended.size()) : 0u);
        if (mdt) {
            appended.insert(appended.end(), mdt->begin(), mdt->end());
            PadTo16(appended);
        }
        out.insert(out.end(), record.begin(), record.end());
    }
    out.insert(out.end(), mds.begin() + static_cast<std::ptrdiff_t>(meshes), mds.end());
    out.insert(out.end(), appended.begin(), appended.end());
    return out;
}

Bytes FrameRecord(int index, std::string_view name, int parent, const Mat3 &rotation, const Vec3 &position) {
    Bytes record(mds::kRecord, 0);
    Set32(record, mds::kIndex, static_cast<std::uint32_t>(index));
    Set32(record, mds::kSize, mds::kRecord);
    std::copy(name.begin(), name.end(), record.begin() + mds::kName);
    Set32(record, mds::kParent, static_cast<std::uint32_t>(parent));
    for (std::size_t i = 0; i < 3; i++) {
        for (std::size_t j = 0; j < 3; j++) {
            SetF32(record, mds::kRotation + i * 16 + j * 4, static_cast<float>(rotation[i][j]));
        }
        SetF32(record, mds::kPosition + i * 4, static_cast<float>(position[i]));
    }
    SetF32(record, mds::kPosition + 12, 1.0f);
    return record;
}

} // namespace expanded
