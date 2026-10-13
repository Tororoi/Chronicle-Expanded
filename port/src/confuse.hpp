#pragma once

#include <cstdint>

struct PortDecodedTexture;

// The Confuse weapon ability: bit 14 of a weapon's flags (WEAPON_HAVE::flags, the ability word every hit
// carries as COLLISION_HIT::weapon_flags), beside Poison, Stop, Drain and the rest. Babel's Spear has it
// from its weapon data; a SynthSphere made from a weapon that has it carries it on as it carries any ability.
inline constexpr int kWeaponFlagConfuse = 0x4000;
// The option bit and its name, the system bank's message 0x45 (Big bucks, bit 1, is 0x46).
inline constexpr int kConfuseOptionBit = 14;
inline constexpr int kConfuseNameMessage = 0x45;

// The menus' option name of a bit, as the system bank numbers them.
int ConfuseOptionMessage(int bit);

// The Confuse icon laid into the charaface sheet's empty tile beside Steal, as the sheet is decoded.
void ConfuseIconTexture(const char *name, PortDecodedTexture &texture);
