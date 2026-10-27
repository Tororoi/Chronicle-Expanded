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
#include "expanded/data/sheet_look.hpp"

namespace expanded {

// A Gemron pack's records: its model, its motion and its shadow's, its bind poses, its texture bank and the bank's body
// and second sheets.
struct GemronPack {
    const char *mds, *mot, *shadow_mot, *bbp, *img, *sheet, *sheet2;
};

// Ice Gemron's (dun/monstor/e112a.chr), the Bomb and Atla Gemrons' base, and Holy Gemron's (e115a.chr), the Crystal's.
inline constexpr GemronPack kIceGemronPack = {"e112a.mds", "e112a.mot", "e112s.mot", "e112a.bbp", "e112a01.img", "e112a01", "e112a02"};
inline constexpr GemronPack kHolyGemronPack = {"e115a.mds", "e115a.mot", "e115s.mot", "e115a.bbp", "e115a01.img", "e115a01", "e115a02"};

// The frame named name, throwing (naming its model) when there is none.
const MdsFrame &RequireFrame(const std::vector<MdsFrame> &frames, std::string_view name, std::string_view mds);

// The look on the Gemron's texture bank: the look on its body sheet, the look's base on its second.
void ApplyLook(ImgBank &bank, const SheetLook &look, const GemronPack &gemron);

// A track's value at a frame: its first or last key outside its range, else the pair around it blended (BlendKeys).
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
// end and one past it; in a motion, or in a .mot record.
void DeathClip(MotFile &mot, int start, int from, int to, int end);
void DeathClip(ChrRecord &mot, int start, int from, int to, int end);

// info.cfg: the death key (11) on start-end at 0.35, its loop (12) on end, material animation on; scroll, if any, a
// second TEX_ANIME group after the first.
void DeathCfg(ChrPack &pack, int start, int end, const char *scroll);

} // namespace expanded
