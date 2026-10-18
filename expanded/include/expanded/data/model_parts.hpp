#pragma once

// The parts of a .chr pack the built monsters edit: an .mds's frame records and meshes, an MDT mesh, a texture bank
// and its 8-bit TIM2 pictures, and the rig maths over them. Includes no game header, so dcdata can build with it.

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "expanded/data/chr_pack.hpp"

namespace expanded {

using Vec3 = std::array<double, 3>;
using Mat3 = std::array<Vec3, 3>; // rows
using Mat4 = std::array<std::array<double, 4>, 4>;

Vec3   Add(const Vec3 &a, const Vec3 &b);
Vec3   Scale(const Vec3 &a, double s);
Vec3   Cross(const Vec3 &a, const Vec3 &b);
double Dot(const Vec3 &a, const Vec3 &b);
Vec3   Unit(const Vec3 &a);
Mat3   MatMul(const Mat3 &a, const Mat3 &b);
Vec3   MatVec(const Mat3 &m, const Vec3 &v);
// The rotation by angle radians about unit axis, right-hand rule (Rodrigues).
Mat3 AxisRotation(const Vec3 &axis, double angle);
// The rotation taking +Y onto unit d.
Mat3 RotationYTo(const Vec3 &d);
// Two .mot rotation keys (w first) slerped the shorter way round, linear past 0.9995.
std::array<double, 4> SlerpKeys(const std::array<float, 4> &a, const std::array<float, 4> &b, double t);

// One .mds frame record (0x70 bytes): its index, name @8, mesh offset @0x28 (0 for none), parent @0x2C, then its
// local matrix (rotation rows @0x30, translation @0x60); world is its bind pose (row vectors, world = local x parent).
struct MdsFrame {
    int         index = 0;
    int         parent = -1;
    int         mesh = 0;
    std::string name;
    Mat4        world{};
    Bytes       record;

    Mat3 WorldRotation() const;
    Vec3 WorldPosition() const;
    // A world direction in the frame's own axes.
    Vec3 ToLocal(const Vec3 &world_direction) const;
};

// The frames of an .mds payload, posed.
std::vector<MdsFrame> ReadMdsSkeleton(std::span<const unsigned char> mds);

// The frame named name, or null.
const MdsFrame *FindFrame(const std::vector<MdsFrame> &frames, std::string_view name);

// The MDT mesh at offset in an .mds.
Bytes MdtAt(std::span<const unsigned char> mds, std::size_t offset);

// An .mds with some frames' meshes replaced (name -> MDT; nullopt drops the mesh) and records appended after the last
// (record, MDT), every mesh offset laid again.
Bytes ReplaceMeshes(std::span<const unsigned char> mds, const std::map<std::string, std::optional<Bytes>> &replaced,
                    const std::vector<std::pair<Bytes, Bytes>> &appended = {});

// A new frame record: index, name, parent, identity matrix, no mesh.
Bytes FrameRecord(int index, std::string_view name, int parent);

// An MDT mesh read into its blocks and laid back out byte for byte (blocks in their recorded order with the padding
// between them). A display list record is (position, normal, texture coordinate[, colour]) indices; the block the
// header calls UV @0x18 holds the normals and the one it calls NORM @0x30 the texture coordinates, as the files have
// them.
struct MdtMesh {
    std::array<std::uint32_t, 16>      header{};
    bool                               colours = false;
    std::vector<std::array<double, 4>> positions, normals, uvs, colour_values;

    struct Submesh {
        int                           prim = 0; // 3 list, 4 strip
        int                           material = 0;
        std::vector<std::vector<int>> records;
    };

    std::vector<Submesh>         submeshes;
    std::array<int, 4>           preamble{};
    std::vector<Bytes>           materials;
    std::vector<std::string>     order;
    std::map<std::string, Bytes> pads;

    static MdtMesh Parse(std::span<const unsigned char> mdt);
    Bytes          Build() const;
};

// A texture bank (IMG\0 or IM2\0): a 0x10 header (count @4), 0x30 entries (name, bank-relative offset @0x20), the TIM2
// pictures after the table. An IM2 bank's 8-bit pictures are in the GS's PSMT8 block order, an IMG bank's row-major.
struct ImgBank {
    std::array<unsigned char, 4>               magic{};
    std::vector<std::pair<std::string, Bytes>> entries; // each picture to the next entry (or the end)

    static ImgBank Parse(std::span<const unsigned char> bank);
    const Bytes   &Block(std::string_view name) const;
    Bytes          Build() const;
};

// 8-bit TIM2 pictures (the picture header 0x10 into the file: CLUT size +4, image size +8, header size +0xC, type
// +0x13, width +0x14, height +0x16).
namespace tim8 {

inline constexpr int kPicture = 0x10;

struct Picture {
    int   width = 0, height = 0;
    Bytes pixels; // row-major
    Bytes clut;   // as the file has it
};

Picture Read(std::span<const unsigned char> tim, bool swizzled);
// An 8-bit TIM2 on a vanilla 8-bit picture's headers: sizes set, the GS register words cleared.
Bytes Build(std::span<const unsigned char> templ, int width, int height, std::span<const unsigned char> pixels,
            std::span<const unsigned char> clut);
// Nearest-texel resample to n x n, row-major.
Bytes Resample(std::span<const unsigned char> tim, bool swizzled, int n);
// Row-major pixels into the GS block order, and back.
Bytes Swizzle(std::span<const unsigned char> row_major, int width, int height);
Bytes Unswizzle(std::span<const unsigned char> blocks, int width, int height);
// The picture's pixels swizzled into the block order.
Bytes Swizzled(std::span<const unsigned char> tim);
// Where an index's colour sits in a CLUT as the GS reads it (bits 3 and 4 swapped).
int Csm1(int index);

} // namespace tim8

} // namespace expanded
