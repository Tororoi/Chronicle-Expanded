#pragma once

#include <array>
#include <cstdint>
#include <vector>

class CTexture;
struct PortDecodedTexture;

namespace expanded {

// Glows in a colour of a feature's own, drawn as the torches' glow is: the floor's torch picture (lightling) recoloured
// as a radial gradient, built as the floor enters it.

enum class GlowLook {
    kGold, // a pale gold core out to a deeper gold edge
    kBlue, // a cyan core out to a deep blue edge
    kFire, // a warm core out to a deep red-orange edge
    kRing, // nothing at the core out to a deep red edge: a red ring
    kCount,
};

// The torch's straight-alpha RGBA (width x height) recoloured: each pixel keeps its alpha and its share of the peak
// luminance, its colour going from core at the centre to outer at the glow's edge.
std::vector<uint8_t> RecolourGlow(const uint8_t *rgba, int width, int height, std::array<int, 3> core, std::array<int, 3> outer);

// The texture of a look on this floor, or null before the floor entered the torch picture.
CTexture *GlowTexture(GlowLook look);

// Draws a look's glow as a steady disc diameter units across, centred nearer units from at toward the camera and facing
// it, added to what is behind it, tested against depth from pull units nearer still (so a weapon's own mesh does not
// cut it).
void DrawGlowDisc(GlowLook look, const float *at, float diameter, float pull, float nearer = 0.0f);

// Hook: the texture manager decoded a picture, before it creates it.
void GlowPictureEntered(const char *name, const PortDecodedTexture &decoded);

} // namespace expanded
