#include "expanded/data/model_parts.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace expanded {

namespace {

std::int32_t GetI32(std::span<const unsigned char> data, std::size_t offset) {
    return static_cast<std::int32_t>(Get32(data, offset));
}

std::uint16_t Get16(std::span<const unsigned char> data, std::size_t offset) {
    return static_cast<std::uint16_t>(data[offset] | (data[offset + 1] << 8));
}

void Set16(std::span<unsigned char> data, std::size_t offset, std::uint16_t value) {
    data[offset] = static_cast<unsigned char>(value);
    data[offset + 1] = static_cast<unsigned char>(value >> 8);
}

void SetF32(std::span<unsigned char> data, std::size_t offset, float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    Set32(data, offset, bits);
}

void Append32(Bytes &out, std::uint32_t value) {
    for (int i = 0; i < 4; i++) {
        out.push_back(static_cast<unsigned char>(value >> (8 * i)));
    }
}

void PadTo16(Bytes &out) {
    while (out.size() % 16 != 0) {
        out.push_back(0);
    }
}

Mat4 MatMul4(const Mat4 &a, const Mat4 &b) {
    Mat4 out{};
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            out[r][c] = ExactSum({a[r][0] * b[0][c], a[r][1] * b[1][c], a[r][2] * b[2][c], a[r][3] * b[3][c]});
        }
    }
    return out;
}

constexpr std::size_t kFrameRecord = 0x70;

} // namespace

double ExactSum(std::initializer_list<double> values) {
    double sum = 0.0, compensation = 0.0;
    for (double x : values) {
        double t = sum + x;
        compensation += std::fabs(sum) >= std::fabs(x) ? (sum - t) + x : (x - t) + sum;
        sum = t;
    }
    return sum + compensation;
}

Vec3 Add(const Vec3 &a, const Vec3 &b) {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Vec3 Scale(const Vec3 &a, double s) {
    return {a[0] * s, a[1] * s, a[2] * s};
}

Vec3 Cross(const Vec3 &a, const Vec3 &b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

double Dot(const Vec3 &a, const Vec3 &b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Vec3 Unit(const Vec3 &a) {
    double n = std::sqrt(Dot(a, a));
    return Scale(a, 1.0 / (n == 0.0 ? 1.0 : n));
}

Mat3 MatMul(const Mat3 &a, const Mat3 &b) {
    Mat3 out{};
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            out[r][c] = a[r][0] * b[0][c] + a[r][1] * b[1][c] + a[r][2] * b[2][c];
        }
    }
    return out;
}

Vec3 MatVec(const Mat3 &m, const Vec3 &v) {
    return {Dot(m[0], v), Dot(m[1], v), Dot(m[2], v)};
}

Mat3 AxisRotation(const Vec3 &k, double angle) {
    double c = std::cos(angle), s = std::sin(angle), C = 1 - c, x = k[0], y = k[1], z = k[2];
    return {
        {
         {c + x * x * C, x * y * C - z * s, x * z * C + y * s},
         {y * x * C + z * s, c + y * y * C, y * z * C - x * s},
         {z * x * C - y * s, z * y * C + x * s, c + z * z * C},
         }
    };
}

Mat3 RotationYTo(const Vec3 &direction) {
    Vec3   d = Unit(direction);
    Vec3   y = {0.0, 1.0, 0.0};
    double c = std::clamp(Dot(y, d), -1.0, 1.0);
    Vec3   axis = Cross(y, d);
    double s = std::sqrt(Dot(axis, axis));
    if (s < 1e-6) {
        return c > 0 ? Mat3{
                           {{1, 0, 0}, {0, c, 0}, {0, 0, c}}
        }
                     : Mat3{{{1, 0, 0}, {0, -1, 0}, {0, 0, -1}}};
    }
    return AxisRotation(Scale(axis, 1.0 / s), std::atan2(s, c));
}

std::array<double, 4> SlerpKeys(const std::array<float, 4> &a, const std::array<float, 4> &bf, double t) {
    std::array<double, 4> b = {bf[0], bf[1], bf[2], bf[3]};
    double                d = double(a[0]) * b[0] + double(a[1]) * b[1] + double(a[2]) * b[2] + double(a[3]) * b[3];
    if (d < 0) {
        for (double &v : b) {
            v = -v;
        }
        d = -d;
    }
    double w0, w1;
    if (d > 0.9995) {
        w0 = 1 - t;
        w1 = t;
    } else {
        double th = std::acos(std::min(1.0, d)), sn = std::sin(th);
        w0 = std::sin((1 - t) * th) / sn;
        w1 = std::sin(t * th) / sn;
    }
    std::array<double, 4> q;
    for (int i = 0; i < 4; i++) {
        q[i] = w0 * a[i] + w1 * b[i];
    }
    double n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    for (double &v : q) {
        v /= (n == 0 ? 1.0 : n);
    }
    return q;
}

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
    int                   count = static_cast<int>(Get32(mds, 8));
    std::size_t           table = Get32(mds, 12);
    std::vector<MdsFrame> frames(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        std::size_t rec = table + static_cast<std::size_t>(i) * kFrameRecord;
        MdsFrame   &frame = frames[static_cast<std::size_t>(i)];
        frame.index = i;
        frame.name = NameAt(mds, rec + 8, 0x20);
        frame.mesh = static_cast<int>(Get32(mds, rec + 0x28));
        frame.parent = GetI32(mds, rec + 0x2C);
        if (frame.parent >= i) {
            throw std::runtime_error("an .mds frame's parent comes after it");
        }
        auto record = Slice(mds, rec, kFrameRecord, "an .mds frame record");
        frame.record.assign(record.begin(), record.end());
        Mat4 local{};
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                local[r][c] = GetF32(mds, rec + 0x30 + r * 16 + c * 4);
            }
        }
        for (int c = 0; c < 3; c++) {
            local[3][c] = GetF32(mds, rec + 0x60 + c * 4);
        }
        local[3][3] = 1.0;
        frame.local = local;
        frame.world = frame.parent < 0 ? local : MatMul4(local, frames[static_cast<std::size_t>(frame.parent)].world);
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

