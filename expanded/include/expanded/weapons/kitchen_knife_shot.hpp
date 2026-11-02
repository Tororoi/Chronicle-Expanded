#pragma once

namespace dc::game {
class CCharacter;
class CSHOT;
} // namespace dc::game

namespace expanded {

// Super Steve carrying the Kitchen Knife's SynthSphere: every pellet flies as an Aquamarine, the dungeon's dropped gem
// at its own size thrown as the Bomb Gemron's bomb is (the apple shot with the gem's model, data/gemron_models.hpp), held
// still with its point turned back along its flight, toward Xiao. The pellet's sprite is left out (its size 0); its hit
// is as ever. A pellet fired with every one of the effect's kGemShotSlots in the air flies as a plain pellet.
inline constexpr int kGemShotSlots = 8;
// A gentle cyan white over the gem: added to its ambient (red, green, blue).
inline constexpr float kGemTint[3] = {40.0f, 70.0f, 75.0f};

// One step (once a frame the monsters step unpaused): the gems ride their pellets; the floor's monsters put back.
void GemShotStep();
void GemShotReset();

// Hooks: Xiao fired a pellet into slot of shots; a pellet ended.
void GemShotFired(CSHOT &shots, int slot);
void GemShotEnded(const CSHOT &shots, int slot);
// Hook: a character's light as it is drawn: a gem's tint.
void GemShotLight(const CCharacter &character, float *ambient);

} // namespace expanded
