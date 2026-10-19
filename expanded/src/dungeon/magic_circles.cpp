#include "expanded/dungeon/magic_circles.hpp"

#include <algorithm>
#include <cstdlib>

#include "battlemenu.hpp"
#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/ownership.hpp"
#include "itemdata.hpp"
#include "menu_dungeon.hpp"
#include "menu_inventory.hpp"
#include "menu_misc.hpp"
#include "monstorunit.hpp"
#include "shot_freefuncs.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// Retail's figures.
constexpr int   kStaminaFrames = 1800;
constexpr int   kRageFrames = 300;
constexpr float kMoneyUp = 1.2f, kMoneyDown = 0.2f;
constexpr int   kMoneyUpAdd = 10, kMoneyMax = 0xFFFF;
constexpr int   kGoodSound = 0xE1, kBadSound = 0xE2;

int Roll(int base, int range, int magnitude) {
    return (base + std::rand() % range) * magnitude;
}

// LocalWeaponDataChange's loss on one of count values, its roll times the magnitude.
void LoseOne(char *values, int count, int base, int range, int magnitude) {
    char &value = values[std::rand() % count];
    value = static_cast<char>(std::max(0, value - Roll(base, range, magnitude)));
}

void StatDown(WEAPON_HAVE &weapon, int m) {
    if (IsDefaultWeapon(weapon.item_no) >= 0) {
        return;
    }
    int stat = std::rand() % 4;
    int loss = Roll(2, 3, m);
    switch (stat) {
        case 0:
            weapon.attack = static_cast<s16>(std::max(1, weapon.attack - loss));
            break;
        case 1:
            weapon.endurance = static_cast<s16>(std::max(0, weapon.endurance - loss));
            break;
        case 2:
            weapon.speed = static_cast<s16>(std::max(0, weapon.speed - loss));
            break;
        case 3:
            weapon.magic = static_cast<s16>(std::max(0, weapon.magic - loss));
            break;
    }
    LoseOne(weapon.elem, 5, 2, 3, m);
    LoseOne(weapon.vs_monster, 3, 2, 2, m);
    LoseOne(&weapon.vs_monster[3], 3, 2, 2, m);
    LoseOne(&weapon.vs_monster[6], 4, 2, 2, m);
}

void WhpUp(WEAPON_HAVE &weapon, int m) {
    if (IsDefaultWeapon(weapon.item_no) < 0) {
        weapon.durability = static_cast<s16>(std::min(99, weapon.durability + Roll(3, 3, m)));
    }
}

void WhpDown(WEAPON_HAVE &weapon, int m) {
    if (IsDefaultWeapon(weapon.item_no) >= 0) {
        return;
    }
    weapon.durability = static_cast<s16>(weapon.durability - Roll(3, 3, m));
    weapon.durability_f = std::min(weapon.durability_f, static_cast<float>(weapon.durability));
    if (weapon.durability <= 0) {
        weapon.durability = 1;
    }
}

constexpr int kMembers = 6;
constexpr int kCarried = 11;

// A weapon a circle can fill: one in the slot, not a default weapon (retail fills those with nothing).
bool Fillable(const WEAPON_HAVE &weapon) {
    return weapon.item_no >= ITEM_WEAPON_START && IsDefaultWeapon(weapon.item_no) < 0;
}

// The ABS full, never below what a weapon already holds: an overtrained one keeps its surplus.
void FillAbs(WEAPON_HAVE &weapon) {
    if (Fillable(weapon)) {
        weapon.experience = std::max(weapon.experience, static_cast<s16>(GetWeaponMaxExp(&weapon)));
    }
}

void CureWhp(WEAPON_HAVE &weapon) {
    if (Fillable(weapon)) {
        weapon.durability_f = weapon.durability;
    }
}

// The weapons a filling circle reaches past the active one: at 2 every member's equipped weapon, at 3 every
// weapon the party carries.
template <typename Fill>
void FillParty(CUserStatus &status, const WEAPON_HAVE &active, int m, Fill fill) {
    for (int chara = 0; chara < kMembers; chara++) {
        int equipped = status.equipped_weapon_slot[chara];
        for (int slot = 0; slot < kCarried; slot++) {
            WEAPON_HAVE &weapon = status.chara_weapons[chara][slot];
            if (&weapon != &active && (m >= 3 || slot == equipped)) {
                fill(weapon);
            }
        }
    }
}

} // namespace

int CircleMagnitude() {
    return 1 + (WeaponOwned(ITEM_WEAPON_CRYSKNIFE) ? 1 : 0) + (WeaponOwned(ITEM_WEAPON_MAGICAL_HAMMER) ? 1 : 0);
}

bool AmplifiedCircle(int kind, WEAPON_HAVE &weapon) {
    int m = CircleMagnitude();
    if (UserStatus == nullptr || (m == 1 && kind != TRAP_CIRCLE_ABS_FULL)) {
        return false;
    }
    CUserStatus &status = *UserStatus;
    int          chara = status.cur_chara;
    int          had = status.money;
    int          sound = kGoodSound;
    switch (kind) {
        case TRAP_CIRCLE_STAMINA:
            BtSetStatusErr(AILMENT_STAMINA);
            if ((status.ailments[chara] & AILMENT_STAMINA) != 0) {
                status.ailment_frames[chara] = static_cast<s16>(kStaminaFrames * m);
            }
            break;
        case TRAP_CIRCLE_MONEY_UP:
            status.money = static_cast<u16>(std::min(kMoneyMax, had + m * (static_cast<int>(had * kMoneyUp) + kMoneyUpAdd)));
            break;
        case TRAP_CIRCLE_ABS_FULL:
            FillAbs(weapon);
            if (m > 1) {
                FillParty(status, weapon, m, FillAbs);
            }
            break;
        case TRAP_CIRCLE_WHP_UP:
            WhpUp(weapon, m);
            break;
        case TRAP_CIRCLE_WHP_CURE:
            WeaponDataChangeByRGate(&weapon, RGATE_WHP_CURE);
            FillParty(status, weapon, m, CureWhp);
            break;
        case TRAP_CIRCLE_ALL_MONSTER_STAMINA:
            if (NowMonstorUnit != nullptr) {
                for (MONSTOR &monster : NowMonstorUnit->monster) {
                    monster.anger_timer = kRageFrames * m;
                }
            }
            sound = kBadSound;
            break;
        case TRAP_CIRCLE_MONEY_DOWN:
            status.money = static_cast<u16>(std::max(0, static_cast<int>(had - kMoneyDown * m * had)));
            sound = kBadSound;
            break;
        case TRAP_CIRCLE_STAT_DOWN:
            StatDown(weapon, m);
            sound = kBadSound;
            break;
        case TRAP_CIRCLE_WHP_DOWN:
            WhpDown(weapon, m);
            sound = kBadSound;
            break;
        case TRAP_CIRCLE_WHP_QUARTER:
            weapon.durability_f = 1.0f;
            sound = kBadSound;
            break;
        default:
            return false;
    }
    SetWeaponAttachStatus(NowWeaponHave);
    SndSePlay(sound, -1, 0);
    return true;
}

} // namespace expanded
