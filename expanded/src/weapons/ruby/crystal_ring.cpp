#include "expanded/weapons/ruby/crystal_ring.hpp"

#include <cmath>
#include <optional>
#include <string>

#include "battlemenu.hpp"
#include "bt_shot_effect.hpp"
#include "collisiondata.hpp"
#include "dataread.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "dungeoneventdata.hpp"
#include "dungeoneventman.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/core/party.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "main.hpp"
#include "mathutil.hpp"
#include "menu_misc.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

namespace dc::game {
extern s32 mode;
} // namespace dc::game

namespace expanded {

namespace {

constexpr int   kHeldElement = WEAPON_ELEMENT_COLD;
constexpr int   kElementBits = HIT_ELEMENT_FIRE | HIT_ELEMENT_COLD | HIT_ELEMENT_THUNDER | HIT_ELEMENT_WIND | HIT_ELEMENT_HOLY;
constexpr int   kFlying = 1;               // a shot's phase in flight
constexpr float kMinFlight = 0.01f;        // the squared speed below which an orb is not flying
constexpr int   kZeroPointsMessage = 0xA1; // the weapon menu's warning 0: an element with no points

// The ice arrow turned on the monsters: its element alone on its hit, and nothing planted in flight (a crystal's
// planting sets it).
BT_SHOT_EFFECT ArrowShot() {
    BT_SHOT_EFFECT shot = IceArrowShot();
    shot.target = kHitsMonsters;
    shot.hit_flags &= kElementBits;
    shot.radius[kFlying] = 0.0f;
    return shot;
}

BT_SHOT_EFFECT arrow_shot = ArrowShot();

// An item lifted the curse: until the floor ends or the ring leaves her hand.
bool cured = false;

// 1 while every arrow in flight is on a crystal (planting all the way), 0 otherwise, -1 not yet set.
int planting = -1;

// The last orb she let go flew on as an arrow, so a full charge's second orb, drawn over it, goes with it.
bool orb_became_arrow = false;

bool Cursing(const CUserStatus &status, int chara) {
    const WEAPON_HAVE *weapon = EquippedWeapon(status, chara);
    return chara == CHARA_RUBY && weapon != nullptr && CarriesWeapon(*weapon, ITEM_WEAPON_CRYSTAL_RING);
}

void Lifted(CUserStatus &, int) {
    cured = true;
}

constexpr CurseRule kCursedPrism = {.cursing = Cursing, .lifted = Lifted, .immune = false};

// Whether the rings are held to Ice: unless the curse was lifted on this floor.
bool Held() {
    return WeaponAbilitiesOn() && !(cured && mode == GAME_MODE_DUNGEON);
}

// The weapon the weapon menu has selected, or nullptr.
const WEAPON_HAVE *MenuSelected() {
    if (BtlMenuStatusPt == nullptr || WepMenu.chara < 0 || WepMenu.chara >= kPartyMembers || WepMenu.weapon_slot < 0 ||
        WepMenu.weapon_slot >= kWeaponSlots) {
        return nullptr;
    }
    return &BtlMenuStatusPt->chara_weapons[WepMenu.chara][WepMenu.weapon_slot];
}

// The 0-point warning reads as the curse's while the ring it holds is the menu's selection.
std::optional<std::string> CursedWarning() {
    const WEAPON_HAVE *weapon = MenuSelected();
    if (weapon == nullptr || HeldElement(*weapon) < 0) {
        return std::nullopt;
    }
    return LocalizeText("expanded.crystal_ring.element_cursed", "Element can't be\nselected due to\ncurse.");
}

[[maybe_unused]] const bool kRegistered =
    (AddCurseRule(&kCursedPrism), SetMenuMessage(kZeroPointsMessage, CursedWarning),
     RegisterWeaponShotEffect(ITEM_WEAPON_CRYSTAL_RING, WeaponShotForm::Weapon, &arrow_shot, kPrismArrows, kIceArrowFile), true);

// Where an arrow aims at a monster: its lock-on point, else above its feet.
void MonsterAim(CMonstorUnit &unit, int monster, float *aim) {
    MONSTOR &live = unit.monster[monster];
    if (live.lockon_frame != nullptr) {
        sceVu0CopyVector(aim, live.lockon_position);
        return;
    }
    unit.chara[monster][0].GetPosition(aim);
    aim[1] += kPrismAimLift;
}

// Her door crystal nearest from across the ground within kPrismCrystalReach, still shut: false with none.
bool CrystalAim(const float *from, float *aim) {
    if (NowEventMan == nullptr) {
        return false;
    }
    float best = kPrismCrystalReach;
    bool  found = false;
    for (CDungeonEventData &event : NowEventMan->event) {
        if (event.CheckSwitch() == 0 || event.event->chara_no != CHARA_RUBY) {
            continue;
        }
        float distance = std::hypot(event.pos[0] - from[0], event.pos[2] - from[2]);
        if (distance <= best) {
            best = distance;
            sceVu0CopyVector(aim, event.pos);
            found = true;
        }
    }
    return found;
}

// Turns a flying arrow kPrismTurnDegrees toward aim, wherever aim lies, at its own speed: its heading swings toward the
// part of the way to aim that lies across it. Within that angle of aim, or with aim dead ahead or behind, the arrow
// flies on as it points.
void Turn(CSHOT_EFFECT &arrows, int slot, const float *aim) {
    float      *flight = arrows.velocity[slot];
    const float speed = std::sqrt(flight[0] * flight[0] + flight[1] * flight[1] + flight[2] * flight[2]);
    if (speed == 0.0f) {
        return;
    }
    const float angle = kPrismTurnDegrees * PI / 180.0f;
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);
    float       at[4];
    arrows.chara[slot].GetPosition(at);
    float heading[4] = {flight[0] / speed, flight[1] / speed, flight[2] / speed, 1.0f};
    float to[3] = {aim[0] - at[0], aim[1] - at[1], aim[2] - at[2]};
    float along = heading[0] * to[0] + heading[1] * to[1] + heading[2] * to[2];
    float across[3] = {to[0] - along * heading[0], to[1] - along * heading[1], to[2] - along * heading[2]};
    float gap = std::sqrt(across[0] * across[0] + across[1] * across[1] + across[2] * across[2]);
    if (gap == 0.0f || sine * along - cosine * gap >= 0.0f) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        heading[i] = heading[i] * cosine + across[i] / gap * sine;
        flight[i] = heading[i] * speed;
    }
    sceVu0FMATRIX facing;
    LookAtMatrixZ(facing, heading);
    arrows.chara[slot].frame->SetTransMatrix(facing);
    arrows.chara[slot].SetPosition(at);
}

