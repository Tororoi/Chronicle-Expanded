#pragma once

#include <array>
#include <cstdint>
#include <vector>

// The geometry the town camera collides with: triangles in a bounding-volume tree, swept by a sphere.

namespace expanded {

using Point = std::array<float, 3>;

struct Triangle {
    Point a, b, c;
};

// Triangles a sphere is swept through.
class SweepMesh {
public:
    // Takes the triangles, leaving out those with no area.
    void Build(std::vector<Triangle> triangles);

    bool Empty() const { return triangles_.empty(); }

    const std::vector<Triangle> &Triangles() const { return triangles_; }

    // The corners of the box round every triangle.
    const Point &Min() const { return min_; }

    const Point &Max() const { return max_; }

    // How far (0 to 1) from `from` to `to` a sphere of `radius` goes before it touches a triangle, at most `limit`.
    // A triangle the sphere already touches at `from` stops it only while it moves nearer it.
    float Sweep(const Point &from, const Point &to, float radius, float limit = 1.0f) const;

    // The point moved out of every triangle nearer it than `radius`, by `most` at the most.
    Point PushOut(const Point &point, float radius, float most) const;

    // How far (0 to 1) from `from` to `to` a line goes before it meets a triangle, at most `limit`.
    float Ray(const Point &from, const Point &to, float limit = 1.0f) const;

    // Whether a triangle comes nearer the point than `radius`.
    bool Touches(const Point &point, float radius) const;

    // Adds to `push` the way out of each triangle within `radius` of the point, weighted by how far within it is.
    void Push(const Point &point, float radius, Point &push) const;

    // The nearest point of the triangles to `point` within `radius`, and its distance; `radius` when none is.
    float Closest(const Point &point, float radius, Point &nearest) const;

private:
    struct Node {
        Point         min, max;
        std::uint32_t first; // a leaf's first triangle, or a branch's second child (its first follows it)
        std::uint32_t count; // a leaf's triangles; 0 for a branch
    };

    std::uint32_t BuildNode(std::uint32_t first, std::uint32_t count);

    std::vector<Triangle> triangles_;
    std::vector<Node>     nodes_;
    Point                 min_{}, max_{};
};

// What makes a connected piece of a mesh large enough to stop the camera: as wide as `wide` on the ground (posts,
// pillars, signs, lamps and barrels are narrower) and as tall as `tall` (low fences and rims are lower), or as wide as
// `broad` (the ground and its slabs, however low), or mostly floor (a plaza's tiles, a roof).
struct Largeness {
    float wide = 40.0f;
    float tall = 25.0f;
    float broad = 200.0f;
    float floor = 400.0f; // the flat area of a piece that is mostly floor, kept however low
};

// The triangles of the mesh's large connected pieces only. Triangles are connected where they share a corner (to
// within a hundredth of a unit: a mesh splits its corners by texture and normal).
std::vector<Triangle> KeepLarge(const std::vector<Triangle> &triangles, const Largeness &largeness = {});

// Whether each triangle is in one of the mesh's large connected pieces.
std::vector<bool> LargeMask(const std::vector<Triangle> &triangles, const Largeness &largeness = {});

// The point of a triangle nearest p.
Point NearestOnTriangle(const Point &p, const Triangle &triangle);

} // namespace expanded
