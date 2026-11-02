#include "expanded/weapons/toan/macho_sword.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>
#include <map>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dngmessageman.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/party.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/ownership.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "frame.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// The ABS MAX line Step shows as a kill fills a weapon.
constexpr int kAbsMaxMessage = 150;
constexpr int kAbsMaxFrames = 480;

void ShowAbsMax(const WEAPON_HAVE &weapon) {
    DngMessMan.insert_mes_1 = GetCommonItemDataSystemMsg(weapon.item_no);
    DngMessMan.insert_value_1 = weapon.level;
    DngMessMan.message = kAbsMaxMessage;
    DngMessMan.timer = kAbsMaxFrames;
    DngMessMan.steev_window = false;
}

// Pumped Up: each member's count, with the equipped slot it was counted with.
struct Pump {
    int kills = 0;
    int slot = -1;
};

Pump pumps[kPartyMembers];

// The hands' frames as their model gave them, before any swelling (by frame).
std::map<CFrame *, sceVu0FMATRIX> hand_locals;

bool Carries(int chara) {
    const WEAPON_HAVE *weapon = UserStatus != nullptr ? EquippedWeapon(*UserStatus, chara) : nullptr;
    return weapon != nullptr && WeaponAbilitiesOn() && CarriesWeapon(*weapon, ITEM_WEAPON_MACHO_SWORD);
}

// Toan's model, out with the Macho Sword in hand and pumped up.
bool ToanPumped(const CFrame *frame) {
    return frame != nullptr && frame == CharaMain.frame && UserStatus != nullptr && UserStatus->cur_chara == CHARA_TOAN &&
           PumpedUpKills(CHARA_TOAN) > 0;
}

float LookShare() {
    return static_cast<float>(std::min(PumpedUpKills(CHARA_TOAN), kPumpedUpFullLook)) / kPumpedUpFullLook;
}

// The part a bone or frame of Toan's model draws, or -1.
int BonePart(const char *name) {
    for (int part = 0; part < kPumpedParts - 1; part++) {
        for (const char *bone : kPumpedUpBones[part]) {
            if (std::strcmp(name, bone) == 0) {
                return part;
            }
        }
    }
    return -1;
}

// m scaled by size about pivot (v -> pivot + (v - pivot) * size, then m).
void ScaleAbout(sceVu0FMATRIX out, const sceVu0FMATRIX m, const float *pivot, float size) {
    for (int c = 0; c < 4; c++) {
        out[c][3] = m[c][3];
    }
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 3; c++) {
            out[r][c] = m[r][c] * (r < 3 ? size : 1.0f);
        }
    }
    // The translation: m applied to pivot * (1 - size), added.
    for (int c = 0; c < 3; c++) {
        float moved = 0.0f;
        for (int r = 0; r < 3; r++) {
            moved += m[r][c] * pivot[r] * (1.0f - size);
        }
        out[3][c] = m[3][c] + moved;
    }
}

} // namespace

bool OvertrainingAbs(WEAPON_HAVE &weapon, int abs, int max_abs) {
    if (!WeaponOwned(ITEM_WEAPON_MACHO_SWORD)) {
        return false;
    }
    int limit = max_abs * kOvertrainingLimit;
    int updated = std::min(weapon.experience + abs, limit);
    if (weapon.experience < max_abs && updated >= max_abs) {
        ShowAbsMax(weapon);
    }
    weapon.experience = static_cast<s16>(std::max<int>(weapon.experience, updated));
    return true;
}

int OvertrainingCarry(const WEAPON_HAVE &weapon, int max_abs) {
    return std::max(0, weapon.experience - max_abs);
}

int OvertrainingGaugeAbs(int abs, int max_abs) {
    return std::min(abs, max_abs);
}

int PumpedUpKills(int chara) {
    if (chara < 0 || chara >= kPartyMembers || UserStatus == nullptr || !Carries(chara) ||
        UserStatus->equipped_weapon_slot[chara] != pumps[chara].slot) {
        return 0;
    }
    return pumps[chara].kills;
}

