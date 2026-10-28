#include "expanded/data/queens_canal.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <tuple>

#include "expanded/data/fishing_scripts.hpp"
#include "expanded/data/model_parts.hpp"

namespace expanded {

namespace {

using scene::Triangle;
using scene::Vec3;

// ---- An MDT in single precision, read from a model and laid out again in the order POS, DL, UV, NORM, COL, MAT. The
// block the header calls UV @0x18 holds the normals and the one it calls NORM @0x30 the texture coordinates; a record is
// (position, normal, texture coordinate[, colour]).

using Vector = std::array<float, 4>;
using Record = std::vector<int>;

struct FloatMesh {
    struct Submesh {
        int                 prim = 0; // 3 list, 4 strip
        int                 material = 0;
        std::vector<Record> records;
    };

    std::array<std::uint32_t, 16> header{};
    std::array<std::uint32_t, 4>  preamble{};
    bool                          colours = false;
    std::vector<Vector>           positions, normals, uvs, colour_values;
    std::vector<Submesh>          submeshes;
    std::vector<Bytes>            materials;
};

std::vector<Vector> ReadVectors(std::span<const unsigned char> mdt, std::size_t at, std::size_t count) {
    std::vector<Vector> out(count);
    for (std::size_t i = 0; i < count; i++) {
        for (std::size_t k = 0; k < 4; k++) {
            out[i][k] = GetF32(mdt, at + i * 16 + k * 4);
        }
    }
    return out;
}

FloatMesh ReadMesh(std::span<const unsigned char> mdt) {
    FloatMesh m;
    for (std::size_t i = 0; i < 16; i++) {
        m.header[i] = Get32(mdt, i * 4);
    }
    std::uint32_t colours = m.header[8];
    std::size_t   list = m.header[10];
    m.colours = colours > 0 && colours < 0x80000000u;
    std::size_t stride = m.colours ? 4 : 3;
    for (std::size_t i = 0; i < 4; i++) {
        m.preamble[i] = Get32(mdt, list + i * 4);
    }
    std::size_t at = list + 0x10, normals = 0, uvs = 0, colour_count = 0;
    for (std::uint32_t s = 0; s < m.preamble[2]; s++) {
        FloatMesh::Submesh sub;
        sub.prim = static_cast<int>(Get32(mdt, at));
        std::size_t count = Get32(mdt, at + 4);
        sub.material = static_cast<int>(Get32(mdt, at + 8));
        at += 0xC;
        for (std::size_t r = 0; r < count; r++) {
            Record record(stride);
            for (std::size_t k = 0; k < stride; k++) {
                record[k] = static_cast<int>(Get32(mdt, at + (r * stride + k) * 4));
            }
            normals = std::max(normals, static_cast<std::size_t>(record[1] + 1));
            uvs = std::max(uvs, static_cast<std::size_t>(record[2] + 1));
            if (m.colours) {
                colour_count = std::max(colour_count, static_cast<std::size_t>(record[3] + 1));
            }
            sub.records.push_back(std::move(record));
        }
        at += count * stride * 4;
        m.submeshes.push_back(std::move(sub));
    }
    m.positions = ReadVectors(mdt, m.header[4], m.header[3]);
    m.normals = ReadVectors(mdt, m.header[6], normals);
    if (m.header[12] > 0) {
        m.uvs = ReadVectors(mdt, m.header[12], uvs);
    }
    if (m.colours) {
        m.colour_values = ReadVectors(mdt, colours, colour_count);
    }
    std::size_t materials = m.header[14];
    for (std::size_t i = 0; i < (m.header[2] - materials) / MdtMesh::kMaterialSize; i++) {
        std::span<const unsigned char> material = Slice(mdt, materials + i * MdtMesh::kMaterialSize, MdtMesh::kMaterialSize, "a material");
        m.materials.emplace_back(material.begin(), material.end());
    }
    return m;
}

Bytes LayMesh(const FloatMesh &m) {
    std::size_t stride = m.colours ? 4 : 3;
    Bytes       list;
    for (std::uint32_t word : {m.preamble[0], m.preamble[1], static_cast<std::uint32_t>(m.submeshes.size()), m.preamble[3]}) {
        Append32(list, word);
    }
    for (const FloatMesh::Submesh &sub : m.submeshes) {
        Append32(list, static_cast<std::uint32_t>(sub.prim));
        Append32(list, static_cast<std::uint32_t>(sub.records.size()));
        Append32(list, static_cast<std::uint32_t>(sub.material));
        for (const Record &record : sub.records) {
            for (std::size_t k = 0; k < stride; k++) {
                Append32(list, static_cast<std::uint32_t>(record[k]));
            }
        }
    }
    Bytes out(0x40, 0);
    auto  emit = [&](const Bytes &block) {
        PadTo16(out);
        std::size_t at = out.size();
        out.insert(out.end(), block.begin(), block.end());
        return static_cast<std::uint32_t>(at);
    };
    auto vectors = [](const std::vector<Vector> &values) {
        Bytes block;
        for (const Vector &v : values) {
            for (float f : v) {
                Append32(block, std::bit_cast<std::uint32_t>(f));
            }
        }
        return block;
    };
    Bytes materials;
    for (const Bytes &material : m.materials) {
        materials.insert(materials.end(), material.begin(), material.end());
    }
    std::uint32_t positions = emit(vectors(m.positions)), list_at = emit(list), normals = emit(vectors(m.normals));
    std::uint32_t uvs = m.uvs.empty() ? 0 : emit(vectors(m.uvs));
    std::uint32_t colours = m.colours ? emit(vectors(m.colour_values)) : 0;
    std::uint32_t materials_at = emit(materials);
    PadTo16(out);
    std::array<std::uint32_t, 16> header = m.header;
    header[2] = static_cast<std::uint32_t>(out.size());
    header[3] = static_cast<std::uint32_t>(m.positions.size());
    header[4] = positions;
    header[6] = normals;
    header[8] = m.colours ? colours : m.header[8];
    header[9] = static_cast<std::uint32_t>(list.size());
    header[10] = list_at;
    header[12] = m.uvs.empty() ? 0 : uvs;
    header[14] = materials_at;
    for (std::size_t i = 0; i < 16; i++) {
        Set32(out, i * 4, header[i]);
    }
    return out;
}

Vector Lerp(const Vector &a, const Vector &b, float t) {
    Vector out;
    for (std::size_t i = 0; i < 4; i++) {
        out[i] = a[i] + (b[i] - a[i]) * t;
    }
    return out;
}

// A submesh's triangles as record triples: a list's in threes, a strip's with every other one turned.
std::vector<std::array<Record, 3>> Triangles(const FloatMesh::Submesh &sub) {
    std::vector<std::array<Record, 3>> out;
    const std::vector<Record>         &r = sub.records;
    if (sub.prim == 3) {
        for (std::size_t i = 0; i + 2 < r.size(); i += 3) {
            out.push_back({r[i], r[i + 1], r[i + 2]});
        }
    } else if (sub.prim == 4) {
        for (std::size_t i = 0; i + 2 < r.size(); i++) {
            out.push_back((i & 1) != 0 ? std::array<Record, 3>{r[i], r[i + 2], r[i + 1]} : std::array<Record, 3>{r[i], r[i + 1], r[i + 2]});
        }
    }
    return out;
}

// Drops the entries of a record slot's stream no record uses, the records renumbered in the old order.
void Compact(FloatMesh &m, std::size_t slot, std::vector<Vector> &stream) {
    std::set<int> used;
    for (const FloatMesh::Submesh &sub : m.submeshes) {
        for (const Record &record : sub.records) {
            used.insert(record[slot]);
        }
    }
    std::map<int, int>  renumbered;
    std::vector<Vector> kept;
    for (int old : used) {
        renumbered[old] = static_cast<int>(kept.size());
        kept.push_back(stream[static_cast<std::size_t>(old)]);
    }
    stream = std::move(kept);
    for (FloatMesh::Submesh &sub : m.submeshes) {
        for (Record &record : sub.records) {
            record[slot] = renumbered[record[slot]];
        }
    }
}

// ---- The ladder: the Factory's (part e05a01, frame hasigo1), yawed about 9.5 degrees off the X axis and standing on
// the Factory's lower floor.

constexpr const char *kFactoryPart = "e05a01", *kFactoryLadder = "hasigo1";
// The rails' straightening: the vertices measured, below this height and north of this z.
constexpr float kRailTop = 85.0f, kRailNorth = -40.0f;
// Cut at the gap between the rungs above the Factory floor, the cut snapped to a level and the whole lowered so the foot
// that stood on the Factory's upper floor stands on the canal's walkway (70); then centred on kLadderX, its feet (the
// vertices above the walkway's height) at z 52.
constexpr float kCutY = 22.0f, kSnapY = 20.0f, kDropY = 20.0f, kWalkwayFeet = 69.0f, kFeetZ = 52.0f;

std::string Key(const Record &record) {
    std::string key;
    for (std::size_t k = 0; k < record.size(); k++) {
        key += (k > 0 ? "," : "") + std::to_string(record[k]);
    }
    return key;
}

void ShapeLadder(FloatMesh &m) {
    // Square to the X axis: the rails' slope in z over x, by least squares.
    double mx = 0, mz = 0;
    int    count = 0;
    for (const Vector &v : m.positions) {
        if (v[1] < kRailTop && v[2] < kRailNorth) {
            mx += v[0];
            mz += v[2];
            count++;
        }
    }
    mx /= count;
    mz /= count;
    double num = 0, den = 0;
    for (const Vector &v : m.positions) {
        if (v[1] < kRailTop && v[2] < kRailNorth) {
            num += (v[0] - mx) * (v[2] - mz);
            den += (v[0] - mx) * (v[0] - mx);
        }
    }
    double length = std::sqrt(num * num + den * den);
    float  c = static_cast<float>(den / length), s = static_cast<float>(num / length);
    auto   turn = [&](std::vector<Vector> &vectors) {
        for (Vector &v : vectors) {
            float x = v[0], z = v[2];
            v[0] = x * c + z * s;
            v[2] = -x * s + z * c;
        }
    };
    turn(m.positions);
    turn(m.normals);

    // Cut below kCutY: each triangle clipped, a vertex made on each edge it crosses (shared between triangles).
    std::size_t                   first_new = m.positions.size();
    std::size_t                   stride = m.colours ? 4 : 3;
    std::map<std::string, Record> made;
    auto                          cut = [&](const Record &ra, const Record &rb) {
        std::string   ka = Key(ra), kb = Key(rb);
        bool          a_first = ka.compare(kb) <= 0;
        const Record &a = a_first ? ra : rb, &b = a_first ? rb : ra;
        std::string   key = a_first ? ka + "|" + kb : kb + "|" + ka;
        if (auto found = made.find(key); found != made.end()) {
            return found->second;
        }
        Vector pa = m.positions[static_cast<std::size_t>(a[0])], pb = m.positions[static_cast<std::size_t>(b[0])];
        float  t = (kCutY - pa[1]) / (pb[1] - pa[1]);
        Record record(stride, 0);
        m.positions.push_back(Lerp(pa, pb, t));
        record[0] = static_cast<int>(m.positions.size() - 1);
        Vector normal = Lerp(m.normals[static_cast<std::size_t>(a[1])], m.normals[static_cast<std::size_t>(b[1])], t);
        m.normals.push_back(normal);
        record[1] = static_cast<int>(m.normals.size() - 1);
        if (!m.uvs.empty()) {
            Vector uv = Lerp(m.uvs[static_cast<std::size_t>(a[2])], m.uvs[static_cast<std::size_t>(b[2])], t);
            m.uvs.push_back(uv);
            record[2] = static_cast<int>(m.uvs.size() - 1);
        }
        if (m.colours) {
            Vector colour = Lerp(m.colour_values[static_cast<std::size_t>(a[3])], m.colour_values[static_cast<std::size_t>(b[3])], t);
            m.colour_values.push_back(colour);
            record[3] = static_cast<int>(m.colour_values.size() - 1);
        }
        made[key] = record;
        return record;
    };
    std::vector<FloatMesh::Submesh> cut_submeshes;
    for (const FloatMesh::Submesh &sub : m.submeshes) {
        FloatMesh::Submesh out{3, sub.material, {}};
        for (const std::array<Record, 3> &triangle : Triangles(sub)) {
            std::vector<Record> polygon;
            for (std::size_t i = 0; i < 3; i++) {
                const Record &a = triangle[i], &b = triangle[(i + 1) % 3];
                bool          a_in = m.positions[static_cast<std::size_t>(a[0])][1] >= kCutY;
                bool          b_in = m.positions[static_cast<std::size_t>(b[0])][1] >= kCutY;
                if (a_in) {
                    polygon.push_back(a);
                }
                if (a_in != b_in) {
                    polygon.push_back(cut(a, b));
                }
            }
            for (std::size_t k = 1; k + 1 < polygon.size(); k++) {
                out.records.push_back(polygon[0]);
                out.records.push_back(polygon[k]);
                out.records.push_back(polygon[k + 1]);
            }
        }
        if (!out.records.empty()) {
            cut_submeshes.push_back(std::move(out));
        }
    }
    m.submeshes = std::move(cut_submeshes);

    for (std::size_t i = 0; i < m.positions.size(); i++) {
        m.positions[i][1] = (i >= first_new ? kSnapY : m.positions[i][1]) - kDropY;
    }
    Compact(m, 0, m.positions);
    Compact(m, 1, m.normals);
    if (!m.uvs.empty()) {
        Compact(m, 2, m.uvs);
    }
    if (m.colours) {
        Compact(m, 3, m.colour_values);
    }

    float low_x = std::numeric_limits<float>::max(), high_x = std::numeric_limits<float>::lowest(), feet = std::numeric_limits<float>::lowest();
    for (const Vector &v : m.positions) {
        low_x = std::min(low_x, v[0]);
        high_x = std::max(high_x, v[0]);
        if (v[1] > kWalkwayFeet) {
            feet = std::max(feet, v[2]);
        }
    }
    float dx = kLadderX - (low_x + high_x) / 2.0f, dz = kFeetZ - feet;
    for (Vector &v : m.positions) {
        v[0] += dx;
        v[2] += dz;
    }
}

// A one-frame .mds of mesh under a frame record (record's own, or a new one named name), the matrix identity.
Bytes OneFrameModel(std::uint32_t version, std::span<const unsigned char> record, std::string_view name, const Bytes &mesh) {
    Bytes out(0x10 + 0x70, 0);
    out[0] = 'M';
    out[1] = 'D';
    out[2] = 'S';
    Set32(out, 4, version);
    Set32(out, 8, 1);
    Set32(out, 0xC, 0x10);
    if (!record.empty()) {
        std::copy(record.begin(), record.end(), out.begin() + 0x10);
    } else {
        Set32(out, 0x10 + 4, 0x70);
        for (std::size_t i = 0; i < name.size(); i++) {
            out[0x18 + i] = static_cast<unsigned char>(name[i]);
        }
        for (std::size_t i = 0; i < 4; i++) {
            SetF32(out, 0x10 + 0x30 + i * 0x14, 1.0f);
        }
    }
    Set32(out, 0x10 + 0x28, 0x80);
    Set32(out, 0x10 + 0x2C, 0xFFFFFFFF);
    out.insert(out.end(), mesh.begin(), mesh.end());
    return out;
}

// The scene's model holding frame name: the .mds's offset and the frame record's.
std::pair<std::size_t, std::size_t> FrameRecord(std::span<const unsigned char> scene, std::size_t mds, std::string_view name) {
    std::size_t table = mds + Get32(scene, mds + 0xC), count = Get32(scene, mds + 8);
    for (std::size_t i = 0; i < count; i++) {
        std::size_t record = table + i * 0x70;
        if (NameAt(scene, record + 8, 0x20) == name) {
            return {mds, record};
        }
    }
    Bad(std::format("no frame {} in its model", name));
}

std::size_t FindText(std::span<const unsigned char> data, std::string_view text, std::size_t from) {
    auto at = std::search(data.begin() + static_cast<std::ptrdiff_t>(from), data.end(), text.begin(), text.end(),
                          [](unsigned char a, char b) { return a == static_cast<unsigned char>(b); });
    return at == data.end() ? std::string_view::npos : static_cast<std::size_t>(at - data.begin());
}

std::size_t FindTextBefore(std::span<const unsigned char> data, std::string_view text, std::size_t before) {
    for (std::size_t i = std::min(before, data.size() - text.size()) + 1; i-- > 0;) {
        if (std::equal(text.begin(), text.end(), data.begin() + static_cast<std::ptrdiff_t>(i), [](char a, unsigned char b) { return static_cast<unsigned char>(a) == b; })) {
            return i;
        }
    }
    return std::string_view::npos;
}

// ---- The ladder's events (EPARTS_FUNC_DATA, 0xC0 each): a kind, hours always on, a link between the ladder's ends, the
// frame they hang on, a position and turn in the part (the world here), a radius, and two values.

constexpr std::uint32_t kLadderBottomEvent = 0x13, kLadderTopEvent = 0x14, kScriptEvent = 0x12;
// The rungs climbed from each end, as the Factory's ladder has them; the message point's radius, about the ladder's 6.
constexpr float kBottomRungs = 12.0f, kTopRungs = 2.0f, kMessageRadius = 8.0f;

Bytes Event(std::uint32_t kind, const std::array<float, 3> &position, float radius, float first, float second) {
    Bytes event(0xC0, 0);
    Set32(event, 0x10, kind);
    SetF32(event, 0x18, 0.0f);
    SetF32(event, 0x1C, 24.0f);
    for (std::size_t i = 0; kLadderPart[i] != '\0'; i++) {
        event[0x30 + i] = static_cast<unsigned char>(kLadderPart[i]);
    }
    for (std::size_t k = 0; k < 3; k++) {
        SetF32(event, 0x40 + k * 4, position[k]);
        SetF32(event, 0x60 + k * 4, radius);
    }
    SetF32(event, 0x70, first);
    SetF32(event, 0x74, second);
    return event;
}

// ---- The canal floor's collision.

void Quad(std::vector<Triangle> &out, const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &d) {
    out.push_back({a, b, c});
    out.push_back({a, c, d});
}

// The canal: floor at 0 from x -200 to 1300 between its walls at z -50 and 50, in 300-wide pieces; the walls 70 high.
constexpr double kCanalWest = -200, kCanalEast = 1300, kCanalNorth = -50, kCanalSouth = 50, kWallTop = 70, kPiece = 300;
constexpr double kWestEndTop = 50, kEastEndTop = 64;
// The bridges' feet: their x spans, and the z spans against each wall, 47 high.
constexpr std::array<std::array<double, 2>, 2> kBridgeX = {
    {{774.9, 825.1}, {-73.1, -22.9}}
};
constexpr std::array<std::array<double, 2>, 2> kBridgeFootZ = {
    {{35.0, 50.0}, {-50.0, -36.0}}
};
constexpr double kBridgeFootTop = 47;
// The camera's walls, as high as the camera stands over the canal floor and below where a bank's fishing camera looks
// down into the water.
constexpr double kCameraWallTop = 30;

// The canal's west end: open under the walkway west of its end wall (x -600..-200, at 50), seen from the floor, and
// closed by quads like the walkway's ledge added to its ground's frame grid1__n.
constexpr const char           *kCapPart = "e03g05", *kCapFrame = "grid1__n";
constexpr double                kCapY = 50;
constexpr std::array<double, 5> kCapX = {-600, -500, -400, -300, -200}, kCapV = {0, 0.5, 0, 0.5, 0};
// A part's header, through the offsets of its blocks.
constexpr std::size_t kPartTableEnd = 0xE0;

// ---- The water and ripples.

constexpr const char *kCanalWaterTune[2] = {
    "WATER_SURFACE \"\",48, 16,\r\n\t\t\t-320, 0, -70,\r\n\t\t\t320, 0, 70,\r\n\t\t\t0, 31, 0,\r\n\t\t\t0.1, 0.015, 0.0, 2.0,\r\n"
    "\t\t\t128, 128, 128,\r\n\t\t\t1, 0, 0\r\n\tWATER_SHAKE\t-1, -1, -0.5, 0.0",
    "WATER_SURFACE \"\",64, 14,\r\n\t\t\t-320, 0, -70,\r\n\t\t\t320, 0, 70,\r\n\t\t\t0, 31, 0,\r\n\t\t\t0.1, 0.015, 0.0, 2.0,\r\n"
    "\t\t\t128, 128, 128,\r\n\t\t\t1, 0, 0\r\n\tWATER_SHAKE\t-1, -1, -0.5, 0.0",
};

constexpr const char *kNoruneRipple = "hamon__A01z", *kRippleMaterial = "e01b22";
constexpr float       kRippleHalf = 5.5f;

constexpr const char *kQueensImages = "gedit/e03/img.pak";
constexpr const char *kFactoryScene = "gedit/e05/scene.scn", *kNoruneScene = "gedit/e01/scene.scn";
constexpr const char *kEastHarborScript = "gedit/s09/event.stb";

// The water's ripple frames: e01b22 the first, e01b23 all eight one above the next.
constexpr const char *kRippleStill = "e01b22", *kRippleFrames = "e01b23";
constexpr int         kRippleSize = 64, kRippleFrameCount = 8;

using Rgba = std::array<int, 4>;

// Python's float floor division.
double FloorDivide(double a, double b) {
    double mod = std::fmod(a, b);
    double div = (a - mod) / b;
    if (mod != 0 && ((b < 0) != (mod < 0))) {
        div -= 1.0;
    }
    if (div == 0) {
        return std::copysign(0.0, a / b);
    }
    double floor = std::floor(div);
    return div - floor > 0.5 ? floor + 1.0 : floor;
}

Rgba Bilinear(const std::vector<Rgba> &pixels, int width, int height, double fx, double fy) {
    fx = std::clamp(fx, 0.0, width - 1.0);
    fy = std::clamp(fy, 0.0, height - 1.0);
    int         x0 = static_cast<int>(fx), y0 = static_cast<int>(fy), x1 = std::min(x0 + 1, width - 1), y1 = std::min(y0 + 1, height - 1);
    double      dx = fx - x0, dy = fy - y0;
    const Rgba &c00 = pixels[static_cast<std::size_t>(y0 * width + x0)], &c10 = pixels[static_cast<std::size_t>(y0 * width + x1)];
    const Rgba &c01 = pixels[static_cast<std::size_t>(y1 * width + x0)], &c11 = pixels[static_cast<std::size_t>(y1 * width + x1)];
    Rgba        out;
    for (std::size_t i = 0; i < 4; i++) {
        out[i] = static_cast<int>(c00[i] * (1 - dx) * (1 - dy) + c10[i] * dx * (1 - dy) + c01[i] * (1 - dx) * dy + c11[i] * dx * dy + 0.5);
    }
    return out;
}

// A picture's offset in a pack, in the first bank (an IMG or IM2 header, 1 to 63 pictures) naming it.
std::size_t FindPicture(std::span<const unsigned char> pak, std::string_view name) {
    for (std::string_view magic : {std::string_view("IMG\0", 4), std::string_view("IM2\0", 4)}) {
        for (std::size_t at = FindText(pak, magic, 0); at != std::string_view::npos; at = FindText(pak, magic, at + 4)) {
            std::size_t count = Get32(pak, at + 4);
            if (count == 0 || count >= 64) {
                continue;
            }
            for (std::size_t e = 0; e < count; e++) {
                std::size_t entry = at + 0x10 + e * 0x30;
                if (entry + 0x30 > pak.size()) {
                    break;
                }
                if (NameAt(pak, entry, 0x20) == name) {
                    return at + Get32(pak, entry + 0x20);
                }
            }
        }
    }
    Bad(std::format("the pack has no picture {}", name));
}

// The bobber's ripple: the top-left quarter of the effect bank's hamon (its ring), drawn at 64 by 64 and brightened.
std::vector<Rgba> RingPicture(std::span<const unsigned char> pak) {
    std::size_t bank = std::string_view::npos;
    for (std::size_t at = 0; at < pak.size() && pak[at] != 0; at += Get32(pak, at + 0x48)) {
        if (NameAt(pak, at, 0x20) == "effect.img") {
            bank = at + Get32(pak, at + 0x40);
            break;
        }
    }
    if (bank == std::string_view::npos) {
        Bad("the pack has no effect.img");
    }
    std::size_t picture = std::string_view::npos;
    for (std::size_t e = 0; e < Get32(pak, bank + 4); e++) {
        if (NameAt(pak, bank + 0x10 + e * 0x30, 0x20) == "hamon") {
            picture = bank + Get32(pak, bank + 0x10 + e * 0x30 + 0x20);
        }
    }
    if (picture == std::string_view::npos) {
        Bad("the effect bank has no hamon");
    }
    std::span<const unsigned char> tim = pak.subspan(picture);
    tim8::Picture                  p = tim8::Read(tim, true);
    if (p.width != 64 || p.height != 64) {
        Bad("hamon is not 64 by 64");
    }
    std::vector<Rgba> quarter(32 * 32);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            std::size_t c = static_cast<std::size_t>(tim8::Csm1(p.pixels[static_cast<std::size_t>(y * 64 + x)])) * 4;
            quarter[static_cast<std::size_t>(y * 32 + x)] = {p.clut[c], p.clut[c + 1], p.clut[c + 2], p.clut[c + 3]};
        }
    }
    std::vector<Rgba> ring(64 * 64);
    int               peak = 0;
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            Rgba c = Bilinear(quarter, 32, 32, x / 2.0, y / 2.0);
            ring[static_cast<std::size_t>(y * 64 + x)] = c;
            peak = std::max({peak, c[0], c[1], c[2]});
        }
    }
    peak = std::max(peak, 1);
    constexpr double kGain = 2.5;
    for (Rgba &c : ring) {
        int light = std::max({c[0], c[1], c[2]});
        c = {std::min(255, static_cast<int>(FloorDivide(c[0] * 255 * kGain, peak))), std::min(255, static_cast<int>(FloorDivide(c[1] * 255 * kGain, peak))),
             std::min(255, static_cast<int>(FloorDivide(c[2] * 255 * kGain, peak))), std::min(0x80, static_cast<int>(FloorDivide(light * 0x80 * kGain, peak)))};
    }
    return ring;
}

