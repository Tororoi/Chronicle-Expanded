#pragma once

// What the Gemron builds share: motion keys read and blended, frames grafted onto the rig (its gems' meshes dropped),
// the death copied past the vanilla keys and the cfg keyed onto the copy. Includes no game header, so dcdata can build
// with it.

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"

namespace expanded {

// A track's value at a frame: its first or last key outside its range, else the pair around it blended.
std::array<double, 4> TrackAt(const MotTrack &track, double frame);
// A track of node on channel, its other words the reference's.
MotTrack TrackLike(const MotTrack &reference, std::uint32_t node, std::uint32_t w1, std::uint32_t channel);

// A grafted frame's record: its index and parent; the root's rotation and position given, the rest's position scaled.
Bytes GraftRecord(const Bytes &raw, int index, int parent, const std::optional<Mat3> &r, const std::optional<Vec3> &t,
                  double scale);
Bytes ScaledMesh(std::span<const unsigned char> mds, std::size_t offset, double s);

struct Graft {
    std::vector<Bytes>   records;
    std::map<int, Bytes> meshes; // by the record's place in the graft
};

// The .mds with the gems' meshes (and glow shells) dropped, some replaced by name and the grafts' records appended.
Bytes GraftMds(std::span<const unsigned char> mds, const std::vector<Graft> &grafts, const std::map<std::string, Bytes> &replace);

// Appends count zero bind poses to the pack's .bbp, one a grafted frame (the Gemron is not skinned).
void AppendBindPoses(ChrPack &pack, std::string_view bbp_name, std::size_t count);

// The vanilla death (frames from-to) copied to start on, every track keyed on each frame of it, its last pose held to
// end and one past it.
void DeathClip(MotFile &mot, int start, int from, int to, int end);

// info.cfg: the death key (11) on start-end at 0.35, its loop (12) on end, material animation on; scroll, if any, a
// second TEX_ANIME group after the first.
void DeathCfg(ChrPack &pack, int start, int end, const char *scroll);

} // namespace expanded
