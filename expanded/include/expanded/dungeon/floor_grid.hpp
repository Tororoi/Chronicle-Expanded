#pragma once

#include "dungeonmap.hpp"

namespace expanded {

// A dungeon floor's grid: tile (col, row) centred at (col, row) times kDungeonTile.
inline constexpr float kDungeonTile = 160.0f;

// The middle of a room (x, z).
inline void RoomMiddle(const ROOM_INFO &room, float *x, float *z) {
    *x = (static_cast<float>(room.x) + static_cast<float>(room.width - 1) * 0.5f) * kDungeonTile;
    *z = (static_cast<float>(room.y) + static_cast<float>(room.height - 1) * 0.5f) * kDungeonTile;
}

} // namespace expanded