// The ring spreading over eight frames, fading in and out: the largest first.
std::vector<std::vector<Rgba>> RingFrames(std::span<const unsigned char> pak) {
    std::vector<Rgba>               ring = RingPicture(pak);
    constexpr std::array<double, 8> kFade = {0.55, 1.0, 1.0, 1.0, 1.0, 0.9, 0.7, 0.4};
    std::vector<std::vector<Rgba>>  frames;
    for (int f = 0; f < kRippleFrameCount; f++) {
        double            scale = 0.70 + 0.45 * f / 7.0;
        std::vector<Rgba> frame(64 * 64);
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                double sx = 32 + (x - 32) / scale, sy = 32 + (y - 32) / scale;
                Rgba   c{};
                if (0 <= sx && sx <= 63 && 0 <= sy && sy <= 63) {
                    c = Bilinear(ring, 64, 64, sx, sy);
                }
                c[3] = static_cast<int>(c[3] * kFade[static_cast<std::size_t>(f)]);
                frame[static_cast<std::size_t>(y * 64 + x)] = c;
            }
        }
        frames.push_back(std::move(frame));
    }
    std::reverse(frames.begin(), frames.end());
    return frames;
}

Bytes QueensCanalScene(std::span<const unsigned char> scene, const std::vector<Bytes> &sources) {
    const Bytes &factory = sources[0], &norune = sources[1], &templ = sources[2];
    Bytes        header = scene::PartHeader(templ, "s04a01");
    Bytes        ladder = CarveCanalLadder(factory);
    Bytes        floor = scene::TriangleCollision(std::string(kLadderPart) + "_a", CanalFloorCollision());
    Bytes        camera = scene::TriangleCollision(std::string(kLadderPart) + "_c", CanalCameraWalls());
    Bytes        out = scene::AppendPart(CapCanalWestEnd(scene), kLadderPart, header, ladder, LadderEvents(), floor, false, camera);
    out = scene::AppendPart(out, kWadingRipple, header, RippleDecal(norune, kRippleHalf), {}, {}, true, {}, true);
    Bytes rail = RippleDecal(norune, kRippleHalf / 2.0f);
    for (const char *part : kRailRipples) {
        out = scene::AppendPart(out, part, header, rail, {}, {}, true, {}, true);
    }
    return out;
}

