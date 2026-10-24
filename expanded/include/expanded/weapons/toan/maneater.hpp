#pragma once

namespace expanded {

// Maneater "Blood Price": Toan is cursed while it is equipped, again on every floor (weapons/curse.hpp), but lifting
// the curse with an item costs nothing; it stays off until the next floor. While the sword's WHP is in the game's
// low warning (the HUD gauge blinking at a tenth of its maximum or less, but not broken), it feeds on him: each
// kBloodPriceFrames, 1 HP to mend 1 WHP, never leaving him below 1 HP. Super Steve carrying its SynthSphere curses
// Xiao the same way and mends itself from her. Times are game frames.
inline constexpr int   kBloodPriceFrames = 60;
inline constexpr float kBloodPriceWarning = 0.1f;

bool BloodPriceCursing(int chara);

// Hook: a dungeon step: each member's Maneater feeds once a kBloodPriceFrames.
void BloodPriceStep();

} // namespace expanded
