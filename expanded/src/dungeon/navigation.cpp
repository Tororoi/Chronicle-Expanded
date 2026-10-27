#include "expanded/dungeon/navigation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>

#include "collision.hpp"
#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "dungeonparts.hpp"
#include "expanded/dungeon/floor_grid.hpp"
#include "gameutil.hpp"

namespace expanded {

namespace {

constexpr int   kTiles = 20;
constexpr float kOrigin = -kDungeonTile / 2.0f; // the grid's corner: tile 0 spans -80 to 80
constexpr int   kCells = static_cast<int>(kTiles * kDungeonTile / kNavCell);
constexpr float kReach = 48.0f;      // the widest clearance a tile's polygons serve
constexpr float kHeight = 80.0f;     // how far above and below the ground they are gathered
constexpr float kStep = 30.0f;       // the most the floor may rise or fall from the ground
constexpr float kWallHeight = 5.0f;  // the engine's: walls are tested this high
constexpr int   kSearchLimit = 6000; // cells A* expands at most
constexpr int   kPolyLimit = 8192;

enum Cell : std::uint8_t {
    kUnknown,
    kWalkable,
    kBlocked,
};

// The polygons around one tile, near one ground height.
std::map<std::pair<int, int>, std::vector<CCPoly>> tile_polys;

// One body's grid: the cells' walkability for its clearance near its ground.
struct Layer {
    std::vector<Cell> cells = std::vector<Cell>(kCells * kCells, kUnknown);
};

std::map<std::pair<int, int>, Layer> layers;

// Regions: the cells a body can reach from a start, found a few at a time.
struct Region {
    std::vector<bool> inside;
    std::queue<int>   open;
    float             clearance;
    float             ground;
    int               left; // cells it may still take
};

std::vector<Region> regions;

bool DungeonGridded() {
    return NowDngMap != nullptr && NowDngMap->map_type == 1;
}

bool DungeonOpen(int col, int row) {
    return NowDngMap->cells[col + row * kTiles].parts_no != MAP_PARTS_NONE;
}

int DungeonPolys(float *center, float radius, float height_ratio, CCPoly *polys, int) {
    return setCollisionData(NowDngMap, polys, center, radius, height_ratio);
}

const NavigationFloor  kDungeonFloor = {DungeonGridded, DungeonOpen, DungeonPolys};
const NavigationFloor *floor_now = &kDungeonFloor;

// The cells of the square ring r out from a centre, as a row-by-row scan of the square meets them; visit returns
// true to stop. Whether it stopped.
template <typename Visit>
bool ForRing(int r, Visit visit) {
    for (int dj = -r; dj <= r; dj++) {
        int step = (dj == -r || dj == r || r == 0) ? 1 : 2 * r;
        for (int di = -r; di <= r; di += step) {
            if (visit(di, dj)) {
                return true;
            }
        }
    }
    return false;
}

int Bucket(float ground) {
    return static_cast<int>(std::floor(ground / 10.0f));
}

bool Gridded() {
    return floor_now->gridded();
}

float CellCenter(int index) {
    return kOrigin + (static_cast<float>(index) + 0.5f) * kNavCell;
}

int CellOf(float world) {
    return static_cast<int>(std::floor((world - kOrigin) / kNavCell));
}

bool InGrid(int i, int j) {
    return i >= 0 && j >= 0 && i < kCells && j < kCells;
}

const std::vector<CCPoly> &TilePolys(int tile, float ground) {
    auto key = std::make_pair(tile, Bucket(ground));
    auto found = tile_polys.find(key);
    if (found != tile_polys.end()) {
        return found->second;
    }
    static std::vector<CCPoly> scratch(kPolyLimit);
    float                      radius = kDungeonTile / 2.0f + kReach;
    float                      center[4] = {kDungeonTile * static_cast<float>(tile % kTiles), ground, kDungeonTile * static_cast<float>(tile / kTiles), 1.0f};
    int                        count = std::clamp(floor_now->polys(center, radius, kHeight / radius, scratch.data(), kPolyLimit), 0, kPolyLimit);
    // Kept at the size found: the cache holds a tile for every height a body crossed it at.
    return tile_polys.emplace(key, std::vector<CCPoly>(scratch.begin(), scratch.begin() + count)).first->second;
}

bool Wall(const CCPoly &poly) {
    float length = std::sqrt(poly.normal[0] * poly.normal[0] + poly.normal[1] * poly.normal[1] + poly.normal[2] * poly.normal[2]);
    return length > 0.0f && std::fabs(poly.normal[1] / length) < 0.5f;
}

// Floor under the cell near the ground, and no wall within clearance around it.
Cell Test(int i, int j, float clearance, float ground) {
    float x = CellCenter(i);
    float z = CellCenter(j);
    int   col = static_cast<int>(std::floor((x - kOrigin) / kDungeonTile));
    int   row = static_cast<int>(std::floor((z - kOrigin) / kDungeonTile));
    if (col < 0 || row < 0 || col >= kTiles || row >= kTiles || !floor_now->open(col, row)) {
        return kBlocked;
    }
    std::vector<CCPoly> &polys = const_cast<std::vector<CCPoly> &>(TilePolys(col + row * kTiles, ground));
    int                  count = static_cast<int>(polys.size());
    float                from[4] = {x, ground + kStep, z, 1.0f};
    float                hit[4];
    if (CheckHitVertical(polys.data(), count, from, -2.0f * kStep, hit, 0) < 0) {
        return kBlocked;
    }
    float at[4] = {x, hit[1] + kWallHeight, z, 1.0f};
    for (int k = 0; k < 8; k++) {
        float angle = static_cast<float>(k) * 0.785398f;
        float to[4] = {x + std::sin(angle) * clearance, at[1], z + std::cos(angle) * clearance, 1.0f};
        int   poly = CheckHit(polys.data(), count, at, to, hit, 0, 0);
        if (poly >= 0 && Wall(polys[static_cast<std::size_t>(poly)])) {
            return kBlocked;
        }
    }
    return kWalkable;
}

struct Grid {
    Layer &layer;
    float  clearance;
    float  ground;