// The ripples drawn after the canal's own water parts, over its surface.
constexpr const char *kLastWaterPart = "e03c02";

Bytes QueensCanalCfg(std::span<const unsigned char> cfg) {
    Bytes out = scene::PlaceGround(cfg, kLadderPart, "e03g04", "", {0, 0, 0}, 0);
    out = scene::PlaceGround(out, kWadingRipple, kLastWaterPart, "", {0, static_cast<int>(kRippleParkY), 0}, 0, "WATER", "WATER");
    for (std::size_t i = 0; i < std::size(kRailRipples); i++) {
        out = scene::PlaceGround(out, kRailRipples[i], kLastWaterPart, "",
                                 {static_cast<int>(kRailRippleX[i]), static_cast<int>(kLowTide), static_cast<int>(kRailRippleZ)}, 0, "WATER", "WATER");
    }
    return TuneCanalWater(out);
}

void QueensCanalScripts(stb::Appender &appender) {
    appender.Add(kLadderMessageLabel, PointMessageScript(kTideTooHighMessage));
}

Bytes BuildDockArrival(const std::vector<Bytes> &s) {
    if (stb::Label(s[0], kDockArrivalEvent)) {
        Bad(std::format("East Harbor's event script has a label {} already", kDockArrivalEvent));
    }
    stb::Appender appender(s[0]);
    appender.Add(kDockArrivalEvent, ArrivalScript(kDockArrival, kDockArrivalYaw));
    return appender.Build();
}

