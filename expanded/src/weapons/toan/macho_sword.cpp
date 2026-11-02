#include "expanded/weapons/toan/macho_sword.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>
#include <map>
#include <numbers>

#include "bound.hpp"
#include "character.hpp"
#include "cloth.hpp"
#include "collisiondata.hpp"
#include "dngmessageman.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/party.hpp"
#include "expanded/data/toan_grip.hpp"
#include "expanded/data/wolf_steve.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/ownership.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "frame.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "mdt.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"
#include "visualvu1.hpp"

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

// The cloth's upper-arm bounds as their model gave them (across the arm: up and along their span).
std::map<CBound *, std::array<float, 2>> arm_bounds;

void SetAcross(CBound &bound, float up, float span) {
    bound.extent[1] = up;
    bound.extent[2] = span;
    bound.reciprocal[1] = 1.0f / up;
    bound.reciprocal[2] = 1.0f / span;
}

// The member's cloth: its upper-arm bounds grown across the arm by grow (0: as the model gave them).
void FitCloth(const PumpedUpRig &rig, float grow) {
    if (CharaMain.cloth == nullptr) {
        return;
    }
    for (int i = 0; i < CHARA_CLOTH_MAX; i++) {
        CCloth *piece = CharaMain.cloth[i];
        for (CBound *bound = piece != nullptr ? piece->bound : nullptr; bound != nullptr; bound = bound->next) {
            if (bound->from_frame == nullptr ||
                std::none_of(std::begin(rig.bones[kPumpedUpper]), std::end(rig.bones[kPumpedUpper]),
                             [&](const char *name) { return std::strcmp(bound->from_frame->name, name) == 0; })) {
                continue;
            }
            auto [it, fresh] = arm_bounds.try_emplace(bound, std::array<float, 2>{bound->extent[1], bound->extent[2]});
            SetAcross(*bound, it->second[0] + grow, it->second[1] + grow);
        }
    }
}

// The hands' frames as their model gave them, before any swelling (by frame).
std::map<CFrame *, sceVu0FMATRIX> hand_locals;

bool Carries(int chara) {
    const WEAPON_HAVE *weapon = UserStatus != nullptr ? EquippedWeapon(*UserStatus, chara) : nullptr;
    return weapon != nullptr && WeaponAbilitiesOn() && CarriesWeapon(*weapon, ITEM_WEAPON_MACHO_SWORD);
}

// Toan's model, out with the Macho Sword in hand and pumped up.
// The active member's rig when its model is out and pumped up, else null.
const PumpedUpRig *Pumped(const CFrame *frame) {
    if (frame == nullptr || frame != CharaMain.frame || UserStatus == nullptr || PumpedUpKills(UserStatus->cur_chara) <= 0) {
        return nullptr;
    }
    return PumpedUpRigOf(UserStatus->cur_chara);
}

float LookShare() {
    int kills = UserStatus != nullptr ? PumpedUpKills(UserStatus->cur_chara) : 0;
    return static_cast<float>(std::min(kills, kPumpedUpFullLook)) / kPumpedUpFullLook;
}

