#pragma once

// The Gold Bullion's item model, which the game never had (its item has no model, so its drop showed another's): one
// bullion bar, a trapezoid kGoldBarLength long, its foot kGoldBarFoot and its top kGoldBarTop wide and kGoldBarHeight
// high, as a share of the Saving Book's own model (dun/item/main_data/savesyo, whose frame, material and pictures' bank
// it is built on) so it stands as large as the game's items. Its edges and corners are bevelled, kGoldBarRound of its
// height across, in one flat cut each (44 triangles, low-poly as the game's items), its normals smooth over the bevels
// and its faces flat between. Drawn
// it reflects its floor tile as the Midas Touch's statues do (weapons/goro/midas_touch.hpp); its texture coordinates
// sample the gold ball (gold_statue.hpp) where each corner would reflect from above and in front, for where it is drawn
// with no reflection. Built at extraction into kGoldBarModel and kGoldBarPictures; the game finds them for the Gold
// Bullion through ExpandedItemModel (core/item_models.hpp). Includes no game header, so dcdata can build with it.

#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kGoldBarSource = "dun/item/main_data/savesyo";
inline constexpr const char *kGoldBar = "expanded/item/gold_bar"; // .mds and .img
inline constexpr const char *kGoldBarModel = "expanded/item/gold_bar.mds";
inline constexpr const char *kGoldBarPictures = "expanded/item/gold_bar.img";
inline constexpr const char *kGoldBarPicture = "gldbar";
inline constexpr double      kGoldBarLength = 0.9, kGoldBarFoot = 0.45, kGoldBarTop = 0.32, kGoldBarHeight = 0.22;
inline constexpr double      kGoldBarTilt = 0.6; // the view the faces reflect from: this far above (0 level, 1 overhead)
inline constexpr double      kGoldBarRound = 0.2;

Bytes BuildGoldBarModel(std::span<const unsigned char> mds);
Bytes BuildGoldBarPictures(std::span<const unsigned char> img);

std::vector<DataBuild> GoldBarBuilds();

} // namespace expanded