std::string Rebuilt(std::string_view path) {
    return std::string(kRebuiltDiscFolder) + std::string(path);
}

} // namespace

Bytes CapCanalWestEnd(std::span<const unsigned char> scene) {
    std::vector<scene::SubFile> files = scene::SubFiles(scene);
    auto                        ground = std::ranges::find(files, std::string_view(kCapPart), &scene::SubFile::name);
    if (ground == files.end()) {
        Bad(std::format("the Queens scene has no {}", kCapPart));
    }
    std::span<const unsigned char> sub = Slice(scene, ground->offset, ground->size, "a sub-file");
    std::size_t                    named = FindText(sub, std::string(kCapFrame) + '\0', 0);
    if (named == std::string_view::npos || named < 8) {
        Bad(std::format("{} has no frame {}", kCapPart, kCapFrame));
    }
    std::size_t mesh_at = Get32(sub, named - 8 + 0x28);
    std::size_t model = FindTextBefore(sub, std::string_view("MDS\0", 4), named);
    std::size_t mdt = model + mesh_at;
    if (NameAt(sub, mdt, 4) != "MDT") {
        Bad(std::format("{}'s mesh is not an MDT", kCapFrame));
    }
    MdtMesh     m = MdtMesh::Parse(sub.subspan(mdt));
    std::size_t old_size = m.header[2];

    // The walkway's west ledge, a quad over x -700..-600 at 50: its corners' records, by (x, z).
    std::map<std::pair<long, long>, std::vector<int>> ledge;
    int                                               ledge_sub = -1;
    for (std::size_t s = 0; s < m.submeshes.size(); s++) {
        const MdtMesh::Submesh &sub_mesh = m.submeshes[s];
        if (sub_mesh.prim != 3) {
            continue;
        }
        for (std::size_t k = 0; k + 2 < sub_mesh.records.size(); k += 3) {
            bool on_ledge = true;
            for (std::size_t i = 0; i < 3; i++) {
                const auto &p = m.positions[static_cast<std::size_t>(sub_mesh.records[k + i][0])];
                on_ledge = on_ledge && std::abs(p[1] - kCapY) < 0.5 && -701 <= p[0] && p[0] <= -599 && -51 <= p[2] && p[2] <= 51;
            }
            if (!on_ledge) {
                continue;
            }
            for (std::size_t i = 0; i < 3; i++) {
                const auto &p = m.positions[static_cast<std::size_t>(sub_mesh.records[k + i][0])];
                ledge[{std::lround(p[0]), std::lround(p[2])}] = sub_mesh.records[k + i];
            }
            ledge_sub = static_cast<int>(s);
        }
    }
    for (std::pair<long, long> corner : {
             std::pair<long, long>{-600, -50},
             {-700, -50},
             {-700, 50 },
             {-600, 50 }
    }) {
        if (ledge_sub < 0 || !ledge.contains(corner)) {
            Bad("the canal's west ledge is not where it was");
        }
    }
    int up = ledge[{-600, -50}][1];
    // The ledge's texture coordinates, by (u, v) in hundredths.
    std::map<std::pair<long, long>, int> texels;
    for (std::pair<long, long> corner : {
             std::pair<long, long>{-600, -50},
             {-700, -50},
             {-700, 50 },
             {-600, 50 }
    }) {
        int         uv = ledge[corner][2];
        const auto &t = m.uvs[static_cast<std::size_t>(uv)];
        texels[{std::lround(t[0] * 100), std::lround(t[1] * 100)}] = uv;
    }
    for (std::pair<long, long> texel : {
             std::pair<long, long>{50, 0 },
             {0,  0 },
             {50, 50},
             {0,  50}
    }) {
        if (!texels.contains(texel)) {
            Bad("the canal's west ledge is not textured as it was");
        }
    }
    std::map<std::pair<long, long>, int> ledge_vertex = {
        {{-600, -50}, ledge[{-600, -50}][0]},
        {{-600, 50},  ledge[{-600, 50}][0] }
    };
    double w = m.positions[static_cast<std::size_t>(ledge_vertex[{-600, -50}])][3];
    auto   vertex = [&](double x, double z) {
        if (auto found = ledge_vertex.find({std::lround(x), std::lround(z)}); found != ledge_vertex.end()) {
            return found->second;
        }
        m.positions.push_back({x, kCapY, z, w});
        return static_cast<int>(m.positions.size() - 1);
    };
    // The texture's half wide across the canal, its v running back and forth along it.
    auto record = [&](double x, double z, double v) {
        long u = z < 0 ? 50 : 0;
        return std::vector<int>{vertex(x, z), up, texels[{u, std::lround(v * 100)}]};
    };
    MdtMesh::Submesh cap{3, m.submeshes[static_cast<std::size_t>(ledge_sub)].material, {}};
    for (std::size_t i = 0; i + 1 < std::size(kCapX); i++) {
        std::vector<int> a = record(kCapX[i], kCanalNorth, kCapV[i]), b = record(kCapX[i + 1], kCanalNorth, kCapV[i + 1]);
        std::vector<int> c = record(kCapX[i + 1], kCanalSouth, kCapV[i + 1]), d = record(kCapX[i], kCanalSouth, kCapV[i]);
        cap.records.insert(cap.records.end(), {a, b, c, a, c, d});
    }
    m.submeshes.push_back(std::move(cap));
    std::size_t stride = m.colours ? 4 : 3, list = 16;
    for (const MdtMesh::Submesh &sub_mesh : m.submeshes) {
        list += 12 + sub_mesh.records.size() * stride * 4;
    }
    std::size_t materials = 0;
    for (const Bytes &material : m.materials) {
        materials += material.size();
    }
    std::map<std::string, std::size_t> sizes = {
        {"POS",  m.positions.size() * 16                    },
        {"DL",   list                                       },
        {"UV",   m.normals.size() * 16                      },
        {"NORM", m.uvs.size() * 16                          },
        {"COL",  m.colours ? m.colour_values.size() * 16 : 0},
        {"MAT",  materials                                  },
    };
    for (const std::string &block : m.order) {
        m.pads[block] = Bytes((16 - sizes[block] % 16) % 16, 0);
    }
    Bytes capped = m.Build();
    PadTo16(capped);
    std::size_t delta = capped.size() - old_size;

    // The sub-file with the mesh grown: the model's later meshes and the part's later blocks moved by as much.
    Bytes grown(sub.begin(), sub.begin() + static_cast<std::ptrdiff_t>(mdt));
    grown.insert(grown.end(), capped.begin(), capped.end());
    grown.insert(grown.end(), sub.begin() + static_cast<std::ptrdiff_t>(mdt + old_size), sub.end());
    std::size_t count = Get32(grown, model + 8), table = Get32(grown, model + 0xC);
    for (std::size_t i = 0; i < count; i++) {
        std::size_t at = model + table + i * 0x70 + 0x28;
        if (GetI32(grown, at) > static_cast<std::int32_t>(mesh_at)) {
            Set32(grown, at, static_cast<std::uint32_t>(Get32(grown, at) + delta));
        }
    }
    for (std::size_t at = 8; at < kPartTableEnd; at += 4) {
        std::uint32_t value = Get32(grown, at);
        if (mdt + old_size <= value && value < ground->size) {
            Set32(grown, at, static_cast<std::uint32_t>(value + delta));
        }
    }
    Bytes out(scene.begin(), scene.begin() + static_cast<std::ptrdiff_t>(ground->offset));
    out.insert(out.end(), grown.begin(), grown.end());
    out.insert(out.end(), scene.begin() + static_cast<std::ptrdiff_t>(ground->offset + ground->size), scene.end());
    for (std::size_t i = 0; i < files.size(); i++) {
        std::size_t entry = 0x10 + i * 0x30;
        if (files[i].offset == ground->offset) {
            Set32(out, entry + 0x14, static_cast<std::uint32_t>(files[i].size + delta));
        } else if (files[i].offset > ground->offset) {
            Set32(out, entry + 0x10, static_cast<std::uint32_t>(files[i].offset + delta));
        }
    }
    return out;
}

