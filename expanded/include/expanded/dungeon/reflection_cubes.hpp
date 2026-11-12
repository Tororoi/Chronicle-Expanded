#pragma once

namespace dc::render {
struct Draw3DReflectionCube;
} // namespace dc::render

namespace expanded {

// Each floor tile's surroundings as a reflection cube: the map seen from kReflectionEyeHeight above the tile's middle,
// six square faces of kReflectionFace texels, captured a few tiles a tick from the floor's first drawn tick (the map
// parts and the background models only: no characters, effects or menus) into one texture for the floor.
inline constexpr int   kReflectionFace = 64;
inline constexpr float kReflectionEyeHeight = 20.0f;
inline constexpr int   kReflectionTilesPerTick = 16;

// Captures the next tiles, while the floor has tiles left. Called where the dungeon's draw has set the floor's light,
// before it draws the map.
void ReflectionCubesCapture();

// The cube of the tile under (x, z), or null while it is not captured (or the floor is not laid on the grid).
const dc::render::Draw3DReflectionCube *ReflectionCubeAt(float x, float z);

} // namespace expanded
