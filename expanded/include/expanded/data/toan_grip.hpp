#pragma once

// Toan's sword hand rigged finger by finger, built into his dungeon pack at extraction (dun/mainchara/c01d.chr, rebuilt
// into kRebuiltDiscFolder) so the Macho Sword's Pumped Up can close his grip as his hand grows. Includes no game header,
// so dcdata can build with it.
//
// The hand (the mesh r_hand, rigid under the wrist joint jnt18_1 on the disc) becomes a skinned mesh, as the Dark
// Genie's hands are: each finger and the thumb gets kGripJointsPerDigit joints, one a knuckle, each under the one
// before (the first under the wrist), turned as the frames' own axes are. A digit's vertices are counted out in rings
// from where it leaves the hand: each ring is weighted wholly to its own knuckle (the rings past the last to the last);
// its knuckle at the hand (the hand's vertices from kGripKnuckleFrom to the cut, each taken by the finger nearest it
// across the hand) half to the first joint and half to the wrist, so it bends with the finger; the palm wholly to the
// wrist. The first joint sits in the middle of the knuckle at the hand, each next at the middle of its ring. Untouched,
// the joints hold the hand as the disc has it. The fingers are the hand's pieces past kGripFingerCut across its length
// once the thumb (the part below kGripThumbSide on its thumb side, past kGripThumbFrom) is set apart, index finger
// first. The scale the disc's hand frame carries is baked into its vertices (the skin takes a frame back out rigidly).

#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kToanDungeonFile = "dun/mainchara/c01d.chr";
inline constexpr const char *kGripHand = "r_hand";
inline constexpr const char *kGripWrist = "jnt18_1";
inline constexpr int         kGripDigits = 5; // the index finger to the little finger, then the thumb
inline constexpr int         kGripThumb = 4;
inline constexpr int         kGripJointsPerDigit = 3;
inline constexpr const char *kGripJoints[kGripDigits][kGripJointsPerDigit] = {
    {"r_index_1",  "r_index_2",  "r_index_3" },
    {"r_middle_1", "r_middle_2", "r_middle_3"},
    {"r_ring_1",   "r_ring_2",   "r_ring_3"  },
    {"r_little_1", "r_little_2", "r_little_3"},
    {"r_thumb_1",  "r_thumb_2",  "r_thumb_3" },
};
inline constexpr double kGripFingerCut = 1.08;
inline constexpr double kGripKnuckleFrom = 0.9;
inline constexpr double kGripThumbFrom = 0.7;
inline constexpr double kGripThumbSide = -0.2;

// Toan's dungeon pack with the hand rigged.
Bytes BuildToanGrip(std::span<const unsigned char> toan);

std::vector<DataBuild> ToanGripBuilds();

} // namespace expanded
