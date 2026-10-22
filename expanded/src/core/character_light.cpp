#include "expanded/core/character_light.hpp"

#include "expanded/dungeon/scene_light.hpp"
#include "expanded/weapons/solar_flash.hpp"

namespace expanded {

void CharacterLight(const CCharacter &character, float *ambient) {
    // A charge's white on Toan takes the place of the room's.
    if (!SolarFlashOwnsLight(character)) {
        SceneCharacterLight(character, ambient);
    }
    SolarFlashLight(character, ambient);
}

} // namespace expanded
