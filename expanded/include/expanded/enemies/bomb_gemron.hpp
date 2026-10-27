#pragma once

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// The Bomb Gemron (expanded/docs/gemrons.md): Holy Gemron's script and stats under its own look and record, weak to
// fire and immune to the rest. It throws bombs, blows up as its death reaches frame 122, and self-destructs when near
// death with the player close: its guard's return reversed into a lit fuse, the blast on its final pose, no kill.

// Hook: _SET_SHOT's target, which Holy Gemron's script sets 7.4 over the player's feet; the bomb is aimed at 14, the
// height its contact test looks at.
void BombGemronAim(const CMonstorUnit &unit, int monster, float *target);

// Hook: the monster's step before its script; true while the self-destruct has it.
bool BombGemronStep(CMonstorUnit &unit, int monster);

// Its fuse: a spark kept alight at its big bomb's wick, walking down the wick as the death or the self-destruct burns
// it, with a burst of sparks every 15 frames, and the bomb reddening as it burns, out with the blast. Hooks: the
// monster draw before and after a unit draws (its posed bone), and the dungeon's effect draw.
void BombGemronDrawing(CMonstorUnit &unit, int monster);
void BombGemronDrawn(CMonstorUnit &unit, int monster);
void BombGemronEffectsDraw();

} // namespace expanded
