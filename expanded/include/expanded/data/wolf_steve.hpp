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
inline constexpr const char *kMachoFile = "gedit/e01/chara/p01a.chr";
// Xiao's weapon menu reads her weapons' packs from this bundle (its c04w13.chr the same pack).
inline constexpr const char *kXiaoMenuWeaponsFile = "commenu/c04wtes.chr";

inline constexpr const char *kSteveParts = "c04w13_steve";
inline constexpr const char *kSteveHands = "c04w11_steve";
// The wolf's frames start hidden (the "__v" name flag); the game shows them in wolf form.
inline constexpr const char *kWolfParts = "c04w13_wolf__v";
inline constexpr const char *kWolfHands = "c04w11_wolf__v";

// Macho's hat (the crown and brim of his head mesh kMachoHead: its kMachoHatMaterials, drawn with his picture
// kMachoHatSheet, swizzled into Steve's bank) on Steve's head, for the Macho Sword's SynthSphere; his hair (the head in
// its yellow) on a frame of its own, hidden under it. The hat sits on the body frame in its mesh's coordinates (+y up, the
// face +x): turned from Macho's (face +z) and set at its seat (the middle of its crown, kMachoHatSeat up Macho's head),
// then sized, tilted back, turned about the up and moved by kMachoHatFit (the hat viewer's, approved 2026-11-02), onto
// the top of Steve's head, kSteveHeadTop.
inline constexpr const char *kSteveHair = "c04w13_hair";
inline constexpr const char *kMachoHat = "c04w13_hat__v";
inline constexpr const char *kMachoMds = "p01a.mds", *kMachoImg = "p01a01.img", *kMachoHead = "obj3", *kMachoHatSheet = "p01a02";
inline constexpr int         kMachoHatMaterials[] = {0, 1};
inline constexpr double      kMachoHatSeat = 18.9;
inline constexpr double      kSteveHeadTop[3] = {-0.22, 2.4, 0.0};

struct MachoHatFit {
    double size = 0.22, lift = -0.40, forward = 0.27, side = 0.0, tilt = 4.0, turn = -180.0; // degrees
};

inline constexpr MachoHatFit kMachoHatFit{};

// The Macho Sword's flexed arms on Steve, for its SynthSphere: his own arms (his body mesh's vertices past kSteveArmOut
// across him, in front of kSteveBandBack, the slingshot band's ends kSteveBandEnds kept) on a frame of their own
// (kSteveArms), and the sword's (its mesh kMachoSwordMesh's triangles past kSwordArmOut across the hilt, between
// kSwordArmLow and kSwordArmHigh along it, its fists, above kSwordFistFrom and inside kSwordFistIn, left off) on another
// (kMachoArms), in his sleeve's texture: each corner the texture point of the nearest vertex of his own arm, at the
// thin look. Each arm is laid along his shoulder-elbow-wrist line by where it sits along the sword's (its shoulder its
// innermost vertices, its elbow its outermost, its wrist those past kSwordWristFrom), its distance off the line times the
// look's muscle: his shoulder on the side of his body at his arm's old shoulder ring (its vertices below kSteveElbowFrom
// up him), his elbow at his elbow ring's middle (below kSteveWristFrom), each moved by the look; his wrist the middle of
// his glove's opening. kMachoArms holds three sets of vertices: the drawn ones (the thin look), then the thin look's
// and the buff's to lay them between (macho_sword.hpp grows them with the kills). The looks are the arms viewer's,
// approved 2026-11-02.
inline constexpr const char *kMachoSwordFile = "commenu/weapon/c01w12.chr";
inline constexpr const char *kMachoSwordMds = "c01w12.mds", *kMachoSwordMesh = "c01w12__m";
inline constexpr const char *kSteveCfg = "c04w13.cfg";
inline constexpr const char *kSteveArms = "c04w13_arms";
inline constexpr const char *kMachoArms = "c04w13_macho__v";
inline constexpr double      kSteveArmOut = 0.4, kSteveBandBack = -0.3, kSteveElbowFrom = 1.8, kSteveWristFrom = 2.4;
inline constexpr int         kSteveBandEnds[] = {37, 38, 49, 50};
inline constexpr double      kSwordArmOut = 0.45, kSwordArmLow = 1.0, kSwordArmHigh = 3.25;
inline constexpr double      kSwordFistFrom = 2.3, kSwordFistIn = 1.3, kSwordWristFrom = 2.3;
inline constexpr int         kMachoArmsSets = 3;

struct MachoArmsLook {
    double muscle, shoulder_out, shoulder_up, elbow_out, elbow_up;
};

inline constexpr MachoArmsLook kMachoArmsThin{0.5, -0.26, -0.37, 0.0, -0.42};
inline constexpr MachoArmsLook kMachoArmsBuff{0.85, -0.26, -0.56, 0.5, -0.45};

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

// Super Steve's pack with the wolf frames, his hair's, the hat's and the arms' in, from his pack, the Lamb's Sword's,
// Macho's and the Macho Sword's.
Bytes BuildWolfSteve(std::span<const unsigned char> steve, std::span<const unsigned char> lamb, std::span<const unsigned char> macho,
                     std::span<const unsigned char> macho_sword);

std::vector<DataBuild> WolfSteveBuilds();

} // namespace expanded
