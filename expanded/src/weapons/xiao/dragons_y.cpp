#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/xiao/dragons_y.hpp"

#include <cmath>
#include <initializer_list>

#include "bt_shot_effect.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/shot_charge.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kXiao = 1;        // the shot's owner, as her pellets' hits are
constexpr int kAilments = 0x1F; // a hit's flags kept: its element, not the ball's ailment
// Xiao locked on: her stance's strafes, and her guarded steps.
constexpr int kLockedMotions[] = {8, 9, 10, 19, 20, 21, 22, 33};
// The Gemron balls by element (fire, ice, thunder, wind, holy), and the Black Dragon's for none.
constexpr int kElementShots[5] = {5, 20, 23, 24, 25};
constexpr int kNoElementShot = 22;

ShotCharge     charge{kDragonShotFrames};
BT_SHOT_EFFECT balls[kDragonShots];
bool           built = false;

void Build() {
    if (built) {
        return;
    }
    for (int i = 0; i < kDragonShots; i++) {
        int index = i < 5 ? kElementShots[i] : kNoElementShot;
        balls[i] = *BtEntryEffectTbl[index];
        balls[i].target = kHitsMonsters;
        balls[i].hit_flags &= kAilments;
    }
    built = true;
}

constexpr int kEffectSlots = 8;
u32           struck[kDragonShots][kEffectSlots] = {}; // the monsters each ball has hit, one bit each
int           planted[kHitRecords] = {};                  // the ball (its shot and slot) each marked hit is from

bool Ours(const BT_SHOT_EFFECT *shot) {
    return built && shot >= balls && shot < balls + kDragonShots;
}

} // namespace

bool LockOnSpeedActive() {
    if (UserStatus == nullptr || UserStatus->cur_chara != CHARA_XIAO) {
        return false;
    }
    for (int weapon : {ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_DIVINEBEASTTITLE, ITEM_WEAPON_ANGEL_SHOOTER, ITEM_WEAPON_ANGE_GEAR}) {
        if (WeaponActive(weapon)) {
            return true;
        }
    }
    return false;
}

bool DragonShotActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && WeaponActive(ITEM_WEAPON_DRAGON_S_Y);
}

const BT_SHOT_EFFECT *DragonShot(int element) {
    Build();
    return &balls[element >= 0 && element < 5 ? element : 5];
}

float LockOnMoveScale() {
    return LockOnSpeedActive() ? kLockOnSpeed : 1.0f;
}

void LockOnSpeedMotion(CCharacter &chara, int chara_no) {
    if (chara_no != CHARA_XIAO || lockOnTargetFlag == 0 || !LockOnSpeedActive() || chara.motion_speed >= 0.0f) {
        return;
    }
    for (int motion : kLockedMotions) {
        if (chara.motion_no == motion && chara.motion_type.motion_info != nullptr) {
            chara.motion_speed = chara.motion_type.motion_info[motion].speed * kLockOnSpeed;
            return;
        }
    }
}

void DragonShotStep() {
    bool active = DragonShotActive();
    charge.Step(active);
    // The selected element's ball is loaded ahead of the shot.
    if (active && NowWeaponHave != nullptr) {
        const BT_SHOT_EFFECT *ball = DragonShot(NowWeaponHave->best_elem);
        if (ShotEffectIndex(ball) < 0) {
            EnterShotEffect(const_cast<BT_SHOT_EFFECT *>(ball), kDragonShots);
        }
    }
}

void DragonShotFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots || !DragonShotActive() || !charge.TakeCharged() || NowWeaponHave == nullptr) {
        return;
    }
    BT_SHOT_EFFECT *ball = const_cast<BT_SHOT_EFFECT *>(DragonShot(NowWeaponHave->best_elem));
    int             index = EnterShotEffect(ball, kDragonShots);
    if (index < 0) {
        return;
    }
    CSHOT_EFFECT *effect = ShotEffectAt(index);
    float         from[4] = {shots.pos[slot][0], shots.pos[slot][1], shots.pos[slot][2], 1.0f};
    float         to[4] = {from[0] + shots.vector[slot][0], from[1] + shots.vector[slot][1], from[2] + shots.vector[slot][2], 1.0f};
    effect->Set(from, to, kXiao, 0, -1, nullptr, -1);
    if (effect->current_slot < 0) {
        return; // no room: the pellet flies
    }
    struck[ball - balls][effect->current_slot] = 0;
    effect->SetDmg(static_cast<int>(std::lround(static_cast<float>(shots.damage[slot]) * kDragonShotDamage)));
    effect->SetWepStatus(NowWeaponHave->flags);
    effect->SetVsMonster(NowWeaponHave->vs_monster);
    shots.used[slot] = 0;
}

void DragonShotHit(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit) {
    if (!Ours(effect.effect_data) || slot < 0 || slot >= kEffectSlots) {
        return;
    }
    // Marked, so a monster it has already hit refuses its later ones (it keeps planting them as it bursts).
    int record = static_cast<int>(&hit - NowColData->hit);
    if (record >= 0 && record < kHitRecords) {
        hit.weapon_flags |= kDragonShotMark;
        planted[record] = static_cast<int>(effect.effect_data - balls) * kEffectSlots + slot;
    }
    const float *v = effect.velocity[slot];
    float        length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length <= 0.0f) {
        return;
    }
    sceVu0FVECTOR at;
    const_cast<CCharacter &>(effect.chara[slot]).GetPosition(at);
    for (int i = 0; i < 3; i++) {
        hit.knockback_origin[i] = at[i] - v[i] / length * kDragonShotBehind;
    }
    hit.knockback_origin[3] = 1.0f;
    hit.knockback_speed = kDragonShotKickSpeed;
    hit.knockback_decay = kDragonShotKickDecay;
    hit.knockback_mode = kKnockbackFromOrigin;
}

bool DragonShotRefused(int monster, const COLLISION_HIT &hit) {
    int record = NowColData != nullptr ? static_cast<int>(&hit - NowColData->hit) : -1;
    if ((hit.weapon_flags & kDragonShotMark) == 0 || record < 0 || record >= kHitRecords || monster < 0 || monster >= 32) {
        return false;
    }
    u32 &hit_already = struck[planted[record] / kEffectSlots][planted[record] % kEffectSlots];
    u32  bit = 1u << monster;
    if (hit_already & bit) {
        return true;
    }
    hit_already |= bit;
    return false;
}

} // namespace expanded
