#include "monstorunit.hpp"

#include <cstdio>
#include <cstdlib>

#include <libvu0.h>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "framevu1.hpp"
#include "hitmark.hpp"
#include "hitvalue.hpp"
#include "itemdata.hpp"
#include "mathutil.hpp"
#include "mglib.hpp"
#include "monstorunit_port.hpp"
#include "snd.hpp"
#include "stealitem.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

#include "platform/config.hpp"

// Retail's DrawShadowMonstor, which casts the shadows of the monsters taking part. A dormant
// monster within video.detail_distance of the player draws its model (DrawMonstorDetail,
// dun/gameloop.cpp), so it casts its shadow here too.
PORT_OVERRIDE void CMonstorUnit::DrawShadowMonstor() {
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR light = {0.0f, 1.0f, 0.0f, 0.0f};
    float         detail = ConfigDetailDistance();

    for (int i = 0; i < 16; i++) {
        bool drawn = monster[i].state == 2 || (monster[i].state == 1 && monster[i].player_distance < detail);

        if (drawn && monster[i].shadow_visible != 0 && monster[i].revealed != 0) {
            if (chara[i][0].shadow_frame != NULL) {
                CCharacter *character = &chara[i][0];
                character->ShadowStep();
                character->GetPosition(position);
                character->GetRotation(rotation);
                chara[i][0].shadow_frame->SetPosition(position);
                chara[i][0].shadow_frame->SetRotation(0.0f, rotation[1], 0.0f);
                position[1] -= monster[i].shadow_length;
                MGDrawShadowFast(chara[i][0].shadow_frame, position, light);
            }
        }
    }
}

/* Retail's MonstorTable (ps2/src/monstorunit.cpp), with one fix: sixteen regular monsters ship with
   drops_items 0, so a kill skips the death drop entirely (no money, attachment, gate key or rare
   item) though most of them have a money chance and a rare item set. Here they drop as the others
   do. Wine Keg and Gacious keep 0: they are MONSTER_KIND_NO_LOCK_ON, which never drops. */
