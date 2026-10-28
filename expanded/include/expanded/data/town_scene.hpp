#pragma once

// A town's scene.scn and mapinfo.cfg as the builds edit them. The scene: a directory of sub-files (0x30-byte entries
// from 0x10, its count @4), each a part: a 0x160-byte header (its archive fields, then its definition @kDefinition),
// its event functions, its model (.mds) and its collision (.mds of collision meshes). The mapinfo places a GROUND part
// with its position and its rotation in degrees. Includes no game header, so dcdata can build with it.

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "expanded/data/bytes.hpp"

namespace expanded::scene {

using Vec3 = std::array<double, 3>;
using Triangle = std::array<Vec3, 3>;

struct SubFile {
    std::string name;
    std::size_t offset = 0;
    std::size_t size = 0;
};

// The directory, in order.
std::vector<SubFile> SubFiles(std::span<const unsigned char> scene);

// Frame name's record and mesh out of the first model holding it, as a one-frame .mds with the record at the root.
Bytes CarveFrame(std::span<const unsigned char> scene, std::string_view frame);

// Part's header.
Bytes PartHeader(std::span<const unsigned char> scene, std::string_view part);

// TIM2 picture name out of the first texture bank of a pack holding it, without the padding after it.
Bytes CarvePicture(std::span<const unsigned char> pack, std::string_view name);

// An IM2 texture bank of named TIM2 pictures, each 16-aligned.
Bytes PictureBank(const std::vector<std::pair<std::string, Bytes>> &pictures);

// A solid box, {x0, x1, y0, y1, z0, z1}, as a one-frame collision .mds whose frame is named name.
Bytes BoxCollision(std::string_view name, const std::array<float, 6> &box);

// An event function: a script point always on, at position in the part, firing label within radius.
Bytes ScriptEvent(const std::array<float, 3> &position, float radius, int label);

// The scene with the stand point of villager (PEOPLE2's order) in part moved to position, in the part's frame.
Bytes MoveStandPoint(std::span<const unsigned char> scene, std::string_view part, int villager, const std::array<float, 3> &position);

// The scene with a part appended under name: header's copy, its events, its model (at the part's origin when
// at_origin) and its collision (none when empty).
Bytes AppendPart(std::span<const unsigned char> scene, std::string_view name, std::span<const unsigned char> header,
                 std::span<const unsigned char> model, std::span<const unsigned char> events, std::span<const unsigned char> collision,
                 bool at_origin = true);

// The mapinfo with part placed as a GROUND after anchor's last GROUND block, collision its atari model.
Bytes PlaceGround(std::span<const unsigned char> cfg, std::string_view part, std::string_view anchor, std::string_view collision,
                  const std::array<int, 3> &position, int yaw);

// Where the mapinfo's GROUND and WATER lines place the parts whose names start with prefix, in its order.
std::vector<Vec3> PlacedAt(std::span<const unsigned char> cfg, std::string_view prefix);

// A model frame's mesh as the mapinfo places it: its world vertices and its triangles' vertex indices.
struct PlacedMesh {
    std::string                     frame;
    std::string                     sub;
    int                             instance = 0;
    std::vector<Vec3>               vertices;
    std::vector<std::array<int, 3>> triangles;
};

// Every mesh frame of every sub-file a GROUND or WATER line places, in the mapinfo's order.
std::vector<PlacedMesh> PlacedMeshes(std::span<const unsigned char> scene, std::span<const unsigned char> cfg);

} // namespace expanded::scene