const TownAdditions kQueensCanal = {
    .sources = {kFactoryScene, kNoruneScene, "gedit/s04/scene.scn"},
    .scene = QueensCanalScene,
    .cfg = QueensCanalCfg,
    .scripts = QueensCanalScripts,
};

Bytes CarveCanalLadder(std::span<const unsigned char> factory_scene) {
    std::size_t part = std::string_view::npos;
    for (const scene::SubFile &file : scene::SubFiles(factory_scene)) {
        if (file.name == kFactoryPart) {
            part = file.offset;
            break;
        }
    }
    if (part == std::string_view::npos) {
        Bad(std::format("the Factory's scene has no part {}", kFactoryPart));
    }
    std::size_t mds = FindText(factory_scene, std::string_view("MDS\0", 4), part);
    if (mds == std::string_view::npos) {
        Bad("the ladder's part has no model");
    }
    auto [model, record] = FrameRecord(factory_scene, mds, kFactoryLadder);
    std::size_t mesh = Get32(factory_scene, record + 0x28);
    std::size_t mdt = factory_scene[model + mesh] == 'M' ? model + mesh : mesh;
    if (NameAt(factory_scene, mdt, 4) != "MDT") {
        Bad("the ladder's mesh is not an MDT");
    }
    FloatMesh m = ReadMesh(factory_scene.subspan(mdt));
    ShapeLadder(m);
    return OneFrameModel(1, {}, kLadderPart, LayMesh(m));
}