    bool Walkable(int i, int j) {
        if (!InGrid(i, j)) {
            return false;
        }
        Cell &cell = layer.cells[static_cast<std::size_t>(i + j * kCells)];
        if (cell == kUnknown) {
            cell = Test(i, j, clearance, ground);
        }
        return cell == kWalkable;
    }

    // Every cell along the line walkable; with leaving, those it starts in that are not (the body there already)
    // are passed over.
    bool Clear(float ax, float az, float bx, float bz, bool leaving = false) {
        float dx = bx - ax;
        float dz = bz - az;
        int   steps = std::max(1, static_cast<int>(std::sqrt(dx * dx + dz * dz) / (kNavCell / 2.0f)));
        for (int s = 0; s <= steps; s++) {
            float t = static_cast<float>(s) / static_cast<float>(steps);
            bool  walkable = Walkable(CellOf(ax + dx * t), CellOf(az + dz * t));
            leaving = leaving && !walkable;
            if (!walkable && !leaving) {
                return false;
            }
        }
        return true;
    }

    // The walkable cell nearest (i, j) within reach cells, or false.
    bool Nearest(int &i, int &j, int reach) {
        for (int r = 0; r <= reach; r++) {
            bool found = ForRing(r, [&](int di, int dj) {
                if (!Walkable(i + di, j + dj)) {
                    return false;
                }
                i += di;
                j += dj;
                return true;
            });
            if (found) {
                return true;
            }
        }
        return false;
    }

