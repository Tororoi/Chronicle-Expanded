#pragma once

namespace expanded {

// The bits Expanded marks a planted hit's weapon_flags with, one an effect, above the game's own (elements, ailments,
// abilities), so a hook can tell its hits apart.
inline constexpr int kPrickMark = 0x02000000;        // the Cactus's "Desert Bloom" pricks
inline constexpr int kSpiritBeastMark = 0x08000000;  // the Divine Beast Title cat's strike
inline constexpr int kDragonShotMark = 0x10000000;   // Dragon's Y's dragon balls
inline constexpr int kChargingBullMark = 0x20000000; // Matador's charged pellet
inline constexpr int kBlastMark = 0x40000000;        // a blast (blast.hpp)

} // namespace expanded
