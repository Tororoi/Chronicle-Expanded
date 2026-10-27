#include "expanded/weapons/weapon_hit.hpp"

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "userstatus.hpp"

namespace expanded {

COLLISION_HIT *PlantWeaponHit(float *at, int damage, int life, float radius, int attribute) {
    int hit = NowColData->Set(at, damage, life, radius, 0.0f, kHitsMonsters, kSwingReaction, attribute, 0);
    if (hit < 0) {
        return nullptr;
    }
    COLLISION_HIT &record = NowColData->hit[hit];
    record.owner = UserStatus != nullptr ? UserStatus->cur_chara : 0;
    record.attack_no = 0;
    if (NowWeaponHave != nullptr) {
        record.vs_monster = NowWeaponHave->vs_monster;
        record.weapon_flags = NowWeaponHave->flags;
    }
    return &record;
}

} // namespace expanded