float PumpedUpMultiplier() {
    if (UserStatus == nullptr) {
        return 1.0f;
    }
    return std::pow(kPumpedUpRate, static_cast<float>(PumpedUpKills(UserStatus->cur_chara)));
}

float PumpedUpSize(PumpedUpPart part) {
    return 1.0f + (kPumpedUpLook[part] - 1.0f) * LookShare();
}

void PumpedUpKill(const CMonstorUnit &, int monster) {
    if (UserStatus == nullptr) {
        return;
    }
    int chara = UserStatus->cur_chara;
    if (chara < 0 || chara >= kPartyMembers || !Carries(chara)) {
        return;
    }
    Pump &pump = pumps[chara];
    if (pump.slot != UserStatus->equipped_weapon_slot[chara]) {
        pump = {0, UserStatus->equipped_weapon_slot[chara]};
    }
    pump.kills++;
    LogEvent("pumped_up", std::format("\"monster\":{},\"kills\":{},\"attack\":{:.3f}", monster, pump.kills, PumpedUpMultiplier()));
}

void PumpedUpReset() {
    for (Pump &pump : pumps) {
        pump = {};
    }
    hand_locals.clear();
}

void PumpedUpFrame() {
    if (UserStatus == nullptr) {
        return;
    }
    for (int chara = 0; chara < kPartyMembers; chara++) {
        if (pumps[chara].kills > 0 && (!Carries(chara) || UserStatus->equipped_weapon_slot[chara] != pumps[chara].slot)) {
            pumps[chara] = {}; // the weapon changed: the count goes with it
        }
    }
}

int PumpedUpAttack(int attack) {
    float multiplier = PumpedUpMultiplier();
    if (multiplier == 1.0f) {
        return attack;
    }
    return static_cast<int>(std::lround(static_cast<double>(attack) * multiplier));
}

void PumpedUpPelletHit(COLLISION_HIT &hit) {
    float multiplier = PumpedUpMultiplier();
    if (multiplier != 1.0f) {
        hit.damage = static_cast<s32>(std::lround(static_cast<double>(hit.damage) * multiplier));
    }
}

void PumpedUpPosed(CFrame *frame) {
    if (frame == nullptr || frame != CharaMain.frame) {
        return;
    }
    if (!ToanPumped(frame)) {
        // Back to their own size, once.
        for (auto &[hand, local] : hand_locals) {
            hand->SetTransMatrix(local);
        }
        hand_locals.clear();
        return;
    }
    float size = PumpedUpSize(kPumpedHand);
    for (const char *name : kPumpedUpHands) {
        CFrame *hand = frame->SearchFrame(const_cast<char *>(name));
        if (hand == nullptr) {
            continue;
        }
        auto [it, fresh] = hand_locals.try_emplace(hand);
        if (fresh) {
            sceVu0CopyMatrix(it->second, hand->local);
        }
        // About the wrist: the parent's origin, in the hand's own frame.
        sceVu0FMATRIX inverse, scaled;
        sceVu0InversMatrix(inverse, it->second);
        ScaleAbout(scaled, it->second, inverse[3], size);
        hand->SetTransMatrix(scaled);
    }
}

void PumpedUpSkinBone(CFrame *frame, int bone, float (*matrix)[4]) {
    if (!ToanPumped(frame)) {
        return;
    }
    auto *frames = static_cast<CFrameVu1 *>(frame);
    int   part = BonePart(frames[bone].name);
    if (part < 0 || frames[bone].child == nullptr) {
        return;
    }
    // Across the bone: every axis of its own but the one its child joint lies along.
    const float *along = frames[bone].child->local[3];
    int          axis = 0;
    for (int a = 1; a < 3; a++) {
        if (std::fabs(along[a]) > std::fabs(along[axis])) {
            axis = a;
        }
    }
    float size = PumpedUpSize(static_cast<PumpedUpPart>(part));
    for (int a = 0; a < 3; a++) {
        if (a == axis) {
            continue;
        }
        for (int c = 0; c < 3; c++) {
            matrix[a][c] *= size;
        }
    }
}

} // namespace expanded
