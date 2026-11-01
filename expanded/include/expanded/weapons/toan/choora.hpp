#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Choora "Armor Piercer": with the Choora in hand, or Super Steve carrying its SynthSphere, the weapon's hits on an
// armored monster deal kArmorPiercerDamage times their damage, and an armored monster it kills gives kArmorPiercerAbs
// times its ABS. Armored: the Crabby Hermit, the Living Armor, the Black Knight and his mount, Arthur, Alexander, the
// Steel Giant, the Pirate's Chariot, the Moon Bug, the Statue, the Statue Dog, the Golem, the Titan, Gol, Sil,
// Rockanoff, Blizzard and Nikapous, each with its Enhanced form.
inline constexpr float kArmorPiercerDamage = 2.0f;
inline constexpr int   kArmorPiercerAbs = 2;

// Whether the monster in the slot is armored.
bool Armored(const CMonstorUnit &unit, int monster);

// A weapon hit's damage on a monster after its anti-monster bonus (CheckDmg).
float ArmorPiercerDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage);

// The ABS a kill gives, the monster's own abs as the engine reads it.
int ArmorPiercerAbs(const CMonstorUnit &unit, int monster, int abs);

} // namespace expanded