Bytes RippleDecal(std::span<const unsigned char> norune_scene, float half) {
    std::size_t named = FindText(norune_scene, std::string(kNoruneRipple) + '\0', 0);
    if (named == std::string_view::npos || named < 8) {
        Bad(std::format("Norune's scene has no {}", kNoruneRipple));
    }
    std::size_t mds = FindTextBefore(norune_scene, std::string_view("MDS\0", 4), named - 8);
    if (mds == std::string_view::npos) {
        Bad("Norune's ripple has no model");
    }
    auto [model, record] = FrameRecord(norune_scene, mds, kNoruneRipple);
    std::size_t                    mdt = model + Get32(norune_scene, record + 0x28);
    std::span<const unsigned char> material = Slice(norune_scene, mdt + Get32(norune_scene, mdt + 0x38), MdtMesh::kMaterialSize, "a material");
    if (NameAt(material, MdtMesh::kMaterialTexture, 0x20) != kRippleMaterial) {
        Bad(std::format("Norune's ripple is not in {}", kRippleMaterial));
    }

    // Four corners, two triangles, the picture once across them: blocks POS, DL, the one normal (up), the corners'
    // texture coordinates, the material.
    const std::array<std::array<float, 3>, 4> corners = {
        {{-half, 0, -half}, {half, 0, -half}, {half, 0, half}, {-half, 0, half}}
    };
    const std::array<std::array<float, 2>, 4> texels = {
        {{0, 0}, {1, 0}, {1, 1}, {0, 1}}
    };
    const std::array<std::array<int, 3>, 6> records = {
        {{0, 0, 0}, {1, 0, 1}, {2, 0, 2}, {0, 0, 0}, {2, 0, 2}, {3, 0, 3}}
    };
    constexpr std::size_t kPositions = 0x40, kList = kPositions + 4 * 16, kListSize = 16 + 12 + records.size() * 3 * 4;
    constexpr std::size_t kNormal = (kList + kListSize + 0xF) & ~std::size_t{0xF}, kTexels = kNormal + 16, kMaterial = kTexels + 4 * 16;
    constexpr std::size_t kSize = kMaterial + MdtMesh::kMaterialSize;
    Bytes                 mesh(kSize, 0);
    mesh[0] = 'M';
    mesh[1] = 'D';
    mesh[2] = 'T';
    constexpr std::array<std::uint32_t, 16> kHeader = {0x0054444D, 0x40, kSize, 4, kPositions, 1, kNormal, 0, 0xFFFFFFFF, kListSize, kList, 4,
                                                       kTexels, 1, kMaterial, 0xCDCDCDCD};
    for (std::size_t i = 0; i < 16; i++) {
        Set32(mesh, i * 4, kHeader[i]);
    }
    for (std::size_t v = 0; v < 4; v++) {
        for (std::size_t k = 0; k < 3; k++) {
            SetF32(mesh, kPositions + v * 16 + k * 4, corners[v][k]);
        }
        SetF32(mesh, kPositions + v * 16 + 12, 1.0f);
    }
    Set32(mesh, kList, 0xCDCDCDCD);
    Set32(mesh, kList + 4, 0x10);
    Set32(mesh, kList + 8, 1);
    Set32(mesh, kList + 12, 0xCDCDCDCD);
    Set32(mesh, kList + 16, 3);
    Set32(mesh, kList + 20, static_cast<std::uint32_t>(records.size()));
    for (std::size_t r = 0; r < records.size(); r++) {
        for (std::size_t k = 0; k < 3; k++) {
            Set32(mesh, kList + 28 + (r * 3 + k) * 4, static_cast<std::uint32_t>(records[r][k]));
        }
    }
    SetF32(mesh, kNormal + 4, 1.0f);
    SetF32(mesh, kNormal + 12, 1.0f);
    for (std::size_t t = 0; t < 4; t++) {
        SetF32(mesh, kTexels + t * 16, texels[t][0]);
        SetF32(mesh, kTexels + t * 16 + 4, texels[t][1]);
        SetF32(mesh, kTexels + t * 16 + 8, 1.0f);
    }
    std::copy(material.begin(), material.end(), mesh.begin() + kMaterial);
    return OneFrameModel(Get32(norune_scene, model + 4), Slice(norune_scene, record, 0x70, "a frame record"), {}, mesh);
}