// On a crystal the arrows plant all the way, with no wait between; otherwise the strike is the one hit.
void SetPlanting(CSHOT_EFFECT &arrows, int on_crystal) {
    if (on_crystal == planting) {
        return;
    }
    arrow_shot.radius[kFlying] = on_crystal != 0 ? kPrismCrystalRadius : 0.0f;
    for (int slot = 0; slot < arrows.slot_count; slot++) {
        if (arrows.active[slot] != 0) {
            arrows.wait[slot] = static_cast<char>(on_crystal != 0 ? 0 : kPrismStrikeWait);
        }
    }
    planting = on_crystal;
}

// Each flying arrow turned toward its target.
void Steer(CMonstorUnit &unit, CSHOT_EFFECT &arrows) {
    int  locked = LockedTarget();
    bool on_monster = false;
    bool on_crystal = false;
    for (int slot = 0; slot < arrows.slot_count; slot++) {
        if (arrows.active[slot] == 0 || arrows.phase[slot] != kFlying) {
            continue;
        }
        float at[4];
        arrows.chara[slot].GetPosition(at);
        float aim[4];
        int   monster = locked >= 0 ? locked : NearestMonster(unit, at, kPrismMonsterReach);
        if (monster >= 0) {
            on_monster = true;
            MonsterAim(unit, monster, aim);
        } else if (CrystalAim(at, aim)) {
            on_crystal = true;
        } else {
            continue;
        }
        Turn(arrows, slot, aim);
    }
    SetPlanting(arrows, on_crystal && !on_monster ? 1 : 0);
}

} // namespace

bool CursedPrismActive() {
    return Playing(CHARA_RUBY) && WeaponWielded(ITEM_WEAPON_CRYSTAL_RING) && CurseOn(CHARA_RUBY) == &kCursedPrism &&
           NowMainEffect == &CharaMainShotEffect();
}

int HeldElement(const WEAPON_HAVE &weapon) {
    return weapon.item_no == ITEM_WEAPON_CRYSTAL_RING && Held() ? kHeldElement : -1;
}

bool ElementRefused(const WEAPON_HAVE &weapon, int element) {
    int held = HeldElement(weapon);
    return held >= 0 && element != held;
}

