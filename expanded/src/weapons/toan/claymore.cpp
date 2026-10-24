#include "expanded/weapons/toan/claymore.hpp"

#include <libvu0.h>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kToan = 0;

// Whether the sword in hand was grown, so it is put back at its own size once.
bool grown = false;

int FreeHit(const CCollisionData &hits) {
    for (int i = 0; i < 96; i++) {
        if (hits.active[i] == 0) {
            return i;
        }
    }
    return -1;
}

} // namespace

bool GreatswordActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_TOAN && NowWeapon == &MainWeapon &&
           WeaponWielded(ITEM_WEAPON_CLAYMORE);
}

void GreatswordStep() {
    bool grow = GreatswordActive() && MainWeapon.frame != nullptr;
    if (grow) {
        MainWeapon.SetScale(kGreatswordScale, kGreatswordScale, kGreatswordScale);
        grown = true;
    } else if (grown) {
        if (MainWeapon.frame != nullptr) {
            MainWeapon.SetScale(1.0f, 1.0f, 1.0f);
        }
        grown = false;
    }
}

void GreatswordHits(const float *hit_pos) {
    if (!GreatswordActive() || NowColData == nullptr || MainWeapon.frame == nullptr) {
        return;
    }
    // The stock hit point: as far from the grip as the stock blade's.
    sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, grip, stock;
    MainWeapon.frame->GetWorldPosition(grip, origin);
    for (int axis = 0; axis < 3; axis++) {
        stock[axis] = grip[axis] + (hit_pos[axis] - grip[axis]) / kGreatswordScale;
    }
    stock[3] = 1.0f;
    CCollisionData &hits = *NowColData;
    for (int i = 0; i < 96; i++) {
        const COLLISION_HIT &swing = hits.hit[i];
        if (hits.active[i] == 0 || swing.owner != kToan || swing.pos[0] != hit_pos[0] || swing.pos[1] != hit_pos[1] ||
            swing.pos[2] != hit_pos[2]) {
            continue;
        }
        int copy = FreeHit(hits);
        if (copy < 0) {
            return;
        }
        hits.hit[copy] = swing;
        hits.active[copy] = hits.active[i];
        sceVu0CopyVector(hits.hit[copy].pos, stock);
    }
}

} // namespace expanded
