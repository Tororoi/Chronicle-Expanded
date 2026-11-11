#include "expanded/weapons/goro/inferno/cape_flap.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>

#include "character.hpp"
#include "cloth.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "expanded/weapons/player_state.hpp"
#include "frame.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// A cape as he wore it when the flame first blew it: its own rest shape, and which way a swing lifts it off his back.
struct Cape {
    CCloth *cloth = nullptr;
    float   home[16][16][3] = {};
    float   gravity[3] = {};
    float   stiffness[3] = {};
    float   sign = 1.0f;
};

Cape  capes[CHARA_CLOTH_MAX];
bool  blown = false; // a cape's rest shape is not its own
float phase = 0.0f;

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(CapeFlapReset), true);

// v turned by angle about the unit axis (Rodrigues).
void Turn(const float *v, const float *axis, float angle, float *out) {
    float c = std::cos(angle), s = std::sin(angle), dot = v[0] * axis[0] + v[1] * axis[1] + v[2] * axis[2];
    float cross[3] = {axis[1] * v[2] - axis[2] * v[1], axis[2] * v[0] - axis[0] * v[2], axis[0] * v[1] - axis[1] * v[0]};
    for (int k = 0; k < 3; k++) {
        out[k] = v[k] * c + cross[k] * s + axis[k] * dot * (1.0f - c);
    }
}

// Across the collar, in the cape's frame: its first column's pin to its last's.
bool Across(const Cape &cape, float *axis) {
    const CCloth &cloth = *cape.cloth;
    for (int k = 0; k < 3; k++) {
        axis[k] = cape.home[cloth.num_i - 1][0][k] - cape.home[0][0][k];
    }
    float length = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    if (!(length > 1e-4f)) {
        return false;
    }
    for (int k = 0; k < 3; k++) {
        axis[k] /= length;
    }
    return true;
}

// Which way about the collar lifts the hem behind him: the swing that carries its middle column's hem back from his
// facing, in the world.
float LiftSign(const Cape &cape, const float *axis) {
    const CCloth &cloth = *cape.cloth;
    if (cloth.frame == nullptr) {
        return 1.0f;
    }
    int          a = cloth.num_i / 2, hem = cloth.num_j - 1;
    const float *pin = cape.home[a][0];
    float        hang[3] = {cape.home[a][hem][0] - pin[0], cape.home[a][hem][1] - pin[1], cape.home[a][hem][2] - pin[2]}, turned[3];
    Turn(hang, axis, 0.5f, turned);
    float was[4] = {pin[0] + hang[0], pin[1] + hang[1], pin[2] + hang[2], 1.0f}, now[4] = {pin[0] + turned[0], pin[1] + turned[1], pin[2] + turned[2], 1.0f};
    float was_world[4], now_world[4];
    cloth.frame->GetWorldPosition(was_world, was);
    cloth.frame->GetWorldPosition(now_world, now);
    float rotation[4] = {0.0f, CharaMain.rotation.y, 0.0f, 0.0f};
    if (CharaMain.frame != nullptr) {
        CharaMain.frame->GetRotation(rotation);
    }
    float back[2] = {-std::sin(rotation[1]), -std::cos(rotation[1])};
    return (now_world[0] - was_world[0]) * back[0] + (now_world[2] - was_world[2]) * back[1] >= 0.0f ? 1.0f : -1.0f;
}

Cape *Kept(CCloth *cloth) {
    for (Cape &cape : capes) {
        if (cape.cloth == cloth) {
            return &cape;
        }
    }
    for (Cape &cape : capes) {
        if (cape.cloth == nullptr) {
            cape.cloth = cloth;
            for (int a = 0; a < cloth->num_i; a++) {
                for (int b = 0; b < cloth->num_j; b++) {
                    std::copy(cloth->home[a][b], cloth->home[a][b] + 3, cape.home[a][b]);
                }
            }
            std::copy(cloth->gravity, cloth->gravity + 3, cape.gravity);
            std::copy(cloth->stiffness, cloth->stiffness + 3, cape.stiffness);
            float axis[3];
            cape.sign = Across(cape, axis) ? LiftSign(cape, axis) : 1.0f;
            return &cape;
        }
    }
    return nullptr;
}