CSHOT_EFFECT *PrismArrows() {
    return LoadedShotEffect(ShotEffectIndex(&arrow_shot));
}

void HoldCrystalRings(CUserStatus &status) {
    if (!Held()) {
        return;
    }
    for (WEAPON_HAVE &weapon : status.chara_weapons[CHARA_RUBY]) {
        if (weapon.item_no == ITEM_WEAPON_CRYSTAL_RING) {
            weapon.best_elem = kHeldElement;
        }
    }
    if (NowWeaponHave != nullptr && NowWeaponHave->item_no == ITEM_WEAPON_CRYSTAL_RING) {
        NowWeaponHave->best_elem = kHeldElement;
    }
}

void PrismOrbThrown(CSHOT_EFFECT &effect, int slot, float scale, bool repeat) {
    CSHOT_EFFECT *arrows = PrismArrows();
    if (arrows == nullptr || &effect != &CharaMainShotEffect() || slot < 0 || slot >= effect.slot_count || effect.active[slot] == 0 ||
        !CursedPrismActive()) {
        return;
    }
    // A full charge's second orb only shimmers over the first: the first's arrow is the only one.
    if (repeat) {
        if (orb_became_arrow) {
            effect.OffEffect(slot);
        }
        return;
    }
    orb_became_arrow = false;
    const float *flight = effect.velocity[slot];
    if (flight[0] * flight[0] + flight[1] * flight[1] + flight[2] * flight[2] < kMinFlight) {
        return;
    }
    float from[4];
    effect.chara[slot].GetPosition(from);
    float to[4] = {from[0] + flight[0], from[1] + flight[1], from[2] + flight[2], 1.0f};
    from[3] = 1.0f;
    arrows->Set(from, to, CHARA_RUBY, BtRubyDoorKey, -1, nullptr, kFlying);
    int fired = arrows->current_slot;
    if (fired < 0) { // with no room, the orb flies
        return;
    }
    if (scale > 1.0f) { // a charged throw's arrow, a little faster
        float *arrow = arrows->velocity[fired];
        float  speed = std::sqrt(arrow[0] * arrow[0] + arrow[1] * arrow[1] + arrow[2] * arrow[2]);
        for (int i = 0; i < 3; i++) {
            arrow[i] *= kPrismChargedSpeed / speed;
        }
    }
    arrows->SetDmg(effect.damage[slot]);
    arrows->SetWepStatus(effect.weapon_status[slot]);
    arrows->SetVsMonster(effect.vs_monster[slot]);
    arrows->SetWait(planting == 1 ? 0 : kPrismStrikeWait);
    LogEvent("ice_arrow", "\"damage\":" + std::to_string(effect.damage[slot]));
    effect.OffEffect(slot);
    orb_became_arrow = true;
}

void CursedPrismStep(CMonstorUnit &unit) {
    if (UserStatus != nullptr) {
        // Off her hand, a lifted curse is forgotten: the next wearing starts afresh.
        if (!Cursing(*UserStatus, CHARA_RUBY)) {
            cured = false;
        }
        HoldCrystalRings(*UserStatus);
    }
    CSHOT_EFFECT *arrows = PrismArrows();
    if (arrows == nullptr || !CursedPrismActive()) {
        planting = -1;
        return;
    }
    Steer(unit, *arrows);
}

void CursedPrismFloorEnds() {
    cured = false;
    planting = -1;
    arrow_shot.radius[kFlying] = 0.0f;
    if (UserStatus != nullptr) {
        HoldCrystalRings(*UserStatus);
    }
}

void CrystalRingMenuStep() {
    if (BtlMenuStatusPt != nullptr) {
        HoldCrystalRings(*BtlMenuStatusPt);
    }
}

void CrystalRingBuiltUp(WEAPON_HAVE &weapon) {
    if (HeldElement(weapon) < 0) {
        return;
    }
    weapon.best_elem = kHeldElement;
    // The game reads her shot as the menu opens, on an equip and leaving the element page: a build-up reads it too.
    if (BtlMenuMode == BATTLE_MENU_MODE_DUNGEON && BtlMenuStatusPt != nullptr && BtlMenuStatusPt->cur_chara == CHARA_RUBY &&
        WepMenu.chara == CHARA_RUBY) {
        StartReadBG();
        DngWepEffectReadStart();
    }
}

} // namespace expanded
