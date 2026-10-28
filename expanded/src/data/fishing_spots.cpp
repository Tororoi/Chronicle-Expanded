#include "expanded/data/fishing_spots.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <format>
#include <set>

#include "expanded/data/fishing_scripts.hpp"
#include "expanded/data/mes.hpp"

namespace expanded {

namespace {

using scene::Triangle;

// The canal's containment walls along both banks, as the mod authored them: three corners a row.
const double kQueensCanalWalls[][9] = {
    {-100, 0,  -50,  -200, 70, -50,  -200, 50, -50},
    {-100, 0,  -50,  -100, 70, -50,  -200, 70, -50},
    {-200, 50, -50,  -200, 0,  -50,  -100, 0,  -50},
    {-200, 50, -50,  -200, 70, -50,  -200, 70, 50 },
    {-200, 50, -50,  -200, 70, 50,   -200, 50, 50 },
    {-600, 50, -50,  -600, 0,  -50,  -200, 0,  -50},
    {-600, 50, -50,  -200, 0,  -50,  -200, 50, -50},
    {-200, 50, 50,   -200, 0,  50,   -600, 0,  50 },
    {-200, 50, 50,   -600, 0,  50,   -600, 50, 50 },
    {-200, 50, 50,   -100, 70, 50,   -100, 0,  50 },
    {-100, 0,  50,   -200, 0,  50,   -200, 50, 50 },
    {-200, 50, 50,   -200, 70, 50,   -100, 70, 50 },
    {-100, 70, 50,   0,    0,  50,   -100, 0,  50 },
    {-100, 70, 50,   0,    70, 50,   0,    0,  50 },
    {0,    70, -50,  -100, 70, -50,  -100, 0,  -50},
    {0,    70, -50,  -100, 0,  -50,  0,    0,  -50},
    {100,  70, -50,  0,    70, -50,  0,    0,  -50},
    {100,  70, -50,  0,    0,  -50,  100,  0,  -50},
    {200,  70, -50,  100,  70, -50,  100,  0,  -50},
    {200,  70, -50,  100,  0,  -50,  200,  0,  -50},
    {300,  70, -50,  200,  70, -50,  200,  0,  -50},
    {300,  70, -50,  200,  0,  -50,  300,  0,  -50},
    {400,  70, -50,  300,  70, -50,  300,  0,  -50},
    {400,  70, -50,  300,  0,  -50,  400,  0,  -50},
    {500,  70, -50,  400,  70, -50,  400,  0,  -50},
    {500,  70, -50,  400,  0,  -50,  500,  0,  -50},
    {500,  0,  -50,  600,  70, -50,  500,  70, -50},
    {500,  0,  -50,  600,  0,  -50,  600,  70, -50},
    {700,  70, -50,  600,  0,  -50,  700,  0,  -50},
    {700,  70, -50,  600,  70, -50,  600,  0,  -50},
    {800,  70, -50,  700,  0,  -50,  800,  0,  -50},
    {800,  70, -50,  700,  70, -50,  700,  0,  -50},
    {900,  70, -50,  800,  70, -50,  800,  0,  -50},
    {900,  70, -50,  800,  0,  -50,  900,  0,  -50},
    {0,    70, 50,   100,  0,  50,   0,    0,  50 },
    {0,    70, 50,   100,  70, 50,   100,  0,  50 },
    {100,  70, 50,   200,  0,  50,   100,  0,  50 },
    {100,  70, 50,   200,  70, 50,   200,  0,  50 },
    {200,  70, 50,   300,  0,  50,   200,  0,  50 },
    {200,  70, 50,   300,  70, 50,   300,  0,  50 },
    {300,  70, 50,   400,  0,  50,   300,  0,  50 },
    {300,  70, 50,   400,  70, 50,   400,  0,  50 },
    {500,  0,  50,   400,  0,  50,   400,  70, 50 },
    {500,  0,  50,   400,  70, 50,   500,  70, 50 },
    {500,  70, 50,   600,  0,  50,   500,  0,  50 },
    {500,  70, 50,   600,  70, 50,   600,  0,  50 },
    {600,  70, 50,   700,  0,  50,   600,  0,  50 },
    {600,  70, 50,   700,  70, 50,   700,  0,  50 },
    {700,  70, 50,   800,  0,  50,   700,  0,  50 },
    {700,  70, 50,   800,  70, 50,   800,  0,  50 },
    {800,  70, 50,   900,  70, 50,   900,  0,  50 },
    {800,  70, 50,   900,  0,  50,   800,  0,  50 },
    {1000, 98, -100, 900,  70, -100, 900,  70, -50},
    {1000, 98, -100, 900,  70, -50,  1000, 98, -50},
    {1000, 98, 100,  1000, 98, 50,   900,  70, 50 },
    {900,  70, 50,   1000, 98, 200,  1000, 98, 100},
    {900,  70, 50,   900,  70, 200,  1000, 98, 200},
};

// The model frames objN of the placed meshes, every triangle.
std::vector<Triangle> FramesTriangles(const std::vector<scene::PlacedMesh> &placed, const std::set<int> &numbers) {
    std::vector<Triangle> triangles;
    for (const scene::PlacedMesh &mesh : placed) {
        std::string_view name = mesh.frame;
        if (!name.starts_with("obj")) {
            continue;
        }
        int         number = 0;
        const char *digits = name.data() + 3;
        if (std::from_chars(digits, name.data() + name.size(), number).ptr == digits || !numbers.contains(number)) {
            continue;
        }
        for (const auto &[a, b, c] : mesh.triangles) {
            triangles.push_back({mesh.vertices[static_cast<std::size_t>(a)], mesh.vertices[static_cast<std::size_t>(b)],
                                 mesh.vertices[static_cast<std::size_t>(c)]});
        }
    }
    return triangles;
}

// Queens: the canal's bridges (obj40, obj44) and pipes (obj9), and its containment walls.
std::vector<Triangle> QueensCollision(std::span<const unsigned char> scene, std::span<const unsigned char> cfg) {
    std::vector<scene::PlacedMesh> placed = scene::PlacedMeshes(scene, cfg);
    std::vector<Triangle>          triangles = FramesTriangles(placed, {40, 44});
    std::vector<Triangle>          pipes = FramesTriangles(placed, {9});
    triangles.insert(triangles.end(), pipes.begin(), pipes.end());
    for (const auto &wall : kQueensCanalWalls) {
        triangles.push_back({
            {{wall[0], wall[1], wall[2]}, {wall[3], wall[4], wall[5]}, {wall[6], wall[7], wall[8]}}
        });
    }
    return triangles;
}

const FishingSpot kSpots[] = {
    // Queens' canal from its north bank, 39 over the water: fished from the stance by the sign, facing south across it.
    {.code = "e03",
     .map_no = 2,
     .area = 6,
     .anchor = "e03g04",
     .sign = {250, 70, -64},
     .sign_yaw = 180,
     .sign_collision = true,
     .atari = "kanban_a.mds",
     .trigger = {0.0f, 0.0f, 6.0f},
     .x1 = -240.0f,
     .z1 = -100.0f,
     .x2 = 900.0f,
     .z2 = 150.0f,
     .water = 31.0f,
     .ground = 10.0f,
     .stance = {250.0f, 70.0f, -70.0f},
     .facing = 0.0f,
     .line = 1.35f,
     .fish_depth = 13.0f,
     .fish = {kBobo},
     .rare_one_in = 0,
     .collision = QueensCollision},
};

// The languages a town's messages come in: _1 to _6.
constexpr int kLanguages = 6;

// The sign: Muska Lacka's oasis sign as a part of its own, cloned from a Brownboo part's header, and a solid box.
constexpr const char          *kSignScene = "gedit/e04/scene.scn", *kSignPictures = "gedit/e04/img.pak", *kSignPicture = "e01b24";
constexpr const char          *kPartTemplateScene = "gedit/s04/scene.scn", *kPartTemplate = "s04a01";
constexpr const char          *kSignPart = "kanban", *kSignFrame = "kanban", *kSignCollisionFrame = "kanban_a";
constexpr std::array<float, 6> kSignBox = {-6.5f, 6.5f, 0.0f, 16.0f, -1.0f, 2.0f};
// Norune's fishing messages.
constexpr const char *kNoruneTalk = "gedit/e01/e01talk_{}.mes", *kNoruneEvents = "gedit/e01/e01_{}.mes";

using Files = const std::vector<Bytes> &;

std::string Rebuilt(std::string_view path) {
    return std::string(kRebuiltDiscFolder) + std::string(path);
}

Bytes BuildSignPictures(Files s) {
    return scene::PictureBank({
        {kSignPicture, scene::CarvePicture(s[0], kSignPicture)}
    });
}

// Norune's catch message added to the town's talk.
Bytes BuildTalk(Files s) {
    return mes::Append(s[1], {
                                 {kFishingCatchMessage, mes::Extract(s[0], kFishingCatchMessage)}
    });
}

// Norune's fishing menus added to the town's event messages.
Bytes BuildEventMessages(Files s) {
    std::vector<std::pair<int, mes::Words>> menus;
    for (int id : {kFishingEntryMessage, kFishingNoRodMessage, kFishingQuitMessage}) {
        menus.emplace_back(id, mes::Extract(s[0], id));
    }
    return mes::Append(s[1], menus);
}

Bytes BuildScene(const FishingSpot &spot, Files s) {
    Bytes sign = scene::CarveFrame(s[1], kSignFrame);
    Bytes header = scene::PartHeader(s[2], kPartTemplate);
    Bytes events = scene::ScriptEvent(spot.trigger, kFishingSignRadius, kFishingSignLabel);
    Bytes collision = spot.sign_collision ? scene::BoxCollision(kSignCollisionFrame, kSignBox) : Bytes{};
    return scene::AppendPart(s[0], kSignPart, header, sign, events, collision);
}

} // namespace

std::span<const FishingSpot> FishingSpots() {
    return kSpots;
}

const FishingSpot *FishingSpotOfMap(int map_no) {
    auto spot = std::ranges::find(kSpots, map_no, &FishingSpot::map_no);
    return spot == std::end(kSpots) ? nullptr : &*spot;
}

const FishingSpot *FishingSpotOfArea(int area) {
    auto spot = std::ranges::find(kSpots, area, &FishingSpot::area);
    return spot == std::end(kSpots) ? nullptr : &*spot;
}

std::string FishingCollisionPath(const FishingSpot &spot) {
    return std::format("expanded/fishing/{}.col", spot.code);
}

Bytes FishingCollisionFile(int map_no, const std::vector<Triangle> &triangles) {
    Bytes out = ToBytes("DCFC");
    Append32(out, 1);
    Append32(out, static_cast<std::uint32_t>(map_no));
    Append32(out, static_cast<std::uint32_t>(triangles.size()));
    for (const Triangle &triangle : triangles) {
        for (const scene::Vec3 &point : triangle) {
            for (double value : point) {
                Append32(out, std::bit_cast<std::uint32_t>(static_cast<float>(value)));
            }
        }
    }
    return out;
}

std::vector<Triangle> ReadFishingCollision(std::span<const unsigned char> file) {
    if (file.size() < 16 || ToText(file.first(4)) != "DCFC") {
        Bad("not a fishing collision file");
    }
    std::size_t           count = Get32(file, 12);
    std::vector<Triangle> triangles(count);
    Slice(file, 16, count * 36, "a fishing collision file's triangles");
    for (std::size_t t = 0; t < count; t++) {
        for (std::size_t v = 0; v < 3; v++) {
            for (std::size_t k = 0; k < 3; k++) {
                triangles[t][v][k] = GetF32(file, 16 + t * 36 + v * 12 + k * 4);
            }
        }
    }
    return triangles;
}

std::vector<DataBuild> FishingSpotBuilds() {
    std::vector<DataBuild> builds;
    builds.push_back(DataBuild::FromSources("sign picture", kFishingSignPictures, {kSignPictures}, BuildSignPictures));
    for (const FishingSpot &spot : kSpots) {
        std::string folder = std::format("gedit/{}/", spot.code);
        std::string scene = folder + "scene.scn", cfg = folder + "mapinfo.cfg", script = folder + "event.stb";
        builds.push_back(DataBuild::FromSources(std::format("{} scene", spot.code), Rebuilt(scene), {scene, kSignScene, kPartTemplateScene},
                                                [&spot](Files s) { return BuildScene(spot, s); }));
        builds.push_back(DataBuild::FromSources(std::format("{} mapinfo", spot.code), Rebuilt(cfg), {cfg}, [&spot](Files s) {
            return scene::PlaceGround(s[0], kSignPart, spot.anchor, spot.atari, spot.sign, spot.sign_yaw);
        }));
        builds.push_back(DataBuild::FromSources(std::format("{} event script", spot.code), Rebuilt(script), {script},
                                                [&spot](Files s) { return AddFishingScripts(s[0], spot); }));
        for (int language = 1; language <= kLanguages; language++) {
            std::string talk = std::format("{}{}talk_{}.mes", folder, spot.code, language);
            std::string events = std::format("{}{}_{}.mes", folder, spot.code, language);
            builds.push_back(DataBuild::FromSources(std::format("{} talk messages {}", spot.code, language), Rebuilt(talk),
                                                    {std::vformat(kNoruneTalk, std::make_format_args(language)), talk}, BuildTalk));
            builds.push_back(DataBuild::FromSources(std::format("{} event messages {}", spot.code, language), Rebuilt(events),
                                                    {std::vformat(kNoruneEvents, std::make_format_args(language)), events}, BuildEventMessages));
        }
        if (spot.collision != nullptr) {
            builds.push_back(DataBuild::FromSources(std::format("{} fishing collision", spot.code), FishingCollisionPath(spot), {scene, cfg},
                                                    [&spot](Files s) { return FishingCollisionFile(spot.map_no, spot.collision(s[0], s[1])); }));
        }
    }
    return builds;
}

} // namespace expanded
