#pragma once

// Super Steve's wolf form, built into his own pack (commenu/weapon/c04w13.chr, rebuilt into kRebuiltDiscFolder): the
// Lamb's Sword's wolf head (its wolf mesh's crossguard, with its tongue) on his neck with his own hair fitted into its top,
// and its two pommel paws in place of his feet and hands. The pack draws Steve as ever; the game swaps to the wolf by
// frame (WolfSteveShow). Includes no game header, so dcdata can build with it.
//
// Steve's head and feet leave his skinned body mesh's display list for a frame of their own (kSteveParts, its vertices
// kept so the .wgt still fits), his white hands his glove mesh's (kSteveHands, the green bands round his wrists left);
// the wolf's head, his hair and the foot paws are kWolfParts, the hand paws kWolfHands. Each new frame sits on Steve's
// body or glove frame, rest-posed there.

#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kSuperSteveFile = "commenu/weapon/c04w13.chr";
inline constexpr const char *kLambSwordFile = "commenu/weapon/c01w15.chr";
// Xiao's weapon menu reads her weapons' packs from this bundle (its c04w13.chr the same pack).
inline constexpr const char *kXiaoMenuWeaponsFile = "commenu/c04wtes.chr";

inline constexpr const char *kSteveParts = "c04w13_steve";
inline constexpr const char *kSteveHands = "c04w11_steve";
// The wolf's frames start hidden (the "__v" name flag); the game shows them in wolf form.
inline constexpr const char *kWolfParts = "c04w13_wolf__v";
inline constexpr const char *kWolfHands = "c04w11_wolf__v";

// The fit (the Wolf Steve viewer's, approved 2026-11-01), in the bind pose's units: Steve's long axis is z, his face
// +x. The neck cut is kNeck along z; the wolf's head (the sword's, its face +y, top +z) is scaled by head_scale, its
// front-to-back by head_depth more, and lifted head_lift off the cut; the foot paws hang head down from the ankles,
// foot_out further out to their side; the hand paws stand up on the green bands, hand_out further out.
struct WolfSteveFit {
    double head_scale = 0.28, head_depth = 3.6, head_lift = 0.08, head_forward = 0.0;
    double foot_scale = 0.48, foot_lift = -0.06, foot_out = 0.12;
    double hand_scale = 0.52, hand_lift = -0.09, hand_out = 0.05;
    double hair_height = 1.0;
};

inline constexpr WolfSteveFit kWolfSteveFit{};
inline constexpr double       kNeck = 0.5, kNeckRing = 0.42;

// Super Steve's pack with the wolf frames in, from his pack and the Lamb's Sword's.
Bytes BuildWolfSteve(std::span<const unsigned char> steve, std::span<const unsigned char> lamb);

std::vector<DataBuild> WolfSteveBuilds();

} // namespace expanded
