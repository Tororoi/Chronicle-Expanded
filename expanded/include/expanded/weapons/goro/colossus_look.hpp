#pragma once

namespace expanded {

// Tall Hammer "Colossus"'s look: as Goro grows past his own size, the animal skin and the leather he wears turn to the
// Tall Hammer's gold and his shirt to its turquoise, and the gold takes a light gold tint, each by his share of the way
// from his size to kColossusMost (none at his size or smaller, all of it at kColossusMost).
//   - The regions are his body sheets' texels (kColossusLookSheets; his face and hair sheet is left alone), each by its
//     own colour: green the shirt; the browns (the fur, the belt) the gold; flesh, the red strip, bone, metal and the
//     blackest lines kept; and the tan wraps (kColossusWrapsSheet's block kColossusWraps) kept whole.
//   - A texel's hue turns toward its region's (kColossusGoldHue, kColossusTurquoiseHue) along the shorter way round,
//     its saturation and lightness its own.
//   - The tint lights the gold as the game's character tint lights a model (its texture times one plus the tint):
//     kColossusTint times kColossusTintStrength at kColossusMost.
// The sheets are drawn from their baked pictures, so the look is written into those (a sheet at another size, a
// texture pack's, is left as it is).
inline constexpr const char *kColossusLookPack = "dun/mainchara/c06b.chr";
inline constexpr const char *kColossusLookBank = "c06a01.img";
inline constexpr const char *kColossusLookSheets[] = {"c06a02", "c06a03"};
inline constexpr const char *kColossusWrapsSheet = "c06a02";
inline constexpr int         kColossusWraps[4] = {96, 144, 160, 224};                  // x, y from; x, y to
inline constexpr float       kColossusGoldHue = 42.0f, kColossusTurquoiseHue = 193.0f; // the Tall Hammer's (c06w10)
inline constexpr float       kColossusTint[3] = {1.0f, 0.85f, 0.5f};                   // light gold
inline constexpr float       kColossusTintStrength = 0.5f;

// The look's regions (a texel's), and a colour (0 to 1) in one at a share of the look.
enum class ColossusRegion {
    Kept,
    Gold,
    Turquoise
};
ColossusRegion ColossusRegionOf(const char *sheet, int x, int y, const float *rgb);
void           ColossusLookColour(ColossusRegion region, float share, const float *rgb, float *out);

// His share of the look at a size (1: his own).
float ColossusLookShare(float size);

// A dungeon step with Goro in play at his size: the sheets brought to its share.
void ColossusLookStep(float size);

} // namespace expanded
