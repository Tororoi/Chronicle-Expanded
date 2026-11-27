#include "expanded/town/camera.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "camerafollow.hpp"
#include "character.hpp"
#include "ebattle.hpp"
#include "editarea.hpp"
#include "editground.hpp"
#include "editloop.hpp"
#include "editmapscript.hpp"
#include "expanded/core/options.hpp"
#include "expanded/town/camera_sweep.hpp"
#include "framevu1.hpp"
#include "main.hpp"
#include "mapparts.hpp"
#include "render/draw3d.hpp"

namespace expanded {

using namespace dc::game;

// expanded/src/town/camera_meshes.cpp.
const dc::render::Draw3DVisual *DrawnMesh(dc::game::CFrame *frame);

namespace {

constexpr const char *kModernCamera = "modern_camera";

[[maybe_unused]] const bool kRegistered =
    AddToggle({.key = kModernCamera,
               .label = "Modern Camera",
               .help = "\"Modern Camera\"\nThe town camera stops\nshort of what it would\nsee into. Off, it works\nas in the original game.",
               .on = true});

// The draw pass of the water's parts (CEditGround::DrawWater).
constexpr int kWaterPass = 0x15;

// One drawn mesh of the town, in the world as its frame last stood.
struct Piece {
    const dc::render::Draw3DVisual *visual = nullptr;
    float                           matrix[4][4] = {};
    double                          shape = 0.0; // the vertices' sum: an area's ground is remade in place
    SweepMesh                       mesh;
    std::uint32_t                   seen = 0;
};

struct Town {
    const CEditGround                        *ground = nullptr;
    int                                       map = -1;
    std::uint32_t                             generation = 0;
    std::unordered_map<const CFrame *, Piece> pieces;
};

Town town;

bool Solid(const CFrame &frame) {
    const CFrameAttr &attr = frame.attr;
    return attr.depth_write != 0 && attr.blend_mode == 0 && attr.ignore_depth == 0 && attr.billboard == 0;
}

void Transform(const float m[4][4], const float *v, Point &out) {
    for (int axis = 0; axis < 3; axis++) {
        out[axis] = v[0] * m[0][axis] + v[1] * m[1][axis] + v[2] * m[2][axis] + m[3][axis];
    }
}

double Shape(const dc::render::Draw3DVisual &visual) {
    double sum = 0.0;
    for (const auto &vertex : visual.vertices) {
        sum += vertex.position[0] + 3.0 * vertex.position[1] + 7.0 * vertex.position[2];
    }
    return sum;
}

void Build(Piece &piece, const dc::render::Draw3DVisual &visual, const float matrix[4][4], double shape, const char *piece_name) {
    piece.visual = &visual;
    piece.shape = shape;
    std::memcpy(piece.matrix, matrix, sizeof(piece.matrix));
    std::vector<Triangle> triangles;
    triangles.reserve(visual.triangles.size() / 3);
    for (std::size_t i = 0; i + 2 < visual.triangles.size(); i += 3) {
        Triangle t;
        Transform(matrix, visual.vertices[visual.triangles[i]].position, t.a);
        Transform(matrix, visual.vertices[visual.triangles[i + 1]].position, t.b);
        Transform(matrix, visual.vertices[visual.triangles[i + 2]].position, t.c);
        triangles.push_back(t);
    }
    // Only its large pieces stop the camera.
    const std::vector<bool> large = LargeMask(triangles);
    std::vector<Triangle>   kept;
    kept.reserve(triangles.size());
    for (std::size_t i = 0; i < triangles.size(); i++) {
        if (large[i]) {
            kept.push_back(triangles[i]);
        }
    }
    piece.mesh.Build(std::move(kept));
}

// The frame's mesh and its children's, as CFrameVu1::DrawVu1 draws them.
void Gather(CFrame *frame) {
    const short draw_on = frame->attr.draw_on;
    if (!(draw_on & 1) && (draw_on & 2)) {
        return;
    }
    if (draw_on != 0 && Solid(*frame)) {
        const dc::render::Draw3DVisual *built = DrawnMesh(frame);
        if (built != nullptr && !built->triangles.empty()) {
            sceVu0FMATRIX matrix;
            frame->GetLWMatrix(matrix);
            Piece       &piece = town.pieces[frame];
            const double shape = Shape(*built);
            if (piece.visual != built || piece.shape != shape || std::memcmp(piece.matrix, matrix, sizeof(piece.matrix)) != 0) {
                Build(piece, *built, matrix, shape, frame->name);
            }
            piece.seen = town.generation;
        }
    }
    if (!(draw_on & 2)) {
        for (CFrame *child = frame->child; child != nullptr; child = child->brother) {
            if (!(child->attr.draw_on & 4)) {
                Gather(child);
            }
        }
    }
}

void Gather(CMapParts &part) {
    if (part.handle < 0 || part.draw_on == 0 || part.category_no == kWaterPass || part.frame[0] == nullptr) {
        return;
    }
    // Where CObjectFrame::Draw puts it, drawn or not.
    CFrameVu1 *frame = part.frame[0];
    frame->SetPosition(part.pos);
    frame->SetRotation(part.rotation.x, part.rotation.y, part.rotation.z);
    frame->SetScale(part.scale[0], part.scale[1], part.scale[2]);
    Gather(frame);
}

// The town's meshes as they stand now: new or moved ones rebuilt, gone ones let go.
void Update(CEditGround &ground) {
    if (town.ground != &ground || town.map != MapNo) {
        TownCameraReset();
        town.ground = &ground;
        town.map = MapNo;
    }
    town.generation++;
    for (CMapParts &part : ground.parts) {
        Gather(part);
    }
    for (CMapParts &part : ground.fixed_parts) {
        Gather(part);
    }
    // The Georama areas' ground, where CEditArea::DrawGrid puts it.
    for (CEditArea *area : ground.areas) {
        if (area == nullptr) {
            break;
        }
        if (area->grid_frame != nullptr) {
            area->grid_frame->SetPosition(area->offset_x, area->offset_y, area->offset_z);
            Gather(area->grid_frame);
        }
    }
    std::erase_if(town.pieces, [](const auto &entry) { return entry.second.seen != town.generation; });
}

bool Meets(const Piece &piece, const Point &from, const Point &to, float radius) {
    for (int axis = 0; axis < 3; axis++) {
        if (std::max(from[axis], to[axis]) < piece.mesh.Min()[axis] - radius ||
            std::min(from[axis], to[axis]) > piece.mesh.Max()[axis] + radius) {
            return false;
        }
    }
    return true;
}

float Sweep(const Point &from, const Point &to, float radius) {
    float best = 1.0f;
    for (const auto &[frame, piece] : town.pieces) {
        if (!piece.mesh.Empty() && Meets(piece, from, to, radius)) {
            best = piece.mesh.Sweep(from, to, radius, best);
        }
    }
    return best;
}

Point PushOut(Point point, float radius) {
    for (const auto &[frame, piece] : town.pieces) {
        if (!piece.mesh.Empty() && Meets(piece, point, point, radius)) {
            point = piece.mesh.PushOut(point, radius, radius);
        }
    }
    return point;
}

Point Lerp(const Point &a, const Point &b, float t) {
    return {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t};
}

// The nearest point of the town to `point` within `radius`, and its distance; `radius` when none is.
float Closest(const Point &point, float radius, Point &nearest) {
    float best = radius;
    for (const auto &[frame, piece] : town.pieces) {
        Point near{};
        if (!piece.mesh.Empty() && Meets(piece, point, point, radius) && piece.mesh.Closest(point, best, near) < best) {
            best = std::hypot(near[0] - point[0], near[1] - point[1], near[2] - point[2]);
            nearest = near;
        }
    }
    return best;
}

// The eye's sphere: the walking view's near plane (5, EditLoop) reaches out to its corners about 5.9 from the eye on
// the widest screen, so a sphere this big keeps the near plane out of what it touches.
constexpr float kEyeRadius = 7.0f;

// The sphere the look-at point is brought out from the player's chest with.
constexpr float kPivotRadius = 2.0f;

// A wall the eye could rise over: its top no higher than this above where the eye meets it...
constexpr float kLowWall = 25.0f;

// ...and no thicker than the eye's sphere can pass over in this far.
constexpr float kPastWall = 2.0f * kEyeRadius + 8.0f;

// Whether the eye's sphere, stopped at `at` on its way to `eye`, has met a low, thin wall it could rise over (a fence,
// a bridge's or a roof's parapet), and where on the way it is past it.
bool LowWall(const Point &at, const Point &eye, Point &past) {
    Point nearest{};
    if (Closest(at, kEyeRadius + 0.5f, nearest) >= kEyeRadius + 0.5f) {
        return false;
    }
    const float out[3] = {at[0] - nearest[0], at[1] - nearest[1], at[2] - nearest[2]};
    const float length = std::hypot(out[0], out[1], out[2]);
    const float way[3] = {eye[0] - at[0], eye[1] - at[1], eye[2] - at[2]};
    // Met from above it is the wall's top edge when the eye rises or goes level past it, else a floor.
    if (length <= 0.0f || out[1] < -0.5f * length || (out[1] > 0.5f * length && way[1] < 0.0f)) {
        return false; // a ceiling, or a floor the eye goes down into
    }
    const float across = std::hypot(way[0], way[2]);
    const float all = std::hypot(way[0], way[1], way[2]);
    if (across < 0.3f * all || across < kPastWall) {
        return false; // going mostly up or down, or ending at the wall
    }
    const Point over = {at[0], at[1] + kLowWall, at[2]};
    const Point beyond = {over[0] + way[0] / across * kPastWall, over[1], over[2] + way[2] / across * kPastWall};
    past = Lerp(at, eye, kPastWall / across);
    const Point above = {past[0], past[1] + kLowWall, past[2]};
    // Open above it, over it and above where the eye comes down past it (not under a roof the wall holds up).
    return Sweep(at, over, kEyeRadius) >= 1.0f && Sweep(over, beyond, kEyeRadius) >= 1.0f &&
           Sweep(past, above, kEyeRadius) >= 1.0f;
}

// How far (0 to 1) from the pivot to the eye its sphere goes before it meets the town, passing low walls.
float Reach(const Point &pivot, const Point &eye) {
    Point from = pivot;
    for (int wall = 0;; wall++) {
        const float t = Sweep(from, eye, kEyeRadius);
        const Point at = Lerp(from, eye, t);
        Point       past{};
        if (t >= 1.0f || wall == 4 || !LowWall(at, eye, past)) {
            if (t >= 1.0f) {
                return 1.0f;
            }
            const float whole = std::hypot(eye[0] - pivot[0], eye[1] - pivot[1], eye[2] - pivot[2]);
            const float left = std::hypot(eye[0] - at[0], eye[1] - at[1], eye[2] - at[2]);
            return whole > 0.0f ? 1.0f - left / whole : 0.0f;
        }
        from = past;
    }
}

} // namespace

bool TownCameraOn(bool fishing) {
    return !fishing && OptionOn(kModernCamera);
}

void TownCameraSolve(CCamera *shown, CEditGround *ground, int mode) {
    if (shown != &MainCamera || ground == nullptr || mode != ED_MODE_WALK || viewMode != 0 || Chara == nullptr ||
        !MainCamera.follow_on || CCamera::StopCamera || !TownCameraOn(false)) {
        return;
    }
    Update(*ground);

    const Point chest = {Chara->pos[0], Chara->pos[1] + 0.5f * Chara->body_height, Chara->pos[2]};
    const Point look = {MainCamera.ref[0], MainCamera.ref[1], MainCamera.ref[2]};
    const Point pivot = PushOut(Lerp(chest, look, Sweep(chest, look, kPivotRadius)), kEyeRadius);
    const Point eye = {MainCamera.pos[0], MainCamera.pos[1], MainCamera.pos[2]};
    const float reach = Reach(pivot, eye);
    if (reach < 1.0f) {
        // The camera eases back out from here as the way clears.
        const Point stopped = Lerp(pivot, eye, reach);
        std::copy(stopped.begin(), stopped.end(), MainCamera.pos);
    }
}

void TownCameraReset() {
    town = {};
}

} // namespace expanded
