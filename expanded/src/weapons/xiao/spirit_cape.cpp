#include "expanded/weapons/xiao/spirit_cape.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

#include "bound.hpp"
#include "character.hpp"
#include "cloth.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "frame.hpp"
#include "mglib.hpp"
#include "texture.hpp"
#include "texture_port.hpp"

namespace expanded {

namespace {

constexpr const char *kCapeFrame = "cat_cape";
constexpr const char *kMaskFrame = "cat_mask";
constexpr const char *kCapeTexture = "catcape"; // the mask's too
constexpr int         kSkippedByParent = 4;     // CFrameAttr::draw_on: the parent's draw passes over it
constexpr float       kWindEase = 1.6f;         // > 1 keeps the shoulders down and flies the tail
constexpr float       kRippleSwell = 0.65f;     // at the crest
constexpr float       kRippleReach = 0.5f;      // the share of the hang down which the swell grows to full
constexpr float       kRippleRows = 10.0f;      // a wavelength, in rows
constexpr float       kSpringAcross = 0.42f, kSpringAlong = 0.12f, kSpringUp = 0.16f;
constexpr int         kBounds = 4;

struct Cape {
    CCloth *cloth = nullptr;
    CFrame *mask = nullptr;
    int     wide = 0, hang = 0;
    float   sized = 1.0f;
    float   rest[16][16][4] = {}; // the rest shape as the pack has it: the wind rides on it
    float   span = 0.0f;          // collar to hem along the cape
    float   phase = 0.0f;
    int     painted = -1;
};

Cape cape;

void Place(const sceVu0FMATRIX m, const float *p, float *out) {
    for (int c = 0; c < 3; c++) {
        out[c] = p[0] * m[0][c] + p[1] * m[1][c] + p[2] * m[2][c] + m[3][c];
    }
    out[3] = 1.0f;
}

// The cape's texture (the mask's too) painted the element's colour: a flat picture whose 256 palette entries are its
// one colour.
void Paint(int element) {
    if (cape.painted == element) {
        return;
    }
    CTexture *texture = TexManager.GetTexture(const_cast<char *>(kCapeTexture), kFloorCharacterBlock);
    if (texture == nullptr) {
        return;
    }
    const unsigned char *rgb = kSpiritCapeLooks[element].rgb;
    u_int                clut[256];
    std::fill(std::begin(clut), std::end(clut), rgb[0] | (rgb[1] << 8) | (rgb[2] << 16) | (0x80u << 24));
    if (PortLoadClut(reinterpret_cast<sceGsTex0 *>(&texture->tex0)->CBP, clut)) {
        cape.painted = element;
    }
}

} // namespace

bool SpiritCapeTake(CCharacter &cat) {
    cape = Cape{};
    for (int i = 0; i < CHARA_CLOTH_MAX; i++) {
        CCloth *cloth = cat.cloth[i];
        if (cloth != nullptr && cloth->frame != nullptr && std::strcmp(cloth->frame->name, kCapeFrame) == 0) {
            cape.cloth = cloth;
            cat.cloth[i] = nullptr;
            break;
        }
    }
    if (cape.cloth == nullptr || cape.cloth->num_i <= 0 || cape.cloth->num_i > 16 || cape.cloth->num_j <= 1 || cape.cloth->num_j > 16) {
        cape.cloth = nullptr;
        return false;
    }
    CCloth *cloth = cape.cloth;
    cape.wide = cloth->num_i;
    cape.hang = cloth->num_j;
    std::memcpy(cape.rest, cloth->home, sizeof(cape.rest));
    int mid = cape.wide / 2;
    cape.span = std::fabs(cape.rest[mid][0][0] - cape.rest[mid][cape.hang - 1][0]);
    cloth->wind = nullptr;
    std::memcpy(cloth->last, cloth->point, sizeof(cloth->last)); // a quiet first step
    cape.mask = FrameNamed(cat.frame, kMaskFrame);
    if (cape.mask != nullptr) {
        cape.mask->attr.draw_on |= kSkippedByParent;
    }
    return true;
}

CCloth *SpiritCape() {
    return cape.cloth;
}

void SpiritCapeSize(float scale) {
    if (cape.cloth == nullptr || scale == cape.sized) {
        return;
    }
    float   ratio = scale / cape.sized;
    CCloth *cloth = cape.cloth;
    // The ties were measured in the pack's units; the sheet they pull toward comes through the cat's scaled frame.
    for (int a = 0; a < cape.wide; a++) {
        for (int b = 0; b < cape.hang; b++) {
            cloth->rest[a][b][0] *= ratio;
            cloth->rest[a][b][1] *= ratio;
        }
    }
    cloth->wind_effect *= ratio;
    int count = 0;
    for (CBound *bound = cloth->bound; bound != nullptr && count < kBounds; bound = bound->next, count++) {
        for (int k = 0; k < 3; k++) {
            bound->extent[k] *= ratio;
            bound->reciprocal[k] = bound->extent[k] > 0.0f ? 1.0f / bound->extent[k] : 0.0f;
        }
    }
    cape.sized = scale;
}

void SpiritCapeReseed() {
    if (cape.cloth == nullptr || cape.cloth->frame == nullptr) {
        return;
    }
    CCloth       *cloth = cape.cloth;
    sceVu0FMATRIX m;
    cloth->frame->GetLWMatrix(m);
    for (int a = 0; a < cape.wide; a++) {
        for (int b = 0; b < cape.hang; b++) {
            Place(m, cape.rest[a][b], cloth->point[a][b]);
            std::memcpy(cloth->last[a][b], cloth->point[a][b], sizeof(cloth->last[a][b]));
            std::fill(std::begin(cloth->speed[a][b]), std::end(cloth->speed[a][b]), 0.0f);
        }
    }
    Place(m, cloth->position, cloth->last_position);
}

void SpiritCapeStep(float yaw, float floor_y, float scale) {
    if (cape.cloth == nullptr) {
        return;
    }
    CCloth      *cloth = cape.cloth;
    const float *hem = cloth->point[cape.wide / 2][cape.hang - 1], *pin = cloth->point[0][0];
    float        span = std::sqrt((hem[0] - pin[0]) * (hem[0] - pin[0]) + (hem[1] - pin[1]) * (hem[1] - pin[1]) + (hem[2] - pin[2]) * (hem[2] - pin[2]));
    if (span > kSpiritCapeTearSpan * scale || std::isnan(span)) {
        SpiritCapeReseed();
        return;
    }
    // The springs: stiff across the cape, slack along it, turned with the cat (the engine's axes are the world's).
    float fx = std::sin(yaw) * std::sin(yaw), fz = std::cos(yaw) * std::cos(yaw);
    cloth->stiffness[0] = kSpringAlong * fx + kSpringAcross * fz;
    cloth->stiffness[1] = kSpringUp;
    cloth->stiffness[2] = kSpringAlong * fz + kSpringAcross * fx;
    // The wind: the rest shape blown back off the back, each row drawn in as it rises so the sheet never lengthens.
    cape.phase += 2.0f * std::numbers::pi_v<float> / kSpiritCapeRippleFrames;
    for (int b = 1; b < cape.hang; b++) {
        float t = static_cast<float>(b) / (cape.hang - 1);
        float swell = kRippleSwell * std::min(1.0f, t / kRippleReach);
        float lift = kSpiritCapeLift * std::pow(t, kWindEase) + swell * std::sin(cape.phase - b * 2.0f * std::numbers::pi_v<float> / kRippleRows);
        float d = cape.span * t, trim = d - std::sqrt(std::max(0.0f, d * d - lift * lift));
        for (int a = 0; a < cape.wide; a++) {
            cloth->home[a][b][0] = cape.rest[a][b][0] + trim; // back toward the collar
            cloth->home[a][b][1] = cape.rest[a][b][1] - lift; // up off the back
        }
    }
    cloth->wind = nullptr;
    cloth->floor_y = floor_y;
    cloth->Step(0);
}

void SpiritCapeDraw(const CCharacter &cat, int element, float lit) {
    if (cape.cloth == nullptr) {
        return;
    }
    element = element >= 0 && element < 6 ? element : 5;
    Paint(element);
    sceVu0FVECTOR scene, ambient;
    MGGetAmbient(scene);
    std::memcpy(ambient, scene, sizeof(ambient));
    if (cat.fade[0] < 1.0f) {
        for (int c = 0; c < 3; c++) {
            ambient[c] *= cat.fade[0];
        }
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] += cat.ambient_offset[c] + cat.ambient_tint[c] + kSpiritCapeLooks[element].tint[c] * lit;
    }
    if (!(cat.ambient_offset[3] < 0.0f)) {
        ambient[3] = cat.ambient_offset[3];
    }
    MGSetAmbient(ambient);
    if (cape.mask != nullptr) {
        MGDraw(cape.mask);
    }
    cape.cloth->Draw();
    MGSetAmbient(scene);
}

void SpiritCapeReset() {
    cape = Cape{};
}

} // namespace expanded
