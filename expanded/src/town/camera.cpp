#include "expanded/town/camera.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <vector>

#include "camerafollow.hpp"
#include "character.hpp"
#include "collision.hpp"
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
    std::vector<PassPart>           passing;        // its pass-through pieces, in the world
    bool                            hugged = false; // the camera passes it while the player stands on it (kHuggedParts)
    SweepMesh                       whole;          // all of it, its small steps too, when it is hugged
    std::uint32_t                   seen = 0;
};

struct Town {
    const CEditGround                        *ground = nullptr;
    int                                       map = -1;
    std::uint32_t                             generation = 0;
    std::unordered_map<const CFrame *, Piece> pieces;
    float                                     shown = -1.0f;               // how far from the point looked at the eye was shown
    bool                                      moved = false;               // whether the eye shown is not the game's own
    float                                     out = 0.0f;                  // how fast it was easing back out last frame
    float                                     in = 0.0f;                   // how fast it was moving in last frame
    bool                                      on_hugged = false;           // whether the player stands on a hugged part
    float                                     lift = 0.0f;                 // how far over the ground it was lifted last frame...
    Point                                     pad{};                       // ...and pushed off what is near its view
    bool                                      held = false;                // whether it was solved last frame...
    Point                                     last_own{};                  // ...from the game's own eye here...
    Point                                     last_shown{};                // ...showing the eye here
    std::vector<dc::render::Draw3DVisual *>   culled;                      // the meshes drawn without their insides
    Point                                     own{};                       // the game's own eye and the point it looks at, put back
    Point                                     own_look{};                  // once the frame is drawn
    float                                     aim[3] = {0.0f, 0.0f, 1.0f}; // the way the game's own eye looks
    bool                                      faded = false;               // whether the player is drawn faded this frame...
    float                                     player_alpha = 1.0f;         // ...this much of the way seen...
    float                                     own_alpha = -1.0f;           // ...over the alpha it had
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

// Parts whose blocking the camera sets beyond what their size would, by town and the start of their frames' names (or
// the whole name, for names that start others), and the part they are in (a root frame's name) where that is given:
// all of each part blocks, its tall pieces block however narrow (its small trim still passing), none of it does, or all
// but its pillars does.
// The first that fits a frame holds.
enum class Blocks {
    All,
    Tall,
    None,
    AllButPillars
};

struct BlockingPart {
    int         map;
    const char *frames;
    Blocks      blocks;
    bool        whole_name = false;
    const char *under = nullptr;
};

constexpr BlockingPart kBlockingParts[] = {
    {.map = 0, .frames = "door", .blocks = Blocks::Tall}, // Norune's doors to the Divine Beast's cave
    {.map = 1, .frames = "suisya", .blocks = Blocks::All}, // Matataki's watermills: the wheels' housings...
    {.map = 1, .frames = "kuruma", .blocks = Blocks::All}, // ...the wheels...
    {.map = 1, .frames = "obj5", .blocks = Blocks::All, .whole_name = true}, // ...and the mill houses
    {.map = 1, .frames = "h01", .blocks = Blocks::Tall}, // Pao's house's pillars
    {.map = 1, .frames = "h04", .blocks = Blocks::Tall}, // Kye & Momo's house's walls
    {.map = 2, .frames = "hiyoke", .blocks = Blocks::None, .under = "eo3h09"}, // Queens: King's Hideout's sunshade
    {.map = 3, .frames = "e04a", .blocks = Blocks::Tall}, // Muska Lacka's totem poles: on their bases...
    {.map = 3, .frames = "1", .blocks = Blocks::All, .whole_name = true}, // ...their middles...
    {.map = 3, .frames = "2", .blocks = Blocks::All, .whole_name = true}, // ...and tops
    {.map = 4, .frames = "hasigo", .blocks = Blocks::None, .under = "e05a01"}, // the Moon Factory's Lookout: its ladders
    {.map = 4, .frames = "", .blocks = Blocks::All, .under = "e05a01"}, // pass, the rest blocks
    {.map = 11, .frames = "obj70", .blocks = Blocks::All}, // Goro's house...
    {.map = 11, .frames = "obj71", .blocks = Blocks::All},
    {.map = 11, .frames = "nuki", .blocks = Blocks::All},
    {.map = 11, .frames = "win", .blocks = Blocks::All}, // ...its windows...
    {.map = 11, .frames = "obj66", .blocks = Blocks::All},
    {.map = 11, .frames = "ki", .blocks = Blocks::All}, // ...and the trees round it, trunks and crowns
    {.map = 11, .frames = "ha1", .blocks = Blocks::All},
    {.map = 14, .frames = "h03", .blocks = Blocks::Tall}, // Brownboo's stilt houses' legs (its central building's, a
                                                                // walkway running among them, pass)
    {.map = 19, .frames = "hasi", .blocks = Blocks::None}, // East Harbor's walkway out to the submarine
    {.map = 23, .frames = "kanban", .blocks = Blocks::None}, // Yellow Drops: its fishing sign passes...
    {.map = 23, .frames = "", .blocks = Blocks::All}, // ...everything else blocks
    {.map = 24, .frames = "", .blocks = Blocks::All}, // Dark Heaven Castle from outside, far...
    {.map = 40, .frames = "", .blocks = Blocks::All}, // ...near...
    {.map = 50, .frames = "", .blocks = Blocks::All}, // ...and far again
    {.map = 38, .frames = "", .blocks = Blocks::AllButPillars}, // inside, its entrance: only the pillars carrying flames pass
};

// Parts the player climbs round (their stairs wind about their walls, parapets and terraces): while the player stands on
// one, the line to the eye passes them all, though they hide the player a moment, rather than the eye moving in; not
// with the eye inside one, which it moves in out of as from any wall. The eye's view still keeps off them.
struct HuggedPart {
    int         map;
    const char *frames;
    const char *under = nullptr; // the part they are in (a root frame's name), where that is given
};

constexpr HuggedPart kHuggedParts[] = {
    {.map = 2, .frames = "betu"}, // Queens: Jack's Store's annex, its stairs up the side
    {.map = 2, .frames = "eo3h09"}, // King's Hideout, its stairs up to the roof
    {.map = 2, .frames = "e03h08"}, // Basker's Store...
    {.map = 2, .frames = "2f", .under = "e03h08"}, // ...and its upper floor, its stairs up the side and the back
};

bool Named(const char *name, const BlockingPart &part) {
    return part.whole_name ? std::strcmp(name, part.frames) == 0
                           : std::strncmp(name, part.frames, std::strlen(part.frames)) == 0;
}

// Whether a frame is in the part rooted at a frame of this name.
bool Under(const CFrame &frame, const char *root) {
    for (const CFrame *up = frame.reference != 0 ? nullptr : frame.parent; up != nullptr;
         up = up->reference != 0 ? nullptr : up->parent) {
        if (std::strncmp(up->name, root, std::strlen(root)) == 0) {
            return true;
        }
    }
    return false;
}

const BlockingPart *BlockingPartOf(int map, const CFrame &frame) {
    for (const BlockingPart &part : kBlockingParts) {
        if (part.map == map && Named(frame.name, part) && (part.under == nullptr || Under(frame, part.under))) {
            return &part;
        }
    }
    return nullptr;
}

bool Hugged(int map, const CFrame &frame) {
    for (const HuggedPart &part : kHuggedParts) {
        if (part.map == map && std::strncmp(frame.name, part.frames, std::strlen(part.frames)) == 0 &&
            (part.under == nullptr || Under(frame, part.under))) {
            return true;
        }
    }
    return false;
}

void Build(Piece &piece, const dc::render::Draw3DVisual &visual, const float matrix[4][4], double shape, const CFrame &frame) {
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
    const BlockingPart *part = BlockingPartOf(MapNo, frame);
    Blocking            blocking;
    blocking.tall_blocks = part != nullptr && part->blocks == Blocks::Tall;
    blocking.pillars_only = part != nullptr && part->blocks == Blocks::AllButPillars;
    const std::vector<bool> large = part != nullptr && (part->blocks == Blocks::All || part->blocks == Blocks::None)
                                        ? std::vector<bool>(triangles.size(), part->blocks == Blocks::All)
                                        : BlockingMask(triangles, blocking);
    piece.passing = PassThroughParts(triangles, large);
    piece.hugged = Hugged(MapNo, frame);
    piece.whole.Build(piece.hugged ? triangles : std::vector<Triangle>{});

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
                Build(piece, *built, matrix, shape, *frame);
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

// Whether the line to the player passes a piece: none, or a hugged part while the player stands on one (the eye's view
// still keeps off it).
bool Passes(const Piece &piece) {
    return piece.mesh.Empty() || (piece.hugged && town.on_hugged);
}

float Sweep(const Point &from, const Point &to, float radius) {
    float best = 1.0f;
    for (const auto &[frame, piece] : town.pieces) {
        if (!Passes(piece) && Meets(piece.mesh, from, to, radius)) {
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

// Whether the player stands on a piece (its stairs, its terrace): it lies within kStandsOn of their feet, up or down
// (the drawn step need not sit where they tread), there or a little way round them (the edge of a step).
constexpr float kStandsOn = 3.0f;
constexpr float kFootSpread = 2.0f;

bool StandsOn(const Piece &piece) {
    constexpr float kAround[5][2] = {
        {0.0f,  0.0f },
        {1.0f,  0.0f },
        {-1.0f, 0.0f },
        {0.0f,  1.0f },
        {0.0f,  -1.0f}
    };
    for (const auto &[x, z] : kAround) {
        const Point above = {Chara->pos[0] + x * kFootSpread, Chara->pos[1] + kStandsOn, Chara->pos[2] + z * kFootSpread};
        const Point under = {above[0], Chara->pos[1] - kStandsOn, above[2]};
        if (piece.whole.Ray(above, under) < 1.0f) {
            return true;
        }
    }
    return false;
}

// Whether the eye is inside a hugged part: some of it (of any of its meshes: a floor's roof, the floor under) lies
// straight over the eye, and straight under it or round it on all but one side (a hollow building drawn with no floor;
// under an overhang outside, more of it is open).
constexpr int kWalledSides = 3;

bool InsideHugged(const Point &eye) {
    bool over = false, under = false;
    bool sides[4] = {};
    for (const auto &[frame, piece] : town.pieces) {
        if (!piece.hugged || piece.whole.Empty()) {
            continue;
        }
        const Point &low = piece.whole.Min();
        const Point &high = piece.whole.Max();
        const Point  top = {eye[0], high[1] + 1.0f, eye[2]};
        const Point  bottom = {eye[0], low[1] - 1.0f, eye[2]};
        over = over || (top[1] > eye[1] && piece.whole.Ray(eye, top) < 1.0f);
        under = under || (bottom[1] < eye[1] && piece.whole.Ray(eye, bottom) < 1.0f);
        const Point ends[4] = {
            {high[0] + 1.0f, eye[1], eye[2]        },
            {low[0] - 1.0f,  eye[1], eye[2]        },
            {eye[0],         eye[1], high[2] + 1.0f},
            {eye[0],         eye[1], low[2] - 1.0f }
        };
        for (int side = 0; side < 4; side++) {
            sides[side] = sides[side] || piece.whole.Ray(eye, ends[side]) < 1.0f;
        }
    }
    const int walled = int(sides[0]) + int(sides[1]) + int(sides[2]) + int(sides[3]);
    return over && (under || walled >= kWalledSides);
}

// Whether the player stands on a hugged part (any of its meshes: a building's upper floor carries its stairs).
bool OnHugged() {
    for (const auto &[frame, piece] : town.pieces) {
        if (piece.hugged && !piece.whole.Empty() && StandsOn(piece)) {
            return true;
        }
    }
    return false;
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

// How far the eye is lifted over ground at the most, and the halvings that find the least lift.
constexpr float kGroundLift = 60.0f;

// How near straight up the line meets ground from: at least this share of its way out of the surface; the line to the
// feet, meeting steps at their edges, less.
constexpr float kGroundFacing = 0.5f;
constexpr float kFeetGroundFacing = 0.3f;

// The player's feet the eye keeps in view over ground, this far over where they stand.
constexpr float kFeetRise = 3.0f;

// The share of the way up toward showing the feet the eye rises a frame, and back down as the ground falls away; it
// rises at once as far as the line to the head needs.
constexpr float kLiftRise = 0.15f;
constexpr float kLiftFall = 0.1f;

// How many frames ahead along its way the eye looks for ground it will need to rise off, every so many frames.
constexpr int kAhead = 8;
constexpr int kAheadStep = 2;

// How many frames ahead it looks for walls it will be pulled in by (every kAheadStep), moving in at a pace to be there
// by then.
constexpr int kPullAhead = 16;

// How much its pace in may gather or slacken a frame.
constexpr float kInGather = 0.75f;

// Ground the line meets counts so no higher than a step over where it meets it, nor further under it than this, within
// this far across of it; and the collision faces gathered there at the most.
constexpr float kStepRise = 16.0f;
constexpr float kTreadSlack = kSightRadius + 1.5f; // the line's sphere stops its radius off the face
constexpr float kFootReach = 4.0f;
constexpr int   kPolyRoom = 4096;
constexpr int   kLiftSearches = 8;

// The share of the way back out the eye goes each frame once the line to the player clears, its pace gathering by at
// most kOutGather a frame so that it does not lurch out the moment a wall stops cutting it.
constexpr float kEaseOut = 0.1f;
constexpr float kOutGather = 0.3f;

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

// The eye's view: the near plane's rectangle out in front of it, looking the way the game's own eye does wherever it
// is moved to, past the point it looks at too.
struct View {
    float forward[3], right[3], up[3];
    float half_width, half_height;
};

View ViewOf() {
    View  view{};
    float f[3] = {town.aim[0], town.aim[1], town.aim[2]};
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

// The eye moved out of what blocks until its view clears every surface near it (not the hugged parts the player stands
// on, unless `hugged`).
Point ClearView(Point eye, bool hugged = true) {
    const View  view = ViewOf();
    const float most = kMargin + std::hypot(kNear, view.half_width, view.half_height);
    for (int pass = 0; pass < 2; pass++) {
        for (const auto &[frame, piece] : town.pieces) {
            if (piece.mesh.Empty() || (!hugged && piece.hugged && town.on_hugged) || !Meets(piece.mesh, eye, eye, most)) {
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

// Whether what cuts the line at `cut` is ground: the ground the player walks on (their own collision's walkable
// faces) lies under it, or no more than a step over it (a step's tread behind its riser). Roofs, statues and walls, with
// no walkable face so near, are not.
bool Ground(const Point &cut) {
    if (town.ground == nullptr) {
        return false;
    }
    CBoxVu0 box{};
    box.min[0] = cut[0] - kFootReach, box.max[0] = cut[0] + kFootReach;
    box.min[1] = cut[1] - kTreadSlack, box.max[1] = cut[1] + kStepRise;
    box.min[2] = cut[2] - kFootReach, box.max[2] = cut[2] + kFootReach;
    static std::vector<CCPoly> polys(kPolyRoom);
    const int                  count = const_cast<CEditGround *>(town.ground)->PickUpPoly(polys.data(), box, 0);
    for (int i = 0; i < std::min(count, kPolyRoom); i++) {
        const CCPoly &poly = polys[i];
        const float   length = std::hypot(poly.normal[0], poly.normal[1], poly.normal[2]);
        if (length <= 0.0f || poly.normal[1] < kGroundFacing * length) {
            continue;
        }
        // The face's height under the cut.
        const float height = poly.vertex[0][1] - (poly.normal[0] * (cut[0] - poly.vertex[0][0]) +
                                                  poly.normal[2] * (cut[2] - poly.vertex[0][2])) /
                                                     poly.normal[1];
        if (height >= cut[1] - kTreadSlack && height <= cut[1] + kStepRise) {
            return true;
        }
    }
    return false;
}

// The eye lifted over the ground that cuts the line to it at `cut` (a slope or stairs behind a player going down them,
// a crest), as little as clears the line; false when what cuts it is no ground or no lift within reach clears it.
bool Lifted(const Point &pivot, const Point &eye, const Point &cut, Point &lifted) {
    Point again{};
    if (!Ground(cut) || !Clear(pivot, {eye[0], eye[1] + kGroundLift, eye[2]}, again)) {
        return false;
    }
    float low = 0.0f, high = kGroundLift;
    for (int step = 0; step < kLiftSearches; step++) {
        const float mid = 0.5f * (low + high);
        if (Clear(pivot, {eye[0], eye[1] + mid, eye[2]}, again)) {
            high = mid;
        } else {
            low = mid;
        }
    }
    lifted = {eye[0], eye[1] + high, eye[2]};
    return true;
}

// Whether ground hides the player's feet from the eye: the line down to them meets it from above.
bool FeetHidden(const Point &eye, const Point &feet) {
    const float t = Sweep(eye, feet, kSightRadius);
    if (t >= 1.0f) {
        return false;
    }
    const Point cut = Lerp(eye, feet, t);
    Point       near{};
    if (Closest(cut, kSightRadius + 1.0f, near) >= kSightRadius + 1.0f) {
        return false;
    }
    const float out[3] = {cut[0] - near[0], cut[1] - near[1], cut[2] - near[2]};
    return out[1] >= kFeetGroundFacing * std::hypot(out[0], out[1], out[2]) && Ground(cut);
}

// The least lift that shows the player's feet over the ground between them and the eye (stairs or a slope going down
// before the eye), or none when nothing hides them or no lift within reach shows them.
float FeetLift(const Point &eye, const Point &feet) {
    if (!FeetHidden(eye, feet) || FeetHidden({eye[0], eye[1] + kGroundLift, eye[2]}, feet)) {
        return 0.0f;
    }
    float low = 0.0f, high = kGroundLift;
    for (int step = 0; step < kLiftSearches; step++) {
        const float mid = 0.5f * (low + high);
        if (FeetHidden({eye[0], eye[1] + mid, eye[2]}, feet)) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return high;
}

// The eye stopped short of where the line to it is cut, its sphere's width nearer the pivot and out of what is near.
Point ShortOf(const Point &pivot, const Point &eye, const Point &cut) {
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
            back = Clearance(ViewOf(), n);
        }
    }
    if (there <= back) {
        return pivot;
    }
    return ClearView(Lerp(pivot, eye, (there - back) / whole));
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

// How the eye's place on the game's line of sight is found (stepped in by the stride, then halved), and how thin the
// line it must see the point looked at along (a sphere's sweep, thin enough to pass a wall its view keeps the margin
// from).
constexpr float kSightStride = 1.0f;
constexpr int   kSightSteps = 8;
constexpr float kSightLine = 0.05f;

// Whether the eye at `eye` is inside a hugged part, or so near one its view would meet it.
bool InHugged(const Point &eye) {
    if (InsideHugged(eye)) {
        return true;
    }
    const View  view = ViewOf();
    const float room = kMargin + std::hypot(kNear, view.half_width, view.half_height);
    for (const auto &[frame, piece] : town.pieces) {
        Point nearest{};
        if (piece.hugged && !piece.mesh.Empty() && Meets(piece.mesh, eye, eye, room) && piece.mesh.Closest(eye, room, nearest) < room) {
            return true;
        }
    }
    return false;
}

// The eye pulled in on the game's own line of sight, from `eye` toward `look`, as far out as `most` from `look` with its
// view clear of what stands near it, nothing between it and `look` and not inside a hugged part: the player stays where
// the game frames them, nearer. `placed` when not even the point looked at is.
Point OnSightLine(const Point &look, const Point &eye, float most, const Point &placed) {
    const float whole = std::hypot(eye[0] - look[0], eye[1] - look[1], eye[2] - look[2]);
    if (whole <= 0.0f) {
        return placed;
    }
    // A point on the line, raised off the ground under its view if it must be but never pushed aside: what stands near
    // it (a wall) it keeps clear of by being nearer.
    auto at = [&](float distance) { return ClearView(Lerp(look, eye, distance / whole)); };
    auto fits = [&](float distance) {
        const Point on = Lerp(look, eye, distance / whole);
        const Point cleared = ClearView(on);
        const Point walls = ClearView(on, false);
        return std::hypot(walls[0] - on[0], walls[2] - on[2]) < 0.01f && Sweep(look, cleared, kSightLine) >= 1.0f &&
               !(town.on_hugged && InsideHugged(cleared));
    };
    float out = std::min(most, whole);
    if (fits(out)) {
        return at(out);
    }
    // Stepped in from the far end, so that what it nears on the way (a ledge beside the line) does not stop it short.
    float in = out;
    do {
        in = std::max(0.0f, in - kSightStride);
        if (fits(in)) {
            break;
        }
        out = in;
    } while (in > 0.0f);
    if (!fits(in)) {
        return placed;
    }
    for (int step = 0; step < kSightSteps; step++) {
        const float middle = 0.5f * (in + out);
        (fits(middle) ? in : out) = middle;
    }
    return at(in);
}

// The player's body, as the camera fades it: upright from their feet to their height, as wide as they are.
constexpr float kBodyRadiusShare = 0.5f;

// The player fades out over this distance before the near plane reaches their body, and is not drawn once it does.
constexpr float kFadeReach = 5.0f;

// How much of the player the eye at `eye` shows: 1 clear of their body by the fade's reach, 0 with its near plane in it.
float PlayerSeen(const Point &eye) {
    const float radius = kBodyRadiusShare * Chara->body_width;
    const float low = Chara->pos[1] + radius;
    const float high = std::max(low, Chara->pos[1] + Chara->body_height - radius);
    const Point axis = {Chara->pos[0], std::clamp(eye[1], low, high), Chara->pos[2]};
    float       n[3] = {eye[0] - axis[0], eye[1] - axis[1], eye[2] - axis[2]};
    const float distance = std::hypot(n[0], n[1], n[2]);
    if (distance <= radius) {
        return 0.0f;
    }
    n[0] /= distance, n[1] /= distance, n[2] /= distance;
    const float gap = distance - radius - (Clearance(ViewOf(), n) - kMargin);
    return std::clamp(gap / kFadeReach, 0.0f, 1.0f);
}

// The eye shown at `at` for the frame drawn, looking at the point the game's eye looks at (lifted over the ground it is
// off the game's line) but, near it, the way the game's own eye does, so that it does not turn about in the player's
// head.
constexpr float kAimNear = 4.0f;
constexpr float kAimFar = 10.0f;

void Show(const Point &at) {
    town.moved = true;
    town.own = {MainCamera.pos[0], MainCamera.pos[1], MainCamera.pos[2]};
    town.own_look = {MainCamera.ref[0], MainCamera.ref[1], MainCamera.ref[2]};
    const float game[3] = {town.own_look[0] - town.own[0], town.own_look[1] - town.own[1], town.own_look[2] - town.own[2]};
    const float reach = std::hypot(game[0], game[1], game[2]);
    const float toward[3] = {town.own_look[0] - at[0], town.own_look[1] - at[1], town.own_look[2] - at[2]};
    const float near = std::hypot(toward[0], toward[1], toward[2]);
    const float share = near > 0.0f ? std::clamp((near - kAimNear) / (kAimFar - kAimNear), 0.0f, 1.0f) : 0.0f;
    float       way[3];
    for (int axis = 0; axis < 3; axis++) {
        way[axis] = (reach > 0.0f ? (1.0f - share) * game[axis] / reach : 0.0f) + (near > 0.0f ? share * toward[axis] / near : 0.0f);
    }
    const float length = std::hypot(way[0], way[1], way[2]);
    for (int axis = 0; axis < 3; axis++) {
        MainCamera.ref[axis] = length > 0.0f ? at[axis] + way[axis] / length * reach : town.own_look[axis];
        MainCamera.pos[axis] = at[axis];
    }
}

// The player drawn as faded as the eye shows them, this frame only.
void FadePlayer() {
    if (Chara == nullptr || town.player_alpha >= 1.0f) {
        return;
    }
    town.faded = true;
    town.own_alpha = Chara->ambient_offset[3];
    const float alpha = town.own_alpha < 0.0f ? 128.0f : town.own_alpha;
    Chara->ambient_offset[3] = alpha * town.player_alpha;
}

} // namespace

bool TownCameraOn() {
    return OptionOn(kModernCamera);
}

void TownCameraSolve(CCamera *shown, CEditGround *ground, int mode) {
    if (shown != &MainCamera || ground == nullptr || (mode != ED_MODE_WALK && mode != ED_MODE_FISHING) ||
        viewMode != 0 || Chara == nullptr || !MainCamera.follow_on || CCamera::StopCamera || !TownCameraOn()) {
        // Paused or changing the time of day, the game's eye stands still: it is shown where it last was.
        const Point own = {MainCamera.pos[0], MainCamera.pos[1], MainCamera.pos[2]};
        if (shown == &MainCamera && town.held && own == town.last_own && TownCameraOn()) {
            Show(town.last_shown);
            FadePlayer();
            return;
        }
        town.held = false;
        town.out = 0.0f;
        town.in = 0.0f;
        town.player_alpha = 1.0f;
        town.shown = -1.0f;
        town.lift = 0.0f;
        town.pad = {};
        Insides(nullptr);
        return;
    }
    Update(*ground);
    town.on_hugged = OnHugged() && !InsideHugged({MainCamera.pos[0], MainCamera.pos[1], MainCamera.pos[2]});

    const Point chest = {Chara->pos[0], Chara->pos[1] + 0.5f * Chara->body_height, Chara->pos[2]};
    const Point look = {MainCamera.ref[0], MainCamera.ref[1], MainCamera.ref[2]};
    // Walking it pulls in toward the player's head, brought out from their chest; fishing, toward the float it watches,
    // whatever lies between it and the player.
    const Point from = mode == ED_MODE_FISHING ? look : Lerp(chest, look, Sweep(chest, look, kPivotRadius));
    const Point pivot = PushOut(from, kEyeRadius);
    const Point eye = {MainCamera.pos[0], MainCamera.pos[1], MainCamera.pos[2]};
    if (const float length = std::hypot(look[0] - eye[0], look[1] - eye[1], look[2] - eye[2]); length > 0.0f) {
        for (int axis = 0; axis < 3; axis++) {
            town.aim[axis] = (look[axis] - eye[axis]) / length;
        }
    }
    // Seen along a clear line, only the eye's own sphere is kept out of what is near it; else it stops short of the cut.
    Point      cut{};
    const bool seen = Clear(pivot, eye, cut);
    // Under the ground behind the player it rises out, keeping its distance; cut by anything else it moves in.
    Point       lifted{};
    const bool  lifts = !seen && Lifted(pivot, eye, cut, lifted);
    const float need = lifts ? lifted[1] - eye[1] : 0.0f;
    // Over ground it rises at a pace to keep the player's feet in view too (walking, not fishing), and settles slowly.
    const Point feet = {Chara->pos[0], Chara->pos[1] + kFeetRise, Chara->pos[2]};
    const float aim = !(seen || lifts) ? 0.0f : mode == ED_MODE_WALK ? std::max(need, FeetLift(eye, feet))
                                                                     : need;
    float       lift = town.lift + (aim > town.lift ? kLiftRise : kLiftFall) * (aim - town.lift);
    lift = std::max(lift, need);
    if (lift > need && !Clear(pivot, {eye[0], eye[1] + lift, eye[2]}, lifted)) {
        lift = need; // not into what hangs over it
    }
    town.lift = lift;
    Point solved = !(seen || lifts) ? ShortOf(pivot, eye, cut) : ClearView({eye[0], eye[1] + lift, eye[2]});
    if (!(seen || lifts)) {
        // Cut, it moves in along the game's own line of sight, no further out than stopping short of the cut put it.
        const float most = std::hypot(solved[0] - look[0], solved[1] - look[1], solved[2] - look[2]) + kMargin;
        solved = OnSightLine(look, eye, most, solved);
    }
    // Pushed off what is near its view as far as the margin needs at once, the rest at a pace, and let go slowly, so
    // the faces of a slope or a corner passing by do not jolt it.
    if (seen || lifts) {
        const Point base = {eye[0], eye[1] + lift, eye[2]};
        const float push[3] = {solved[0] - base[0], solved[1] - base[1], solved[2] - base[2]};
        const float size = std::hypot(push[0], push[1], push[2]);
        // What it will need where it is heading, a few frames on, it starts rising for now.
        float aim[3] = {push[0], push[1], push[2]};
        for (int frames = kAheadStep; town.held && frames <= kAhead; frames += kAheadStep) {
            const Point ahead = {base[0] + frames * (eye[0] - town.last_own[0]), base[1] + frames * (eye[1] - town.last_own[1]),
                                 base[2] + frames * (eye[2] - town.last_own[2])};
            aim[1] = std::max(aim[1], ClearView(ahead)[1] - ahead[1]);
        }
        const float before = std::hypot(town.pad[0], town.pad[1], town.pad[2]);
        const float pace = std::hypot(aim[0], aim[1], aim[2]) > before ? kLiftRise : kLiftFall;
        Point       eased{};
        for (int axis = 0; axis < 3; axis++) {
            eased[axis] = town.pad[axis] + pace * (aim[axis] - town.pad[axis]);
        }
        if (size > 0.0f) {
            const float along = (eased[0] * push[0] + eased[1] * push[1] + eased[2] * push[2]) / size;
            const float least = size - kMargin;
            if (along < least) {
                for (int axis = 0; axis < 3; axis++) {
                    eased[axis] += push[axis] / size * (least - along);
                }
            }
        }
        const Point padded = {base[0] + eased[0], base[1] + eased[1], base[2] + eased[2]};
        if (Clear(pivot, padded, lifted)) {
            town.pad = eased;
            solved = padded;
        } else {
            town.pad = {push[0], push[1], push[2]};
        }
        // Kept off a wall beside its view by moving in along its line, not aside, as when the wall cuts the line, so the
        // player does not shift on screen nor the eye jump as the wall starts or stops cutting it.
        // (A hugged part's wall it is only kept off aside: moving in along the line would take it through the part.)
        if (const Point walls = ClearView(base, false); std::hypot(walls[0] - base[0], walls[2] - base[2]) > 0.01f) {
            const Point raised = {base[0], solved[1], base[2]};
            const float whole = std::hypot(raised[0] - look[0], raised[1] - look[1], raised[2] - look[2]);
            solved = OnSightLine(look, raised, whole, solved);
        }
    } else {
        town.pad = {};
    }
    // Where it is turning or heading to it will be pulled in, a few frames on, it starts moving in toward now, so the pull
    // in does not start at once: each frame as far as it must to be in that far by then, its pace changing gradually,
    // and no further in than the furthest in it will need. Never further out than now.
    float soon = std::numeric_limits<float>::max(); // the furthest in it will need to be, a few frames on
    if (town.held && town.shown >= 0.0f) {
        const float now = std::hypot(solved[0] - look[0], solved[1] - look[1], solved[2] - look[2]);
        float       pace = 0.0f;   // how far in it moves this frame to be as far in as each pull-in by the time it comes...
        float       deepest = now; // ...and no further in than the furthest in of them
        for (int frames = kAheadStep; frames <= kPullAhead; frames += kAheadStep) {
            const Point later = {eye[0] + frames * (eye[0] - town.last_own[0]), eye[1] + frames * (eye[1] - town.last_own[1]),
                                 eye[2] + frames * (eye[2] - town.last_own[2])};
            Point       later_cut{}, later_lifted{}, there{};
            if (Clear(pivot, later, later_cut)) {
                // Clear, but beside a wall it will move in along its line for.
                const Point cleared = ClearView(later);
                if (const Point walls = ClearView(later, false); std::hypot(walls[0] - later[0], walls[2] - later[2]) <= 0.01f) {
                    continue;
                }
                const Point raised = {later[0], cleared[1], later[2]};
                there = OnSightLine(look, raised, std::hypot(raised[0] - look[0], raised[1] - look[1], raised[2] - look[2]),
                                    cleared);
            } else if (Lifted(pivot, later, later_cut, later_lifted)) {
                continue;
            } else {
                const Point stop = ShortOf(pivot, later, later_cut);
                const float most = std::hypot(stop[0] - look[0], stop[1] - look[1], stop[2] - look[2]) + kMargin;
                there = OnSightLine(look, later, most, stop);
            }
            const float ahead = std::hypot(there[0] - look[0], there[1] - look[1], there[2] - look[2]);
            pace = std::max(pace, (town.shown - ahead) / static_cast<float>(frames));
            deepest = std::min(deepest, ahead);
        }
        // Its pace in gathers and slackens by at most kInGather a frame.
        soon = deepest;
        pace = std::clamp(pace, town.in - kInGather, town.in + kInGather);
        // ...and slackens in time to stop where it will need to be furthest in.
        pace = std::min(pace, std::sqrt(2.0f * kInGather * std::max(town.shown - deepest, 0.0f)));
        if (float toward = std::max(town.shown - pace, deepest); pace > 0.0f && toward < now && now > 0.0f) {
            // Moving in into a hugged part, it goes on at once to the near side of it, rather than stopping inside it.
            while (town.on_hugged && toward > 0.0f && InHugged(Lerp(look, solved, toward / now))) {
                toward = std::max(0.0f, toward - kSightStride);
            }
            solved = Lerp(look, solved, toward / now);
        }
    }
    // In at once, back out eased.
    const float wanted = std::hypot(solved[0] - look[0], solved[1] - look[1], solved[2] - look[2]);
    if (town.shown >= 0.0f && wanted > town.shown && wanted > 0.0f) {
        // Gathering pace, and slackening in time to stop where it will soon be pulled in to.
        town.out = std::min({kEaseOut * (wanted - town.shown), town.out + kOutGather,
                             std::sqrt(2.0f * kOutGather * std::max(soon - town.shown, 0.0f))});
        float eased = town.shown + town.out;
        // On the game's line of sight (lifted over the ground if it is), moved in along it rather than aside to keep its
        // view clear.
        const Point line = seen || lifts ? Point{eye[0], eye[1] + lift, eye[2]} : eye;
        // Easing out into a hugged part (or so near it its view would meet it), it goes on at once to the far side of it,
        // rather than stopping inside it.
        const float reach = std::hypot(line[0] - look[0], line[1] - look[1], line[2] - look[2]);
        if (town.on_hugged && reach > 0.0f) {
            while (eased < wanted && InHugged(Lerp(look, line, eased / reach))) {
                eased = std::min(wanted, eased + kSightStride);
            }
        }
        solved = OnSightLine(look, line, eased, ClearView(Lerp(look, solved, eased / wanted)));
    } else {
        town.out = 0.0f;
    }
    const float distance = std::hypot(solved[0] - look[0], solved[1] - look[1], solved[2] - look[2]);
    // Its paces out and in are how far it went: held in, it gathers none.
    town.out = town.shown >= 0.0f ? std::clamp(distance - town.shown, 0.0f, town.out) : 0.0f;
    town.in = town.shown >= 0.0f ? std::max(town.shown - distance, 0.0f) : 0.0f;
    town.shown = distance;
    const Point placed = solved;
    Insides(&placed);
    town.held = true;
    town.last_own = eye;
    town.last_shown = placed;
    town.player_alpha = PlayerSeen(placed);
    if (placed != eye) {
        Show(placed);
    }
    FadePlayer();
}

void TownCameraRestore() {
    if (town.moved) {
        town.moved = false;
        std::copy(town.own.begin(), town.own.end(), MainCamera.pos);
        std::copy(town.own_look.begin(), town.own_look.end(), MainCamera.ref);
    }
    if (town.faded) {
        town.faded = false;
        if (Chara != nullptr) {
            Chara->ambient_offset[3] = town.own_alpha;
        }
    }
}

bool TownCameraHidesPlayer(const dc::game::CCharacter *player) {
    return town.faded && player == Chara && town.player_alpha <= 0.0f;
}

float TownCameraHighRise(float height, float lowest) {
    if (!TownCameraOn()) {
        return lowest;
    }
    return std::min(lowest, height + std::clamp(kHighRise * (lowest - height), kHighRiseLeast, kHighRiseMost));
}

void TownCameraReset() {
    // The meshes it drew without their insides go with the town.
    town = {};
}

} // namespace expanded
