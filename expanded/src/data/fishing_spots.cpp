#include "expanded/data/fishing_spots.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <format>
#include <set>

#include "expanded/data/fishing_collision.hpp"
#include "expanded/data/fishing_scripts.hpp"
#include "expanded/data/mes.hpp"
#include "expanded/data/queens_canal.hpp"
#include "expanded/data/yellow_drops_bank.hpp"

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

// Yellow Drops: the walls along the bulged west bank and the pillars' bases.
std::vector<Triangle> YellowDropsCollision(std::span<const unsigned char>, std::span<const unsigned char>) {
    return YellowDropsBankWalls();
}

// Queens' canal: its walls at ±47, 1 inside their faces; the float hanging under 45 within 60 of the middle (from its
// floor, not its banks); reeled in past ±49.5; and its two bridges' legs, from the canal's floor to 47, the decks over
// 50.
constexpr FishingCanal kQueensCasting = {
    .wall = 47.0f,
    .reach = 60.0f,
    .low = 45.0f,
    .drag = 49.5f,
    .deck = 50.0f,
    .legs = {{
        {-73.07f, -22.93f, 35.0f, 50.0f, 0.0f, 47.0f},
        {-73.07f, -22.93f, -50.0f, -36.0f, 0.0f, 47.0f},
        {774.93f, 825.07f, 35.0f, 50.0f, 0.0f, 47.0f},
        {774.93f, 825.07f, -50.0f, -36.0f, 0.0f, 47.0f},
    }},
};

const FishingSpot kSpots[] = {
    // Brownboo's pond from the boardwalk by the sign, 10 over the water: fished facing north across it, the fish 6
    // under. The landings at the tops of its ladders (s04r*), high over the pond, are left out of what it gathers, so
    // no float rests there.
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
     .fish_depth = 6.0f,
     .fish = {fish::kPiccoly, fish::kPiccoly, fish::kNegie, fish::kGummy},
     .rare_one_in = 50,
     .collision = BrownbooCollision,
     .edit_scene = BrownbooScene,
     .edit_cfg = nullptr,
     .camera_height = 40.0f,
     .camera_top = 0.0f,
     .canal = nullptr,
     .additions = nullptr,
     .floor_drop = {.parts = "s04r", .radius = 45.0f, .min_y = 25.0f}},
    // Queens' canal from its north bank, 39 over the afternoon's water (the tide sets it): fished from the stance by the
    // sign, facing south across it, a cast paying the line out to twice retail's, the fish at retail's depth (at low tide
    // the canal floor's); the camera's eye stays 4 under the walls' tops (70).
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
     .line = 2.0f,
     .fish_depth = 12.0f,
     .fish = {fish::kBobo},
     .rare_one_in = 0,
     .collision = QueensCollision,
     .edit_scene = nullptr,
     .edit_cfg = nullptr,
     .camera_height = 40.0f,
     .camera_top = 66.0f,
     .canal = &kQueensCasting,
     .additions = &kQueensCanal},
    // Queens' canal floor at low tide, down the ladder: fished standing in the water by the sign under the east bridge,
    // facing east along the canal, its fish let go about the sign.
    {.code = "e03",
     .map_no = 2,
     .area = 8,
     .part = "kanbanc",
     .anchor = "e03g04",
     .sign = {800, 0, 0},
     .sign_yaw = -90,
     .sign_collision = true,
     .atari = "kanbanc_a.mds",
     .trigger = {0.0f, 0.0f, 6.0f},
     .label = kCanalFloorSignLabel,
     .x1 = 600.0f,
     .z1 = -100.0f,
     .x2 = 1000.0f,
     .z2 = 150.0f,
     .water = kLowTide,
     .ground = 0.0f,
     .stance = {794.0f, 0.0f, 0.0f},
     .facing = 1.5708f,
     .line = 1.0f,
     .fish_depth = kLowTideFishDepth,
     .fish = {fish::kBobo},
     .rare_one_in = 0,
     .collision = QueensCollision,
     .edit_scene = nullptr,
     .edit_cfg = nullptr,
     .camera_height = 20.0f,
     .camera_top = 0.0f,
     .canal = &kQueensCasting,
     .additions = nullptr},
    // Yellow Drops from its west bank, bulged out into the water, 25 over the raised water: fished facing west, a cast
    // paying the line out to twice retail's, the fish 8 under; the camera lower over the high bank.
    {.code = "s13",
     .map_no = 23,
     .area = 7,
     .anchor = "s1301",
     .sign = {-465, 30, 40},
     .sign_yaw = 90,
     .sign_collision = true,
     .atari = "",
     .trigger = {0.0f, 0.0f, 0.0f},
     .x1 = -692.0f,
     .z1 = -156.0f,
     .x2 = -378.0f,
     .z2 = 270.0f,
     .water = kYellowDropsWater + 1.0f,
     .ground = -15.0f,
     .stance = {-468.0f, 30.0f, 40.0f},
     .facing = -1.5708f,
     .line = 2.0f,
     .fish_depth = 8.0f,
     .fish = {fish::kTarton, fish::kNonky, fish::kNegie, fish::kBon},
     .rare_one_in = 0,
     .collision = YellowDropsCollision,
     .edit_scene = YellowDropsWestBank,
     .edit_cfg = YellowDropsWater,
     .camera_height = 30.0f,
     .camera_top = 0.0f,
     .canal = nullptr},
};