    // A* from start towards goal: the path's cells (start first) to goal, or to the reached cell nearest it.
    std::vector<int> Path(int si, int sj, int gi, int gj) {
        auto index = [](int i, int j) { return i + j * kCells; };
        auto heuristic = [&](int i, int j) {
            int dx = std::abs(i - gi);
            int dz = std::abs(j - gj);
            return static_cast<float>(std::max(dx, dz)) + 0.4142f * static_cast<float>(std::min(dx, dz));
        };
        std::map<int, float> cost;
        std::map<int, int>   came;
        using Open = std::tuple<float, int>;
        std::priority_queue<Open, std::vector<Open>, std::greater<>> open;
        int                                                          start = index(si, sj);
        int                                                          best = start;
        float                                                        best_h = heuristic(si, sj);
        cost[start] = 0.0f;
        open.emplace(best_h, start);
        int expanded = 0;
        while (!open.empty() && expanded < kSearchLimit) {
            auto [f, node] = open.top();
            open.pop();
            int   i = node % kCells;
            int   j = node / kCells;
            float g = cost[node];
            if (f > g + heuristic(i, j) + 1e-3f) {
                continue;
            }
            expanded++;
            float h = heuristic(i, j);
            if (h < best_h) {
                best_h = h;
                best = node;
            }
            if (i == gi && j == gj) {
                break;
            }
            for (int dj = -1; dj <= 1; dj++) {
                for (int di = -1; di <= 1; di++) {
                    if ((di == 0 && dj == 0) || !Walkable(i + di, j + dj)) {
                        continue;
                    }
                    if (di != 0 && dj != 0 && (!Walkable(i + di, j) || !Walkable(i, j + dj))) {
                        continue;
                    }
                    int   next = index(i + di, j + dj);
                    float step = (di != 0 && dj != 0) ? 1.4142f : 1.0f;
                    auto  known = cost.find(next);
                    if (known == cost.end() || g + step < known->second) {
                        cost[next] = g + step;
                        came[next] = node;
                        open.emplace(g + step + heuristic(i + di, j + dj), next);
                    }
                }
            }
        }
        std::vector<int> path;
        for (int node = best;; node = came[node]) {
            path.push_back(node);
            if (node == start) {
                break;
            }
        }
        std::reverse(path.begin(), path.end());
        return path;
    }
};

Grid GridFor(float clearance, float ground) {
    auto key = std::make_pair(static_cast<int>(std::lround(clearance)), Bucket(ground));
    return Grid{layers[key], clearance, ground};
}

void Copy(float *to, const float *from) {
    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2];
}

// The path from start to (gx, gz): its cells, the start snapped onto walkable ground; false when there is none.
bool Search(Grid &grid, const float *start, float gx, float gz, std::vector<int> &path) {
    int si = CellOf(start[0]);
    int sj = CellOf(start[2]);
    if (!grid.Nearest(si, sj, 4)) {
        return false;
    }
    int gi = CellOf(gx);
    int gj = CellOf(gz);
    path = grid.Path(si, sj, gi, gj);
    return true;
}

} // namespace

bool NavigateTowards(const float *from, const float *goal, float clearance, float ground, float *waypoint, float *reached) {
    Copy(waypoint, goal);
    Copy(reached, goal);
    if (!Gridded()) {
        return false;
    }
    Grid grid = GridFor(clearance, ground);
    if (grid.Walkable(CellOf(goal[0]), CellOf(goal[2])) && grid.Clear(from[0], from[2], goal[0], goal[2])) {
        return true;
    }
    std::vector<int> path;
    if (!Search(grid, from, goal[0], goal[2], path)) {
        return true;
    }
    int end = path.back();
    if (end != CellOf(goal[0]) + CellOf(goal[2]) * kCells) {
        reached[0] = CellCenter(end % kCells);
        reached[2] = CellCenter(end / kCells);
    }
    // The furthest cell of the path in a clear line, steered for; the reached spot itself when it is.
    for (std::size_t k = path.size(); k-- > 0;) {
        float x = CellCenter(path[k] % kCells);
        float z = CellCenter(path[k] / kCells);
        if (k == 0 || grid.Clear(from[0], from[2], x, z)) {
            waypoint[0] = k + 1 == path.size() ? reached[0] : x;
            waypoint[2] = k + 1 == path.size() ? reached[2] : z;
            break;
        }
    }
    return true;
}