// clang-format off
PORT_OVERRIDE MONSTOR_MODEL MonstorTable[167] = {
    // 0: Master Jacket
    {{"e01a", "", "", ""}, "e01a", 75, MONSTER_FAMILY_UNDEAD, {110, 80, 100, 80, 130}, 6.0f, 3, 0, {-1, -1}, 5, {0, 0}, 7, 50, MONSTER_KIND_NORMAL, {0, 0}, 1, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 80, ITEM_STAMINA_DRINK, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 1: Skeleton Soldier
    {{"e03a", "", "", ""}, "e03a", 23, MONSTER_FAMILY_UNDEAD, {110, 90, 100, 100, 160}, 6.0f, 0, 0, {-1, -1}, 3, {0, 0}, 4, 30, MONSTER_KIND_NORMAL, {0, 0}, 3, {0, 0}, -1, 1, 100, 90, ITEM_BREAD, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 2: Statue
    {{"e05a", "", "", ""}, "e05a", 38, MONSTER_FAMILY_STONE, {100, 100, 100, 100, 100}, 6.0f, 3, 20, {-1, -1}, 3, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 5, {0, 0}, ITEM_STONE, 1, 90, 50, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.699999988f},
    // 3: Dasher
    {{"e06a", "", "", ""}, "e06a", 23, MONSTER_FAMILY_BEAST, {100, 100, 100, 100, 100}, 6.0f, 1, 0, {-1, -1}, 3, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 6, {0, 0}, ITEM_BREAD, 1, 100, 90, ITEM_PRICKLY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 4: Werewolf
    {{"e07a", "", "", ""}, "e07a", 180, MONSTER_FAMILY_BEAST, {100, 100, 100, 100, 150}, 7.5f, 5, 0, {-1, -1}, 12, {0, 0}, 15, 50, MONSTER_KIND_NORMAL, {0, 0}, 7, {0, 0}, ITEM_STAND_IN_POWDER, 1, 90, 70, ITEM_ATTACH_SPEED, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 5: FliFli
    {{"e08a", "", "", ""}, "e08a", 120, MONSTER_FAMILY_PLANT, {180, 100, 100, 100, 100}, 6.5f, 0, 0, {0, -1}, 3, {0, 0}, 7, 30, MONSTER_KIND_NORMAL, {0, 0}, 8, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 100, 70, ITEM_POISONOUS_APPLE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 6: Hornet (drops items, unlike retail)
    {{"e09a", "", "", ""}, "e09a", 60, MONSTER_FAMILY_SKY, {100, 120, 100, 120, 100}, 6.5f, 0, 0, {-1, -1}, 3, {0, 0}, 7, 30, MONSTER_KIND_NORMAL, {0, 0}, 9, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 100, 70, ITEM_ATTACH_WIND, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 7: Halloween
    {{"e10a", "", "", ""}, "e10a", 150, MONSTER_FAMILY_PLANT, {150, 100, 100, 100, 100}, 5.0f, 3, 10, {3, -1}, 3, {0, 0}, 7, 40, MONSTER_KIND_NORMAL, {0, 0}, 10, {0, 0}, ITEM_BOMB_NUTS, 1, 100, 70, ITEM_BREAD, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 8: Cannibal Plant
    {{"e11a", "", "", ""}, "e11a", 60, MONSTER_FAMILY_PLANT, {180, 100, 100, 100, 100}, 6.5f, 2, 0, {2, -1}, 3, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 11, {0, 0}, ITEM_GOOEY_PEACH, 1, 100, 70, ITEM_REGULAR_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.0f},
    // 9: Earth Digger
    {{"e12a", "", "", ""}, "e12a", 120, MONSTER_FAMILY_BEAST, {100, 100, 80, 80, 100}, 6.5f, 2, 0, {11, -1}, 3, {0, 0}, 7, 30, MONSTER_KIND_NORMAL, {0, 0}, 12, {0, 0}, ITEM_MINON, 1, 100, 70, ITEM_MIMI, {50, 50, 120, 50, 50, 50}, {0, 0}, 1.0f},
    // 10: Sunday
    {{"e14a", "", "", ""}, "e14a", 60, MONSTER_FAMILY_MAGE, {100, 100, 100, 110, 100}, 5.0f, 0, 0, {-1, -1}, 3, {0, 0}, 6, 40, MONSTER_KIND_NORMAL, {0, 0}, 14, {0, 0}, ITEM_MELLOW_BANANA, 1, 100, 70, ITEM_REGULAR_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 11: Monday
    {{"e15a", "", "", ""}, "e15a", 60, MONSTER_FAMILY_MAGE, {100, 100, 100, 110, 100}, 5.0f, 0, 0, {-1, -1}, 3, {0, 0}, 6, 40, MONSTER_KIND_NORMAL, {0, 0}, 15, {0, 0}, ITEM_TASTY_WATER, 1, 100, 70, ITEM_CHEESE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 12: Tuesday
    {{"e16a", "", "", ""}, "e16a", 60, MONSTER_FAMILY_MAGE, {100, 100, 100, 110, 100}, 5.0f, 0, 0, {19, -1}, 3, {0, 0}, 6, 40, MONSTER_KIND_NORMAL, {0, 0}, 16, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 100, 70, ITEM_REGULAR_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 13: Wednesday
    {{"e17a", "", "", ""}, "e17a", 60, MONSTER_FAMILY_MAGE, {100, 100, 100, 110, 100}, 5.0f, 0, 0, {-1, -1}, 3, {0, 0}, 6, 40, MONSTER_KIND_NORMAL, {0, 0}, 17, {0, 0}, ITEM_TASTY_WATER, 1, 100, 70, ITEM_REGULAR_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 14: Thursday
    {{"e18a", "", "", ""}, "e18a", 60, MONSTER_FAMILY_MAGE, {100, 100, 100, 110, 100}, 5.0f, 0, 0, {4, -1}, 3, {0, 0}, 6, 40, MONSTER_KIND_NORMAL, {0, 0}, 18, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 100, 70, ITEM_REGULAR_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 15: Friday
    {{"e19a", "", "", ""}, "e19a", 60, MONSTER_FAMILY_MAGE, {100, 100, 100, 110, 100}, 5.0f, 0, 0, {-1, -1}, 3, {0, 0}, 6, 40, MONSTER_KIND_NORMAL, {0, 0}, 19, {0, 0}, ITEM_BREAD, 1, 100, 70, ITEM_REGULAR_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 16: Saturday
    {{"e20a", "", "", ""}, "e20a", 60, MONSTER_FAMILY_MAGE, {100, 100, 100, 110, 100}, 5.0f, 0, 0, {-1, -1}, 3, {0, 0}, 6, 40, MONSTER_KIND_NORMAL, {0, 0}, 20, {0, 0}, ITEM_BREAD, 1, 100, 70, ITEM_REGULAR_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 17: Witch Hellza (drops items, unlike retail)
    {{"e21a", "", "", ""}, "e21a", 270, MONSTER_FAMILY_MAGE, {70, 70, 70, 70, 100}, 8.0f, 0, 0, {5, -1}, 5, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 21, {0, 0}, ITEM_POISONOUS_APPLE, 1, 85, 50, ITEM_ATTACH_MAGICAL_POWER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 18: Witch Illza (drops items, unlike retail)
    {{"e22a", "", "", ""}, "e22a", 120, MONSTER_FAMILY_MAGE, {90, 90, 90, 90, 100}, 8.0f, 0, 0, {4, -1}, 3, {0, 0}, 4, 30, MONSTER_KIND_NORMAL, {0, 0}, 22, {0, 0}, ITEM_POISONOUS_APPLE, 1, 90, 50, ITEM_ATTACH_MAGICAL_POWER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 19: Gunny
    {{"e23a", "", "", ""}, "e23a", 250, MONSTER_FAMILY_SEA, {120, 100, 150, 120, 100}, 6.0f, 5, 20, {6, -1}, 4, {0, 0}, 8, 30, MONSTER_KIND_NORMAL, {0, 0}, 23, {0, 0}, ITEM_SOAP, 1, 95, 70, ITEM_EVY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 20: Gyon
    {{"e24a", "", "", ""}, "e24a", 225, MONSTER_FAMILY_SEA, {120, 100, 150, 100, 100}, 7.0f, 0, 0, {2, -1}, 4, {0, 0}, 8, 30, MONSTER_KIND_NORMAL, {0, 0}, 24, {0, 0}, ITEM_ANTIGOO_AMULET, 1, 100, 70, ITEM_FLAPPING_FISH, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 21: Pirate's Chariot
    {{"e25a", "", "", ""}, "e25a", 270, MONSTER_FAMILY_METAL, {120, 80, 140, 100, 100}, 8.0f, 5, 30, {15, -1}, 8, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 25, {0, 0}, ITEM_BOMB, 1, 95, 60, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.5f},
    // 22: Auntie Medu
    {{"e26a", "", "", ""}, "e26a", 300, MONSTER_FAMILY_DINO, {100, 140, 100, 100, 100}, 6.0f, 3, 0, {11, -1}, 10, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 26, {0, 0}, ITEM_THROBBING_CHERRY, 1, 100, 60, ITEM_ICE_BLOCK, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 23: Captain
    {{"e27a", "", "", ""}, "e27a", 225, MONSTER_FAMILY_UNDEAD, {110, 100, 80, 80, 150}, 6.0f, 3, 0, {-1, -1}, 6, {0, 0}, 12, 30, MONSTER_KIND_NORMAL, {0, 0}, 27, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 70, ITEM_ROTTEN_FISH, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 24: Corcea
    {{"e28a", "", "", ""}, "e28a", 150, MONSTER_FAMILY_UNDEAD, {110, 100, 100, 140, 130}, 6.0f, 0, 0, {-1, -1}, 4, {0, 0}, 8, 30, MONSTER_KIND_NORMAL, {0, 0}, 28, {0, 0}, ITEM_HOLY_WATER, 1, 100, 70, ITEM_ATTACH_ATTACK, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 25: Golem
    {{"e30a", "", "", ""}, "e30a", 375, MONSTER_FAMILY_STONE, {100, 100, 110, 110, 100}, 14.0f, 8, 0, {7, -1}, 4, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 30, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 50, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.5f},
    // 26: Mr. Blare
    {{"e31a", "", "", ""}, "e31a", 225, MONSTER_FAMILY_MAGE, {0, 170, 100, 100, 100}, 5.0f, 0, 10, {16, 5}, 5, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 31, {0, 0}, ITEM_FIRE_GEM, 1, 100, 70, ITEM_ATTACH_FIRE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 27: Dune
    {{"e32a", "", "", ""}, "e32a", 525, MONSTER_FAMILY_STONE, {100, 100, 80, 120, 100}, 11.0f, 0, 0, {-1, -1}, 10, {0, 0}, 18, 30, MONSTER_KIND_NORMAL, {0, 0}, 32, {0, 0}, -1, 1, 100, 70, ITEM_STONE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 28: Titan
    {{"e33a", "", "", ""}, "e33a", 750, MONSTER_FAMILY_STONE, {100, 100, 110, 110, 100}, 14.0f, 10, 50, {7, -1}, 12, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 33, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 70, ITEM_STONE, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.5f},
    // 29: King Mimic (Divine Beast Cave)
    {{"e34a", "", "", ""}, "e34a", 90, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 3, 10, {-1, -1}, 4, {0, 0}, 20, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 34, {0, 0}, ITEM_ESCAPE_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 30: Mimic (Divine Beast Cave)
    {{"e35a", "", "", ""}, "e35a", 68, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 1, 10, {-1, -1}, 3, {0, 0}, 10, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 35, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 31: King Mimic (Sun & Moon Temple)
    {{"e36a", "", "", ""}, "e36a", 525, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 5, 20, {-1, -1}, 15, {0, 0}, 20, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 36, {0, 0}, ITEM_STAND_IN_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 32: Mimic (Sun & Moon Temple)
    {{"e37a", "", "", ""}, "e37a", 270, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 5, 20, {-1, -1}, 6, {0, 0}, 12, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 37, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 33: King Mimic (Moon Sea)
    {{"e38a", "", "", ""}, "e38a", 600, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 8, 30, {-1, -1}, 12, {0, 0}, 20, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 38, {0, 0}, ITEM_REVIVAL_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 34: Mimic (Moon Sea)
    {{"e39a", "", "", ""}, "e39a", 450, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 8, 30, {-1, -1}, 6, {0, 0}, 15, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 39, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 35: Arthur
    {{"e40a", "", "", ""}, "e40a", 600, MONSTER_FAMILY_METAL, {80, 100, 150, 80, 80}, 9.0f, 10, 60, {5, -1}, 15, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 40, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 36: Ghost (drops items, unlike retail)
    {{"e42a", "", "", ""}, "e42a", 15, MONSTER_FAMILY_UNDEAD, {110, 100, 100, 100, 120}, 3.5999999f, 0, 0, {9, -1}, 3, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 42, {0, 0}, ITEM_ANTIDOTE_AMULET, 1, 100, 90, ITEM_ANTICURSEAMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 37: Alexander
    {{"e43a", "", "", ""}, "e43a", 675, MONSTER_FAMILY_METAL, {150, 130, 100, 120, 130}, 7.0f, 10, 50, {5, -1}, 15, {0, 0}, 17, 50, MONSTER_KIND_NORMAL, {0, 0}, 43, {0, 0}, ITEM_WIND_GEM, 1, 100, 70, ITEM_ATTACH_FIRE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 38: Heart
    {{"e44a", "", "", ""}, "e44a", 525, MONSTER_FAMILY_MAGE, {50, 150, 100, 100, 100}, 5.0f, 3, 0, {10, 12}, 6, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 44, {0, 0}, ITEM_STAMINA_DRINK, 1, 80, 50, ITEM_ANTICURSEAMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 39: Club
    {{"e45a", "", "", ""}, "e45a", 525, MONSTER_FAMILY_MAGE, {150, 100, 100, 50, 100}, 5.0f, 3, 0, {-1, -1}, 6, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 45, {0, 0}, ITEM_PREMIUM_WATER, 1, 80, 50, ITEM_ANTIGOO_AMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 40: Diamond
    {{"e46a", "", "", ""}, "e46a", 525, MONSTER_FAMILY_MAGE, {100, 100, 50, 150, 100}, 5.0f, 3, 0, {-1, -1}, 6, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 46, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 80, 50, ITEM_ANTIDOTE_AMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 41: Spade
    {{"e47a", "", "", ""}, "e47a", 525, MONSTER_FAMILY_MAGE, {150, 50, 100, 100, 100}, 5.0f, 3, 0, {-1, -1}, 6, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 47, {0, 0}, ITEM_HOLY_WATER, 1, 80, 50, ITEM_ANTI_FREEZE_AMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 42: Joker
    {{"e48a", "", "", ""}, "e48a", 600, MONSTER_FAMILY_MAGE, {50, 50, 50, 50, 150}, 5.0f, 3, 0, {-1, -1}, 6, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 48, {0, 0}, ITEM_PREMIUM_CHICKEN, 1, 50, 10, ITEM_MIGHTY_HEALING, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 43: Bomber Head
    {{"e49a", "", "", ""}, "e49a", 180, MONSTER_FAMILY_MAGE, {200, 75, 125, 100, 75}, 4.0f, 8, 20, {16, -1}, 4, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 49, {0, 0}, ITEM_BOMB, 1, 100, 70, ITEM_BOMB, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 44: Mummy
    {{"e50a", "", "", ""}, "e50a", 150, MONSTER_FAMILY_UNDEAD, {150, 50, 100, 100, 120}, 4.0f, 0, 0, {-1, -1}, 4, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 50, {0, 0}, -1, 1, 100, 70, ITEM_ANTICURSEAMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 45: Lich (drops items, unlike retail)
    {{"e51a", "", "", ""}, "e51a", 300, MONSTER_FAMILY_UNDEAD, {20, 20, 20, 20, 160}, 4.0f, 5, 0, {11, -1}, 12, {0, 0}, 15, 80, MONSTER_KIND_NORMAL, {0, 0}, 51, {0, 0}, ITEM_REVIVAL_POWDER, 1, 80, 30, ITEM_ATTACH_MAGICAL_POWER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 46: Curse Dancer
    {{"e52a", "", "", ""}, "e52a", 300, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 160}, 5.0f, 0, 0, {-1, -1}, 5, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 52, {0, 0}, ITEM_THROBBING_CHERRY, 1, 100, 70, ITEM_ANTICURSEAMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 47: Living Armor
    {{"e55a", "", "", ""}, "e55a", 450, MONSTER_FAMILY_STONE, {100, 100, 100, 80, 80}, 4.0f, 10, 50, {-1, -1}, 6, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 55, {0, 0}, -1, 1, 100, 50, ITEM_STONE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 48: White Fang
    {{"e56a", "", "", ""}, "e56a", 525, MONSTER_FAMILY_BEAST, {100, 100, 100, 100, 150}, 7.5f, 0, 0, {-1, -1}, 10, {0, 0}, 12, 30, MONSTER_KIND_NORMAL, {0, 0}, 56, {0, 0}, -1, 1, 100, 70, ITEM_CHEESE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 49: Moon Bug
    {{"e57a", "", "", ""}, "e57a", 450, MONSTER_FAMILY_METAL, {50, 120, 150, 50, 100}, 4.0f, 8, 40, {15, -1}, 5, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 57, {0, 0}, ITEM_BOMB, 1, 90, 70, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 50: Phantom (drops items, unlike retail)
    {{"e58a", "", "", ""}, "e58a", 150, MONSTER_FAMILY_SKY, {100, 125, 100, 125, 100}, 5.0f, 0, 0, {-1, -1}, 4, {0, 0}, 8, 30, MONSTER_KIND_NORMAL, {0, 0}, 58, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 100, 70, ITEM_ATTACH_WIND, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 51: Dragon
    {{"e59a", "", "", ""}, "e59a", 90, MONSTER_FAMILY_DINO, {50, 120, 100, 100, 100}, 17.5f, 5, 40, {5, -1}, 5, {0, 0}, 15, 50, MONSTER_KIND_NORMAL, {0, 0}, 59, {0, 0}, ITEM_FIRE_GEM, 1, 90, 70, ITEM_ATTACH_HOLY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 52: Cave Bat (drops items, unlike retail)
    {{"e60a", "", "", ""}, "e60a", 12, MONSTER_FAMILY_SKY, {100, 100, 100, 150, 100}, 3.0f, 0, 0, {-1, -1}, 3, {0, 0}, 4, 30, MONSTER_KIND_NORMAL, {0, 0}, 60, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 100, 90, ITEM_PRICKLY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 53: Evil Bat (drops items, unlike retail)
    {{"e61a", "", "", ""}, "e61a", 150, MONSTER_FAMILY_SKY, {100, 100, 100, 120, 100}, 3.0f, 0, 0, {-1, -1}, 4, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 61, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 100, 70, ITEM_PREMIUM_CHICKEN, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 54: Hell Pockle
    {{"e62a", "", "", ""}, "e62a", 270, MONSTER_FAMILY_MAGE, {100, 100, 100, 120, 100}, 5.0f, 2, 0, {-1, -1}, 5, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 62, {0, 0}, -1, 1, 100, 70, ITEM_BREAD, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 55: Rash Dasher
    {{"e63a", "", "", ""}, "e63a", 600, MONSTER_FAMILY_BEAST, {50, 150, 100, 100, 100}, 6.0f, 2, 10, {-1, -1}, 6, {0, 0}, 12, 30, MONSTER_KIND_NORMAL, {0, 0}, 63, {0, 0}, ITEM_PREMIUM_CHICKEN, 1, 100, 70, ITEM_ATTACH_SPEED, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 56: Steel Giant
    {{"e64a", "", "", ""}, "e64a", 750, MONSTER_FAMILY_METAL, {80, 100, 125, 80, 100}, 14.0f, 10, 50, {7, -1}, 12, {0, 0}, 15, 50, MONSTER_KIND_NORMAL, {0, 0}, 64, {0, 0}, ITEM_REPAIR_POWDER, 1, 95, 50, ITEM_MIGHTY_HEALING, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 57: Blizzard
    {{"e65a", "", "", ""}, "e65a", 750, MONSTER_FAMILY_METAL, {100, 100, 140, 140, 100}, 14.0f, 5, 0, {7, -1}, 8, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 65, {0, 0}, ITEM_ICE_GEM, 1, 100, 50, ITEM_ATTACH_ICE, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.5f},
    // 58: Moon Digger
    {{"e66a", "", "", ""}, "e66a", 420, MONSTER_FAMILY_BEAST, {150, 125, 80, 80, 100}, 5.0f, 2, 0, {11, -1}, 6, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 66, {0, 0}, ITEM_POTATO_CAKE, 1, 100, 70, ITEM_MIMI, {45, 45, 130, 45, 45, 45}, {0, 0}, 1.0f},
    // 59: Dark Flower
    {{"e67a", "", "", ""}, "e67a", 300, MONSTER_FAMILY_PLANT, {150, 100, 100, 100, 100}, 6.5f, 5, 0, {2, -1}, 5, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 67, {0, 0}, -1, 1, 100, 70, ITEM_PREMIUM_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.0f},
    // 60: Cursed Rose
    {{"e68a", "", "", ""}, "e68a", 225, MONSTER_FAMILY_PLANT, {150, 100, 100, 100, 130}, 6.5f, 2, 0, {2, -1}, 4, {0, 0}, 6, 30, MONSTER_KIND_NORMAL, {0, 0}, 68, {0, 0}, -1, 1, 100, 70, ITEM_TASTY_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.0f},
    // 61: Billy
    {{"e69a", "", "", ""}, "e69a", 300, MONSTER_FAMILY_MAGE, {100, 100, 0, 100, 100}, 5.0f, 5, 10, {18, 23}, 6, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 69, {0, 0}, ITEM_THUNDER_GEM, 1, 100, 70, ITEM_ATTACH_THUNDER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 62: Vulcan
    {{"e70a", "", "", ""}, "e70a", 480, MONSTER_FAMILY_STONE, {-50, 180, 100, 100, 100}, 7.0f, 5, 40, {-1, -1}, 12, {0, 0}, 4, 30, MONSTER_KIND_NORMAL, {0, 0}, 70, {0, 0}, ITEM_ATTACH_FIRE, 1, 100, 70, ITEM_STONE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 63: Crabby Hermit
    {{"e71a", "", "", ""}, "e71a", 300, MONSTER_FAMILY_SEA, {100, 100, 125, 100, 100}, 10.0f, 5, 20, {6, -1}, 4, {0, 0}, 12, 30, MONSTER_KIND_NORMAL, {0, 0}, 71, {0, 0}, ITEM_THROBBING_CHERRY, 1, 95, 70, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 64: Space Gyon
    {{"e72a", "", "", ""}, "e72a", 525, MONSTER_FAMILY_SEA, {75, 100, 125, 100, 100}, 5.0f, 0, 0, {2, -1}, 5, {0, 0}, 4, 30, MONSTER_KIND_NORMAL, {0, 0}, 72, {0, 0}, ITEM_SOAP, 1, 100, 70, ITEM_FLAPPING_FISH, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 65: Blue Dragon
    {{"e73a", "", "", ""}, "e73a", 600, MONSTER_FAMILY_DINO, {125, 50, 100, 100, 100}, 17.5f, 5, 30, {20, -1}, 12, {0, 0}, 18, 50, MONSTER_KIND_NORMAL, {0, 0}, 73, {0, 0}, ITEM_ICE_GEM, 1, 80, 50, ITEM_ATTACH_ATTACK, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.5f},
    // 66: Black Dragon
    {{"e74a", "", "", ""}, "e74a", 900, MONSTER_FAMILY_DINO, {50, 50, 50, 50, 130}, 17.5f, 10, 60, {22, -1}, 20, {0, 0}, 22, 50, MONSTER_KIND_NORMAL, {0, 0}, 74, {0, 0}, ITEM_MIGHTY_HEALING, 1, 50, 40, ITEM_ATTACH_MAGICAL_POWER, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.5f},
    // 67: Mask of Prajna
    {{"e75a", "", "", ""}, "e75a", 375, MONSTER_FAMILY_UNDEAD, {100, 100, 100, 100, 145}, 7.0f, 5, 10, {1, -1}, 12, {0, 0}, 15, 50, MONSTER_KIND_NORMAL, {0, 0}, 75, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 80, 70, ITEM_ATTACH_MAGICAL_POWER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 68: Crescent Baron
    {{"e76a", "", "", ""}, "e76a", 450, MONSTER_FAMILY_SKY, {100, 100, 100, 110, 100}, 6.0f, 5, 10, {21, -1}, 12, {0, 0}, 18, 50, MONSTER_KIND_NORMAL, {0, 0}, 76, {0, 0}, -1, 1, 80, 70, ITEM_MELLOW_BANANA, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 69: Rockanoff
    {{"e77a", "", "", ""}, "e77a", 30, MONSTER_FAMILY_STONE, {100, 100, 100, 100, 100}, 6.5f, 5, 20, {-1, -1}, 3, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 77, {0, 0}, ITEM_STONE, 1, 90, 60, ITEM_STONE, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.800000012f},
    // 70: King Mimic (Wise Owl Forest)
    {{"e78a", "", "", ""}, "e78a", 150, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 5, 10, {-1, -1}, 10, {0, 0}, 15, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 78, {0, 0}, ITEM_ESCAPE_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 71: Mimic (Wise Owl Forest)
    {{"e79a", "", "", ""}, "e79a", 90, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 2, 10, {-1, -1}, 3, {0, 0}, 6, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 79, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 72: King Mimic (Shipwreck)
    {{"e80a", "", "", ""}, "e80a", 300, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 5, 20, {-1, -1}, 15, {0, 0}, 15, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 80, {0, 0}, ITEM_ESCAPE_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 73: Mimic (Shipwreck)
    {{"e81a", "", "", ""}, "e81a", 150, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 5, 20, {-1, -1}, 4, {0, 0}, 6, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 81, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 74: King Mimic (Gallery of Time)
    {{"e82a", "", "", ""}, "e82a", 675, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 5, 30, {-1, -1}, 18, {0, 0}, 25, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 82, {0, 0}, ITEM_ESCAPE_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 75: Mimic (Gallery of Time)
    {{"e83a", "", "", ""}, "e83a", 450, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 5, 20, {-1, -1}, 6, {0, 0}, 20, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 83, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 76: Ice Arrow
    {{"korinoya", "", "", ""}, "c13_korinoya", 100, MONSTER_FAMILY_MAGE, {200, 0, 100, 100, 100}, 2.0f, 5, 0, {-1, -1}, 17, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 84, {0, 0}, -1, 0, 70, 0, -1, {0, 0, 0, 100, 0, 0}, {0, 0}, 0.0f},
    // 77: Sam
    {{"e86a", "", "", ""}, "e86a", 180, MONSTER_FAMILY_MAGE, {200, 0, 100, 100, 100}, 5.0f, 0, 0, {20, 17}, 4, {0, 0}, 8, 30, MONSTER_KIND_NORMAL, {0, 0}, 85, {0, 0}, ITEM_ICE_GEM, 1, 100, 70, ITEM_ATTACH_ICE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 78: Dran
    {{"c12a", "", "", ""}, "c12a", 250, MONSTER_FAMILY_BEAST, {100, 150, 100, 100, 50}, 45.0f, 10, 20, {13, -1}, 10, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 112, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 79: Master Utan
    {{"c14a", "c14b", "c14c", "c14d"}, "c14a", 700, MONSTER_FAMILY_BEAST, {100, 100, 100, 100, 100}, 35.0f, 12, 0, {14, -1}, 20, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 114, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 80: Ice Queen
    {{"c13a", "", "", ""}, "c13a", 700, MONSTER_FAMILY_MAGE, {150, -50, 80, 80, 120}, 13.0f, 10, 0, {-1, -1}, 30, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 113, {0, 0}, -1, 0, 40, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 81: King's Curse Coffin
    {{"c15a", "", "", ""}, "c15a", 2000, MONSTER_FAMILY_UNDEAD, {110, 100, 100, 150, 125}, 6.0f, 10, 40, {-1, -1}, 40, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 115, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 82: King's Curse
    {{"c15b", "", "", ""}, "c15b", 1000, MONSTER_FAMILY_UNDEAD, {100, 100, 100, 100, 100}, 4.0f, 10, 0, {-1, -1}, 40, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 100, {0, 0}, -1, 0, 0, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 83: Minotaur Joe
    {{"c16a", "", "", ""}, "c16a", 2000, MONSTER_FAMILY_BEAST, {100, 100, 150, 100, 100}, 25.0f, 12, 40, {-1, -1}, 50, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 116, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 84: Dark Genie
    {{"c17a", "", "", ""}, "c17a", 2000, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 120}, 14.0f, 25, 30, {5, -1}, 60, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 117, {0, 0}, -1, 0, 30, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 85: Dark Genie Right Hand
    {{"c17b", "", "", ""}, "c17b", 3200, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 120}, 8.0f, 0, 20, {-1, -1}, 20, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 118, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 86: Dark Genie Left Hand
    {{"c17c", "", "", ""}, "c17c", 3200, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 120}, 8.0f, 0, 20, {-1, -1}, 20, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 119, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 87: (DG effect c17_)
    {{"c17_hikari", "", "", ""}, "c17_hikari", 90, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 100}, 5.0f, 0, 20, {-1, -1}, 20, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 88: (DG companion c17_)
    {{"c17_kaze", "", "", ""}, "c17_kaze", 90, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 100}, 5.0f, 0, 20, {-1, -1}, 20, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 89: (DG companion c17_)
    {{"c17_beem", "", "", ""}, "c17_beem", 90, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 100}, 5.0f, 0, 20, {-1, -1}, 17, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 90: (DG companion c17_)
    {{"c17_beem_s", "", "", ""}, "c17_beem_s", 90, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 100}, 5.0f, 0, 20, {-1, -1}, 20, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 91: Wine Keg
    {{"e85a", "", "", ""}, "e85a", 80, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 100}, 7.0f, 0, 0, {-1, -1}, 0, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 121, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 1.0f},
    // 92: Ice Aura
    {{"b3_reiki", "", "", ""}, "c13_reiki", 80, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 100}, 5.0f, 0, 0, {-1, -1}, 0, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 93: (DG companion c17_)
    {{"c17_syougeki", "", "", ""}, "c17_syougeki", 80, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 100}, 5.0f, 0, 0, {-1, -1}, 0, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 94: Gol (drops items, unlike retail)
    {{"e90a", "", "", ""}, "e90a", 600, MONSTER_FAMILY_STONE, {120, 90, 100, 100, 100}, 14.0f, 8, 0, {7, -1}, 5, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 90, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 50, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.5f},
    // 95: Sil (drops items, unlike retail)
    {{"e91a", "", "", ""}, "e91a", 500, MONSTER_FAMILY_STONE, {90, 120, 100, 100, 100}, 14.0f, 10, 0, {7, -1}, 5, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 91, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 50, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.5f},
    // 96: Yammich
    {{"e101a", "", "", ""}, "e101a", 13, MONSTER_FAMILY_UNDEAD, {100, 100, 100, 70, 130}, 7.0f, 0, 1, {-1, -1}, 3, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 301, {0, 0}, ITEM_STONE, 1, 90, 100, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 97: Statue Dog
    {{"e103a", "", "", ""}, "e103a", 15, MONSTER_FAMILY_STONE, {100, 100, 100, 100, 60}, 9.0f, 3, 10, {-1, -1}, 2, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 303, {0, 0}, ITEM_STONE, 1, 90, 100, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.600000024f},
    // 98: Opar
    {{"e104a", "", "", ""}, "e104a", 28, MONSTER_FAMILY_SEA, {100, 60, 130, 100, 100}, 15.0f, 1, 2, {29, -1}, 3, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 304, {0, 0}, ITEM_ROTTEN_FISH, 1, 90, 100, ITEM_PETITE_FISH, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.0f},
    // 99: Haley Holey
    {{"e105a", "", "", ""}, "e105a", 50, MONSTER_FAMILY_PLANT, {140, 100, 100, 100, 100}, 7.0f, 3, 10, {-1, -1}, 3, {0, 0}, 7, 40, MONSTER_KIND_NORMAL, {0, 0}, 305, {0, 0}, ITEM_CARROT, 1, 100, 70, ITEM_BATTAN, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 100: King Prickly
    {{"e106a", "", "", ""}, "e106a", 63, MONSTER_FAMILY_BEAST, {150, 100, 100, 100, 100}, 6.0f, 3, 10, {-1, -1}, 3, {0, 0}, 7, 40, MONSTER_KIND_NORMAL, {0, 0}, 306, {0, 0}, -1, 1, 100, 70, ITEM_PRICKLY, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.0f},
    // 101: Ice Barrier
    {{"baria", "", "", ""}, "c13_baria", 100, MONSTER_FAMILY_MAGE, {200, 0, 100, 100, 100}, 2.0f, 5, 0, {-1, -1}, 17, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 70, 0, -1, {0, 0, 0, 100, 0, 0}, {0, 0}, 0.0f},
    // 102: Ice Prison
    {{"kori", "", "", ""}, "c13_kori", 100, MONSTER_FAMILY_MAGE, {200, 0, 100, 100, 100}, 2.0f, 5, 0, {-1, -1}, 17, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 70, 0, -1, {0, 0, 0, 100, 0, 0}, {0, 0}, 0.0f},
    // 103: Ice Meteor
    {{"i_meteo", "", "", ""}, "c13_i_meteo", 100, MONSTER_FAMILY_MAGE, {200, 0, 100, 100, 100}, 2.0f, 5, 0, {-1, -1}, 17, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 70, 0, -1, {0, 0, 0, 100, 0, 0}, {0, 0}, 0.0f},
    // 104: Ice Tornado
    {{"i_tatumaki", "", "", ""}, "c13_i_tatumaki", 100, MONSTER_FAMILY_MAGE, {200, 0, 100, 100, 100}, 2.0f, 5, 0, {-1, -1}, 17, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 70, 0, -1, {0, 0, 0, 100, 0, 0}, {0, 0}, 0.0f},
    // 105: Gacious
    {{"e124a", "", "", ""}, "e124a", 1800, MONSTER_FAMILY_UNDEAD, {70, 100, 100, 100, 140}, 14.0f, 8, 0, {-1, -1}, 5, {0, 0}, 5, 30, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 317, {0, 0}, -1, 0, 100, 90, -1, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 106: Dark Genie (Final Form)
    {{"c23a", "", "", ""}, "c23a", 5000, MONSTER_FAMILY_UNDEAD, {70, 100, 100, 100, 140}, 14.0f, 8, 0, {26, -1}, 5, {0, 0}, 5, 30, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 223, {0, 0}, -1, 0, 100, 0, -1, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.0f},
    // 107: DG Final summon (last_mc)
    {{"last_mc", "", "", ""}, "c23_hasira", 100, MONSTER_FAMILY_UNDEAD, {70, 100, 100, 100, 140}, 14.0f, 8, 0, {-1, -1}, 5, {0, 0}, 5, 30, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 100, 90, -1, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 108: DG Final ground wave (last_gw1)
    {{"last_gw1", "", "", ""}, "c23_syougeki", 100, MONSTER_FAMILY_UNDEAD, {70, 100, 100, 100, 140}, 14.0f, 8, 0, {-1, -1}, 5, {0, 0}, 5, 30, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 100, 90, -1, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 109: DG Final beam
    {{"c23_beem", "", "", ""}, "c23_beem", 90, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 100}, 5.0f, 0, 20, {-1, -1}, 17, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 110: DG Final beam (small)
    {{"c23_beem_s", "", "", ""}, "c23_beem_s", 90, MONSTER_FAMILY_MAGE, {100, 100, 100, 100, 100}, 5.0f, 0, 20, {-1, -1}, 20, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 0, {0, 0}, -1, 0, 50, 0, -1, {0, 0, 0, 0, 0, 0}, {0, 0}, 0.0f},
    // 111: Gemron (Fire)
    {{"e111a", "", "", ""}, "e111a", 2500, MONSTER_FAMILY_DINO, {0, 150, 30, 30, 30}, 6.5f, 10, 10, {5, -1}, 15, {0, 0}, 20, 30, MONSTER_KIND_NORMAL, {0, 0}, 311, {0, 0}, -1, 1, 70, 60, ITEM_FIRE_GEM, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 112: Nikapous
    {{"e108a", "", "", ""}, "e108a", 2350, MONSTER_FAMILY_MAGE, {50, 100, 100, 125, 125}, 8.0f, 10, 10, {5, -1}, 15, {0, 0}, 8, 30, MONSTER_KIND_NORMAL, {0, 0}, 308, {0, 0}, ITEM_ANTICURSEAMULET, 1, 100, 70, ITEM_ATTACH_WIND, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 113: White Fang (Enhanced)
    {{"e125a", "", "", ""}, "e125a", 1750, MONSTER_FAMILY_BEAST, {0, 0, 0, 0, 0}, 7.5f, 10, 10, {-1, -1}, 15, {0, 0}, 12, 30, MONSTER_KIND_NORMAL, {0, 0}, 56, {0, 0}, -1, 1, 100, 70, ITEM_CHEESE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 114: Arthur (Enhanced)
    {{"e126a", "", "", ""}, "e126a", 2900, MONSTER_FAMILY_METAL, {50, 50, 150, 80, 80}, 9.0f, 10, 60, {5, -1}, 20, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 40, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 20}, {0, 0}, 1.0f},
    // 115: Sil (Enhanced) (drops items, unlike retail)
    {{"e127a", "", "", ""}, "e127a", 1500, MONSTER_FAMILY_STONE, {100, 100, 100, 100, 100}, 14.0f, 10, 60, {7, -1}, 15, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 91, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 50, -1, {80, 80, 150, 80, 80, 20}, {0, 0}, 0.5f},
    // 116: Halloween (Enhanced)
    {{"e128a", "", "", ""}, "e128a", 1800, MONSTER_FAMILY_PLANT, {150, 50, 50, 50, 50}, 5.0f, 10, 10, {3, -1}, 15, {0, 0}, 7, 40, MONSTER_KIND_NORMAL, {0, 0}, 10, {0, 0}, ITEM_BOMB_NUTS, 1, 100, 70, ITEM_BREAD, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 117: Master Jacket (Enhanced)
    {{"e129a", "", "", ""}, "e129a", 2000, MONSTER_FAMILY_UNDEAD, {110, 80, 100, 80, 130}, 6.0f, 10, 10, {-1, -1}, 15, {0, 0}, 7, 50, MONSTER_KIND_NORMAL, {0, 0}, 1, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 80, ITEM_STAMINA_DRINK, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 118: Vulcan (Enhanced)
    {{"e130a", "", "", ""}, "e130a", 2400, MONSTER_FAMILY_STONE, {0, 180, 100, 100, 100}, 7.0f, 10, 40, {-1, -1}, 15, {0, 0}, 4, 30, MONSTER_KIND_NORMAL, {0, 0}, 70, {0, 0}, ITEM_ATTACH_FIRE, 1, 100, 70, ITEM_STONE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 119: Mummy (Enhanced)
    {{"e131a", "", "", ""}, "e131a", 1500, MONSTER_FAMILY_UNDEAD, {150, 50, 100, 100, 120}, 4.0f, 10, 10, {-1, -1}, 5, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 50, {0, 0}, -1, 1, 100, 70, ITEM_ANTICURSEAMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 120: Diamond (Enhanced)
    {{"e132a", "", "", ""}, "e132a", 1750, MONSTER_FAMILY_MAGE, {100, 100, 50, 150, 100}, 5.0f, 10, 50, {-1, -1}, 10, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 46, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 80, 50, ITEM_ANTIDOTE_AMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 121: Gemron (Ice)
    {{"e112a", "", "", ""}, "e112a", 4000, MONSTER_FAMILY_DINO, {150, 0, 30, 30, 30}, 6.5f, 15, 10, {20, -1}, 20, {0, 0}, 20, 30, MONSTER_KIND_NORMAL, {0, 0}, 312, {0, 0}, -1, 1, 70, 60, ITEM_ICE_GEM, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 122: Horn Head
    {{"e119a", "", "", ""}, "e119a", 2500, MONSTER_FAMILY_UNDEAD, {100, 20, 20, 20, 150}, 10.0f, 15, 10, {-1, -1}, 20, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 319, {0, 0}, -1, 1, 100, 100, ITEM_CARROT, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 123: Auntie Medu (Enhanced)
    {{"e133a", "", "", ""}, "e133a", 3750, MONSTER_FAMILY_DINO, {30, 150, 30, 30, 30}, 6.0f, 15, 10, {11, -1}, 20, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 26, {0, 0}, ITEM_THROBBING_CHERRY, 1, 100, 60, ITEM_ICE_BLOCK, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 124: Rockanoff (Enhanced)
    {{"e134a", "", "", ""}, "e134a", 2500, MONSTER_FAMILY_STONE, {100, 100, 100, 100, 100}, 6.5f, 15, 50, {-1, -1}, 20, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 77, {0, 0}, ITEM_STONE, 1, 90, 60, ITEM_STONE, {80, 80, 150, 80, 80, 20}, {0, 0}, 0.800000012f},
    // 125: Yammich (Enhanced)
    {{"e135a", "", "", ""}, "e135a", 3000, MONSTER_FAMILY_UNDEAD, {20, 20, 20, 20, 130}, 7.0f, 15, 10, {-1, -1}, 20, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 301, {0, 0}, ITEM_STONE, 1, 90, 100, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 126: Witch Hellza (Enhanced) (drops items, unlike retail)
    {{"e136a", "", "", ""}, "e136a", 1500, MONSTER_FAMILY_MAGE, {50, 50, 50, 50, 50}, 8.0f, 15, 10, {5, -1}, 20, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 21, {0, 0}, ITEM_POISONOUS_APPLE, 1, 85, 50, ITEM_ATTACH_MAGICAL_POWER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 127: Steel Giant (Enhanced)
    {{"e137a", "", "", ""}, "e137a", 3900, MONSTER_FAMILY_METAL, {80, 80, 150, 80, 80}, 14.0f, 15, 70, {7, -1}, 25, {0, 0}, 15, 50, MONSTER_KIND_NORMAL, {0, 0}, 64, {0, 0}, ITEM_REPAIR_POWDER, 1, 95, 50, ITEM_MIGHTY_HEALING, {80, 80, 150, 80, 80, 20}, {0, 0}, 1.0f},
    // 128: Club (Enhanced)
    {{"e138a", "", "", ""}, "e138a", 2525, MONSTER_FAMILY_MAGE, {150, 100, 100, 50, 100}, 5.0f, 15, 10, {-1, -1}, 20, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 45, {0, 0}, ITEM_PREMIUM_WATER, 1, 80, 50, ITEM_ANTIGOO_AMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 129: Corcea (Enhanced)
    {{"e139a", "", "", ""}, "e139a", 3250, MONSTER_FAMILY_UNDEAD, {100, 100, 100, 100, 130}, 6.0f, 15, 10, {-1, -1}, 20, {0, 0}, 8, 30, MONSTER_KIND_NORMAL, {0, 0}, 28, {0, 0}, ITEM_HOLY_WATER, 1, 100, 70, ITEM_ATTACH_ATTACK, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 130: Mimic (Demon Shaft)
    {{"e109a", "", "", ""}, "e109a", 3500, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 15, 10, {-1, -1}, 10, {0, 0}, 26, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 309, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 131: King Mimic (Demon Shaft)
    {{"e110a", "", "", ""}, "e110a", 5000, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 15, 50, {-1, -1}, 20, {0, 0}, 35, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 310, {0, 0}, ITEM_ESCAPE_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 150, 100, 100, 20}, {0, 0}, 1.0f},
    // 132: Gemron (Thunder)
    {{"e113a", "", "", ""}, "e113a", 5500, MONSTER_FAMILY_DINO, {30, 30, 0, 150, 30}, 6.5f, 20, 10, {23, -1}, 25, {0, 0}, 20, 30, MONSTER_KIND_NORMAL, {0, 0}, 313, {0, 0}, -1, 1, 70, 60, ITEM_THUNDER_GEM, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 133: Bishop Q
    {{"e116a", "", "", ""}, "e116a", 6000, MONSTER_FAMILY_MAGE, {40, 40, 40, 40, 140}, 11.0f, 20, 10, {11, 5}, 25, {0, 0}, 20, 30, MONSTER_KIND_NORMAL, {0, 0}, 316, {0, 0}, -1, 1, 50, 50, ITEM_ATTACH_MAGICAL_POWER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 134: Cave Bat (Enhanced) (drops items, unlike retail)
    {{"e140a", "", "", ""}, "e140a", 1500, MONSTER_FAMILY_SKY, {100, 100, 100, 150, 100}, 3.0f, 20, 10, {-1, -1}, 25, {0, 0}, 4, 30, MONSTER_KIND_NORMAL, {0, 0}, 60, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 100, 90, 0, {100, 150, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 135: Gol (Enhanced) (drops items, unlike retail)
    {{"e141a", "", "", ""}, "e141a", 6000, MONSTER_FAMILY_STONE, {120, 90, 100, 100, 100}, 14.0f, 30, 80, {7, -1}, 25, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 90, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 50, -1, {100, 100, 150, 100, 100, 20}, {0, 0}, 0.5f},
    // 136: Mask of Prajna (Enhanced)
    {{"e142a", "", "", ""}, "e142a", 5500, MONSTER_FAMILY_UNDEAD, {100, 100, 100, 100, 145}, 7.5f, 20, 10, {1, -1}, 25, {0, 0}, 15, 50, MONSTER_KIND_NORMAL, {0, 0}, 75, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 80, 70, ITEM_ATTACH_MAGICAL_POWER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 137: Gyon (Enhanced)
    {{"e143a", "", "", ""}, "e143a", 5750, MONSTER_FAMILY_SEA, {120, 100, 150, 100, 100}, 7.0f, 20, 10, {2, -1}, 25, {0, 0}, 8, 30, MONSTER_KIND_NORMAL, {0, 0}, 24, {0, 0}, ITEM_ANTIGOO_AMULET, 1, 100, 70, ITEM_FLAPPING_FISH, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 138: Spade (Enhanced)
    {{"e144a", "", "", ""}, "e144a", 5000, MONSTER_FAMILY_MAGE, {150, 50, 100, 100, 100}, 5.0f, 20, 10, {-1, -1}, 25, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 47, {0, 0}, ITEM_HOLY_WATER, 1, 80, 50, ITEM_ANTI_FREEZE_AMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 139: Rash Dasher (Enhanced)
    {{"e145a", "", "", ""}, "e145a", 5000, MONSTER_FAMILY_BEAST, {50, 150, 100, 100, 100}, 6.0f, 20, 10, {-1, -1}, 25, {0, 0}, 12, 30, MONSTER_KIND_NORMAL, {0, 0}, 63, {0, 0}, ITEM_PREMIUM_CHICKEN, 1, 100, 70, ITEM_ATTACH_SPEED, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 140: Captain (Enhanced)
    {{"e146a", "", "", ""}, "e146a", 4000, MONSTER_FAMILY_UNDEAD, {110, 100, 80, 80, 150}, 6.0f, 20, 10, {-1, -1}, 25, {0, 0}, 12, 30, MONSTER_KIND_NORMAL, {0, 0}, 27, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 70, ITEM_ROTTEN_FISH, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 141: Mimic (Demon Shaft) (Enhanced)
    {{"e109a", "", "", ""}, "e147a", 5000, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 20, 10, {-1, -1}, 10, {0, 0}, 26, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 309, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 142: King Mimic (Demon Shaft) (Enhanced)
    {{"e110a", "", "", ""}, "e148a", 7500, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 20, 50, {-1, -1}, 20, {0, 0}, 35, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 310, {0, 0}, ITEM_ESCAPE_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 150, 100, 100, 20}, {0, 0}, 1.0f},
    // 143: Gemron (Wind)
    {{"e114a", "", "", ""}, "e114a", 8000, MONSTER_FAMILY_DINO, {100, 100, 140, 0, 100}, 6.5f, 23, 10, {24, -1}, 30, {0, 0}, 20, 30, MONSTER_KIND_NORMAL, {0, 0}, 314, {0, 0}, -1, 1, 70, 60, ITEM_WIND_GEM, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 144: Silver Gear
    {{"e118a", "", "", ""}, "e118a", 2500, MONSTER_FAMILY_UNDEAD, {30, 30, 30, 30, 150}, 10.0f, 23, 10, {28, -1}, 30, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 318, {0, 0}, -1, 1, 100, 100, ITEM_PETITE_FISH, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 145: Alexander (Enhanced)
    {{"e149a", "", "", ""}, "e149a", 7500, MONSTER_FAMILY_METAL, {100, 100, 120, 100, 100}, 7.0f, 23, 50, {5, -1}, 30, {0, 0}, 17, 50, MONSTER_KIND_NORMAL, {0, 0}, 43, {0, 0}, ITEM_WIND_GEM, 1, 100, 70, ITEM_ATTACH_FIRE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 146: Heart (Enhanced)
    {{"e150a", "", "", ""}, "e150a", 5000, MONSTER_FAMILY_MAGE, {0, 0, 0, 0, 0}, 5.0f, 23, 10, {10, 12}, 30, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 44, {0, 0}, ITEM_STAMINA_DRINK, 1, 80, 50, ITEM_ANTICURSEAMULET, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 147: Bomber Head (Enhanced)
    {{"e151a", "", "", ""}, "e151a", 6000, MONSTER_FAMILY_MAGE, {200, 20, 20, 20, 20}, 4.0f, 23, 20, {16, -1}, 30, {0, 0}, 10, 30, MONSTER_KIND_NORMAL, {0, 0}, 49, {0, 0}, ITEM_BOMB, 1, 100, 70, ITEM_BOMB, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 148: Crabby Hermit (Enhanced)
    {{"e152a", "", "", ""}, "e152a", 6500, MONSTER_FAMILY_SEA, {100, 100, 125, 100, 100}, 10.0f, 23, 20, {6, -1}, 30, {0, 0}, 12, 30, MONSTER_KIND_NORMAL, {0, 0}, 71, {0, 0}, ITEM_THROBBING_CHERRY, 1, 95, 70, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 149: Cursed Rose (Enhanced)
    {{"e153a", "", "", ""}, "e153a", 5000, MONSTER_FAMILY_PLANT, {130, 100, 100, 100, 150}, 6.5f, 23, 10, {2, -1}, 30, {0, 0}, 6, 30, MONSTER_KIND_NORMAL, {0, 0}, 68, {0, 0}, -1, 1, 100, 70, ITEM_TASTY_WATER, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.0f},
    // 150: Pirate's Chariot (Enhanced)
    {{"e154a", "", "", ""}, "e154a", 6750, MONSTER_FAMILY_METAL, {120, 80, 140, 100, 100}, 8.0f, 23, 60, {15, -1}, 30, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 25, {0, 0}, ITEM_BOMB, 1, 95, 60, ITEM_ATTACH_ENDURANCE, {80, 80, 150, 80, 20, 100}, {0, 0}, 0.5f},
    // 151: Space Gyon (Enhanced)
    {{"e155a", "", "", ""}, "e155a", 7800, MONSTER_FAMILY_SEA, {0, 0, 20, 0, 0}, 5.0f, 23, 10, {2, -1}, 30, {0, 0}, 4, 30, MONSTER_KIND_NORMAL, {0, 0}, 72, {0, 0}, ITEM_SOAP, 1, 100, 70, ITEM_FLAPPING_FISH, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 152: Mimic (Demon Shaft) (Enhanced x2)
    {{"e109a", "", "", ""}, "e156a", 6500, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 23, 10, {-1, -1}, 15, {0, 0}, 26, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 309, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 153: King Mimic (Demon Shaft) (Enhanced x2)
    {{"e110a", "", "", ""}, "e157a", 10000, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 23, 50, {-1, -1}, 25, {0, 0}, 35, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 310, {0, 0}, ITEM_ESCAPE_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 150, 100, 100, 20}, {0, 0}, 1.0f},
    // 154: Gemron (Holy)
    {{"e115a", "", "", ""}, "e115a", 12500, MONSTER_FAMILY_DINO, {50, 50, 50, 50, 0}, 6.5f, 30, 10, {25, -1}, 35, {0, 0}, 20, 30, MONSTER_KIND_NORMAL, {0, 0}, 315, {0, 0}, -1, 1, 70, 60, ITEM_HOLY_GEM, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 155: Gacious (Enhanced)
    {{"e117a", "", "", ""}, "e117a", 15000, MONSTER_FAMILY_UNDEAD, {0, 50, 50, 50, 140}, 11.0f, 30, 10, {-1, -1}, 35, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 317, {0, 0}, ITEM_BATTAN, 1, 100, 90, -1, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 156: Evil Bat (Enhanced) (drops items, unlike retail)
    {{"e158a", "", "", ""}, "e158a", 7500, MONSTER_FAMILY_SKY, {150, 150, 150, 150, 200}, 3.0f, 30, 10, {-1, -1}, 35, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 61, {0, 0}, ITEM_ANTIDOTE_DRINK, 1, 100, 70, ITEM_PREMIUM_CHICKEN, {100, 200, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 157: Crescent Baron (Enhanced)
    {{"e159a", "", "", ""}, "e159a", 16000, MONSTER_FAMILY_SKY, {100, 100, 100, 110, 100}, 6.0f, 30, 10, {21, -1}, 35, {0, 0}, 18, 50, MONSTER_KIND_NORMAL, {0, 0}, 76, {0, 0}, -1, 1, 80, 70, ITEM_MELLOW_BANANA, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 158: Statue Dog (Enhanced)
    {{"e160a", "", "", ""}, "e160a", 12500, MONSTER_FAMILY_STONE, {100, 100, 100, 100, 60}, 9.0f, 30, 10, {-1, -1}, 35, {0, 0}, 5, 30, MONSTER_KIND_NORMAL, {0, 0}, 303, {0, 0}, ITEM_STONE, 1, 90, 100, ITEM_ATTACH_ENDURANCE, {100, 100, 100, 100, 100, 100}, {0, 0}, 0.600000024f},
    // 159: Joker (Enhanced)
    {{"e161a", "", "", ""}, "e161a", 9500, MONSTER_FAMILY_MAGE, {50, 50, 50, 50, 150}, 5.0f, 30, 10, {-1, -1}, 35, {0, 0}, 12, 50, MONSTER_KIND_NORMAL, {0, 0}, 48, {0, 0}, ITEM_PREMIUM_CHICKEN, 1, 50, 10, ITEM_MIGHTY_HEALING, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 160: Lich (Enhanced) (drops items, unlike retail)
    {{"e162a", "", "", ""}, "e162a", 10000, MONSTER_FAMILY_UNDEAD, {20, 20, 20, 20, 160}, 4.0f, 30, 10, {11, -1}, 35, {0, 0}, 15, 80, MONSTER_KIND_NORMAL, {0, 0}, 51, {0, 0}, ITEM_REVIVAL_POWDER, 1, 80, 30, ITEM_ATTACH_MAGICAL_POWER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 161: Titan (Enhanced)
    {{"e163a", "", "", ""}, "e163a", 11500, MONSTER_FAMILY_STONE, {100, 100, 110, 110, 100}, 14.0f, 30, 50, {7, -1}, 35, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 33, {0, 0}, ITEM_REPAIR_POWDER, 1, 100, 70, ITEM_STONE, {100, 100, 150, 100, 100, 20}, {0, 0}, 0.5f},
    // 162: Living Armor (Enhanced)
    {{"e164a", "", "", ""}, "e164a", 9500, MONSTER_FAMILY_STONE, {100, 100, 100, 80, 80}, 4.0f, 30, 50, {-1, -1}, 35, {0, 0}, 15, 30, MONSTER_KIND_NORMAL, {0, 0}, 55, {0, 0}, -1, 1, 100, 50, ITEM_STONE, {100, 100, 150, 100, 100, 20}, {0, 0}, 1.0f},
    // 163: Mimic (Demon Shaft) (Enhanced x3)
    {{"e109a", "", "", ""}, "e165a", 7500, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 5.0f, 30, 10, {-1, -1}, 20, {0, 0}, 26, 80, MONSTER_KIND_MIMIC_SMALL, {0, 0}, 309, {0, 0}, ITEM_REPAIR_POWDER, 1, 90, 50, ITEM_DRAN_S_FEATHER, {100, 100, 100, 100, 100, 100}, {0, 0}, 1.0f},
    // 164: King Mimic (Demon Shaft) (Enhanced x3)
    {{"e110a", "", "", ""}, "e166a", 19500, MONSTER_FAMILY_MIMIC, {100, 100, 100, 100, 100}, 12.0f, 30, 50, {-1, -1}, 30, {0, 0}, 35, 80, MONSTER_KIND_MIMIC_LARGE, {0, 0}, 310, {0, 0}, ITEM_ESCAPE_POWDER, 1, 90, 50, ITEM_TREASURE_KEY, {100, 100, 150, 100, 100, 20}, {0, 0}, 1.0f},
    // 165: Black Knight
    {{"c21a", "", "", ""}, "c21a", 40000, MONSTER_FAMILY_METAL, {100, 100, 100, 100, 100}, 14.0f, 8, 100, {30, 33}, 5, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 221, {0, 0}, -1, 0, 50, 0, -1, {100, 100, 100, 100, 100, 50}, {0, 0}, 0.0f},
    // 166: Black Knight Mount
    {{"c22a", "", "", ""}, "c22a", 50000, MONSTER_FAMILY_METAL, {100, 100, 100, 100, 100}, 14.0f, 8, 100, {31, 32}, 5, {0, 0}, 0, 0, MONSTER_KIND_NO_LOCK_ON, {0, 0}, 221, {0, 0}, -1, 0, 50, 0, -1, {100, 100, 100, 100, 100, 50}, {0, 0}, 0.0f},
};
// clang-format on

// Steps the floor has run unpaused: DunMoveChara advances it each step CMonstorUnit::Step runs
// unpaused, whether or not any monster takes part.
static int monster_step_clock;

void MonstorStepClockAdvance() {
    monster_step_clock++;
}

// Steps poison lasts: ten of its 180-step ticks, 30 seconds.
static const int kPoisonSteps = 1800;

// monster_step_clock when each monster was poisoned; read only while its poison_timer runs.
static int poison_start[16];

// Takes hold of a monster with poison. A monster already poisoned keeps its poison as it is, so
// the hit neither restarts its ticks nor lengthens it.
static void Poison(MONSTOR *target, int slot) {
    if (target->poison_timer <= 0) {
        target->poison_timer = 180;
        poison_start[slot] = monster_step_clock;
    }

    target->slow_timer = 0;
}

// Retail's CheckDmg, which deals the monster the hits that reach it, with two changes.
//
// The weapon wear of a landed hit: Ungaga's held charge pays for each shot as it fires it
// (UngagaKey_Play), so the hits its shots land wear nothing more. A guarded hit wears the weapon
// only for an owner's attack 0, which leaves the charge shots (attack 2) out already.
//
// Poison: it lasts kPoisonSteps of monster_step_clock from the hit that poisoned the monster,
// counted on while the monster is out of view, so one kept in view takes its ten ticks and is then
// cured, and one that comes back into view later is cured without a tick. Each tick takes a tenth
// of its life scaled by status_chance (1 at the least), and death ends the poison.
PORT_OVERRIDE int CMonstorUnit::CheckDmg() {
    int            result = MONSTER_DMG_NONE;
    COLLISION_HIT *record;
    int            immune = 0;
    sceVu0FVECTOR  direction, position;

    if (monster[current_monster].stop_timer <= 0) {
        for (int i = 0; i < 16; i++) {
            if (effect2[current_monster].active[i] != 0) {
                float time = chara[current_monster][0].motion_type.state.time;

                if (effect2[current_monster].motion_start[i] < time && !(effect2[current_monster].motion_end[i] <= time)) {
                    int damage = effect2[current_monster].damage[i];

                    if (monster[current_monster].anger_timer > 0) {
                        damage *= 2;
                    }

                    int hit = NowColData->Set(effect2[current_monster].position[i], damage, 2, effect2[current_monster].radius[i], 0.0f, 1, effect2[current_monster].kind[i], effect2[current_monster].flags[i], 0);
                    NowColData->SetUserID(current_monster * 5 + 200, i);

                    if (effect2[current_monster].kind[i] == 3) {
                        float angle = effect2[current_monster].angle[i];

                        if (angle == 0.0f) {
                            sceVu0CopyVector(direction, CharaMain.pos);
                            chara[current_monster][0].GetPosition(position);
                            direction[0] -= position[0];
                            direction[1] = 0;
                            direction[2] -= position[2];
                            direction[3] = 1;
                            sceVu0Normalize(direction, direction);
                            NowColData->SetVelocity(hit, direction, 1.0f);
                        } else {
                            if (!(angle < 180.0f)) {
                                angle -= 360.0f;
                            }

                            angle = DEG_TO_RAD * angle;
                            angle += chara[current_monster][0].GetRotation()->y;

                            if (!(angle <= TWO_PI)) {
                                angle -= TWO_PI;
                            }

                            if (angle < -PI) {
                                angle += TWO_PI;
                            }

                            sceVu0FVECTOR forward = {0, 0, 1, 0};
                            sceVu0FMATRIX rotation_matrix, identity_matrix;
                            sceVu0UnitMatrix(identity_matrix);
                            sceVu0RotMatrixY(rotation_matrix, identity_matrix, angle);
                            sceVu0ApplyMatrix(forward, rotation_matrix, forward);
                            NowColData->SetVelocity(hit, forward, 1.0f);
                        }
                    }
                }
            }
        }
    }

    sceVu0FVECTOR poison_position, guard_position, guard_origin, guard_direction;
    sceVu0FVECTOR steal_position, knockback_position, knockback_origin, hit_direction, player_position;

    if (monster[current_monster].hp <= 0) {
        if (monster[current_monster].stop_timer > 0) {
            monster[current_monster].stop_timer = 0;
            monster[current_monster].motion_reset_pending = true;
        }

        monster[current_monster].poison_timer = 0;
        return MONSTER_DMG_NONE;
    }

    monster[current_monster].last_hit_id = -1;

    if (monster[current_monster].stop_timer > 0) {
        monster[current_monster].palette_override[0] = 160.0f;
        monster[current_monster].palette_override[1] = 160.0f;
        monster[current_monster].palette_override[2] = 160.0f;
        monster[current_monster].palette_override_pending = true;
        monster[current_monster].stop_timer--;

        if (monster[current_monster].stop_timer == 0) {
            monster[current_monster].motion_reset_pending = true;
        }
    }

    if (monster[current_monster].slow_timer > 0) {
        monster[current_monster].palette_override[0] = 25.0f;
        monster[current_monster].palette_override[1] = 37.5f;
        monster[current_monster].palette_override[2] = 63.75f;
        monster[current_monster].palette_override_pending = true;
    }

    if (monster[current_monster].anger_timer > 0) {
        monster[current_monster].palette_override[0] = 127.5f;
        monster[current_monster].palette_override[1] = 80.0f;
        monster[current_monster].palette_override[2] = 15.0f;
        monster[current_monster].palette_override_pending = true;
        monster[current_monster].anger_timer--;
    }

    if (monster[current_monster].poison_timer > 0 && monster_step_clock - poison_start[current_monster] > kPoisonSteps) {
        monster[current_monster].poison_timer = 0;
    }

    if (monster[current_monster].poison_timer > 0) {
        monster[current_monster].palette_override[0] = 47.0f;
        monster[current_monster].palette_override[1] = 0.5f;
        monster[current_monster].palette_override[2] = 63.75f;
        monster[current_monster].palette_override_pending = true;
        monster[current_monster].poison_timer--;

        if (monster[current_monster].poison_timer == 0) {
            monster[current_monster].poison_timer = 180;
            int damage = monster[current_monster].max_hp * monster[current_monster].status_chance / 1000;

            if (damage < 1) {
                damage = 1;
            }

            monster[current_monster].hp -= damage;
            result = MONSTER_DMG_HIT;

            if (monster[current_monster].hp <= 0) {
                monster[current_monster].hp = 0;
                alive_count--;
                result = MONSTER_DMG_KILLED;
                ((CDngStatusData *) UserStatus)->AddKills();
            }

            chara[current_monster][0].GetPosition(poison_position);
            poison_position[1] += chara[current_monster][0].body_height;
            HitValueEntry(NowHitValue, poison_position, damage, HIT_VALUE_MONSTER, NULL);
        }

        if (monster_step_clock - poison_start[current_monster] >= kPoisonSteps) {
            monster[current_monster].poison_timer = 0;
        }
    }

    if (monster[current_monster].invincible_timer > 0) {
        return result;
    }

    monster[current_monster].invincible_blocked = 0;

    for (int i = 0; i < 16; i++) {
        if (effect[current_monster].timer[i] != 0) {
            int   active = true;
            float incoming_time;

            if (effect[current_monster].motion_start[i] != 0.0f && (!(effect[current_monster].motion_start[i] < (incoming_time = chara[current_monster][0].motion_type.state.time)) || effect[current_monster].motion_end[i] < incoming_time)) {
                active = false;
            }

            if (active != 0) {
                int hit;
                int element;
                int owner;
                hit = NowColData->FindMonsterHit(effect[current_monster].position[i], effect[current_monster].radius[i]);
                int rejected = 0;

                if (hit != -1) {
                    int monster_owner = NowColData->GetMonsterOwner(hit);

                    if (monster_owner == current_monster) {
                        rejected = 1;
                    }

                    if (monster_owner != -1 && monster_owner != current_monster) {
                        COLLISION_HIT *other_record = &(*NowColData->Get(hit));
                        float          chance = 100.0f * (float) rand() / 2147483648.0f;

                        if (other_record->flags & 0x1000) {
                            if (chance < (float) monster[current_monster].status_chance && monster[current_monster].anger_timer == 0) {
                                monster[current_monster].anger_timer = 1800;
                                monster[current_monster].poison_timer = 0;
                                monster[current_monster].slow_timer = 0;
                                SndSePlay(SE_POWER_UP, -1, 0);
                            }
                        }

                        rejected = 1;
                    }
                }

                if (hit != -1 && rejected == 0) {
                    for (int j = 0; j < 3; j++) {
                        if (guard[current_monster].active[j] != 0) {
                            float time = chara[current_monster][0].motion_type.state.time;

                            if (guard[current_monster].motion_start[j] <= time && !(guard[current_monster].motion_end[j] < time)) {
                                record = &(*NowColData->Get(hit));

                                if (record->knockback_mode == 2) {
                                    chara[current_monster][0].GetPosition(guard_position);
                                    sceVu0CopyVector(guard_origin, record->knockback_origin);
                                    guard_position[1] = 0;
                                    guard_origin[1] = 0;
                                    sceVu0SubVector(monster[current_monster].knockback_direction, guard_position, guard_origin);
                                    sceVu0Normalize(monster[current_monster].knockback_direction, monster[current_monster].knockback_direction);
                                    monster[current_monster].knockback[0] = 1.5f * record->knockback_speed * monster[current_monster].knockback[2];
                                    monster[current_monster].knockback[1] = 1.5f * record->knockback_decay * monster[current_monster].knockback[2];
                                }

                                int owner = NowColData->hit[hit].owner;

                                if (NowColData->hit[hit].attack_no == 0 && (owner == 0 || owner == 2 || owner == 4)) {
                                    SwordDmgCheck1(0.1f, monster[current_monster].hardness);
                                }

                                guard_direction[0] = 0;
                                guard_direction[1] = 2.5f;
                                guard_direction[2] = 0;
                                guard_direction[3] = 1;
                                HitMark[hitCnt].Set(effect[current_monster].position[i], guard_direction, HIT_MARK_GUARD, 0.8f, 0.005f, 0.02f, 1.3f, 32, monster[current_monster].ground_y);
                                HitPointMark[hitCnt].Set(effect[current_monster].position[i]);
                                SndSePlay(SE_HIT_BLOCKED, -1, 0);
                                rejected = 1;
                                j = 3;
                            }
                        }
                    }
                }

                int hit_id = -1;

                if (hit != -1 && rejected == 0) {
                    COLLISION_HIT *original_record = &(*NowColData->Get(hit));
                    effect[current_monster].hit_slot = i;
                    int attack = NowColData->hit[hit].attack_no;
                    owner = NowColData->GetUserID(hit);

                    if (owner != -1) {
                        hit_id = owner * 10;
                    }

                    if (attack != -1) {
                        hit_id += attack;
                    }

                    monster[current_monster].last_hit_id = hit_id;
                    effect[current_monster].hit_attributes = NowColData->hit[hit].weapon_flags;
                    int element_flags = NowColData->GetFlags(hit);
                    printf("element = %d\n", element_flags);
                    int item;

                    switch (element_flags) {
                        case HIT_ELEMENT_FIRE:
                            element = WEAPON_ELEMENT_FIRE;
                            break;
                        case HIT_ELEMENT_COLD:
                            element = WEAPON_ELEMENT_COLD;
                            break;
                        case HIT_ELEMENT_THUNDER:
                            element = WEAPON_ELEMENT_THUNDER;
                            break;
                        case HIT_ELEMENT_WIND:
                            element = WEAPON_ELEMENT_WIND;
                            break;
                        case HIT_ELEMENT_HOLY:
                            element = WEAPON_ELEMENT_HOLY;
                            break;
                        default:
                            element = WEAPON_ELEMENT_NONE;
                            break;
                    }

                    monster[current_monster].hit_element = element;

                    if (monster[current_monster].steal_item != -1 && (effect[current_monster].hit_attributes & 0x80) && (int) (100.0f * (float) rand() / 2147483648.0f) < 10 && (item = monster[current_monster].steal_item, ((CDngStatusData *) UserStatus)->CheckItemGet(item)) == 0) {
                        chara[current_monster][0].GetPosition(steal_position);
                        steal_position[1] += 12.0f;
                        StealItem.Set(steal_position, monster[current_monster].steal_item);
                        monster[current_monster].steal_item = -1;
                    }

                    int attacker = NowColData->hit[hit].owner;

                    // A shot of Ungaga's held charge (the shot effect records its hits with a scale
                    // rate, his swings with none) wears nothing here: firing it already did.
                    bool charge_shot = attacker == 4 && original_record->scale_rate != 0.0f;

                    if ((attacker == 0 || attacker == 2 || attacker == 4) && !charge_shot) {
                        SwordDmgCheck1(1.0f, monster[current_monster].hardness);
                    }

                    result = MONSTER_DMG_HIT;
                    int boss = 0;

                    if (selectMapNo == DUNGEON_DIVINE_BEAST_CAVE && UserStatus->cur_floor == 14) {
                        boss = 1;
                    }

                    if (selectMapNo == DUNGEON_GALLERY_OF_TIME && UserStatus->cur_floor == 24) {
                        boss = 1;
                    }

                    if (boss == 0) {
                        if (owner == 5 && NowColData->hit[hit].attack_no == 6) {
                            result = MONSTER_DMG_NONE;
                        }

                        if (owner == 1) {
                            result = MONSTER_DMG_NONE;
                        }
                    }

                    record = &(*NowColData->Get(hit));

                    if (record->knockback_mode == 2) {
                        chara[current_monster][0].GetPosition(knockback_position);
                        sceVu0CopyVector(knockback_origin, record->knockback_origin);
                        knockback_position[1] = 0;
                        knockback_origin[1] = 0;
                        sceVu0SubVector(monster[current_monster].knockback_direction, knockback_position, knockback_origin);
                        sceVu0Normalize(monster[current_monster].knockback_direction, monster[current_monster].knockback_direction);
                        monster[current_monster].knockback[0] = record->knockback_speed * monster[current_monster].knockback[2];
                        monster[current_monster].knockback[1] = record->knockback_decay * monster[current_monster].knockback[2];
                    }

                    hit_direction[0] = 0;
                    hit_direction[1] = 1.1f;
                    hit_direction[2] = 0;
                    hit_direction[3] = 1;
                    const float mark_scale = 1.0f;
                    const float mark_spread = 1.0f / 2.0f;
                    HitMark[hitCnt].Set(effect[current_monster].position[i], hit_direction, HIT_MARK_HIT, mark_spread, 0.01f, 0.02f, mark_scale, 32, monster[current_monster].ground_y);
                    HitPointMark[hitCnt].Set(effect[current_monster].position[i]);

                    if (hitCnt == 15) {
                        hitCnt = 0;
                    } else {
                        hitCnt++;
                    }

                    if (element < WEAPON_ELEMENT_NONE) {
                        CDngStatusData *status = (CDngStatusData *) UserStatus;
                        WEAPON_HAVE    *weapon = &status->chara_weapons[owner][status->equipped_weapon_slot[owner]];
                        float           strength = (float) weapon->elem[weapon->best_elem];
                        static int      cnt = 0;
                        CWeaponElFx[cnt].Set(&effect[current_monster].position[i], effect[current_monster].position[i], strength, element, monster[current_monster].body_radius);

                        if (cnt >= 3) {
                            cnt = 0;
                        } else {
                            cnt++;
                        }
                    }

                    float chance = 100.0f * (float) rand() / 2147483648.0f;

                    if (effect[current_monster].hit_attributes & 0x20) {
                        if (100.0f * (float) rand() / 2147483648.0f <= 10.0f && chance < (float) monster[current_monster].status_chance) {
                            Poison(&monster[current_monster], current_monster);
                        } else if (owner == -1) {
                            immune = 1;
                        }
                    }

                    if (effect[current_monster].hit_attributes & 0x40) {
                        if (100.0f * (float) rand() / 2147483648.0f <= 4.0f && chance < (float) monster[current_monster].status_chance) {
                            if (monster[current_monster].stop_timer <= 0) {
                                monster[current_monster].stop_timer = 300;
                                monster[current_monster].poison_timer = 0;
                                monster[current_monster].slow_timer = 0;
                                monster[current_monster].anger_timer = 0;
                                monster[current_monster].movement_speed = 0;
                            } else {
                                monster[current_monster].stop_timer = 0;
                            }
                        } else if (owner == -1) {
                            immune = 1;
                        }
                    }

                    chance = monster[current_monster].status_chance == 0 ? 100.0f : 0.0f;

                    if (original_record->flags & 0x200) {
                        if (chance < (float) monster[current_monster].status_chance) {
                            if (monster[current_monster].stop_timer == 0 && monster[current_monster].anger_timer == 0) {
                                Poison(&monster[current_monster], current_monster);
                            }
                        } else if (owner == -1) {
                            immune = 1;
                        }
                    }

                    if (original_record->flags & 0x100) {
                        if (chance < (float) monster[current_monster].status_chance) {
                            if (monster[current_monster].stop_timer <= 0) {
                                monster[current_monster].stop_timer = 300;
                                monster[current_monster].poison_timer = 0;
                                monster[current_monster].slow_timer = 0;
                                monster[current_monster].anger_timer = 0;
                                monster[current_monster].movement_speed = 0;
                            } else {
                                monster[current_monster].stop_timer = 0;
                            }
                        } else if (owner == -1) {
                            immune = 1;
                        }
                    }

                    if (original_record->flags & 0x800) {
                        if (chance < (float) monster[current_monster].status_chance) {
                            if (monster[current_monster].stop_timer == 0 && monster[current_monster].poison_timer == 0 && monster[current_monster].anger_timer == 0) {
                                monster[current_monster].slow_timer = 180;
                            }
                        } else if (owner == -1) {
                            immune = 1;
                        }
                    }

                    float damage = (float) record->damage;

                    if (owner == 1 || owner == 3 || owner == 5) {
                        sceVu0CopyVector(player_position, CharaMain.pos);
                        float distance = DistVector(player_position, record->pos);

                        if (distance <= 20.0f) {
                            damage *= 1.5f;
                            printf("c_dist = %.3f\n", 1.5);
                        }

                        if (!(distance < 50.0f)) {
                            distance -= 50.0f;
                            distance = (100.0f - distance) / 100.0f;

                            if (distance < 0.5f) {
                                distance = 0.5f;
                            }

                            damage *= distance;
                            printf("c_dist = %.3f\n", distance);
                        }
                    }

                    int   index = GetCurrentMonsterIndex();
                    float defense = (float) monster[index].defense;

                    if (owner == 3) {
                        defense /= 2.0f;
                    }

                    damage -= defense;

                    if (damage <= 0.0f) {
                        damage = 1.0f;
                    }

                    if (element_flags != 0) {
                        float old_damage = damage;
                        float bonus = 0;

                        if (owner != -1) {
                            WEAPON_HAVE *weapon = &UserStatus->chara_weapons[owner][UserStatus->equipped_weapon_slot[owner]];
                            bonus = damage * (0.005f * (float) weapon->magic + 0.004f * (float) weapon->elem[element]);
                        }

                        damage += bonus;
                        damage *= 0.01f * (float) monster[index].attachment_weight[element];

                        if (damage <= 0.0f && !(old_damage <= 0.0f)) {
                            immune = 1;
                        }
                    }

                    char *effectiveness = record->vs_monster;

                    if (effectiveness != NULL) {
                        damage += damage * (0.015f * (float) effectiveness[monster[index].attachment_kind]);
                    }

                    if (owner != -1) {
                        float old_damage = damage;
                        int   multiplier = effect[index].parameter[i][owner];
                        float damage_scale = (float) multiplier;
                        damage = damage / 100.0f * damage_scale;

                        if (damage <= 0.0f) {
                            damage = 0;
                        }

                        if (damage <= 0.0f && !(old_damage <= 0.0f)) {
                            immune = 1;
                        }
                    }

                    int target_kind = NowColData->hit[hit].target_kind;

                    if (target_kind != -1 && monster[index].attachment_kind != target_kind) {
                        damage = 0;
                        immune = 1;
                    }

                    if (monster[index].anger_timer > 0) {
                        damage /= 2.0f;
                    }

                    if (owner == -1) {
                        damage *= 0.01f * (float) monster[index].item_damage_rate;
                    }

                    if (damage <= 0.0f) {
                        damage = 0;
                    }

                    int amount = (int) damage;

                    if (!(damage - (float) amount <= 0.0f)) {
                        amount++;
                    }

                    if ((effect[index].hit_attributes & 0x400) && !(damage < 100.0f)) {
                        CUserStatus *status = UserStatus;

                        if (status->hp[(int) owner] > 0) {
                            float heal = 0.01f * damage;
                            int   ignored = (int) heal;
                            status->AddNowLife(owner, (short) (int) heal, 255.0f);
                        }
                    }

                    if ((effect[current_monster].hit_attributes & 0x1000) && monster[current_monster].kind != MONSTER_KIND_NO_LOCK_ON && 100.0f * (float) rand() / 2147483648.0f < 1.0f) {
                        amount = monster[current_monster].hp;
                    }

                    if (amount > 0 && (owner == 0 || owner == 2 || owner == 4) && element >= 0 && element < WEAPON_ELEMENT_NONE) {
                        SndSePlay(element + 0x65, -1, 0);
                    }

                    BtActStatus.monstor_target = owner;
                    monster[current_monster].hp -= amount;

                    if (monster[current_monster].hp <= 0) {
                        monster[current_monster].last_attacker = owner;
                        monster[current_monster].hp = 0;
                        alive_count--;
                        result = MONSTER_DMG_KILLED;
                        ((CDngStatusData *) UserStatus)->AddKills();

                        if (monster[current_monster].event_flag2 != -1) {
                            monster[current_monster].event_flag2_pending = 1;
                        }
                    }

                    if (immune == 0) {
                        monster[current_monster].palette_target[0] = 255;
                        monster[current_monster].palette_target[1] = 0;
                        monster[current_monster].palette_target[2] = 0;
                        monster[current_monster].palette_cycles = 1;
                        monster[current_monster].palette_step = 0.08f;
                        monster[current_monster].palette_blend = 0;
                        HitValueEntry(NowHitValue, effect[current_monster].position[i], amount, HIT_VALUE_MONSTER, NULL);
                        SndSePlay(SE_MONSTER_HIT, -1, 0);
                    } else {
                        HitValueEntry(NowHitValue, effect[current_monster].position[i], 0, HIT_VALUE_ZERO, NULL);
                        monster[current_monster].palette_target[0] = 255;
                        monster[current_monster].palette_target[1] = 255;
                        monster[current_monster].palette_target[2] = 255;
                        monster[current_monster].palette_cycles = 1;
                        monster[current_monster].palette_step = 0.08f;
                        monster[current_monster].palette_blend = 0;
                        SndSePlay(SE_HIT_NO_DAMAGE, -1, 0);
                    }

                    if (NowColData->hit[hit].target_mask != 3) {
                        NowColData->active[hit] = 0;
                    }

                    if (result == MONSTER_DMG_KILLED) {
                        return MONSTER_DMG_KILLED;
                    }

                    break;
                }
            }
        }
    }

    return result;
}
