#pragma once

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

// Hook: the floor ending (beside the shot effects' clear): every character taken down, the texture block emptied.
void FloorCharactersClear();

// Tests replace the load: it sets chara up from the pack at path, returning false on failure.
using FloorCharacterLoader = bool (*)(CCharacter &chara, CDataAlloc2<1> &arena, const char *path, const char *cfg);
void SetFloorCharacterLoader(FloorCharacterLoader loader);

} // namespace expanded
