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
    kCount,
};

// The torch's straight-alpha RGBA (width x height) recoloured: each pixel keeps its alpha and its share of the peak
// luminance, its colour going from core at the centre to outer at the glow's edge.
std::vector<uint8_t> RecolourGlow(const uint8_t *rgba, int width, int height, std::array<int, 3> core, std::array<int, 3> outer);

// The texture of a look on this floor, or null before the floor entered the torch picture.
CTexture *GlowTexture(GlowLook look);

// Hook: the texture manager decoded a picture, before it creates it.
void GlowPictureEntered(const char *name, const PortDecodedTexture &decoded);

} // namespace expanded
