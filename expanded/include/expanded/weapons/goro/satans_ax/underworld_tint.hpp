#pragma once

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded {

// Pull of the Underworld's tint (satans_ax.hpp): while the pull is on, Satan's Ax is drawn kUnderworldTint lighter in
// red, green and blue (a pink), fading in over kUnderworldTintFade frames as the pull starts and out as it ends.
inline constexpr float kUnderworldTint[3] = {150.0f, 100.0f, 120.0f};
inline constexpr int   kUnderworldTintFade = 15; // a quarter of a second

// One dungeon step: the tint faded toward on or off.
void UnderworldTintStep();
// How far in it is, 0 to 1.
float UnderworldTint();
// The tint added to the ax as it draws (CharacterLight).
void UnderworldLight(const CCharacter &character, float *ambient);
// Off at once (the floor changed; tests too).
void UnderworldTintReset();

} // namespace expanded
