#include "dungeon.h"
#include "expanded/dungeon/reflection_cubes.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "expanded/dungeon/floor_grid.hpp"
#include "expanded/dungeon/floor_height.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "gfx/gfx.hpp"
#include "mglib.hpp"
#include "render/draw3d.hpp"
#include "render/gs_state.hpp"
#include "render/screen.hpp"
#include "renderinfo.hpp"

namespace expanded {

namespace {

using dc::render::Draw3DReflectionCube;

constexpr int   kGrid = 20;
constexpr int   kBlockWidth = 3 * kReflectionFace;  // a cube's block: three faces across,
constexpr int   kBlockHeight = 2 * kReflectionFace; // two down
constexpr int   kBlocksAcross = 16;
constexpr float kGsCentre = 2048.0f;
constexpr float kReach = 480.0f;         // the map parts a face draws: those this near the eye,
constexpr float kMoonSeaReach = 1600.0f; // the Moon Sea's open water further

struct Tile {
    int                  cell = 0;
    bool                 captured = false;
    Draw3DReflectionCube cube;
};

std::vector<Tile>  tiles;
int                tile_of[kGrid * kGrid];
int                next_tile = 0;
const CDungeonMap *laid = nullptr; // the map the tiles were laid out for
gfx::TextureHandle atlas = gfx::kNullTexture;

void Reset() {
    tiles.clear();
    std::fill(std::begin(tile_of), std::end(tile_of), -1);
    next_tile = 0;
    laid = nullptr;
    if (atlas != gfx::kNullTexture) {
        gfx::DestroyTexture(atlas);
        atlas = gfx::kNullTexture;
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(Reset), true);

// The floor's tiles that draw a part, each a block in a texture made for them.
bool LayOut(const CDungeonMap &map) {
    Reset();
    laid = &map;
    for (int cell = 0; cell < kGrid * kGrid; cell++) {
        int part = map.cells[cell].parts_no;
        if (part != MAP_PARTS_NONE && part >= 0 && part < 72) {
            tile_of[cell] = static_cast<int>(tiles.size());
            tiles.push_back(Tile{.cell = cell});
        }
    }
    if (tiles.empty()) {
        return false;
    }
    int              rows = (static_cast<int>(tiles.size()) + kBlocksAcross - 1) / kBlocksAcross;
    int              width = kBlocksAcross * kBlockWidth;
    int              height = rows * kBlockHeight;
    gfx::TextureDesc desc;
    desc.width = static_cast<uint32_t>(width);
    desc.height = static_cast<uint32_t>(height);
    desc.format = gfx::TextureFormat::Rgba8;
    desc.has_alpha = false;
    atlas = gfx::CreateTexture(desc);
    if (atlas == gfx::kNullTexture) {
        tiles.clear();
        return false;
    }
    for (size_t i = 0; i < tiles.size(); i++) {
        Draw3DReflectionCube &cube = tiles[i].cube;
        cube.texture = atlas;
        cube.corner[0] = static_cast<float>(static_cast<int>(i) % kBlocksAcross * kBlockWidth) / static_cast<float>(width);
        cube.corner[1] = static_cast<float>(static_cast<int>(i) / kBlocksAcross * kBlockHeight) / static_cast<float>(height);
        cube.face[0] = static_cast<float>(kReflectionFace) / static_cast<float>(width);
        cube.face[1] = static_cast<float>(kReflectionFace) / static_cast<float>(height);
    }
    return true;
}

void Cross(const float a[3], const float b[3], float out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

// The game camera's handedness: its view's rows (right, down, forward), right along cross(down, forward) or against it.
float Handedness(const RenderInfo &info) {
    float right[3];
    float down[3];
    float ahead[3];
    for (int c = 0; c < 3; c++) {
        right[c] = info.view[c][0];
        down[c] = info.view[c][1];
        ahead[c] = info.view[c][2];
    }
    float across[3];
    Cross(down, ahead, across);
    return right[0] * across[0] + right[1] * across[1] + right[2] * across[2] < 0.0f ? -1.0f : 1.0f;
}

// The view along one face from the eye, set as SetViewMatrix sets the camera's (without its camera-cut check: the
// capture is not the camera moving).
void FaceView(int face, const float eye[3], float hand) {
    const float *ahead = gfx::kReflectCubeAxis[face];
    const float *down = gfx::kReflectCubeDown[face];
    float        right[3];
    Cross(down, ahead, right);
    const float *rows[3] = {right, down, ahead};
    float        scales[3] = {hand, 1.0f, 1.0f};

    sceVu0FMATRIX view = {};
    for (int row = 0; row < 3; row++) {
        float along = 0.0f;
        for (int c = 0; c < 3; c++) {
            view[c][row] = rows[row][c] * scales[row];
            along += view[c][row] * eye[c];
        }
        view[3][row] = -along;
    }
    view[3][3] = 1.0f;

    RenderInfo &info = mgRenderInfo;
    sceVu0CopyMatrix(info.view, view);
    sceVu0CopyMatrix(info.view_scaled, view);
    for (int c = 0; c < 3; c++) {
        info.position[c] = view[3][c];
        info.view_position[c] = eye[c];
    }
    info.position[3] = 0.0f;
    info.view_position[3] = 1.0f;
    sceVu0FMATRIX screen;
    sceVu0UnitMatrix(screen);
    screen[0][0] = info.scale[0];
    screen[1][1] = info.scale[0];
    screen[2][2] = info.offset[2];
    screen[3][3] = 0.0f;
    screen[2][1] = kGsCentre;
    screen[2][0] = kGsCentre;
    screen[2][3] = 1.0f;
    screen[3][2] = info.scale[2];
    sceVu0MulMatrix(info.view_screen, screen, view);
    sceVu0CopyMatrix(info.screen, screen);
}

// The map parts near the eye, and the background models around it.
void DrawMap(CDungeonMap &map, const float eye[3]) {
    float reach = selectMapNo == DUNGEON_MOON_SEA ? kMoonSeaReach : kReach;
    for (int row = 0; row < kGrid; row++) {
        for (int col = 0; col < kGrid; col++) {
            const MAP_CELL &cell = map.cells[col + row * kGrid];
            if (cell.parts_no == MAP_PARTS_NONE || cell.parts_no < 0 || cell.parts_no >= 72) {
                continue;
            }
            float x = kDungeonTile * static_cast<float>(col);
            float z = kDungeonTile * static_cast<float>(row);
            if (std::hypot(x - eye[0], z - eye[2]) > reach) {
                continue;
            }
            CDungeonParts &part = map.parts[cell.parts_no];
            part.direction = cell.direction;
            part.pos[0] = x;
            part.pos[1] = 0.0f;
            part.pos[2] = z;
            part.pos[3] = 1.0f;
            part.Draw();
        }
    }
    float ground[4] = {eye[0], 0.0f, eye[2], 1.0f};
    for (CFrame *model : map.bg_model) {
        if (model != nullptr) {
            model->SetPosition(ground);
            MGDraw(model);
        }
    }
}

void Capture(CDungeonMap &map, Tile &tile, gfx::TextureHandle target, float hand) {
    int   col = tile.cell % kGrid;
    int   row = tile.cell / kGrid;
    float eye[3] = {kDungeonTile * static_cast<float>(col), 0.0f, kDungeonTile * static_cast<float>(row)};
    float ground = 0.0f;
    if (FloorUnder(eye[0], eye[2], kReflectionFloorFrom, &ground)) {
        eye[1] = ground;
    }
    eye[1] += kReflectionEyeHeight;

    const u8 *bg = lightingMode != 0 ? sub_bgColor : main_bgColor;
    uint8_t   clear[4] = {bg[0], bg[1], bg[2], 0x80};
    int       index = static_cast<int>(&tile - tiles.data());
    int       block_x = index % kBlocksAcross * kBlockWidth;
    int       block_y = index / kBlocksAcross * kBlockHeight;
    for (int face = 0; face < 6; face++) {
        gfx::SetRenderTarget(target);
        gfx::Clear(true, clear, true, 0.0f);
        FaceView(face, eye, hand);
        DrawMap(map, eye);
        gfx::Rect from = {0, 0, kReflectionFace, kReflectionFace};
        gfx::Rect to = {block_x + face % 3 * kReflectionFace, block_y + face / 3 * kReflectionFace, kReflectionFace,
                        kReflectionFace};
        gfx::BlitTexture(target, from, atlas, to, gfx::Filter::Linear);
    }
    tile.cube.hand = hand;
    tile.captured = true;
}

} // namespace

void ReflectionCubesCapture() {
    CDungeonMap *map = NowDngMap;
    if (map == nullptr || map->map_type != 1) {
        return;
    }
    if (map != laid && !LayOut(*map)) {
        return;
    }
    if (next_tile >= static_cast<int>(tiles.size())) {
        return;
    }
    gfx::TextureHandle target = gfx::NamedRenderTarget("reflection cube face", kReflectionFace, kReflectionFace, false);
    if (target == gfx::kNullTexture) {
        return;
    }

    // A square 90 degree view: half the face across from its middle to its edge, in the target's own rows.
    const RenderInfo   kept = mgRenderInfo;
    gfx::TextureHandle drawing = gfx::CurrentRenderTarget();
    float              hand = Handedness(kept);
    float              half = 0.5f * static_cast<float>(kReflectionFace);
    float              rows = dc::render::MGPortTargetRowScale(target);
    MGSetRenderInfo(half, kept.near[2], kept.far[2]);
    mgRenderInfo.scale[1] = half / rows;
    mgRenderInfo.offset[0] = kGsCentre - 0.5f * dc::render::kScreenWidth + half;
    mgRenderInfo.offset[1] = kGsCentre - 0.5f * dc::render::kScreenHeight + half / rows;
    std::memcpy(mgRenderInfo.light_direction, kept.light_direction, sizeof(kept.light_direction));
    std::memcpy(mgRenderInfo.light_color, kept.light_color, sizeof(kept.light_color));

    dc::render::Draw3DSetCapturing(true);
    for (int n = 0; n < kReflectionTilesPerTick && next_tile < static_cast<int>(tiles.size()); n++) {
        Capture(*map, tiles[static_cast<size_t>(next_tile++)], target, hand);
    }
    dc::render::Draw3DSetCapturing(false);

    mgRenderInfo = kept;
    gfx::SetRenderTarget(drawing);
}

const Draw3DReflectionCube *ReflectionCubeAt(float x, float z) {
    if (laid == nullptr || laid != NowDngMap) {
        return nullptr;
    }
    int col = static_cast<int>(std::floor((x + 0.5f * kDungeonTile) / kDungeonTile));
    int row = static_cast<int>(std::floor((z + 0.5f * kDungeonTile) / kDungeonTile));
    if (col < 0 || col >= kGrid || row < 0 || row >= kGrid) {
        return nullptr;
    }
    int index = tile_of[col + row * kGrid];
    return index >= 0 && tiles[static_cast<size_t>(index)].captured ? &tiles[static_cast<size_t>(index)].cube : nullptr;
}

} // namespace expanded
