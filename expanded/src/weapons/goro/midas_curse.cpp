#include "expanded/weapons/goro/midas_curse.hpp"

#include <algorithm>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/goro/midas_touch.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "main.hpp"
#include "monstorunit.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace dc::game {
extern s32 mode;
} // namespace dc::game

namespace expanded {

namespace {

bool gold = false;   // Goro turned to gold, until his freeze is healed
int  flash = 0;      // frames of his turning flash left
bool lifted = false; // the curse lifted on this floor: off until the next

bool Cursing(const CUserStatus &status, int chara) {
    return chara == CHARA_GORO && !gold && !lifted && EquippedCarries(status, chara, ITEM_WEAPON_BIG_BUCKS_HAMMER);
}

void Lifted(CUserStatus &, int) {
    TurnGoroGold();
}

constexpr int kMonsterOwners = 200; // a monster's hit's owner: 200 + its slot times kOwnersPerMonster
constexpr int kOwnersPerMonster = 5;

constexpr CurseRule kMidasCurse = {.cursing = Cursing, .lifted = Lifted, .immune = false};

void OnFloor() {
    lifted = false;
}

[[maybe_unused]] const bool kRegistered = (AddCurseRule(&kMidasCurse), AddEnemyFloorListener(OnFloor), true);

// Gold Goro as the one in play.
bool GoldInPlay() {
    return Playing(CHARA_GORO) && GoroGold();
}

} // namespace

bool MidasCursed() {
    return UserStatus != nullptr && CurseOn(CHARA_GORO) == &kMidasCurse && (UserStatus->ailments[CHARA_GORO] & AILMENT_CURSE) != 0;
}

bool GoroGold() {
    return gold && UserStatus != nullptr && (UserStatus->ailments[CHARA_GORO] & AILMENT_FREEZE) != 0;
}

void TurnGoroGold() {
    if (UserStatus == nullptr) {
        return;
    }
    gold = true;
    lifted = true;
    // Frozen and nothing else; 0 frames left: the game's countdown never ends it.
    UserStatus->ailments[CHARA_GORO] = AILMENT_FREEZE;
    UserStatus->ailment_frames[CHARA_GORO] = 0;
    UserStatus->hp[CHARA_GORO] = std::max<s16>(UserStatus->hp[CHARA_GORO], 1);
    if (Playing(CHARA_GORO)) {
        BtActStatus.movement_locked = 1;
    }
    MidasTurnEffect(flash);
}

void MidasCurseStep() {
    if (flash > 0) {
        flash--;
    }
    if (!gold || UserStatus == nullptr) {
        return;
    }
    if ((UserStatus->ailments[CHARA_GORO] & AILMENT_FREEZE) == 0) {
        gold = false; // healed
        return;
    }
    UserStatus->ailment_frames[CHARA_GORO] = 0;
}

int MidasPlayerDamage(int damage) {
    if (!Playing(CHARA_GORO) || !MidasCursed() || damage < UserStatus->hp[CHARA_GORO]) {
        return damage;
    }
    damage = std::max(UserStatus->hp[CHARA_GORO] - 1, 0);
    TurnGoroGold();
    return damage;
}

void MidasPlayerHit(COLLISION_HIT &hit) {
    if (GoldInPlay() && hit.kind == kGuardBreakingHit) {
        hit.kind = kKnockbackHit;
    }
}

bool MidasShrugs() {
    return GoldInPlay();
}

void MidasPlayerReacted(const COLLISION_HIT &hit) {
    if (!GoldInPlay()) {
        return;
    }
    velo__2[0] = 0.0f; // the guard's push-back: none, gold does not move
    velo__2[2] = 0.0f;
    int monster = hit.owner >= kMonsterOwners ? (hit.owner - kMonsterOwners) / kOwnersPerMonster : -1;
    if (NowMonstorUnit == nullptr || monster < 0 || monster >= 16 || NowMonstorUnit->monster[monster].state != 2) {
        return;
    }
    MONSTOR &struck = NowMonstorUnit->monster[monster];
    float    at[4];
    NowMonstorUnit->chara[monster][0].GetPosition(at);
    float away[2] = {at[0] - CharaMain.pos[0], at[2] - CharaMain.pos[2]};
    float apart = std::hypot(away[0], away[1]);
    if (apart <= 0.0f || apart > kGoldRepelReach) {
        return; // a shot from afar: its monster stays
    }
    struck.knockback_direction[0] = away[0] / apart;
    struck.knockback_direction[1] = 0.0f;
    struck.knockback_direction[2] = away[1] / apart;
    struck.knockback_direction[3] = 0.0f;
    struck.knockback[0] = kGoldRepelSpeed * struck.knockback[2];
    struck.knockback[1] = kGoldRepelDecay * struck.knockback[2];
}

bool MidasAllowsAilment() {
    return !GoldInPlay();
}

void GoroDrawing() {
    if (GoldInPlay()) {
        MidasPolish(CharaMain.pos);
    }
}

void GoroDrawn() {
    if (GoldInPlay()) {
        MidasUnpolish();
    }
}

void MenuCharaDrawing(int chara) {
    if (chara == CHARA_GORO && GoroGold() && mode == GAME_MODE_DUNGEON) {
        MidasPolishBall(); // shown away from the floor: the gold ball, which reads as gold anywhere
    }
}

void MenuCharaDrawn(int chara) {
    if (chara == CHARA_GORO && GoroGold() && mode == GAME_MODE_DUNGEON) {
        MidasUnpolish();
    }
}

void GoroGoldLight(const CCharacter &character, float *ambient) {
    if (flash > 0 && Playing(CHARA_GORO) && &character == &CharaMain) {
        MidasFlashLight(flash, ambient);
    }
}

void MidasCurseReset() {
    flash = 0;
    gold = false;
    lifted = false;
}

} // namespace expanded
