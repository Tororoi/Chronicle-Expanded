#include "expanded/dungeon/ground_shadow.hpp"

#include <algorithm>
#include <cstdio>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/prop_models.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/enemy_events.hpp"

namespace expanded {

namespace {

struct Shadow {
    CCharacter *disc = nullptr; // loaded on this floor
    bool        failed = false; // could not be, on this floor
    bool        shown = false;
    float       at[3] = {};
    float       radius = 0.0f;
    const void *owner = nullptr;
};

Shadow shadow;

void OnFloor() {
    GroundShadowReset();
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

CCharacter *Disc() {
    if (shadow.disc == nullptr && !shadow.failed) {
        shadow.disc = EnterFloorCharacter(kShadowDiscFile, kShadowDiscCfg);
        if (shadow.disc == nullptr || shadow.disc->shadow_frame == nullptr) {
            std::fprintf(stderr, "ground shadow: no shadow disc, %s could not be loaded\n", kShadowDiscFile);
            shadow.disc = nullptr;
            shadow.failed = true;
        }
    }
    return shadow.disc;
}

} // namespace

void GroundShadowShow(float x, float ground, float z, float radius, const void *owner) {
    shadow.shown = Disc() != nullptr;
    shadow.owner = owner;
    shadow.at[0] = x;
    shadow.at[1] = ground + kGroundShadowLift;
    shadow.at[2] = z;
    shadow.radius = radius;
}

void GroundShadowHide(const void *owner) {
    if (shadow.owner == owner) {
        shadow.shown = false;
    }
}

bool GroundShadowShown() {
    return shadow.shown;
}

void GroundShadowAt(float *at) {
    std::copy(shadow.at, shadow.at + 3, at);
}

float GroundShadowRadius() {
    return shadow.radius;
}

void GroundShadowDraw() {
    if (!shadow.shown || shadow.disc == nullptr || BtEventMode != 0) {
        return;
    }
    CCharacter *disc = shadow.disc;
    disc->SetPosition(shadow.at[0], shadow.at[1], shadow.at[2]);
    disc->SetRotation(0.0f, 0.0f, 0.0f);
    disc->SetScale(shadow.radius, shadow.radius, shadow.radius);
    disc->DrawShadow();
}

void GroundShadowReset() {
    shadow = Shadow{};
}

} // namespace expanded