bool Worn(const CCloth *cloth) {
    return CharaMain.cloth != nullptr && std::find(CharaMain.cloth, CharaMain.cloth + CHARA_CLOTH_MAX, cloth) != CharaMain.cloth + CHARA_CLOTH_MAX;
}

// Each cape he still wears back to its own rest shape (one he no longer wears may be gone with its model).
void PutBack() {
    for (Cape &cape : capes) {
        if (cape.cloth == nullptr || !Worn(cape.cloth)) {
            continue;
        }
        for (int a = 0; a < cape.cloth->num_i; a++) {
            for (int b = 0; b < cape.cloth->num_j; b++) {
                std::copy(cape.home[a][b], cape.home[a][b] + 3, cape.cloth->home[a][b]);
            }
        }
        std::copy(cape.gravity, cape.gravity + 3, cape.cloth->gravity);
        std::copy(cape.stiffness, cape.stiffness + 3, cape.cloth->stiffness);
    }
    blown = false;
}

} // namespace

float CapeFlapSwing(int b, int hang) {
    float t = hang > 1 ? static_cast<float>(b) / (hang - 1) : 0.0f;
    float swell = kCapeFlapSwell * std::min(1.0f, 2.0f * t);
    return kCapeFlapLift * std::pow(t, kCapeFlapEase) + swell * std::sin(phase - static_cast<float>(b) * 2.0f * std::numbers::pi_v<float> / kCapeFlapRippleRows);
}

void CapeFlapStep() {
    float risen = RisingFlameRisen();
    bool  his = InfernoWielded() || (Playing(CHARA_GORO) && SatansAxFlame()); // the Inferno's or Satan's Ax's, on Goro
    if (risen <= 0.0f || !his || CharaMain.cloth == nullptr) {
        if (blown) {
            PutBack();
        }
        phase = 0.0f;
        return;
    }
    phase += 2.0f * std::numbers::pi_v<float> / kCapeFlapRippleFrames;
    for (CCloth *cloth : std::span(CharaMain.cloth, CHARA_CLOTH_MAX)) {
        Cape *cape = cloth != nullptr ? Kept(cloth) : nullptr;
        float axis[3];
        if (cape == nullptr || !Across(*cape, axis)) {
            continue;
        }
        // Held to the shape, as the cat's cape is: its pull down faded out, its springs stiffened.
        for (int k = 0; k < 3; k++) {
            cloth->gravity[k] = cape->gravity[k] * (1.0f - risen);
            cloth->stiffness[k] = cape->stiffness[k] + (std::max(cape->stiffness[k], kCapeFlapStiffness) - cape->stiffness[k]) * risen;
        }
        // A chain down each column: each row hangs from the one above it by its own length, turned by its swing.
        for (int b = 1; b < cloth->num_j; b++) {
            float angle = cape->sign * risen * CapeFlapSwing(b, cloth->num_j);
            for (int a = 0; a < cloth->num_i; a++) {
                const float *above = cape->home[a][b - 1], *rest = cape->home[a][b];
                float        link[3] = {rest[0] - above[0], rest[1] - above[1], rest[2] - above[2]}, turned[3];
                Turn(link, axis, angle, turned);
                for (int k = 0; k < 3; k++) {
                    cloth->home[a][b][k] = cloth->home[a][b - 1][k] + turned[k];
                }
            }
        }
        blown = true;
    }
}

void CapeFlapReset() {
    if (blown) {
        PutBack();
    }
    for (Cape &cape : capes) {
        cape.cloth = nullptr;
    }
    blown = false;
    phase = 0.0f;
}

} // namespace expanded
