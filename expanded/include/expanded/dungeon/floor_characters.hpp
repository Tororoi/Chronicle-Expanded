#pragma once

#include <string>

#include "dataalloc_fwd.hpp"

class CCharacter;

namespace expanded {

// Characters of Expanded's own on a dungeon floor (a weapon's companion): a character pack read whole from the disc
// into memory of its own and texture block kFloorCharacterBlock, set up from its cfg as the game sets up its own, kept
// until the floor ends. The feature that entered one steps and draws it.
inline constexpr int kFloorCharacterBlock = 0x35;

// The character from the pack at path, set up from its cfg named cfg: the one entered from path already on this floor,
// else a new one, or null when the pack cannot be read or its images do not fit.
CCharacter *EnterFloorCharacter(const char *path, const char *cfg);

// A copy of a character the game has loaded, from the same pack: its own model, motions, skins and cloth, but none of the
// pack's images, its model drawing with the textures of the same names the game loaded for the original. The one entered
// from path already on this floor, else a new one, or null when the pack cannot be read.
CCharacter *EnterFloorCopy(const char *path, const char *cfg);

// The pack of a weapon's model (commenu/weapon/...), as the weapon menu loads it, and through cfg its cfg's name.
std::string WeaponPackPath(int weapon_no, std::string &cfg);

// Draws a character entered on the floor, its images first put back in kFloorCharacterBlock's VRAM.
void DrawFloorCharacter(CCharacter &chara);

// Hook: the floor ending (beside the shot effects' clear): every character taken down, the texture block emptied.
void FloorCharactersClear();

// Tests replace the load: it sets chara up from the pack at path, returning false on failure.
using FloorCharacterLoader = bool (*)(CCharacter &chara, CDataAlloc2<1> &arena, const char *path, const char *cfg);
void SetFloorCharacterLoader(FloorCharacterLoader loader);

} // namespace expanded
