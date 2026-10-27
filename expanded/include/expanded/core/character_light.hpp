#pragma once

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded {

// Hook: CCharacter::Draw, after the game's own tints: the light Expanded's features add to a character for its draw
// (the room's white as it darkens, a charge held on the wielder), added to ambient in its 0-255 units.
void CharacterLight(const CCharacter &character, float *ambient);

} // namespace expanded
