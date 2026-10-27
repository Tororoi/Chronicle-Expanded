#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include "expanded/data/model_parts.hpp"

namespace expanded {

namespace {

std::vector<std::array<double, 4>> ReadVectors(std::span<const unsigned char> mdt, std::size_t at, std::size_t count) {
    std::vector<std::array<double, 4>> out(count);
    for (std::size_t i = 0; i < count; i++) {
        for (int k = 0; k < 4; k++) {
            out[i][k] = GetF32(mdt, at + i * 16 + static_cast<std::size_t>(k) * 4);
        }
    }
    return out;
}

Bytes VectorBytes(const std::vector<std::array<double, 4>> &vectors) {
    Bytes out(vectors.size() * 16);
    for (std::size_t i = 0; i < vectors.size(); i++) {
        for (int k = 0; k < 4; k++) {
            SetF32(out, i * 16 + static_cast<std::size_t>(k) * 4, static_cast<float>(vectors[i][k]));
        }
    }
    return out;
}

} // namespace

MdtMesh MdtMesh::Parse(std::span<const unsigned char> mdt) {
    if (mdt.size() < 0x40 || mdt[0] != 'M' || mdt[1] != 'D' || mdt[2] != 'T') {
        throw std::runtime_error("not an MDT mesh");
    }
    MdtMesh m;
    for (int i = 0; i < 16; i++) {
        m.header[static_cast<std::size_t>(i)] = Get32(mdt, static_cast<std::size_t>(i) * 4);
    }
    std::size_t   total = m.header[2];
    std::size_t   pos = m.header[4], normals = m.header[6], dl_size = m.header[9], dl = m.header[10], uv = m.header[12], mat = m.header[14];
    std::uint32_t col = m.header[8];
    m.colours = col > 0 && col < 0x80000000u;
    int stride = m.colours ? 4 : 3;
    if (total < 0x40 || total > mdt.size()) {
        throw std::runtime_error("MDT size runs past its data");
    }
    for (int i = 0; i < 4; i++) {
        m.preamble[static_cast<std::size_t>(i)] = GetI32(mdt, dl + static_cast<std::size_t>(i) * 4);
    }
    std::size_t o = dl + 0x10;
    int         most_normal = -1, most_uv = -1, most_colour = -1;
    for (int k = 0; k < m.preamble[2]; k++) {
        Submesh sub;
        sub.prim = GetI32(mdt, o);
        int count = GetI32(mdt, o + 4);
        sub.material = GetI32(mdt, o + 8);
        o += 0xC;
        if (count < 0 || o + static_cast<std::size_t>(count * stride * 4) > total) {
            throw std::runtime_error("MDT display list runs past its data");
        }
        for (int r = 0; r < count; r++) {
            std::vector<int> record(static_cast<std::size_t>(stride));
            for (int c = 0; c < stride; c++) {
                record[static_cast<std::size_t>(c)] = GetI32(mdt, o + static_cast<std::size_t>((r * stride + c) * 4));
            }
            most_normal = std::max(most_normal, record[1]);
            most_uv = std::max(most_uv, record[2]);
            if (m.colours) {
                most_colour = std::max(most_colour, record[3]);
            }
            sub.records.push_back(std::move(record));
        }
        o += static_cast<std::size_t>(count * stride * 4);
        m.submeshes.push_back(std::move(sub));
    }
    m.positions = ReadVectors(mdt, pos, m.header[3]);
    m.normals = ReadVectors(mdt, normals, static_cast<std::size_t>(most_normal + 1));
    m.uvs = uv > 0 ? ReadVectors(mdt, uv, static_cast<std::size_t>(most_uv + 1)) : std::vector<std::array<double, 4>>{};
    if (m.colours) {
        m.colour_values = ReadVectors(mdt, col, static_cast<std::size_t>(most_colour + 1));
    }
    for (std::size_t i = 0; mat + (i + 1) * kMaterialSize <= total; i++) {
        m.materials.emplace_back(mdt.begin() + static_cast<std::ptrdiff_t>(mat + i * kMaterialSize), mdt.begin() + static_cast<std::ptrdiff_t>(mat + (i + 1) * kMaterialSize));
    }
    std::vector<std::pair<std::string, std::size_t>> named = {
        {"POS", pos    },
        {"DL",  dl     },
        {"UV",  normals}
    };
    if (uv > 0) {
        named.emplace_back("NORM", uv);
    }
    named.emplace_back("MAT", mat);
    if (m.colours) {
        named.emplace_back("COL", col);
    }
    std::stable_sort(named.begin(), named.end(), [](const auto &a, const auto &b) { return a.second < b.second; });
    std::map<std::string, std::size_t> sizes = {
        {"POS",  m.positions.size() * 16           },
        {"DL",   dl_size                           },
        {"UV",   m.normals.size() * 16             },
        {"NORM", m.uvs.size() * 16                 },
        {"COL",  m.colour_values.size() * 16       },
        {"MAT",  m.materials.size() * kMaterialSize},
    };
    for (std::size_t i = 0; i < named.size(); i++) {
        const auto &[name, at] = named[i];
        std::size_t next = i + 1 < named.size() ? named[i + 1].second : total;
        std::size_t from = at + sizes[name];
        m.order.push_back(name);
        m.pads[name] = next > from ? Bytes(mdt.begin() + static_cast<std::ptrdiff_t>(from), mdt.begin() + static_cast<std::ptrdiff_t>(next)) : Bytes{};
    }
    return m;
}

void MdtMesh::AlignDisplayList() {
    std::size_t dl = 0x10;
    for (const Submesh &sub : submeshes) {
        dl += 0xC + sub.records.size() * (colours ? 16 : 12);
    }
    pads["DL"] = Bytes((16 - dl % 16) % 16, 0);
}

Bytes MdtMesh::Build() const {
    Bytes dl;
    Append32(dl, static_cast<std::uint32_t>(preamble[0]));
    Append32(dl, static_cast<std::uint32_t>(preamble[1]));
    Append32(dl, static_cast<std::uint32_t>(submeshes.size()));
    Append32(dl, static_cast<std::uint32_t>(preamble[3]));
    for (const Submesh &sub : submeshes) {
        Append32(dl, static_cast<std::uint32_t>(sub.prim));
        Append32(dl, static_cast<std::uint32_t>(sub.records.size()));
        Append32(dl, static_cast<std::uint32_t>(sub.material));
        for (const std::vector<int> &record : sub.records) {
            for (int value : record) {
                Append32(dl, static_cast<std::uint32_t>(value));
            }
        }
    }
    Bytes mat;
    for (const Bytes &material : materials) {
        mat.insert(mat.end(), material.begin(), material.end());
    }
    std::map<std::string, Bytes> blocks = {
        {"POS",  VectorBytes(positions)    },
        {"DL",   dl                        },
        {"UV",   VectorBytes(normals)      },
        {"NORM", VectorBytes(uvs)          },
        {"COL",  VectorBytes(colour_values)},
        {"MAT",  mat                       },
    };
    Bytes                              out(0x40, 0);
    std::map<std::string, std::size_t> offsets;
    for (const std::string &name : order) {
        offsets[name] = out.size();
        out.insert(out.end(), blocks[name].begin(), blocks[name].end());
        auto pad = pads.find(name);
        if (pad != pads.end()) {
            out.insert(out.end(), pad->second.begin(), pad->second.end());
        }
    }
    std::array<std::uint32_t, 16> h = header;
    h[2] = static_cast<std::uint32_t>(out.size());
    h[3] = static_cast<std::uint32_t>(positions.size());
    h[4] = static_cast<std::uint32_t>(offsets["POS"]);
    h[6] = static_cast<std::uint32_t>(offsets["UV"]);
    h[8] = colours ? static_cast<std::uint32_t>(offsets["COL"]) : header[8];
    h[9] = static_cast<std::uint32_t>(dl.size());
    h[10] = static_cast<std::uint32_t>(offsets["DL"]);
    h[12] = uvs.empty() ? 0 : static_cast<std::uint32_t>(offsets["NORM"]);
    h[14] = static_cast<std::uint32_t>(offsets["MAT"]);
    for (std::size_t i = 0; i < 16; i++) {
        Set32(out, i * 4, h[i]);
    }
    return out;
}

} // namespace expanded
