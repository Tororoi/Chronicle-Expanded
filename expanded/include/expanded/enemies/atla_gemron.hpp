#pragma once

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// The Atla Gemron (expanded/docs/gemrons.md): Holy Gemron's stats and script under its own model and record, weak to
// wind and immune to the rest. It holds a dungeon atla on its big gem and wears the Atlamillia on its forehead, and
// fires the Atlamillia's burst along its facing, its hit growing as it flies. As it dies the atla bounces free of its
// grip and stays on the floor (expanded/dungeon/atla_drops.hpp): an Atlamillia Sword, an Atlamillia SynthSphere, the
// dungeon's featured weapon's or another weapon's (atla_gemron_loot.hpp).

// Hook: the dungeon step after the shot effects (its shot, and the atla it drops).
void AtlaGemronStep(CMonstorUnit &unit);

// Hooks: the monster draw before and after a unit draws (its Atlamillia's tint; the held atla's pose).
void AtlaGemronDrawing(CMonstorUnit &unit, int monster);
void AtlaGemronDrawn(CMonstorUnit &unit, int monster);

} // namespace expanded
