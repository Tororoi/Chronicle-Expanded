#include "expanded/data/fishing_spots.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <format>
#include <set>

#include "expanded/data/fishing_collision.hpp"
#include "expanded/data/fishing_scripts.hpp"
#include "expanded/data/mes.hpp"

namespace expanded {

namespace {

using scene::Triangle;

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

// Brownboo: the pond's rocks.
std::vector<Triangle> BrownbooCollision(std::span<const unsigned char>, std::span<const unsigned char>) {
    return BrownbooRocks();
}

// Brownboo: villager 8, who stands where the sign's trigger is, stands back along the boardwalk.
Bytes BrownbooScene(std::span<const unsigned char> scene, std::span<const unsigned char>) {
    return scene::MoveStandPoint(scene, "s04g01", 8, {255.0f, 10.57f, -55.0f});
}

// Queens: the canal's bridges (obj40, obj44) and pipes (obj9), and its containment walls.
std::vector<Triangle> QueensCollision(std::span<const unsigned char> scene, std::span<const unsigned char> cfg) {
    std::vector<scene::PlacedMesh> placed = scene::PlacedMeshes(scene, cfg);
    std::vector<Triangle>          triangles = FramesTriangles(placed, {40, 44});
    std::vector<Triangle>          pipes = FramesTriangles(placed, {9});
    triangles.insert(triangles.end(), pipes.begin(), pipes.end());
    std::vector<Triangle> walls = QueensCanalWalls();
    triangles.insert(triangles.end(), walls.begin(), walls.end());
    return triangles;
}

const FishingSpot kSpots[] = {
    // Brownboo's pond from the boardwalk by the sign, 10 over the water: fished facing north across it. The landings
    // at the tops of its ladders (s04r*), high over the pond, are left out of what it gathers, so no float rests there.
    {.code = "s04",
     .map_no = 14,
     .area = 5,
     .anchor = "s04a01",
     .sign = {212, 9, -61},
     .sign_yaw = 0,
     .sign_collision = false,
     .atari = "",
     .trigger = {0.0f, 3.0f, 8.0f},
     .x1 = -250.0f,
     .z1 = -240.0f,
     .x2 = 250.0f,
     .z2 = 240.0f,
     .water = 0.0f,
     .ground = -15.0f,
     .stance = {212.0f, 10.0f, -53.0f},
     .facing = 3.14159f,
     .line = 1.0f,
     .fish_depth = 12.0f,
     .fish = {fish::kPiccoly, fish::kPiccoly, fish::kNegie, fish::kGummy},
     .rare_one_in = 50,
     .collision = BrownbooCollision,
     .edit_scene = BrownbooScene,
     .edit_cfg = nullptr,
     .floor_drop = {.parts = "s04r", .radius = 45.0f, .min_y = 25.0f}},
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
     .fish = {fish::kBobo},
     .rare_one_in = 0,
     .collision = QueensCollision,
     .edit_scene = nullptr,
     .edit_cfg = nullptr},
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
    Bytes town = scene::AppendPart(s[0], kSignPart, header, sign, events, collision);
    return spot.edit_scene != nullptr ? spot.edit_scene(town, s[3]) : town;
}

Bytes BuildMapinfo(const FishingSpot &spot, Files s) {
    Bytes town = scene::PlaceGround(s[0], kSignPart, spot.anchor, spot.atari, spot.sign, spot.sign_yaw);
    return spot.edit_cfg != nullptr ? spot.edit_cfg(town) : town;
}

// The spot's floor drop round each part the mapinfo places.
std::vector<FloorDropZone> FloorDropZones(const FishingSpot &spot, std::span<const unsigned char> cfg) {
    std::vector<FloorDropZone> zones;
    if (!spot.floor_drop.parts.empty()) {
        for (const scene::Vec3 &at : scene::PlacedAt(cfg, spot.floor_drop.parts)) {
            zones.push_back({static_cast<float>(at[0]), static_cast<float>(at[2]), spot.floor_drop.radius, spot.floor_drop.min_y});
        }
    }
    return zones;
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

Bytes FishingCollisionFile(int map_no, const std::vector<Triangle> &triangles, const std::vector<FloorDropZone> &zones) {
    Bytes out = ToBytes("DCFC");
    Append32(out, zones.empty() ? 1 : 2);
    Append32(out, static_cast<std::uint32_t>(map_no));
    Append32(out, static_cast<std::uint32_t>(triangles.size()));
    for (const Triangle &triangle : triangles) {
        for (const scene::Vec3 &point : triangle) {
            for (double value : point) {
                Append32(out, std::bit_cast<std::uint32_t>(static_cast<float>(value)));
            }
        }
    }
    if (!zones.empty()) {
        Append32(out, static_cast<std::uint32_t>(zones.size()));
        for (const FloorDropZone &zone : zones) {
            for (float value : {zone.x, zone.z, zone.radius, zone.min_y}) {
                Append32(out, std::bit_cast<std::uint32_t>(value));
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

std::vector<FloorDropZone> ReadFloorDropZones(std::span<const unsigned char> file) {
    std::size_t at = 16 + ReadFishingCollision(file).size() * 36;
    if (Get32(file, 4) < 2) {
        return {};
    }
    std::size_t                count = Get32(file, at);
    std::vector<FloorDropZone> zones(count);
    Slice(file, at + 4, count * 16, "a fishing collision file's floor drop zones");
    for (std::size_t i = 0; i < count; i++) {
        std::size_t z = at + 4 + i * 16;
        zones[i] = {GetF32(file, z), GetF32(file, z + 4), GetF32(file, z + 8), GetF32(file, z + 12)};
    }
    return zones;
}

std::vector<DataBuild> FishingSpotBuilds() {
    std::vector<DataBuild> builds;
    builds.push_back(DataBuild::FromSources("sign picture", kFishingSignPictures, {kSignPictures}, BuildSignPictures));
    for (const FishingSpot &spot : kSpots) {
        std::string folder = std::format("gedit/{}/", spot.code);
        std::string scene = folder + "scene.scn", cfg = folder + "mapinfo.cfg", script = folder + "event.stb";
        builds.push_back(DataBuild::FromSources(std::format("{} scene", spot.code), Rebuilt(scene), {scene, kSignScene, kPartTemplateScene, cfg},
                                                [&spot](Files s) { return BuildScene(spot, s); }));
        builds.push_back(DataBuild::FromSources(std::format("{} mapinfo", spot.code), Rebuilt(cfg), {cfg},
                                                [&spot](Files s) { return BuildMapinfo(spot, s); }));
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
                                                    [&spot](Files s) { return FishingCollisionFile(spot.map_no, spot.collision(s[0], s[1]), FloorDropZones(spot, s[1])); }));
        }
    }
    return builds;
}

} // namespace expanded
