#pragma once

namespace dc::game {
struct MONSTOR;
} // namespace dc::game

namespace expanded {

// Scorpion "Venom": with the Scorpion in Ungaga's hand, or Super Steve carrying its SynthSphere, each time poison takes
// hold of a monster the character out is cured of their own poison, and their weapon takes 1/kVenomAbsShare of that
// monster's ABS (rounded down), never past the weapon's maximum ABS nor back from above it. A weapon the game gives no
// ABS (a broken starting weapon) takes none. A monster already poisoned keeps its poison and does not count again. Its
// charge shot, and Super Steve's charged shot with the sphere, fly at the Scorpion's Tornado Charge tier (halberd.hpp).
inline constexpr int kVenomAbsShare = 2;

bool VenomActive();

// Hook: poison took hold of a monster.
void VenomPoisoned(const MONSTOR &target);

// Scorpion "Venom Lure": while a Scorpion is owned (any bag or the storage), fish notice the Poisonous Apple on the hook
// from kVenomLure times as far, Bait Flamboyance's units included.
inline constexpr float kVenomLure = 2.0f;

// Bait item_no's notice radius, radius as the game and Bait Flamboyance have it.
float VenomLureRadius(int item_no, float radius);

} // namespace expanded
