#pragma once

// The models of the props Expanded's weapons raise on a floor, and the effects that mark them, built from the disc at
// extraction. Includes no game header, so dcdata can build with it.

#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

// Queens' two trees, whole: georama part 12's display model (the first MDS block of gedit/e03's scene sub-file e03t01;
// sixteen frames under null4 at the origin: trunk cyl28__s at (0, 8), 51 tall, with its crown ha__a7f, trunk cyl283__s at
// (-10, -11), 72.5 tall, with its crown ha2__a7f, ten grass sprites between them, and the 50 by 50 grass square
// grid__a7f, which is textured as Queens' paving and whose mesh is dropped), with the one picture it draws with (e03b04 of
// the building bank e03b01.img, 256 by 256).
inline constexpr const char *kQueensTreesFile = "expanded/chara/queens_trees.chr";
inline constexpr const char *kQueensTreesCfg = "queens_trees.cfg";
inline constexpr const char *kQueensSceneFile = "gedit/e03/scene.scn";
inline constexpr const char *kQueensImagesFile = "gedit/e03/img.pak";

// The shadow disc: a flat unit disc of kShadowDiscSegments segments at height 0 (its fan wound both ways, so a face
// survives whichever way the shadow's draw culls), the model and the shadow model of a character of its own. Its frame
// and materials are the rock's (gedit/s96's iwa.mds); the one picture the materials name, the rock's d02b10, is cut to
// kShadowDiscStandIn texels a side, never drawn. Scaled, the shadow's radius.
inline constexpr const char *kShadowDiscFile = "expanded/chara/shadow_disc.chr";
inline constexpr const char *kShadowDiscCfg = "shadow_disc.cfg";
inline constexpr const char *kShadowDiscSource = "gedit/s96/chara/iwa.chr";
inline constexpr int         kShadowDiscSegments = 16;
inline constexpr int         kShadowDiscStandIn = 8;

// Babel's Spear's beam: the Dark Genie's small beam (the pack of its companion c17_beem_s, KEYs 10-28 its rise, 28-48 its
// loop and 48-70 its vanish), every 8-bit picture's palette recoloured, red and green exchanged (its pink cyan, its purple
// blue) and each entry's alpha kBabelBeamAlpha of its own (rounded half to even), and its cfg (info.cfg) under the name a
// shot effect asks for, kBabelBeamName.cfg.
inline constexpr const char *kBabelBeamFile = "expanded/effect/babel_beam.chr";
inline constexpr const char *kBabelBeamName = "babel_beam";
inline constexpr const char *kBabelBeamSource = "dun/monstor/c17_beem_s.chr";
inline constexpr float       kBabelBeamAlpha = 0.3f;

// The trees' pack, from the scene and the image pack of gedit/e03.
Bytes BuildQueensTrees(std::span<const unsigned char> scene, std::span<const unsigned char> images);

// The disc's pack, from the rock's pack.
Bytes BuildShadowDisc(std::span<const unsigned char> rock);

// The beam's pack, from the genie's beam's.
Bytes BuildBabelBeam(std::span<const unsigned char> beam);

// The props' models and the beam.
std::vector<DataBuild> PropModelBuilds();

} // namespace expanded