Bytes LadderEvents() {
    Bytes out = Event(kLadderBottomEvent, kLadderBottom, 0.0f, 0.0f, kBottomRungs);
    Bytes top = Event(kLadderTopEvent, kLadderTop, 0.0f, 0.0f, kTopRungs);
    Bytes message = Event(kScriptEvent, kLadderTop, kMessageRadius, static_cast<float>(kLadderMessageLabel), 0.0f);
    out.insert(out.end(), top.begin(), top.end());
    out.insert(out.end(), message.begin(), message.end());
    return out;
}

std::vector<Triangle> CanalFloorCollision() {
    std::vector<Triangle> out;
    for (double x = kCanalWest; x < kCanalEast; x += kPiece) {
        double east = x + kPiece;
        Quad(out, {x, 0, kCanalNorth}, {east, 0, kCanalNorth}, {east, 0, kCanalSouth}, {x, 0, kCanalSouth});
        Quad(out, {x, 0, kCanalNorth}, {east, 0, kCanalNorth}, {east, kWallTop, kCanalNorth}, {x, kWallTop, kCanalNorth});
        Quad(out, {x, 0, kCanalSouth}, {east, 0, kCanalSouth}, {east, kWallTop, kCanalSouth}, {x, kWallTop, kCanalSouth});
    }
    Quad(out, {kCanalWest, 0, kCanalNorth}, {kCanalWest, 0, kCanalSouth}, {kCanalWest, kWestEndTop, kCanalSouth}, {kCanalWest, kWestEndTop, kCanalNorth});
    Quad(out, {kCanalEast, 0, kCanalNorth}, {kCanalEast, 0, kCanalSouth}, {kCanalEast, kEastEndTop, kCanalSouth}, {kCanalEast, kEastEndTop, kCanalNorth});
    for (const auto &[x0, x1] : kBridgeX) {
        for (const auto &[z0, z1] : kBridgeFootZ) {
            Quad(out, {x0, 0, z0}, {x1, 0, z0}, {x1, kBridgeFootTop, z0}, {x0, kBridgeFootTop, z0});
            Quad(out, {x0, 0, z1}, {x1, 0, z1}, {x1, kBridgeFootTop, z1}, {x0, kBridgeFootTop, z1});
            Quad(out, {x0, 0, z0}, {x0, 0, z1}, {x0, kBridgeFootTop, z1}, {x0, kBridgeFootTop, z0});
            Quad(out, {x1, 0, z0}, {x1, 0, z1}, {x1, kBridgeFootTop, z1}, {x1, kBridgeFootTop, z0});
        }
    }
    return out;
}

