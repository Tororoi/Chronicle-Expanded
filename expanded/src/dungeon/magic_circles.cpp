#include "expanded/dungeon/magic_circles.hpp"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <iterator>
#include <optional>
#include <string>

#include "battlemenu.hpp"
#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/core/party.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/ownership.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
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

// The most a favoured stat loss raises a stat, element or anti to.
constexpr int kGainMax = 99;

// Monster steps left of the favoured rage circle's slow, and the goo timer it gave each monster slot (0: none).
int slow_left = 0;
int slowed[kFloorMonsters] = {};

void SlowReset() {
    slow_left = 0;
    std::fill(std::begin(slowed), std::end(slowed), 0);
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(SlowReset), true);

std::optional<std::string> SlowedLine() {
    return LocalizeText("expanded.magic_circles.slowed", "Monsters slowed down.");
}

[[maybe_unused]] const bool kSlowedLine = (SetSystemMessage(kMonstersSlowedMessage, SlowedLine), true);

int Roll(int base, int range, int magnitude) {
    return (base + std::rand() % range) * magnitude;
}

// A value raised by gain, to kGainMax at most; one past it already stays as it is.
int Raised(int value, int gain) {
    return std::max(value, std::min(kGainMax, value + gain));
}

// LocalWeaponDataChange's loss on one of count values, its roll times the magnitude; favoured, a gain of it.
void ChangeOne(char *values, int count, int base, int range, int magnitude, bool favoured) {
    char &value = values[std::rand() % count];
    int   roll = Roll(base, range, magnitude);
    value = static_cast<char>(favoured ? Raised(value, roll) : std::max(0, value - roll));
}

// The stat loss: a stat, then one element and one anti of each group, down (attack to 1 at least, the rest to 0),
// or, favoured, up by the same rolls.
void StatChange(WEAPON_HAVE &weapon, int m, bool favoured) {
    if (IsDefaultWeapon(weapon.item_no) >= 0) {
        return;
    }
    int  stat = std::rand() % 4;
    int  roll = Roll(2, 3, m);
    s16 *stats[] = {&weapon.attack, &weapon.endurance, &weapon.speed, &weapon.magic};
    s16 &value = *stats[stat];
    value = static_cast<s16>(favoured ? Raised(value, roll) : std::max(stat == 0 ? 1 : 0, value - roll));
    ChangeOne(weapon.elem, 5, 2, 3, m, favoured);
    ChangeOne(weapon.vs_monster, 3, 2, 2, m, favoured);
    ChangeOne(&weapon.vs_monster[3], 3, 2, 2, m, favoured);
    ChangeOne(&weapon.vs_monster[6], 4, 2, 2, m, favoured);
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
    for (int chara = 0; chara < kPartyMembers; chara++) {
        int equipped = status.equipped_weapon_slot[chara];
        for (int slot = 0; slot < kWeaponSlots; slot++) {
            WEAPON_HAVE &weapon = status.chara_weapons[chara][slot];
            if (&weapon != &active && (m >= 3 || slot == equipped)) {
                fill(weapon);
            }
        }
    }
}

// A monster slot holding a live monster.
bool Live(const MONSTOR &monster) {
    return monster.state != -1 && monster.hp > 0;
}

// The favoured rage circle: every monster slot's goo timer set to frames (a monster gooed already keeps its own goo),
// counted down from frames.
void SlowEveryMonster(CMonstorUnit &unit, int frames) {
    int count = 0;
    for (int i = 0; i < kFloorMonsters; i++) {
        MONSTOR &monster = unit.monster[i];
        if (monster.slow_timer > 0 && monster.slow_timer != slowed[i]) {
            slowed[i] = 0;
            continue;
        }
        monster.slow_timer = slowed[i] = frames;
        count += Live(monster);
    }
    slow_left = frames;
    LogEvent("circle_slow", std::format("\"monsters\":{},\"frames\":{}", count, frames));
}

} // namespace

int CircleMagnitude() {
    return 1 + (WeaponOwned(ITEM_WEAPON_CRYSKNIFE) ? 1 : 0) + (WeaponOwned(ITEM_WEAPON_MAGICAL_HAMMER) ? 1 : 0);
}

bool CirclesFavoured() {
    return WeaponOwned(ITEM_WEAPON_SECRET_ARMLET);
}

int DealtCircle(int kind) {
    int dealt = kind;
    if (CirclesFavoured()) {
        switch (kind) {
            case TRAP_CIRCLE_MONEY_DOWN:
                dealt = TRAP_CIRCLE_MONEY_UP;
                break;
            case TRAP_CIRCLE_WHP_DOWN:
                dealt = TRAP_CIRCLE_WHP_UP;
                break;
            case TRAP_CIRCLE_WHP_QUARTER:
                dealt = TRAP_CIRCLE_WHP_CURE;
                break;
        }
    }
    LogEvent("circle", std::format("\"kind\":{},\"dealt\":{}", kind, dealt));
    return dealt;
}

int ExpandedCircle(int kind, WEAPON_HAVE &weapon) {
    int  m = CircleMagnitude();
    bool favoured = (kind == TRAP_CIRCLE_ALL_MONSTER_STAMINA || kind == TRAP_CIRCLE_STAT_DOWN) && CirclesFavoured();
    if (UserStatus == nullptr || (m == 1 && kind != TRAP_CIRCLE_ABS_FULL && !favoured)) {
        return -1;
    }
    CUserStatus &status = *UserStatus;
    int          chara = status.cur_chara;
    int          had = status.money;
    int          sound = kGoodSound;
    int          shown = kind;
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
            if (favoured) {
                if (NowMonstorUnit != nullptr) {
                    SlowEveryMonster(*NowMonstorUnit, kCircleSlowFrames * m);
                }
                shown = kMonstersSlowedMessage - kCircleMessages;
                break;
            }
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
            StatChange(weapon, m, favoured);
            sound = favoured ? kGoodSound : kBadSound;
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
            return -1;
    }
    SetWeaponAttachStatus(NowWeaponHave);
    SndSePlay(sound, -1, 0);
    return shown;
}

void CircleSlowStep() {
    if (slow_left <= 0 || !MonstersStepping() || --slow_left > 0) {
        return;
    }
    int count = 0;
    for (int i = 0; i < kFloorMonsters && NowMonstorUnit != nullptr; i++) {
        MONSTOR &monster = NowMonstorUnit->monster[i];
        if (slowed[i] > 0 && monster.slow_timer == slowed[i]) {
            monster.slow_timer = 0;
            count += Live(monster);
        }
    }
    SlowReset();
    LogEvent("circle_slow_end", std::format("\"monsters\":{}", count));
}

} // namespace expanded
