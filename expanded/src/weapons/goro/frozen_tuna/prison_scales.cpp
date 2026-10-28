#include "expanded/weapons/goro/frozen_tuna/prison_scales.hpp"

namespace expanded::frozen_tuna {

namespace {

// A body and the prison's size on it: a party member (CHARA_*) or a monster kind (Monster::*).
struct Fit {
    constexpr Fit(Character member, float horizontal, float vertical) : body(-1 - member), size{horizontal, vertical} {}

    constexpr Fit(Monster kind, float horizontal, float vertical) : body(No(kind)), size{horizontal, vertical} {}

    int        body;
    PrisonSize size;
};

// Each body's prison, fitted by eye: its scale across, then up.
// clang-format off
constexpr Fit kPrisonScales[] = {
    {CHARA_XIAO, 1.00f, 1.00f},
    {CHARA_GORO, 1.00f, 1.00f},
    {Monster::MASTER_JACKET, 1.00f, 1.00f}, // 0 Master Jacket
    {Monster::SKELETON_SOLDIER, 1.00f, 1.00f}, // 1 Skeleton Soldier
    {Monster::STATUE, 1.25f, 1.25f}, // 2 Statue
    {Monster::DASHER, 1.30f, 1.30f}, // 3 Dasher
    {Monster::WEREWOLF, 1.50f, 1.50f}, // 4 Werewolf
    {Monster::FLI_FLI, 1.45f, 1.45f}, // 5 FliFli
    {Monster::HORNET, 1.70f, 1.70f}, // 6 Hornet
    {Monster::HALLOWEEN, 1.05f, 1.05f}, // 7 Halloween
    {Monster::CANNIBAL_PLANT, 1.25f, 1.25f}, // 8 Cannibal Plant
    {Monster::EARTH_DIGGER, 1.75f, 1.75f}, // 9 Earth Digger
    {Monster::SUNDAY, 1.00f, 1.00f}, // 10 Sunday
    {Monster::MONDAY, 1.10f, 1.10f}, // 11 Monday
    {Monster::TUESDAY, 1.00f, 1.00f}, // 12 Tuesday
    {Monster::WEDNESDAY, 2.05f, 2.05f}, // 13 Wednesday
    {Monster::THURSDAY, 1.00f, 1.00f}, // 14 Thursday
    {Monster::FRIDAY, 1.00f, 1.00f}, // 15 Friday
    {Monster::SATURDAY, 1.20f, 1.20f}, // 16 Saturday
    {Monster::WITCH_HELLZA, 1.75f, 1.75f}, // 17 Witch Hellza
    {Monster::WITCH_ILLZA, 1.45f, 1.45f}, // 18 Witch Illza
    {Monster::GUNNY, 2.30f, 2.30f}, // 19 Gunny
    {Monster::GYON, 2.95f, 2.95f}, // 20 Gyon
    {Monster::PIRATES_CHARIOT, 3.05f, 3.05f}, // 21 Pirate's Chariot
    {Monster::AUNTIE_MEDU, 2.00f, 2.00f}, // 22 Auntie Medu
    {Monster::CAPTAIN, 1.75f, 1.75f}, // 23 Captain
    {Monster::CORCEA, 1.65f, 1.65f}, // 24 Corcea
    {Monster::GOLEM, 4.75f, 4.75f}, // 25 Golem
    {Monster::MR_BLARE, 1.20f, 1.20f}, // 26 Mr. Blare
    {Monster::DUNE, 4.05f, 4.05f}, // 27 Dune
    {Monster::TITAN, 4.25f, 4.25f}, // 28 Titan
    {Monster::KING_MIMIC_DBC, 5.40f, 5.40f}, // 29 King Mimic (Divine Beast Cave)
    {Monster::MIMIC_DBC, 3.30f, 3.30f}, // 30 Mimic (Divine Beast Cave)
    {Monster::KING_MIMIC_SMT, 6.30f, 6.30f}, // 31 King Mimic (Sun & Moon Temple)
    {Monster::MIMIC_SMT, 3.65f, 3.65f}, // 32 Mimic (Sun & Moon Temple)
    {Monster::KING_MIMIC_MS, 6.30f, 6.30f}, // 33 King Mimic (Moon Sea)
    {Monster::MIMIC_MS, 3.65f, 3.65f}, // 34 Mimic (Moon Sea)
    {Monster::ARTHUR, 5.40f, 5.40f}, // 35 Arthur
    {Monster::GHOST, 1.55f, 1.55f}, // 36 Ghost
    {Monster::ALEXANDER, 4.15f, 4.15f}, // 37 Alexander
    {Monster::HEART, 1.40f, 1.40f}, // 38 Heart
    {Monster::CLUB, 1.75f, 1.75f}, // 39 Club
    {Monster::DIAMOND, 1.40f, 1.40f}, // 40 Diamond
    {Monster::SPADE, 1.40f, 1.40f}, // 41 Spade
    {Monster::JOKER, 1.75f, 1.75f}, // 42 Joker
    {Monster::BOMBER_HEAD, 1.80f, 1.80f}, // 43 Bomber Head
    {Monster::MUMMY, 1.40f, 1.40f}, // 44 Mummy
    {Monster::LICH, 1.60f, 1.60f}, // 45 Lich
    {Monster::CURSE_DANCER, 1.00f, 1.00f}, // 46 Curse Dancer
    {Monster::LIVING_ARMOR, 1.95f, 1.95f}, // 47 Living Armor
    {Monster::WHITE_FANG, 2.60f, 2.60f}, // 48 White Fang
    {Monster::MOON_BUG, 2.30f, 2.30f}, // 49 Moon Bug
    {Monster::PHANTOM, 3.85f, 3.85f}, // 50 Phantom
    {Monster::DRAGON, 4.85f, 4.85f}, // 51 Dragon
    {Monster::CAVE_BAT, 2.65f, 2.65f}, // 52 Cave Bat
    {Monster::EVIL_BAT, 2.65f, 2.65f}, // 53 Evil Bat
    {Monster::HELL_POCKLE, 1.65f, 1.65f}, // 54 Hell Pockle
    {Monster::RASH_DASHER, 2.35f, 2.35f}, // 55 Rash Dasher
    {Monster::STEEL_GIANT, 4.65f, 4.65f}, // 56 Steel Giant
    {Monster::BLIZZARD, 4.55f, 4.55f}, // 57 Blizzard
    {Monster::MOON_DIGGER, 3.75f, 3.75f}, // 58 Moon Digger
    {Monster::DARK_FLOWER, 1.90f, 1.90f}, // 59 Dark Flower
    {Monster::CURSED_ROSE, 2.25f, 2.25f}, // 60 Cursed Rose
    {Monster::BILLY, 1.25f, 1.25f}, // 61 Billy
    {Monster::VULCAN, 4.05f, 4.05f}, // 62 Vulcan
    {Monster::CRABBY_HERMIT, 3.80f, 3.80f}, // 63 Crabby Hermit
    {Monster::SPACE_GYON, 2.95f, 2.95f}, // 64 Space Gyon
    {Monster::BLUE_DRAGON, 4.85f, 4.85f}, // 65 Blue Dragon
    {Monster::BLACK_DRAGON, 5.45f, 5.45f}, // 66 Black Dragon
    {Monster::MASK_OF_PRAJNA, 3.65f, 3.65f}, // 67 Mask of Prajna
    {Monster::CRESCENT_BARON, 1.40f, 1.40f}, // 68 Crescent Baron
    {Monster::ROCKANOFF, 2.45f, 2.45f}, // 69 Rockanoff
    {Monster::KING_MIMIC_WOF, 5.40f, 5.40f}, // 70 King Mimic (Wise Owl Forest)
    {Monster::MIMIC_WOF, 3.30f, 3.30f}, // 71 Mimic (Wise Owl Forest)
    {Monster::KING_MIMIC_SW, 5.40f, 5.40f}, // 72 King Mimic (Shipwreck)
    {Monster::MIMIC_SW, 3.30f, 3.30f}, // 73 Mimic (Shipwreck)
    {Monster::KING_MIMIC_GOT, 5.40f, 5.40f}, // 74 King Mimic (Gallery of Time)
    {Monster::MIMIC_GOT, 3.30f, 3.30f}, // 75 Mimic (Gallery of Time)
    {Monster::ICE_ARROW, 1.00f, 1.00f}, // 76 Ice Arrow
    {Monster::SAM, 1.20f, 1.20f}, // 77 Sam
    {Monster::DRAN, 13.85f, 13.85f}, // 78 Dran
    {Monster::MASTER_UTAN, 6.70f, 6.70f}, // 79 Master Utan
    {Monster::ICE_QUEEN, 2.10f, 2.10f}, // 80 Ice Queen
    {Monster::KINGS_CURSE_COFFIN, 1.95f, 1.95f}, // 81 King's Curse Coffin
    {Monster::KINGS_CURSE, 3.30f, 3.30f}, // 82 King's Curse
    {Monster::MINOTAUR_JOE, 8.80f, 8.80f}, // 83 Minotaur Joe
    {Monster::DARK_GENIE, 20.30f, 20.30f}, // 84 Dark Genie
    {Monster::DARK_GENIE_RIGHT_HAND, 8.60f, 8.60f}, // 85 Dark Genie Right Hand
    {Monster::DARK_GENIE_LEFT_HAND, 9.05f, 9.05f}, // 86 Dark Genie Left Hand
    {Monster::DG_AURA, 18.95f, 18.95f}, // 87 (DG effect c17_)
    {Monster::DG_WIND, 1.00f, 1.00f}, // 88 (DG companion c17_)
    {Monster::DG_BEAM, 1.00f, 1.00f}, // 89 (DG companion c17_)
    {Monster::DG_BEAM_CONTACT, 1.00f, 1.00f}, // 90 (DG companion c17_)
    {Monster::WINE_KEG, 1.80f, 1.80f}, // 91 Wine Keg
    {Monster::ICE_AURA, 1.00f, 1.00f}, // 92 Ice Aura
    {Monster::DG_HAND_AURA, 1.00f, 1.00f}, // 93 (DG companion c17_)
    {Monster::GOL, 4.75f, 4.75f}, // 94 Gol
    {Monster::SIL, 4.75f, 4.75f}, // 95 Sil
    {Monster::YAMMICH, 11.30f, 11.30f}, // 96 Yammich
    {Monster::STATUE_DOG, 3.10f, 3.10f}, // 97 Statue Dog
    {Monster::OPAR, 4.60f, 4.60f}, // 98 Opar
    {Monster::HALEY_HOLEY, 3.90f, 3.90f}, // 99 Haley Holey
    {Monster::KING_PRICKLY, 5.70f, 5.70f}, // 100 King Prickly
    {Monster::ICE_BARRIER, 1.00f, 1.00f}, // 101 Ice Barrier
    {Monster::ICE_PRISON, 1.00f, 1.00f}, // 102 Ice Prison
    {Monster::ICE_METEOR, 1.00f, 1.00f}, // 103 Ice Meteor
    {Monster::ICE_TORNADO, 1.00f, 1.00f}, // 104 Ice Tornado
    {Monster::GACIOUS, 3.00f, 3.00f}, // 105 Gacious
    {Monster::DARK_GENIE_FINAL, 28.25f, 28.25f}, // 106 Dark Genie (Final Form)
    {Monster::DG_FINAL_SUMMON, 1.00f, 1.00f}, // 107 DG Final summon (last_mc)
    {Monster::DG_FINAL_GROUND_WAVE, 1.00f, 1.00f}, // 108 DG Final ground wave (last_gw1)
    {Monster::DG_FINAL_BEAM, 1.00f, 1.00f}, // 109 DG Final beam
    {Monster::DG_FINAL_BEAM_CONTACT, 1.00f, 1.00f}, // 110 DG Final beam (small)
    {Monster::GEMRON_FIRE, 1.75f, 1.75f}, // 111 Gemron (Fire)
    {Monster::NIKAPOUS, 2.20f, 2.20f}, // 112 Nikapous
    {Monster::WHITE_FANG_ENHANCED, 2.60f, 2.60f}, // 113 White Fang (Enhanced)
    {Monster::ARTHUR_ENHANCED, 5.40f, 5.40f}, // 114 Arthur (Enhanced)
    {Monster::SIL_ENHANCED, 4.75f, 4.75f}, // 115 Sil (Enhanced)
    {Monster::HALLOWEEN_ENHANCED, 1.55f, 1.55f}, // 116 Halloween (Enhanced)
    {Monster::MASTER_JACKET_ENHANCED, 1.40f, 1.40f}, // 117 Master Jacket (Enhanced)
    {Monster::VULCAN_ENHANCED, 4.05f, 4.05f}, // 118 Vulcan (Enhanced)
    {Monster::MUMMY_ENHANCED, 1.40f, 1.40f}, // 119 Mummy (Enhanced)
    {Monster::DIAMOND_ENHANCED, 1.40f, 1.40f}, // 120 Diamond (Enhanced)
    {Monster::GEMRON_ICE, 1.75f, 1.75f}, // 121 Gemron (Ice)
    {Monster::HORN_HEAD, 2.00f, 2.00f}, // 122 Horn Head
    {Monster::AUNTIE_MEDU_ENHANCED, 2.00f, 2.00f}, // 123 Auntie Medu (Enhanced)
    {Monster::ROCKANOFF_ENHANCED, 2.45f, 2.45f}, // 124 Rockanoff (Enhanced)
    {Monster::YAMMICH_ENHANCED, 11.30f, 11.30f}, // 125 Yammich (Enhanced)
    {Monster::WITCH_HELLZA_ENHANCED, 1.75f, 1.75f}, // 126 Witch Hellza (Enhanced)
    {Monster::STEEL_GIANT_ENHANCED, 4.65f, 4.65f}, // 127 Steel Giant (Enhanced)
    {Monster::CLUB_ENHANCED, 1.75f, 1.75f}, // 128 Club (Enhanced)
    {Monster::CORCEA_ENHANCED, 1.65f, 1.65f}, // 129 Corcea (Enhanced)
    {Monster::MIMIC_DS, 3.30f, 3.30f}, // 130 Mimic (Demon Shaft)
    {Monster::KING_MIMIC_DS, 5.35f, 5.35f}, // 131 King Mimic (Demon Shaft)
    {Monster::GEMRON_THUNDER, 1.75f, 1.75f}, // 132 Gemron (Thunder)
    {Monster::BISHOP_Q, 2.55f, 2.55f}, // 133 Bishop Q
    {Monster::CAVE_BAT_ENHANCED, 2.65f, 2.65f}, // 134 Cave Bat (Enhanced)
    {Monster::GOL_ENHANCED, 4.75f, 4.75f}, // 135 Gol (Enhanced)
    {Monster::MASK_OF_PRAJNA_ENHANCED, 3.65f, 3.65f}, // 136 Mask of Prajna (Enhanced)
    {Monster::GYON_ENHANCED, 2.95f, 2.95f}, // 137 Gyon (Enhanced)
    {Monster::SPADE_ENHANCED, 1.40f, 1.40f}, // 138 Spade (Enhanced)
    {Monster::RASH_DASHER_ENHANCED, 2.35f, 2.35f}, // 139 Rash Dasher (Enhanced)
    {Monster::CAPTAIN_ENHANCED, 1.75f, 1.75f}, // 140 Captain (Enhanced)
    {Monster::MIMIC_DS_ENHANCED, 3.30f, 3.30f}, // 141 Mimic (Demon Shaft) (Enhanced)
    {Monster::KING_MIMIC_DS_ENHANCED, 5.35f, 5.35f}, // 142 King Mimic (Demon Shaft) (Enhanced)
    {Monster::GEMRON_WIND, 1.75f, 1.75f}, // 143 Gemron (Wind)
    {Monster::SILVER_GEAR, 1.30f, 1.30f}, // 144 Silver Gear
    {Monster::ALEXANDER_ENHANCED, 4.15f, 4.15f}, // 145 Alexander (Enhanced)
    {Monster::HEART_ENHANCED, 1.40f, 1.40f}, // 146 Heart (Enhanced)
    {Monster::BOMBER_HEAD_ENHANCED, 1.80f, 1.80f}, // 147 Bomber Head (Enhanced)
    {Monster::CRABBY_HERMIT_ENHANCED, 3.80f, 3.80f}, // 148 Crabby Hermit (Enhanced)
    {Monster::CURSED_ROSE_ENHANCED, 2.25f, 2.25f}, // 149 Cursed Rose (Enhanced)
    {Monster::PIRATES_CHARIOT_ENHANCED, 3.05f, 3.05f}, // 150 Pirate's Chariot (Enhanced)
    {Monster::SPACE_GYON_ENHANCED, 2.95f, 2.95f}, // 151 Space Gyon (Enhanced)
    {Monster::MIMIC_DS_ENHANCED_TWICE, 3.30f, 3.30f}, // 152 Mimic (Demon Shaft) (Enhanced x2)
    {Monster::KING_MIMIC_DS_ENHANCED_TWICE, 5.35f, 5.35f}, // 153 King Mimic (Demon Shaft) (Enhanced x2)
    {Monster::GEMRON_HOLY, 1.75f, 1.75f}, // 154 Gemron (Holy)
    {Monster::GACIOUS_ENHANCED, 3.00f, 3.00f}, // 155 Gacious (Enhanced)
    {Monster::EVIL_BAT_ENHANCED, 2.65f, 2.65f}, // 156 Evil Bat (Enhanced)
    {Monster::CRESCENT_BARON_ENHANCED, 1.40f, 1.40f}, // 157 Crescent Baron (Enhanced)
    {Monster::STATUE_DOG_ENHANCED, 3.10f, 3.10f}, // 158 Statue Dog (Enhanced)
    {Monster::JOKER_ENHANCED, 1.75f, 1.75f}, // 159 Joker (Enhanced)
    {Monster::LICH_ENHANCED, 1.60f, 1.60f}, // 160 Lich (Enhanced)
    {Monster::TITAN_ENHANCED, 4.25f, 4.25f}, // 161 Titan (Enhanced)
    {Monster::LIVING_ARMOR_ENHANCED, 1.95f, 1.95f}, // 162 Living Armor (Enhanced)
    {Monster::MIMIC_DS_ENHANCED_THRICE, 3.30f, 3.30f}, // 163 Mimic (Demon Shaft) (Enhanced x3)
    {Monster::KING_MIMIC_DS_ENHANCED_THRICE, 5.35f, 5.35f}, // 164 King Mimic (Demon Shaft) (Enhanced x3)
    {Monster::BLACK_KNIGHT, 5.30f, 5.30f}, // 165 Black Knight
    {Monster::BLACK_KNIGHT_MOUNT, 7.75f, 7.75f}, // 166 Black Knight Mount
    {Monster::BOMB_GEMRON, 1.75f, 1.75f}, // 167 Bomb Gemron
    {Monster::CRYSTAL_GEMRON, 1.75f, 1.75f}, // 168 Crystal Gemron
    {Monster::ATLA_GEMRON, 1.75f, 1.75f}, // 169 Atla Gemron
};
// clang-format on

PrisonSize Size(int body) {
    for (const Fit &fit : kPrisonScales) {
        if (fit.body == body) {
            return fit.size;
        }
    }
    return {};
}

} // namespace

PrisonSize PrisonScale(Character member) {
    return Size(-1 - member);
}

PrisonSize PrisonScale(Monster kind) {
    return No(kind) >= 0 ? Size(No(kind)) : PrisonSize{};
}

} // namespace expanded::frozen_tuna