bool NavigateNearest(const float *start, const float *point, float clearance, float ground, float *spot) {
    float waypoint[3];
    return NavigateTowards(start, point, clearance, ground, waypoint, spot);
}

bool NavigateClear(const float *from, const float *to, float clearance, float ground) {
    return !Gridded() || GridFor(clearance, ground).Clear(from[0], from[2], to[0], to[2], true);
}

int NavigateRegion(const float *start, float clearance, float ground, int limit) {
    if (!Gridded()) {
        return -1;
    }
    int si = CellOf(start[0]);
    int sj = CellOf(start[2]);
    if (!GridFor(clearance, ground).Nearest(si, sj, 4)) {
        return -1;
    }
    Region &region = regions.emplace_back();
    region.inside.assign(static_cast<std::size_t>(kCells * kCells), false);
    region.inside[static_cast<std::size_t>(si + sj * kCells)] = true;
    region.open.push(si + sj * kCells);
    region.clearance = clearance;
    region.ground = ground;
    region.left = limit - 1;
    return static_cast<int>(regions.size()) - 1;
}

void NavigateRegionGrow(int region, int budget) {
    if (region < 0 || region >= static_cast<int>(regions.size())) {
        return;
    }
    Region &grown = regions[static_cast<std::size_t>(region)];
    Grid    grid = GridFor(grown.clearance, grown.ground);
    for (; budget > 0 && grown.left > 0 && !grown.open.empty(); budget--) {
        int node = grown.open.front();
        grown.open.pop();
        int i = node % kCells;
        int j = node / kCells;
        for (auto [di, dj] : {
                 std::pair{1,  0 },
                 std::pair{-1, 0 },
                 std::pair{0,  1 },
                 std::pair{0,  -1}
        }) {
            int next = (i + di) + (j + dj) * kCells;
            if (grown.left > 0 && InGrid(i + di, j + dj) && !grown.inside[static_cast<std::size_t>(next)] &&
                grid.Walkable(i + di, j + dj)) {
                grown.inside[static_cast<std::size_t>(next)] = true;
                grown.open.push(next);
                grown.left--;
            }
        }
    }
}

bool NavigateRegionNearest(int region, const float *point, float *spot) {
    Copy(spot, point);
    if (region < 0 || region >= static_cast<int>(regions.size())) {
        return false;
    }
    const std::vector<bool> &inside = regions[static_cast<std::size_t>(region)].inside;
    int                      ci = CellOf(point[0]);
    int                      cj = CellOf(point[2]);
    for (int r = 0; r < kCells; r++) {
        int   best = -1;
        float best_distance = 0.0f;
        ForRing(r, [&](int di, int dj) {
            int i = ci + di;
            int j = cj + dj;
            if (!InGrid(i, j) || !inside[static_cast<std::size_t>(i + j * kCells)]) {
                return false;
            }
            float distance = std::hypot(CellCenter(i) - point[0], CellCenter(j) - point[2]);
            if (best < 0 || distance < best_distance) {
                best = i + j * kCells;
                best_distance = distance;
            }
            return false;
        });
        if (best >= 0) {
            if (r > 0) {
                spot[0] = CellCenter(best % kCells);
                spot[2] = CellCenter(best / kCells);
            }
            return true;
        }
    }
    return true;
}

void NavigationFloorLoaded() {
    tile_polys.clear();
    layers.clear();
    regions.clear();
}

void SetNavigationFloor(const NavigationFloor *floor) {
    floor_now = floor != nullptr ? floor : &kDungeonFloor;
    NavigationFloorLoaded();
}

} // namespace expanded
