#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/weapons/blade_scale.hpp"

#include <libvu0.h>

#include <cmath>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "frame.hpp"

namespace expanded {

namespace {

constexpr int kToan = 0;

int FreeHit(const CCollisionData &hits) {
    for (int i = 0; i < kHitRecords; i++) {
        if (hits.active[i] == 0) {
            return i;
        }
    }
    return -1;
}

} // namespace

void PlantAlongBlade(const float *hit_pos, float scale) {
    if (scale <= 1.0f || NowColData == nullptr || MainWeapon.frame == nullptr) {
        return;
    }
    sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, grip;
    MainWeapon.frame->GetWorldPosition(grip, origin);
    const int       points = static_cast<int>(std::ceil(scale)) - 1;
    CCollisionData &hits = *NowColData;
    for (int i = 0; i < kHitRecords; i++) {
        const COLLISION_HIT &swing = hits.hit[i];
        if (hits.active[i] == 0 || swing.owner != kToan || swing.pos[0] != hit_pos[0] || swing.pos[1] != hit_pos[1] ||
            swing.pos[2] != hit_pos[2]) {
            continue;
        }
        for (int k = 1; k <= points; k++) {
            int copy = FreeHit(hits);
            if (copy < 0) {
                return;
            }
            hits.hit[copy] = swing;
            hits.active[copy] = hits.active[i];
            for (int axis = 0; axis < 3; axis++) {
                hits.hit[copy].pos[axis] = grip[axis] + (hit_pos[axis] - grip[axis]) * static_cast<float>(k) / scale;
            }
            hits.hit[copy].pos[3] = 1.0f;
        }
    }
}

void ScaleSwordInHand(float scale) {
    if (MainWeapon.frame != nullptr) {
        MainWeapon.SetScale(scale, scale, scale);
    }
}

} // namespace expanded
