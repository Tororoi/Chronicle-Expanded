#include "expanded/dungeon/glow_sprite.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "camera.hpp"
#include "draw2d_port.hpp"
#include "dun/gameloop.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "snd.hpp"
#include "texture.hpp"
#include "texture_port.hpp"

namespace expanded {

namespace {

constexpr const char *kTorchPicture = "lightling";
constexpr double      kCross = 0.125; // radius fraction where the core and the edge mix halfway

struct Ramp {
    std::array<int, 3> core;
    std::array<int, 3> outer;
};

constexpr Ramp kRamps[] = {
    {{255, 238, 180}, {255, 176, 40}}, // kGold
};
static_assert(std::size(kRamps) == static_cast<std::size_t>(GlowLook::kCount));

struct Glow {
    unsigned key = 0;
    CTexture texture;
};

Glow glows[static_cast<int>(GlowLook::kCount)];

int Log2(int value) {
    int log = 0;
    while ((1 << log) < value) {
        log++;
    }
    return log;
}

// The picture's pixels as straight RGBA, or empty for a format it is not in.
std::vector<uint8_t> Rgba(const PortDecodedTexture &decoded) {
    if (decoded.levels.empty()) {
        return {};
    }
    const std::vector<uint8_t> &pixels = decoded.levels[0];
    std::size_t                 count = static_cast<std::size_t>(decoded.width) * decoded.height;
    if (decoded.format == gfx::TextureFormat::Rgba8 && pixels.size() >= count * 4) {
        return {pixels.begin(), pixels.begin() + static_cast<std::ptrdiff_t>(count * 4)};
    }
    if (decoded.format == gfx::TextureFormat::Index8 && pixels.size() >= count) {
        std::vector<uint8_t> rgba(count * 4);
        for (std::size_t i = 0; i < count; i++) {
            uint32_t entry = decoded.palette[pixels[i]];
            for (int c = 0; c < 4; c++) {
                rgba[i * 4 + static_cast<std::size_t>(c)] = static_cast<uint8_t>(entry >> (8 * c));
            }
        }
        return rgba;
    }
    return {};
}

} // namespace

std::vector<uint8_t> RecolourGlow(const uint8_t *rgba, int width, int height, std::array<int, 3> core, std::array<int, 3> outer) {
    std::size_t         count = static_cast<std::size_t>(width) * height;
    std::vector<double> lums(count);
    for (std::size_t i = 0; i < count; i++) {
        lums[i] = 0.30 * rgba[i * 4] + 0.59 * rgba[i * 4 + 1] + 0.11 * rgba[i * 4 + 2];
    }
    double peak = count > 0 ? *std::max_element(lums.begin(), lums.end()) : 0.0;
    peak = peak == 0.0 ? 1.0 : peak;
    // The glow's edge: the farthest pixel with any light in it.
    double cx = (width - 1) / 2.0, cy = (height - 1) / 2.0, edge = 0.0;
    for (std::size_t i = 0; i < count; i++) {
        if (lums[i] > peak * 0.02) {
            edge = std::max(edge, std::hypot(double(i % std::size_t(width)) - cx, double(i / std::size_t(width)) - cy));
        }
    }
    edge = edge == 0.0 ? 1.0 : edge;
    double               expo = std::log(0.5) / std::log(kCross);
    std::vector<uint8_t> out(count * 4);
    for (std::size_t i = 0; i < count; i++) {
        double l = lums[i] / peak;
        double d = std::hypot(double(i % std::size_t(width)) - cx, double(i / std::size_t(width)) - cy);
        double t = std::pow(std::min(1.0, d / edge), expo);
        for (std::size_t c = 0; c < 3; c++) {
            out[i * 4 + c] = static_cast<uint8_t>(std::min(255, int((core[c] * (1.0 - t) + outer[c] * t) * l)));
        }
        out[i * 4 + 3] = rgba[i * 4 + 3];
    }
    return out;
}

CTexture *GlowTexture(GlowLook look) {
    Glow &glow = glows[static_cast<int>(look)];
    return glow.key != 0 && PortKeyLive(glow.key) ? &glow.texture : nullptr;
}

void GlowPictureEntered(const char *name, const PortDecodedTexture &decoded) {
    if (name == nullptr || std::strcmp(name, kTorchPicture) != 0) {
        return;
    }
    std::vector<uint8_t> rgba = Rgba(decoded);
    if (rgba.empty()) {
        return;
    }
    int width = static_cast<int>(decoded.width), height = static_cast<int>(decoded.height);
    for (std::size_t look = 0; look < std::size(kRamps); look++) {
        Glow &glow = glows[look];
        if (glow.key != 0 && PortKeyLive(glow.key)) {
            PortReleaseKey(glow.key);
        }
        PortDecodedTexture recoloured;
        recoloured.width = decoded.width;
        recoloured.height = decoded.height;
        recoloured.format = gfx::TextureFormat::Rgba8;
        recoloured.levels.push_back(RecolourGlow(rgba.data(), width, height, kRamps[look].core, kRamps[look].outer));
        unsigned palette = 0;
        glow.key = PortCreateTexture(recoloured, PortTextureOwner::Manager, &palette);
        glow.texture = CTexture();
        std::strcpy(glow.texture.name, name);
        glow.texture.width = static_cast<short>(width);
        glow.texture.height = static_cast<short>(height);
        glow.texture.bpp = 4;
        glow.texture.block = -1;
        glow.texture.tex0 = static_cast<u_long>(glow.key) | (static_cast<u_long>(std::max(1, (width + 63) / 64)) << 14) |
                            (static_cast<u_long>(Log2(width)) << 26) | (static_cast<u_long>(Log2(height)) << 30) | (1ul << 34);
    }
}

void DrawGlowDisc(GlowLook look, const float *at, float diameter, float pull) {
    CTexture *texture = GlowTexture(look);
    if (texture == nullptr || NowCamera__3 == nullptr || diameter <= 0.0f) {
        return;
    }
    CCamera      *camera = reinterpret_cast<CCamera *>(NowCamera__3);
    sceVu0FVECTOR toward, ref, world;
    camera->GetPos(toward);
    camera->GetRef(ref);
    sceVu0SubVector(toward, toward, ref);
    sceVu0Normalize(toward, toward);
    sceVu0ScaleVector(toward, toward, pull);
    sceVu0CopyVector(world, const_cast<float *>(at));
    world[3] = 1.0f;
    int top_left[4], bottom_right[4], near_top_left[4], near_bottom_right[4];
    if (MGRotTransPers3DSprite(top_left, bottom_right, world, diameter, diameter, 1) != 1) {
        return;
    }
    int top_right[4] = {bottom_right[0], top_left[1], top_left[2], top_left[3]};
    int bottom_left[4] = {top_left[0], bottom_right[1], bottom_right[2], bottom_right[3]};
    sceVu0AddVector(world, world, toward);
    world[3] = 1.0f;
    if (MGRotTransPers3DSprite(near_top_left, near_bottom_right, world, diameter, diameter, 0) == 1) {
        top_left[2] = top_right[2] = bottom_left[2] = bottom_right[2] = near_top_left[2];
    }
    // As the torches draw their glow: depth tested but not written, added to what is behind.
    const draw2d::Services &services = draw2d::Get();
    sceGsTest               test = mgPixelTest;
    test.bits.ate = 0;
    test.bits.zte = 1;
    services.set_test(&test);
    sceGsZbuf zbuffer = mgZBuffer;
    zbuffer.bits.zmsk = 1;
    services.set_zbuf(&zbuffer);
    sceGsAlpha alpha = mgAlpha;
    alpha.bits.a = 0;
    alpha.bits.b = 2;
    alpha.bits.c = 0;
    alpha.bits.d = 1;
    services.set_alpha(&alpha);
    set3DSpriteFog(Vif1Packet, texture, CRect_i_(0, 0, texture->width, texture->height), top_left, top_right, bottom_left,
                   bottom_right, 0x80);
    services.set_test(nullptr);
    services.set_zbuf(nullptr);
    services.set_alpha(nullptr);
}

} // namespace expanded