std::vector<Triangle> CanalCameraWalls() {
    // Each wound to face into the canal.
    std::vector<Triangle> out;
    for (double x = kCanalWest; x < kCanalEast; x += kPiece) {
        double east = x + kPiece;
        Quad(out, {x, 0, kCanalNorth}, {east, 0, kCanalNorth}, {east, kCameraWallTop, kCanalNorth}, {x, kCameraWallTop, kCanalNorth});
        Quad(out, {x, 0, kCanalSouth}, {x, kCameraWallTop, kCanalSouth}, {east, kCameraWallTop, kCanalSouth}, {east, 0, kCanalSouth});
    }
    Quad(out, {kCanalWest, 0, kCanalNorth}, {kCanalWest, kCameraWallTop, kCanalNorth}, {kCanalWest, kCameraWallTop, kCanalSouth}, {kCanalWest, 0, kCanalSouth});
    Quad(out, {kCanalEast, 0, kCanalNorth}, {kCanalEast, 0, kCanalSouth}, {kCanalEast, kCameraWallTop, kCanalSouth}, {kCanalEast, kCameraWallTop, kCanalNorth});
    return out;
}

Bytes TuneCanalWater(std::span<const unsigned char> cfg) {
    std::string text = ToText(cfg);
    std::size_t at = text.find(kCanalWaterTune[0]);
    if (at == std::string::npos || text.find(kCanalWaterTune[0], at + 1) != std::string::npos) {
        Bad("the Queens mapinfo has not one canal water surface");
    }
    text.replace(at, std::string_view(kCanalWaterTune[0]).size(), kCanalWaterTune[1]);
    return ToBytes(text);
}

Bytes RingRipples(std::span<const unsigned char> pak) {
    Bytes                          out(pak.begin(), pak.end());
    std::vector<std::vector<Rgba>> frames = RingFrames(pak);
    for (const auto &[name, height, count] : {
             std::tuple{kRippleStill,  kRippleSize,                     1                },
             std::tuple{kRippleFrames, kRippleSize * kRippleFrameCount, kRippleFrameCount}
    }) {
        std::size_t picture = FindPicture(out, name) + tim8::kPicture;
        std::size_t header = Get16(out, picture + tim8::kHeaderSize);
        if (Get16(out, picture + tim8::kWidth) != kRippleSize || Get16(out, picture + tim8::kHeight) != height || out[picture + tim8::kType] != tim8::kType32 ||
            Get32(out, picture + tim8::kClutSize) != 0 || Get32(out, picture + tim8::kImageSize) != static_cast<std::uint32_t>(kRippleSize * height * 4)) {
            Bad(std::format("{} is not a {} by {} 32-bit picture", name, kRippleSize, height));
        }
        for (int f = 0; f < count; f++) {
            const std::vector<Rgba> &frame = frames[static_cast<std::size_t>(f)];
            for (std::size_t i = 0; i < frame.size(); i++) {
                std::size_t at = picture + header + (static_cast<std::size_t>(f) * kRippleSize * kRippleSize + i) * 4;
                for (std::size_t k = 0; k < 4; k++) {
                    out[at + k] = static_cast<unsigned char>(frame[i][k]);
                }
            }
        }
    }
    return out;
}

std::vector<DataBuild> QueensCanalBuilds() {
    return {
        DataBuild::FromSources("east harbor event script", Rebuilt(kEastHarborScript), {kEastHarborScript}, BuildDockArrival),
        DataBuild::FromSources("queens ring ripples", Rebuilt(kQueensImages), {kQueensImages}, [](const std::vector<Bytes> &s) { return RingRipples(s[0]); }),
    };
}

} // namespace expanded
