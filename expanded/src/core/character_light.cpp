#include "expanded/core/character_light.hpp"

#include "expanded/dungeon/scene_light.hpp"
#include "expanded/weapons/big_bang_shot.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/toan/big_bang.hpp"
#include "expanded/weapons/toan/sword_of_zeus.hpp"
#include "expanded/weapons/ungaga/babels_spear.hpp"
#include "expanded/weapons/ungaga/hercules_wrath.hpp"
#include "expanded/weapons/ungaga/terra_sword.hpp"
#include "expanded/weapons/xiao/angel_gear.hpp"
#include "expanded/weapons/xiao/angel_shooter.hpp"
#include "expanded/weapons/xiao/divine_beast_title.hpp"
#include "expanded/weapons/xiao/matador.hpp"
#include "expanded/weapons/zeus_shot.hpp"

namespace expanded {

void CharacterLight(const CCharacter &character, float *ambient) {
    // A charge's white on Toan takes the place of the room's.
    if (!SolarFlashOwnsLight(character)) {
        SceneCharacterLight(character, ambient);
    }
    SolarFlashLight(character, ambient);
    DetonateChargeLight(character, ambient);
    BigBangShotLight(character, ambient);
    ZeusChargeLight(character, ambient);
    ZeusShotLight(character, ambient);
    ChargingBullLight(character, ambient);
    SpiritBeastLight(character, ambient);
    GuardianGraceLight(character, ambient);
    GuardianReflectorLight(character, ambient);
    AirStrikeLight(character, ambient);
    BigRockLight(character, ambient);
    CurseOfBabelLight(character, ambient);
}

} // namespace expanded
