#include "confuse.hpp"

#include <cstring>

#include "itemdata.hpp"
#include "texture_port.hpp"

#include "icon/confuse.hpp"

extern WEAPON_DATA WeaponList[120];

namespace {

// Babel's Spear carries the ability from its weapon data.
const bool kBabelConfuses = [] {
    WeaponList[ITEM_WEAPON_BABEL_S_SPEAR - ITEM_WEAPON_START].flags |= kWeaponFlagConfuse;
    return true;
}();

} // namespace

int ConfuseOptionMessage(int bit) {
    return bit == kConfuseOptionBit ? kConfuseNameMessage : bit + 69;
}

void ConfuseIconTexture(const char *name, PortDecodedTexture &texture) {
    // The tile of bit 14: the column of bits 8 to 14, the seventh row.
    constexpr int kTileX = 0xD8;
    constexpr int kTileY = 0x50 + 0x14 * 6;
    if (name == nullptr || std::strcmp(name, "charaface") != 0 || texture.width != 256 || texture.height != 320 ||
        texture.levels.empty()) {
        return;
    }
    std::vector<uint8_t> &pixels = texture.levels[0];
    bool                  indexed = texture.format == gfx::TextureFormat::Index8;
    size_t                bytes = indexed ? 1 : 4;
    if (pixels.size() < static_cast<size_t>(texture.width) * texture.height * bytes) {
        return;
    }
    auto at = [&](int x, int y) { return (static_cast<size_t>(y) * texture.width + x) * bytes; };
    // Only a sheet whose tile is empty: one colour throughout.
    for (int y = 0; y < kConfuseIconSize; y++) {
        for (int x = 0; x < kConfuseIconSize; x++) {
            if (std::memcmp(&pixels[at(kTileX + x, kTileY + y)], &pixels[at(kTileX, kTileY)], bytes) != 0) {
                return;
            }
        }
    }
    for (int y = 0; y < kConfuseIconSize; y++) {
        for (int x = 0; x < kConfuseIconSize; x++) {
            const uint8_t *rgba = &kConfuseIconRgba[(y * kConfuseIconSize + x) * 4];
            if (rgba[3] < 128) {
                continue;
            }
            uint8_t *out = &pixels[at(kTileX + x, kTileY + y)];
            if (!indexed) {
                std::memcpy(out, rgba, 3);
                out[3] = 255;
                continue;
            }
            // The palette's nearest opaque colour.
            int best = -1;
            int best_distance = 0;
            for (int i = 0; i < 256; i++) {
                uint32_t entry = texture.palette[i];
                int      r = entry & 0xFF;
                int      g = (entry >> 8) & 0xFF;
                int      b = (entry >> 16) & 0xFF;
                int      a = entry >> 24;
                if (a < 128) {
                    continue;
                }
                int distance = (r - rgba[0]) * (r - rgba[0]) + (g - rgba[1]) * (g - rgba[1]) + (b - rgba[2]) * (b - rgba[2]);
                if (best < 0 || distance < best_distance) {
                    best = i;
                    best_distance = distance;
                }
            }
            if (best >= 0) {
                *out = static_cast<uint8_t>(best);
            }
        }
    }
}