Bytes ReplaceMeshes(std::span<const unsigned char> mds, const std::map<std::string, std::optional<Bytes>> &replaced,
                    const std::vector<std::pair<Bytes, Bytes>> &appended) {
    int                               count = static_cast<int>(Get32(mds, 8));
    std::size_t                       table = Get32(mds, 12);
    std::vector<Bytes>                records;
    std::vector<std::optional<Bytes>> meshes;
    for (int i = 0; i < count; i++) {
        std::size_t rec = table + static_cast<std::size_t>(i) * kFrameRecord;
        records.emplace_back(mds.begin() + static_cast<std::ptrdiff_t>(rec), mds.begin() + static_cast<std::ptrdiff_t>(rec + kFrameRecord));
        std::string name = NameAt(mds, rec + 8, 0x20);
        std::size_t off = Get32(mds, rec + 0x28);
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
    std::size_t base = 0x10 + records.size() * kFrameRecord;
    Bytes       blob;
    for (std::size_t i = 0; i < records.size(); i++) {
        if (meshes[i]) {
            Set32(records[i], 0x28, static_cast<std::uint32_t>(base + blob.size()));
            blob.insert(blob.end(), meshes[i]->begin(), meshes[i]->end());
            PadTo16(blob);
        } else {
            Set32(records[i], 0x28, 0);
        }
    }
    Bytes out(mds.begin(), mds.begin() + 8);
    Append32(out, static_cast<std::uint32_t>(records.size()));
    Append32(out, 0x10);
    for (const Bytes &record : records) {
        out.insert(out.end(), record.begin(), record.end());
    }
    out.insert(out.end(), blob.begin(), blob.end());
    return out;
}

Bytes FrameRecord(int index, std::string_view name, int parent) {
    Bytes record(kFrameRecord, 0);
    Set32(record, 0, static_cast<std::uint32_t>(index));
    Set32(record, 4, kFrameRecord);
    std::copy(name.begin(), name.end(), record.begin() + 8);
    Set32(record, 0x2C, static_cast<std::uint32_t>(parent));
    for (int i = 0; i < 4; i++) {
        SetF32(record, 0x30 + static_cast<std::size_t>(i) * 20, 1.0f);
    }
    return record;
}

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
    for (std::size_t i = 0; mat + (i + 1) * 0x60 <= total; i++) {
        m.materials.emplace_back(mdt.begin() + static_cast<std::ptrdiff_t>(mat + i * 0x60), mdt.begin() + static_cast<std::ptrdiff_t>(mat + (i + 1) * 0x60));
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
        {"POS",  m.positions.size() * 16    },
        {"DL",   dl_size                    },
        {"UV",   m.normals.size() * 16      },
        {"NORM", m.uvs.size() * 16          },
        {"COL",  m.colour_values.size() * 16},
        {"MAT",  m.materials.size() * 0x60  },
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

ImgBank ImgBank::Parse(std::span<const unsigned char> bank) {
    if (bank.size() < 0x10 || bank[0] != 'I' || bank[1] != 'M' || (bank[2] != 'G' && bank[2] != '2') || bank[3] != 0) {
        throw std::runtime_error("not an IMG or IM2 bank");
    }
    ImgBank out;
    std::copy(bank.begin(), bank.begin() + 4, out.magic.begin());
    int                                              count = static_cast<int>(Get32(bank, 4));
    std::vector<std::pair<std::string, std::size_t>> table;
    for (int i = 0; i < count; i++) {
        std::size_t entry = 0x10 + static_cast<std::size_t>(i) * 0x30;
        table.emplace_back(NameAt(bank, entry, 0x20), Get32(bank, entry + 0x20));
    }
    for (const auto &[name, offset] : table) {
        std::size_t next = bank.size();
        for (const auto &other : table) {
            if (other.second > offset) {
                next = std::min(next, other.second);
            }
        }
        auto block = Slice(bank, offset, next - offset, "a texture bank entry");
        out.entries.emplace_back(name, Bytes(block.begin(), block.end()));
    }
    return out;
}

const Bytes &ImgBank::Block(std::string_view name) const {
    for (const auto &[entry, block] : entries) {
        if (entry == name) {
            return block;
        }
    }
    throw std::runtime_error("texture bank lacks " + std::string(name));
}

Bytes ImgBank::Build() const {
    Bytes out(magic.begin(), magic.end());
    Append32(out, static_cast<std::uint32_t>(entries.size()));
    out.resize(0x10 + entries.size() * 0x30, 0);
    for (std::size_t i = 0; i < entries.size(); i++) {
        std::size_t        entry = 0x10 + i * 0x30;
        const std::string &name = entries[i].first;
        std::copy_n(name.begin(), std::min<std::size_t>(name.size(), 0x1F), out.begin() + static_cast<std::ptrdiff_t>(entry));
        Set32(out, entry + 0x20, static_cast<std::uint32_t>(out.size()));
        out.insert(out.end(), entries[i].second.begin(), entries[i].second.end());
        PadTo16(out);
    }
    return out;
}

namespace tim8 {

namespace {

int BlockOffset(int x, int y, int w) {
    int block = (y & ~0xF) * w + (x & ~0xF) * 2;
    int swap = (((y + 2) >> 2) & 0x1) * 4;
    int row = (((y & ~3) >> 1) + (y & 1)) & 0x7;
    int column = row * w * 2 + ((x + swap) & 0x7) * 4;
    int byte = ((y >> 1) & 1) + ((x >> 2) & 2);
    return block + column + byte;
}

} // namespace

Picture Read(std::span<const unsigned char> tim, bool swizzled) {
    if (tim.size() < kPicture + 0x30 || tim[0] != 'T' || tim[1] != 'I' || tim[2] != 'M' || tim[3] != '2' || tim[kPicture + 0x13] != 5) {
        throw std::runtime_error("not an 8-bit TIM2 picture");
    }
    std::size_t clut = Get32(tim, kPicture + 4), image = Get32(tim, kPicture + 8), header = Get16(tim, kPicture + 0xC);
    Picture     p;
    p.width = Get16(tim, kPicture + 0x14);
    p.height = Get16(tim, kPicture + 0x16);
    auto pixels = Slice(tim, kPicture + header, image, "a TIM2 picture's pixels");
    p.pixels = swizzled ? Unswizzle(pixels, p.width, p.height) : Bytes(pixels.begin(), pixels.end());
    auto colours = Slice(tim, kPicture + header + image, clut, "a TIM2 picture's CLUT");
    p.clut.assign(colours.begin(), colours.end());
    return p;
}

Bytes Build(std::span<const unsigned char> templ, int width, int height, std::span<const unsigned char> pixels,
            std::span<const unsigned char> clut) {
    if (templ[kPicture + 0x13] != 5 || Get16(templ, kPicture + 0xC) != 0x30) {
        throw std::runtime_error("TIM2 template is not an 8-bit picture with a 0x30 header");
    }
    Bytes out(templ.begin(), templ.begin() + kPicture + 0x30);
    Set32(out, kPicture, static_cast<std::uint32_t>(0x30 + pixels.size() + clut.size()));
    Set32(out, kPicture + 4, static_cast<std::uint32_t>(clut.size()));
    Set32(out, kPicture + 8, static_cast<std::uint32_t>(pixels.size()));
    Set16(out, kPicture + 0x14, static_cast<std::uint16_t>(width));
    Set16(out, kPicture + 0x16, static_cast<std::uint16_t>(height));
    std::fill(out.begin() + kPicture + 0x18, out.begin() + kPicture + 0x30, 0);
    out.insert(out.end(), pixels.begin(), pixels.end());
    out.insert(out.end(), clut.begin(), clut.end());
    return out;
}

Bytes Resample(std::span<const unsigned char> tim, bool swizzled, int n) {
    Picture     p = Read(tim, swizzled);
    std::size_t header = Get16(tim, kPicture + 0xC);
    Bytes       pixels(static_cast<std::size_t>(n * n));
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            pixels[static_cast<std::size_t>(y * n + x)] =
                p.pixels[static_cast<std::size_t>(((y * p.height + p.height / 2) / n) * p.width + (x * p.width + p.width / 2) / n)];
        }
    }
    Bytes out(tim.begin(), tim.begin() + static_cast<std::ptrdiff_t>(kPicture + header));
    Set32(out, kPicture, static_cast<std::uint32_t>(header + 4 * static_cast<std::size_t>(n * n)));
    Set32(out, kPicture + 8, static_cast<std::uint32_t>(n * n));
    Set16(out, kPicture + 0x14, static_cast<std::uint16_t>(n));
    Set16(out, kPicture + 0x16, static_cast<std::uint16_t>(n));
    out.insert(out.end(), pixels.begin(), pixels.end());
    out.insert(out.end(), p.clut.begin(), p.clut.end());
    return out;
}

Bytes Swizzle(std::span<const unsigned char> row_major, int width, int height) {
    Bytes out(static_cast<std::size_t>(width * height));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            out[static_cast<std::size_t>(BlockOffset(x, y, width))] = row_major[static_cast<std::size_t>(y * width + x)];
        }
    }
    return out;
}

Bytes Unswizzle(std::span<const unsigned char> blocks, int width, int height) {
    Bytes out(static_cast<std::size_t>(width * height));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            std::size_t from = static_cast<std::size_t>(BlockOffset(x, y, width));
            if (from < blocks.size()) {
                out[static_cast<std::size_t>(y * width + x)] = blocks[from];
            }
        }
    }
    return out;
}

Bytes Swizzled(std::span<const unsigned char> tim) {
    std::size_t header = Get16(tim, kPicture + 0xC);
    int         width = Get16(tim, kPicture + 0x14), height = Get16(tim, kPicture + 0x16);
    Bytes       out(tim.begin(), tim.end());
    Bytes       blocks = Swizzle(Slice(tim, kPicture + header, static_cast<std::size_t>(width) * height, "a TIM2 picture's pixels"), width, height);
    std::copy(blocks.begin(), blocks.end(), out.begin() + static_cast<std::ptrdiff_t>(kPicture + header));
    return out;
}

int Csm1(int i) {
    return (i & ~0x18) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
}

} // namespace tim8

} // namespace expanded
