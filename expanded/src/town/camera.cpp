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
    std::vector<PassPart>           passing; // its pass-through pieces, in the world
    std::uint32_t                   seen = 0;
};

struct Town {
    const CEditGround                        *ground = nullptr;
    int                                       map = -1;
    std::uint32_t                             generation = 0;
    std::unordered_map<const CFrame *, Piece> pieces;
    float                                     shown = -1.0f; // how far from the point looked at the eye was shown
    bool                                      moved = false; // whether the eye shown is not the game's own
    std::vector<dc::render::Draw3DVisual *>   culled;        // the meshes drawn without their insides
    Point                                     own{};         // the game's own eye, put back once the frame is drawn
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

// Buildings whose legs block the camera, by town and the start of their frames' names: Brownboo's stilt houses (its
// central building's legs, a walkway running among them, pass).
struct Legs {
    int         map;
    const char *frames;
};

constexpr Legs kLegsBlock[] = {
    {14, "h03"},
};

bool LegsBlock(int map, const char *frame) {
    for (const Legs &legs : kLegsBlock) {
        if (legs.map == map && std::strncmp(frame, legs.frames, std::strlen(legs.frames)) == 0) {
            return true;
        }
    }
    return false;
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
    // Only its blocking pieces stop the camera.
    Blocking blocking;
    blocking.tall_blocks = LegsBlock(MapNo, piece_name);
    const std::vector<bool> large = BlockingMask(triangles, blocking);
    piece.passing = PassThroughParts(triangles, large);

    std::vector<Triangle> kept;
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

bool Meets(const SweepMesh &mesh, const Point &from, const Point &to, float radius) {
    for (int axis = 0; axis < 3; axis++) {
        if (std::max(from[axis], to[axis]) < mesh.Min()[axis] - radius ||
            std::min(from[axis], to[axis]) > mesh.Max()[axis] + radius) {
            return false;
        }
    }
    return true;
}

float Sweep(const Point &from, const Point &to, float radius) {
    float best = 1.0f;
    for (const auto &[frame, piece] : town.pieces) {
        if (!piece.mesh.Empty() && Meets(piece.mesh, from, to, radius)) {
            best = piece.mesh.Sweep(from, to, radius, best);
        }
    }
    return best;
}

Point PushOut(Point point, float radius) {
    for (const auto &[frame, piece] : town.pieces) {
        if (!piece.mesh.Empty() && Meets(piece.mesh, point, point, radius)) {
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
        if (!piece.mesh.Empty() && Meets(piece.mesh, point, point, radius) && piece.mesh.Closest(point, best, near) < best) {
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

// The line the eye must see the player along: a thin sphere, so a wall's corner beside the line hides nothing.
constexpr float kSightRadius = 1.0f;

// With no room between the player and the wall behind them, the eye rises this far over the point it looks at...
constexpr float kRise = 15.0f;

// ...leaning this far toward where it would be, so it still looks the way it did...
constexpr float kLean = 2.0f;

// ...from as far as this from that point toward it, rising more the nearer it is (a sphere that fits beside a head).
constexpr float kRoom = 10.0f;
constexpr float kHeadRadius = 4.0f;

// The share of the way back out the eye goes each frame once the line to the player clears.
constexpr float kEaseOut = 0.1f;

// Ground that asks for a high camera (AREA_KIND_HIGH_CAMERA) raises it this share of the way a frame, within these
// steps, where the original game lifts it at once.
constexpr float kHighRise = 0.05f;
constexpr float kHighRiseLeast = 0.35f;
constexpr float kHighRiseMost = 1.0f;

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

// The walking view: its near plane's distance (EditLoop) and the projection's scale, in frame units at a distance of
// one, that the visible frame's half extent is measured against.
constexpr float kNear = 5.0f;
constexpr float kProjection = 800.0f;

// What the eye keeps between its view and what blocks.
constexpr float kMargin = 0.5f;

// The eye's view toward the point it looks at: the near plane's rectangle out in front of it.
struct View {
    float forward[3], right[3], up[3];
    float half_width, half_height;
};

View ViewOf(const Point &eye, const Point &look) {
    View        view{};
    float       f[3] = {look[0] - eye[0], look[1] - eye[1], look[2] - eye[2]};
    const float length = std::hypot(f[0], f[1], f[2]);
    if (length <= 0.0f) {
        f[0] = 0.0f, f[1] = 0.0f, f[2] = 1.0f;
    } else {
        f[0] /= length, f[1] /= length, f[2] /= length;
    }
    // Right is level; up is square to both.
    float       r[3] = {f[2], 0.0f, -f[0]};
    const float level = std::hypot(r[0], r[2]);
    if (level <= 0.0f) {
        r[0] = 1.0f, r[2] = 0.0f;
    } else {
        r[0] /= level, r[2] /= level;
    }
    const float u[3] = {r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0]};
    for (int axis = 0; axis < 3; axis++) {
        view.forward[axis] = f[axis];
        view.right[axis] = r[axis];
        view.up[axis] = u[axis];
    }
    const dc::render::Draw3DExtent extent = dc::render::Draw3DVisibleExtent();
    view.half_width = kNear * extent.half_width / kProjection;
    view.half_height = kNear * extent.half_height / kProjection;
    return view;
}

// How far the eye keeps from a surface whose normal, toward the eye, is `n`: far enough that no corner of the near
// plane reaches it. Square on to a wall ahead that is the near plane's distance, beside one its half width, and with
// the wall behind only the margin.
float Clearance(const View &view, const float n[3]) {
    auto        dot = [&](const float v[3]) { return n[0] * v[0] + n[1] * v[1] + n[2] * v[2]; };
    const float reach = -kNear * dot(view.forward) + view.half_width * std::abs(dot(view.right)) +
                        view.half_height * std::abs(dot(view.up));
    return kMargin + std::max(reach, 0.0f);
}

// The eye moved out of what blocks until its view clears every surface near it.
Point ClearView(Point eye, const Point &look) {
    const View  view = ViewOf(eye, look);
    const float most = kMargin + std::hypot(kNear, view.half_width, view.half_height);
    for (int pass = 0; pass < 2; pass++) {
        for (const auto &[frame, piece] : town.pieces) {
            if (piece.mesh.Empty() || !Meets(piece.mesh, eye, eye, most)) {
                continue;
            }
            for (const Triangle &triangle : piece.mesh.Triangles()) {
                const Point near = NearestOnTriangle(eye, triangle);
                float       n[3] = {eye[0] - near[0], eye[1] - near[1], eye[2] - near[2]};
                const float distance = std::hypot(n[0], n[1], n[2]);
                if (distance >= most || distance <= 0.0f) {
                    continue;
                }
                n[0] /= distance, n[1] /= distance, n[2] /= distance;
                const float needed = Clearance(view, n);
                if (distance < needed) {
                    for (int axis = 0; axis < 3; axis++) {
                        eye[axis] += n[axis] * (needed - distance);
                    }
                }
            }
        }
    }
    return eye;
}

// Whether the line from the pivot to the eye is clear (its low walls passed over), and if not where the town cuts it.
bool Clear(const Point &pivot, const Point &eye, Point &cut) {
    Point from = pivot;
    for (int wall = 0;; wall++) {
        const float t = Sweep(from, eye, kSightRadius);
        if (t >= 1.0f) {
            return true;
        }
        cut = Lerp(from, eye, t);
        Point past{};
        if (wall == 4 || !LowWall(cut, eye, past)) {
            return false;
        }
        from = past;
    }
}

// The eye stopped short of where the line to it is cut, its sphere's width nearer the pivot and out of what is near.
Point ShortOf(const Point &pivot, const Point &eye, const Point &cut, const Point &look) {
    const float way[3] = {eye[0] - pivot[0], eye[1] - pivot[1], eye[2] - pivot[2]};
    const float whole = std::hypot(way[0], way[1], way[2]);
    const float there = std::hypot(cut[0] - pivot[0], cut[1] - pivot[1], cut[2] - pivot[2]);
    if (whole <= 0.0f) {
        return pivot;
    }
    // Back from the cut as far as the view needs from the surface there; clearing the view pushes it the rest of the
    // way out.
    float back = kMargin;
    Point near{};
    if (Closest(cut, kSightRadius + 1.0f, near) < kSightRadius + 1.0f) {
        float       n[3] = {cut[0] - near[0], cut[1] - near[1], cut[2] - near[2]};
        const float length = std::hypot(n[0], n[1], n[2]);
        if (length > 0.0f) {
            n[0] /= length, n[1] /= length, n[2] /= length;
            back = Clearance(ViewOf(cut, look), n);
        }
    }
    if (there <= back) {
        return pivot;
    }
    return ClearView(Lerp(pivot, eye, (there - back) / whole), look);
}

// The faces a mesh whose triangles face out shows only from inside it.
constexpr gfx::CullMode kOutwardBack = gfx::CullMode::Back;

// Draws without their insides the meshes whose pass-through pieces the eye's sphere is in (none with no eye), so the
// eye sees out through them.
void Insides(const Point *eye) {
    for (dc::render::Draw3DVisual *visual : town.culled) {
        visual->inside_cull = gfx::CullMode::None;
    }
    town.culled.clear();
    if (eye == nullptr) {
        return;
    }
    for (auto &[frame, piece] : town.pieces) {
        for (const PassPart &part : piece.passing) {
            bool in = true;
            for (int axis = 0; axis < 3; axis++) {
                in = in && (*eye)[axis] >= part.min[axis] - kEyeRadius && (*eye)[axis] <= part.max[axis] + kEyeRadius;
            }
            if (in) {
                auto *visual = const_cast<dc::render::Draw3DVisual *>(piece.visual);
                visual->inside_cull = part.outward ? kOutwardBack : gfx::CullMode::Front;
                town.culled.push_back(visual);
                break;
            }
        }
    }
}

// The eye at `at`, lifted over the point it looks at as it nears it (or is pushed past it, away from where it would be
// at `eye`), so with a wall at the player's back it looks down at them rather than at the wall through their head.
Point Rise(const Point &look, const Point &eye, const Point &at) {
    const float way[2] = {eye[0] - look[0], eye[2] - look[2]};
    const float across = std::hypot(way[0], way[1]);
    if (across <= 0.0f) {
        return at;
    }
    const float along = ((at[0] - look[0]) * way[0] + (at[2] - look[2]) * way[1]) / across;
    const float lift = std::clamp((kRoom - along) / kRoom, 0.0f, 1.0f);
    if (lift <= 0.0f) {
        return at;
    }
    const Point top = {look[0] + way[0] / across * kLean, look[1] + kRise, look[2] + way[1] / across * kLean};
    return Lerp(at, Lerp(look, top, Sweep(look, top, kHeadRadius)), lift);
}

} // namespace

bool TownCameraOn(bool fishing) {
    return !fishing && OptionOn(kModernCamera);
}

void TownCameraSolve(CCamera *shown, CEditGround *ground, int mode) {
    if (shown != &MainCamera || ground == nullptr || mode != ED_MODE_WALK || viewMode != 0 || Chara == nullptr ||
        !MainCamera.follow_on || CCamera::StopCamera || !TownCameraOn(false)) {
        town.shown = -1.0f;
        Insides(nullptr);
        return;
    }
    Update(*ground);

    const Point chest = {Chara->pos[0], Chara->pos[1] + 0.5f * Chara->body_height, Chara->pos[2]};
    const Point look = {MainCamera.ref[0], MainCamera.ref[1], MainCamera.ref[2]};
    const Point pivot = PushOut(Lerp(chest, look, Sweep(chest, look, kPivotRadius)), kEyeRadius);
    const Point eye = {MainCamera.pos[0], MainCamera.pos[1], MainCamera.pos[2]};
    // Seen along a clear line, only the eye's own sphere is kept out of what is near it; else it stops short of the cut.
    Point      cut{};
    const bool seen = Clear(pivot, eye, cut);
    Point      solved = seen ? ClearView(eye, look) : ShortOf(pivot, eye, cut, look);
    // In at once, back out eased.
    const float wanted = std::hypot(solved[0] - look[0], solved[1] - look[1], solved[2] - look[2]);
    if (town.shown >= 0.0f && wanted > town.shown && wanted > 0.0f) {
        const float eased = town.shown + kEaseOut * (wanted - town.shown);
        solved = ClearView(Lerp(look, solved, eased / wanted), look);
    }
    town.shown = std::hypot(solved[0] - look[0], solved[1] - look[1], solved[2] - look[2]);
    const Point placed = solved == eye ? eye : Rise(look, eye, solved);
    Insides(&placed);
    if (placed != eye) {
        // Shown this frame only: the game eases its own eye on from where it was (TownCameraRestore).
        town.moved = true;
        town.own = eye;
        std::copy(placed.begin(), placed.end(), MainCamera.pos);
    }
}

void TownCameraRestore() {
    if (town.moved) {
        town.moved = false;
        std::copy(town.own.begin(), town.own.end(), MainCamera.pos);
    }
}

float TownCameraHighRise(float height, float lowest, bool fishing) {
    if (!TownCameraOn(fishing)) {
        return lowest;
    }
    return std::min(lowest, height + std::clamp(kHighRise * (lowest - height), kHighRiseLeast, kHighRiseMost));
}

void TownCameraReset() {
    // The meshes it drew without their insides go with the town.
    town = {};
}

} // namespace expanded
