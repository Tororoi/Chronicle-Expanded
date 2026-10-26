#include "expanded/dungeon/ground_shadow.hpp"

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
            std::printf("ground shadow: no shadow disc, %s could not be loaded\n", kShadowDiscFile);
            shadow.disc = nullptr;
            shadow.failed = true;
        }
    }
    return shadow.disc;
}

} // namespace

void GroundShadowShow(float x, float ground, float z, float radius) {
    shadow.shown = Disc() != nullptr;
    shadow.at[0] = x;
    shadow.at[1] = ground + kGroundShadowLift;
    shadow.at[2] = z;
    shadow.radius = radius;
}

void GroundShadowHide() {
    shadow.shown = false;
}

bool GroundShadowShown() {
    return shadow.shown;
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