// The languages a town's messages come in: _1 to _6.
constexpr int kLanguages = 6;

// The sign: Muska Lacka's oasis sign as a part of its own, cloned from a Brownboo part's header, and a solid box.
constexpr const char          *kSignScene = "gedit/e04/scene.scn", *kSignPictures = "gedit/e04/img.pak", *kSignPicture = "e01b24";
constexpr const char          *kPartTemplateScene = "gedit/s04/scene.scn", *kPartTemplate = "s04a01";
constexpr const char          *kSignFrame = "kanban";
constexpr std::array<float, 6> kSignBox = {-6.5f, 6.5f, 0.0f, 16.0f, -1.0f, 2.0f};
// The pictures the sign bank also carries: the canal ladder's metal, from the Factory's bank.
constexpr const char *kLadderPictures = "gedit/e05/img.pak", *kLadderPicture = "e05t06";
// Norune's fishing messages.
constexpr const char *kNoruneTalk = "gedit/e01/e01talk_{}.mes", *kNoruneEvents = "gedit/e01/e01_{}.mes";

using Files = const std::vector<Bytes> &;

std::string Rebuilt(std::string_view path) {
    return std::string(kRebuiltDiscFolder) + std::string(path);
}

Bytes BuildSignPictures(Files s) {
    return scene::PictureBank({
        {kSignPicture,   scene::CarvePicture(s[0], kSignPicture)  },
        {kLadderPicture, scene::CarvePicture(s[1], kLadderPicture)}
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

using Town = std::vector<const FishingSpot *>;

// The scene's sources: the town's scene, the sign's, the part template's, the town's mapinfo, then the additions'.
Bytes BuildScene(const Town &town, Files s) {
    Bytes sign = scene::CarveFrame(s[1], kSignFrame);
    Bytes header = scene::PartHeader(s[2], kPartTemplate);
    Bytes out = s[0];
    for (const FishingSpot *spot : town) {
        Bytes events = scene::ScriptEvent(spot->trigger, kFishingSignRadius, spot->label);
        Bytes collision = spot->sign_collision ? scene::BoxCollision(std::string(spot->part) + "_a", kSignBox) : Bytes{};
        out = scene::AppendPart(out, spot->part, header, sign, events, collision);
    }
    for (const FishingSpot *spot : town) {
        if (spot->edit_scene != nullptr) {
            out = spot->edit_scene(out, s[3]);
        }
    }
    if (const TownAdditions *additions = town.front()->additions) {
        out = additions->scene(out, std::vector<Bytes>(s.begin() + 4, s.end()));
    }
    return out;
}

Bytes BuildMapinfo(const Town &town, Files s) {
    Bytes out = s[0];
    for (const FishingSpot *spot : town) {
        out = scene::PlaceGround(out, spot->part, spot->anchor, spot->atari, spot->sign, spot->sign_yaw);
    }
    for (const FishingSpot *spot : town) {
        if (spot->edit_cfg != nullptr) {
            out = spot->edit_cfg(out);
        }
    }
    if (const TownAdditions *additions = town.front()->additions) {
        out = additions->cfg(out);
    }
    return out;
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

std::vector<const FishingSpot *> TownFishingSpots(int map_no) {
    std::vector<const FishingSpot *> town;
    for (const FishingSpot &spot : kSpots) {
        if (spot.map_no == map_no) {
            town.push_back(&spot);
        }
    }
    return town;
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
    builds.push_back(DataBuild::FromSources("sign picture", kFishingSignPictures, {kSignPictures, kLadderPictures}, BuildSignPictures));
    for (const FishingSpot &spot : kSpots) {
        if (FishingSpotOfMap(spot.map_no) != &spot) {
            continue; // the town's first spot builds its files
        }
        Town                     town = TownFishingSpots(spot.map_no);
        std::string              folder = std::format("gedit/{}/", spot.code);
        std::string              scene = folder + "scene.scn", cfg = folder + "mapinfo.cfg", script = folder + "event.stb";
        std::vector<std::string> scene_sources = {scene, kSignScene, kPartTemplateScene, cfg};
        if (spot.additions != nullptr) {
            scene_sources.insert(scene_sources.end(), spot.additions->sources.begin(), spot.additions->sources.end());
        }
        builds.push_back(DataBuild::FromSources(std::format("{} scene", spot.code), Rebuilt(scene), scene_sources,
                                                [town](Files s) { return BuildScene(town, s); }));
        builds.push_back(DataBuild::FromSources(std::format("{} mapinfo", spot.code), Rebuilt(cfg), {cfg},
                                                [town](Files s) { return BuildMapinfo(town, s); }));
        builds.push_back(DataBuild::FromSources(std::format("{} event script", spot.code), Rebuilt(script), {script},
                                                [town](Files s) {
                                                    const TownAdditions *additions = town.front()->additions;
                                                    return AddFishingScripts(s[0], town, additions != nullptr ? additions->scripts : nullptr);
                                                }));
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