// The part a bone or frame of Toan's model draws, or -1.
int BonePart(const PumpedUpRig &rig, const char *name) {
    for (int part = 0; part < kPumpedParts - 1; part++) {
        for (const char *bone : rig.bones[part]) {
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

// m turned angle radians about its own axis (row-vector: the turn first, then m).
void TurnAbout(sceVu0FMATRIX out, const sceVu0FMATRIX m, int axis, float angle) {
    float turn[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    int i = (axis + 1) % 3, j = (axis + 2) % 3;
    turn[i][i] = std::cos(angle);
    turn[i][j] = std::sin(angle);
    turn[j][i] = -std::sin(angle);
    turn[j][j] = std::cos(angle);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 4; c++) {
            out[r][c] = 0.0f;
            for (int k = 0; k < 3; k++) {
                out[r][c] += turn[r][k] * m[k][c];
            }
        }
    }
    for (int c = 0; c < 4; c++) {
        out[3][c] = m[3][c];
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

static_assert(kPumpedUpRigs[0].chara == CHARA_TOAN);

const PumpedUpRig *PumpedUpRigOf(int chara) {
    for (const PumpedUpRig &rig : kPumpedUpRigs) {
        if (rig.chara == chara) {
            return &rig;
        }
    }
    return nullptr;
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
    if (UserStatus == nullptr || PumpedUpRigOf(UserStatus->cur_chara) == nullptr) {
        return 1.0f;
    }
    return 1.0f + (kPumpedUpLook[part] - 1.0f) * LookShare();
}

float SteveArmsGrowth() {
    return static_cast<float>(std::min(PumpedUpKills(CHARA_XIAO), kPumpedUpFullLook)) / kPumpedUpFullLook;
}

void GrowSteveArms(CFrame *steve) {
    CFrame     *arms = steve != nullptr ? steve->SearchFrame(const_cast<char *>(kMachoArms)) : nullptr;
    CVisualVu1 *visual = arms != nullptr ? static_cast<CFrameVu1 *>(arms)->GetVisual() : nullptr;
    u_int      *data = visual != nullptr ? visual->GetMDTDataAddress() : nullptr;
    if (data == nullptr) {
        return;
    }
    // The drawn vertices, then the thin look's and the buff's.
    auto *mdt = reinterpret_cast<MDT_HEADER *>(data);
    auto *vertices = reinterpret_cast<sceVu0FVECTOR *>(reinterpret_cast<u_char *>(mdt) + mdt->vertex_ofs);
    int   count = static_cast<int>(mdt->vertex_num) / kMachoArmsSets;
    float growth = SteveArmsGrowth();
    bool  moved = false;
    for (int i = 0; i < count; i++) {
        for (int c = 0; c < 3; c++) {
            float thin = vertices[count + i][c], buff = vertices[2 * count + i][c];
            float at = thin + (buff - thin) * growth;
            moved = moved || vertices[i][c] != at;
            vertices[i][c] = at;
        }
    }
    if (moved) {
        arms->attr.remake_pending = 1;
    }
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
    arm_bounds.clear();
}

void PumpedUpFloorEntered() {
    for (Pump &pump : pumps) {
        pump = {};
    }
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
    GrowSteveArms(NowWeapon != nullptr ? NowWeapon->frame : nullptr);
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
    const PumpedUpRig *rig = Pumped(frame);
    if (rig == nullptr) {
        // Back to their own size and pose, once.
        for (auto &[part, local] : hand_locals) {
            part->SetTransMatrix(local);
        }
        hand_locals.clear();
        for (auto &[bound, across] : arm_bounds) {
            SetAcross(*bound, across[0], across[1]);
        }
        arm_bounds.clear();
        return;
    }
    FitCloth(*rig, kPumpedUpArmRadius * (PumpedUpSize(kPumpedUpper) - 1.0f));
    // A frame's own local, as its model gave it.
    auto rest = [](CFrame *part) -> sceVu0FVECTOR * {
        auto [it, fresh] = hand_locals.try_emplace(part);
        if (fresh) {
            sceVu0CopyMatrix(it->second, part->local);
        }
        return it->second;
    };
    // A hand about the point kPumpedUpGripPivot of the way from its wrist (its parent's origin) to the weapon's frame
    // (in its wrist's frame), or that point mirrored across (its second axis turned round), in the hand's own frame.
    float   size = PumpedUpSize(kPumpedHand);
    CFrame *hilt = frame->SearchFrame(const_cast<char *>(kPumpedUpHilt));
    auto    grow = [&](const char *name, bool mirrored) {
        CFrame *hand = name != nullptr ? frame->SearchFrame(const_cast<char *>(name)) : nullptr;
        if (hand == nullptr) {
            return;
        }
        sceVu0FVECTOR *local = rest(hand);
        sceVu0FMATRIX  inverse, scaled;
        sceVu0InversMatrix(inverse, local);
        float at[3] = {0.0f, 0.0f, 0.0f};
        if (hilt != nullptr) {
            at[0] = hilt->local[3][0] * kPumpedUpGripPivot;
            at[1] = hilt->local[3][1] * kPumpedUpGripPivot * (mirrored ? -1.0f : 1.0f);
            at[2] = hilt->local[3][2] * kPumpedUpGripPivot;
        }
        float pivot[3];
        for (int c = 0; c < 3; c++) {
            pivot[c] = inverse[3][c] + at[0] * inverse[0][c] + at[1] * inverse[1][c] + at[2] * inverse[2][c];
        }
        ScaleAbout(scaled, local, pivot, size);
        hand->SetTransMatrix(scaled);
    };
    if (!rig->weapon_hand_rigged) {
        grow(rig->weapon_hand, false); // a rigged hand grows in its skin (PumpedUpSkinStart)
    }
    for (const char *name : rig->other_hand) {
        grow(name, true);
    }
    if (!rig->weapon_hand_rigged) {
        return;
    }
    // The grip closing.
    float share = LookShare();
    for (int d = 0; d < kGripDigits; d++) {
        for (int k = 0; k < kGripJointsPerDigit; k++) {
            CFrame *joint = frame->SearchFrame(const_cast<char *>(kGripJoints[d][k]));
            if (joint == nullptr) {
                continue;
            }
            bool           thumb = d == kGripThumb;
            float          degrees = thumb ? kPumpedUpThumbCurl[k] : kPumpedUpFingerCurl[k];
            int            axis = thumb ? 0 : 2;
            sceVu0FVECTOR *local = rest(joint);
            sceVu0FMATRIX  turned;
            TurnAbout(turned, local, axis, degrees * share * std::numbers::pi_v<float> / 180.0f);
            joint->SetTransMatrix(turned);
        }
    }
}

void PumpedUpSkinStart(CFrame *frame, int mesh, float (*matrix)[4]) {
    const PumpedUpRig *rig = Pumped(frame);
    if (rig == nullptr || !rig->weapon_hand_rigged) {
        return;
    }
    CFrame *hand = &static_cast<CFrameVu1 *>(frame)[mesh];
    if (std::strcmp(hand->name, rig->weapon_hand) != 0) {
        return;
    }
    CFrame *hilt = frame->SearchFrame(const_cast<char *>(kPumpedUpHilt));
    if (hilt == nullptr || hilt->parent != hand->parent) {
        return;
    }
    // In the parent's (the wrist's) frame, where the hilt sits at its own translation; the wrist is its origin.
    sceVu0FMATRIX unit;
    sceVu0UnitMatrix(unit);
    float pivot[3];
    for (int c = 0; c < 3; c++) {
        pivot[c] = hilt->local[3][c] * kPumpedUpGripPivot;
    }
    ScaleAbout(matrix, unit, pivot, PumpedUpSize(kPumpedHand));
}

void PumpedUpSkinBone(CFrame *frame, int bone, float (*matrix)[4]) {
    const PumpedUpRig *rig = Pumped(frame);
    if (rig == nullptr) {
        return;
    }
    auto *frames = static_cast<CFrameVu1 *>(frame);
    int   part = BonePart(*rig, frames[bone].name);
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
