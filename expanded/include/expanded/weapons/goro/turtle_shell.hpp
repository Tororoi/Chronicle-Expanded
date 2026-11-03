#pragma once

namespace dc::game {
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Turtle Shell "Hard Shell": the Turtle Shell (or Super Steve carrying its SynthSphere) wears kHardShellWear as much
// as another weapon would (Durable and Fragile still apply on top), and the character holding it has kHardShellDefense
// more defense, on top of whatever food has raised it to.
inline constexpr float kHardShellWear = 0.5f;
inline constexpr int   kHardShellDefense = 5;

// The wear a hit or swing puts on the weapon (the equipped record), amount the game's.
float HardShellWear(const WEAPON_HAVE &weapon, float amount);

// A character's defense, defense their own, with the bonus their equipped weapon gives.
int HardShellDefense(int chara, int defense);

} // namespace expanded
