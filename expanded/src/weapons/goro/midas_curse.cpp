#include "expanded/weapons/goro/midas_curse.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "battlemenu.hpp"
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

constexpr int kMembers = 6;

bool gold[kMembers] = {};   // turned to gold, until the freeze is healed
int  flash[kMembers] = {};  // frames of the turning flash left
bool lifted[kMembers] = {}; // the curse lifted on this floor: off until the next
bool menu_gold = false;     // the item menu's model is being drawn gold

bool Member(int chara) {
    return chara >= 0 && chara < kMembers;
}

// Goro with Big Bucks Hammer, Xiao with its SynthSphere on Super Steve.
bool Cursing(const CUserStatus &status, int chara) {
    return (chara == CHARA_GORO || chara == CHARA_XIAO) && !gold[chara] && !lifted[chara] &&
           EquippedCarries(status, chara, ITEM_WEAPON_BIG_BUCKS_HAMMER);
}

void Lifted(CUserStatus &, int chara) {
    TurnGold(chara);
}

constexpr int kMonsterOwners = 200; // a monster's hit's owner: 200 + its slot times kOwnersPerMonster
constexpr int kOwnersPerMonster = 5;

constexpr CurseRule kMidasCurse = {.cursing = Cursing, .lifted = Lifted, .immune = false};

void OnFloor() {
    std::fill(std::begin(lifted), std::end(lifted), false);
}

[[maybe_unused]] const bool kRegistered = (AddCurseRule(&kMidasCurse), AddEnemyFloorListener(OnFloor), true);

int InPlay() {
    return UserStatus != nullptr ? UserStatus->cur_chara : -1;
}

// The one in play, gold.
bool GoldInPlay() {
    return MidasGold(InPlay());
}

} // namespace

bool MidasCursed(int chara) {
    return UserStatus != nullptr && Member(chara) && CurseOn(chara) == &kMidasCurse && (UserStatus->ailments[chara] & AILMENT_CURSE) != 0;
}

bool MidasGold(int chara) {
    return UserStatus != nullptr && Member(chara) && gold[chara] && (UserStatus->ailments[chara] & AILMENT_FREEZE) != 0;
}

void TurnGold(int chara) {
    if (UserStatus == nullptr || !Member(chara)) {
        return;
    }
    gold[chara] = true;
    lifted[chara] = true;
    // Frozen and nothing else; 0 frames left: the game's countdown never ends it.
    UserStatus->ailments[chara] = AILMENT_FREEZE;
    UserStatus->ailment_frames[chara] = 0;
    UserStatus->hp[chara] = std::max<s16>(UserStatus->hp[chara], 1);
    if (Playing(chara)) {
        BtActStatus.movement_locked = 1;
    }
    MidasTurnEffect(flash[chara]);
}

void MidasCurseStep() {
    if (UserStatus == nullptr) {
        return;
    }
    for (int chara = 0; chara < kMembers; chara++) {
        if (flash[chara] > 0) {
            flash[chara]--;
        }
        if (!gold[chara]) {
            continue;
        }
        if ((UserStatus->ailments[chara] & AILMENT_FREEZE) == 0) {
            gold[chara] = false; // healed
            continue;
        }
        UserStatus->ailment_frames[chara] = 0;
    }
}

int MidasPlayerDamage(int damage) {
    int chara = InPlay();
    if (!MidasCursed(chara) || damage < UserStatus->hp[chara]) {
        return damage;
    }
    damage = std::max(UserStatus->hp[chara] - 1, 0);
    TurnGold(chara);
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

void PlayerGoldDrawing() {
    if (GoldInPlay()) {
        MidasPolish(CharaMain.pos);
    }
}

void PlayerGoldDrawn() {
    if (GoldInPlay()) {
        MidasUnpolish();
    }
}

void MenuCharaDrawing(int chara) {
    menu_gold = MidasGold(chara) && mode == GAME_MODE_DUNGEON;
    if (menu_gold) {
        MidasPolishBall(); // shown away from the floor: the gold ball, which reads as gold anywhere
    }
}

void MenuCharaDrawn(int) {
    if (menu_gold) {
        MidasUnpolish();
    }
    menu_gold = false;
}

bool MidasGoldLight(const CCharacter &character, float *ambient) {
    int  chara = InPlay();
    bool player = &character == &CharaMain && Member(chara);
    if (player && flash[chara] > 0) {
        MidasFlashLight(flash[chara], ambient);
    }
    if ((player && MidasGold(chara)) || (menu_gold && &character == &MenuCharaFrame)) {
        for (int c = 0; c < 3; c++) {
            ambient[c] += kMidasShine[c];
        }
        return true;
    }
    return player && flash[chara] > 0;
}

void MidasCurseReset() {
    std::fill(std::begin(flash), std::end(flash), 0);
    std::fill(std::begin(gold), std::end(gold), false);
    std::fill(std::begin(lifted), std::end(lifted), false);
    menu_gold = false;
}

} // namespace expanded
