#include "expanded/enemies/scaling.hpp"

#include "monstorunit.hpp"

namespace expanded {

namespace {

// Each MonstorTable kind's strongest _SET_DMG_PARA hit and strongest _SET_SHOT damage in its
// dun/monstor script; 0 for none (a shot without a damage argument deals its BT_SHOT_EFFECT's).
struct ScriptDamage {
    short melee;
    short shot;
};

// clang-format off
constexpr ScriptDamage kScriptDamage[167] = {
    {35, 0}, // 0: Master Jacket
    {21, 0}, // 1: Skeleton Soldier
    {26, 0}, // 2: Statue
    {22, 0}, // 3: Dasher
    {68, 0}, // 4: Werewolf
    {37, 35}, // 5: FliFli
    {42, 0}, // 6: Hornet
    {57, 60}, // 7: Halloween
    {36, 30}, // 8: Cannibal Plant
    {52, 37}, // 9: Earth Digger
    {36, 0}, // 10: Sunday
    {32, 0}, // 11: Monday
    {31, 31}, // 12: Tuesday
    {30, 0}, // 13: Wednesday
    {29, 30}, // 14: Thursday
    {29, 0}, // 15: Friday
    {29, 0}, // 16: Saturday
    {73, 73}, // 17: Witch Hellza
    {47, 47}, // 18: Witch Illza
    {44, 26}, // 19: Gunny
    {59, 59}, // 20: Gyon
    {0, 69}, // 21: Pirate's Chariot
    {74, 60}, // 22: Auntie Medu
    {74, 0}, // 23: Captain
    {43, 0}, // 24: Corcea
    {75, 64}, // 25: Golem
    {81, 90}, // 26: Mr. Blare
    {85, 0}, // 27: Dune
    {105, 90}, // 28: Titan
    {35, 0}, // 29: King Mimic (Divine Beast Cave)
    {33, 0}, // 30: Mimic (Divine Beast Cave)
    {102, 0}, // 31: King Mimic (Sun & Moon Temple)
    {71, 0}, // 32: Mimic (Sun & Moon Temple)
    {118, 0}, // 33: King Mimic (Moon Sea)
    {83, 0}, // 34: Mimic (Moon Sea)
    {116, 0}, // 35: Arthur
    {20, 21}, // 36: Ghost
    {120, 124}, // 37: Alexander
    {107, 107}, // 38: Heart
    {104, 0}, // 39: Club
    {110, 0}, // 40: Diamond
    {113, 0}, // 41: Spade
    {115, 0}, // 42: Joker
    {61, 64}, // 43: Bomber Head
    {54, 0}, // 44: Mummy
    {114, 100}, // 45: Lich
    {90, 0}, // 46: Curse Dancer
    {95, 0}, // 47: Living Armor
    {90, 0}, // 48: White Fang
    {70, 70}, // 49: Moon Bug
    {60, 0}, // 50: Phantom
    {45, 50}, // 51: Dragon
    {17, 0}, // 52: Cave Bat
    {86, 0}, // 53: Evil Bat
    {64, 0}, // 54: Hell Pockle
    {102, 0}, // 55: Rash Dasher
    {98, 64}, // 56: Steel Giant
    {119, 105}, // 57: Blizzard
    {83, 72}, // 58: Moon Digger
    {94, 85}, // 59: Dark Flower
    {49, 46}, // 60: Cursed Rose
    {93, 110}, // 61: Billy
    {94, 0}, // 62: Vulcan
    {83, 76}, // 63: Crabby Hermit
    {78, 75}, // 64: Space Gyon
    {90, 90}, // 65: Blue Dragon
    {130, 135}, // 66: Black Dragon
    {80, 65}, // 67: Mask of Prajna
    {98, 70}, // 68: Crescent Baron
    {35, 0}, // 69: Rockanoff
    {67, 0}, // 70: King Mimic (Wise Owl Forest)
    {47, 0}, // 71: Mimic (Wise Owl Forest)
    {84, 0}, // 72: King Mimic (Shipwreck)
    {59, 0}, // 73: Mimic (Shipwreck)
    {134, 0}, // 74: King Mimic (Gallery of Time)
    {100, 0}, // 75: Mimic (Gallery of Time)
    {69, 0}, // 76: Ice Arrow
    {80, 58}, // 77: Sam
    {45, 17}, // 78: Dran
    {62, 13}, // 79: Master Utan
    {0, 0}, // 80: Ice Queen
    {0, 0}, // 81: King's Curse Coffin
    {91, 0}, // 82: King's Curse
    {125, 0}, // 83: Minotaur Joe
    {85, 0}, // 84: Dark Genie
    {125, 0}, // 85: Dark Genie Right Hand
    {125, 0}, // 86: Dark Genie Left Hand
    {125, 0}, // 87: (DG effect c17_)
    {0, 0}, // 88: (DG companion c17_)
    {175, 0}, // 89: (DG companion c17_)
    {175, 0}, // 90: (DG companion c17_)
    {8, 0}, // 91: Wine Keg
    {74, 0}, // 92: Ice Aura
    {0, 0}, // 93: (DG companion c17_)
    {75, 62}, // 94: Gol
    {75, 62}, // 95: Sil
    {35, 0}, // 96: Yammich
    {35, 0}, // 97: Statue Dog
    {35, 35}, // 98: Opar
    {37, 0}, // 99: Haley Holey
    {50, 0}, // 100: King Prickly
    {0, 0}, // 101: Ice Barrier
    {0, 0}, // 102: Ice Prison
    {69, 0}, // 103: Ice Meteor
    {74, 0}, // 104: Ice Tornado
    {130, 0}, // 105: Gacious
    {85, 130}, // 106: Dark Genie (Final Form)
    {0, 0}, // 107: DG Final summon (last_mc)
    {0, 0}, // 108: DG Final ground wave (last_gw1)
    {175, 0}, // 109: DG Final beam
    {175, 0}, // 110: DG Final beam (small)
    {100, 100}, // 111: Gemron (Fire)
    {150, 150}, // 112: Nikapous
    {122, 0}, // 113: White Fang (Enhanced)
    {130, 0}, // 114: Arthur (Enhanced)
    {115, 102}, // 115: Sil (Enhanced)
    {100, 100}, // 116: Halloween (Enhanced)
    {110, 0}, // 117: Master Jacket (Enhanced)
    {114, 0}, // 118: Vulcan (Enhanced)
    {98, 0}, // 119: Mummy (Enhanced)
    {123, 0}, // 120: Diamond (Enhanced)
    {120, 120}, // 121: Gemron (Ice)
    {130, 0}, // 122: Horn Head
    {122, 122}, // 123: Auntie Medu (Enhanced)
    {130, 0}, // 124: Rockanoff (Enhanced)
    {110, 0}, // 125: Yammich (Enhanced)
    {100, 100}, // 126: Witch Hellza (Enhanced)
    {178, 164}, // 127: Steel Giant (Enhanced)
    {114, 0}, // 128: Club (Enhanced)
    {110, 0}, // 129: Corcea (Enhanced)
    {130, 0}, // 130: Mimic (Demon Shaft)
    {170, 0}, // 131: King Mimic (Demon Shaft)
    {130, 130}, // 132: Gemron (Thunder)
    {130, 130}, // 133: Bishop Q
    {95, 0}, // 134: Cave Bat (Enhanced)
    {135, 132}, // 135: Gol (Enhanced)
    {140, 146}, // 136: Mask of Prajna (Enhanced)
    {100, 100}, // 137: Gyon (Enhanced)
    {110, 0}, // 138: Spade (Enhanced)
    {102, 0}, // 139: Rash Dasher (Enhanced)
    {99, 0}, // 140: Captain (Enhanced)
    {130, 0}, // 141: Mimic (Demon Shaft) (Enhanced)
    {170, 0}, // 142: King Mimic (Demon Shaft) (Enhanced)
    {140, 140}, // 143: Gemron (Wind)
    {0, 110}, // 144: Silver Gear
    {122, 122}, // 145: Alexander (Enhanced)
    {130, 130}, // 146: Heart (Enhanced)
    {160, 160}, // 147: Bomber Head (Enhanced)
    {130, 130}, // 148: Crabby Hermit (Enhanced)
    {140, 140}, // 149: Cursed Rose (Enhanced)
    {0, 114}, // 150: Pirate's Chariot (Enhanced)
    {160, 160}, // 151: Space Gyon (Enhanced)
    {130, 0}, // 152: Mimic (Demon Shaft) (Enhanced x2)
    {170, 0}, // 153: King Mimic (Demon Shaft) (Enhanced x2)
    {150, 150}, // 154: Gemron (Holy)
    {180, 0}, // 155: Gacious (Enhanced)
    {122, 0}, // 156: Evil Bat (Enhanced)
    {150, 150}, // 157: Crescent Baron (Enhanced)
    {110, 0}, // 158: Statue Dog (Enhanced)
    {170, 0}, // 159: Joker (Enhanced)
    {110, 110}, // 160: Lich (Enhanced)
    {160, 160}, // 161: Titan (Enhanced)
    {150, 0}, // 162: Living Armor (Enhanced)
    {130, 0}, // 163: Mimic (Demon Shaft) (Enhanced x3)
    {170, 0}, // 164: King Mimic (Demon Shaft) (Enhanced x3)
    {170, 150}, // 165: Black Knight
    {170, 170}, // 166: Black Knight Mount
};
// clang-format on

} // namespace

EnemyStats NativeStats(int model_no) {
    if (model_no < 0 || model_no >= 167) {
        return {};
    }
    const MONSTOR_MODEL &model = MonstorTable[model_no];
    return {model.max_hp, model.exp, model.defense, model.hardness, kScriptDamage[model_no].melee, kScriptDamage[model_no].shot};
}

} // namespace expanded
