#include "expanded/data/town_scene.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <format>
#include <map>
#include <numbers>
#include <optional>
#include <stdexcept>

#include "expanded/data/model_parts.hpp"

namespace expanded::scene {

namespace {

constexpr std::size_t kDirectory = 0x10, kDirectoryEntry = 0x30;
constexpr std::size_t kPartHeader = 0x160, kDefinition = 0x04;
// A part header's archive fields: the model's offset and size, the collision's offset and size, and the other models'
// offsets (none of them, a part of only a model and a collision).
constexpr std::size_t kModelOffset = 0x48, kModelSize = 0x58, kCollisionOffset = 0x78, kCollisionSize = 0x7C;
constexpr std::size_t kUnusedModels[] = {0x4C, 0x50, 0x54, 0x78, 0x90, 0xA8, 0xC0, 0xD8};
// A part definition's size, its functions' offset from it and their count.
constexpr std::size_t   kDefinitionSize = 0x04, kFunctions = 0x70, kFunctionCount = 0x74;
constexpr std::size_t   kFunction = 0xC0;
constexpr std::uint32_t kStandPoint = 0x01, kScriptEvent = 0x12;
// A texture bank's header and entries, and a TIM2 picture's header.
constexpr std::size_t   kBankHeader = 0x10, kBankEntry = 0x30, kBankOffset = 0x20, kPicture = 0x10;
constexpr std::uint32_t kMdsMagic = 0x0053444D, kMdtMagic = 0x0054444D;

std::optional<std::size_t> Find(std::span<const unsigned char> data, std::string_view needle, std::size_t from) {
    for (std::size_t i = from; i + needle.size() <= data.size(); i++) {
        if (std::equal(needle.begin(), needle.end(), data.begin() + static_cast<std::ptrdiff_t>(i),
                       [](char a, unsigned char b) { return static_cast<unsigned char>(a) == b; })) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> FindBefore(std::span<const unsigned char> data, std::string_view needle, std::size_t before) {
    if (data.size() < needle.size()) {
        return std::nullopt;
    }
    for (std::size_t i = std::min(before, data.size() - needle.size()) + 1; i-- > 0;) {
        if (std::equal(needle.begin(), needle.end(), data.begin() + static_cast<std::ptrdiff_t>(i),
                       [](char a, unsigned char b) { return static_cast<unsigned char>(a) == b; })) {
            return i;
        }
    }
    return std::nullopt;
}

void SetName(std::span<unsigned char> out, std::size_t at, std::string_view name, std::size_t field) {
    for (std::size_t i = 0; i < field; i++) {
        out[at + i] = i < name.size() ? static_cast<unsigned char>(name[i]) : 0;
    }
}

// The text split into lines as Python's str.splitlines splits it, which the mod's bakes used on the mapinfo.
std::vector<std::string_view> SplitLines(std::string_view text) {
    auto                          is_break = [](char c) { return c == '\n' || c == '\r' || c == '\v' || c == '\f' || c == '\x1c' || c == '\x1d' || c == '\x1e' || c == '\x85'; };
    std::vector<std::string_view> lines;
    std::size_t                   i = 0;
    while (i < text.size()) {
        std::size_t j = i;
        while (j < text.size() && !is_break(text[j])) {
            j++;
        }
        lines.push_back(text.substr(i, j - i));
        if (j >= text.size()) {
            break;
        }
        if (text[j] == '\r' && j + 1 < text.size() && text[j + 1] == '\n') {
            j++;
        }
        i = j + 1;
    }
    return lines;
}

// A line of numbers (its comment cut) of at least three, or none.
std::optional<Vec3> NumberRow(std::string_view line) {
    line = line.substr(0, line.find("//"));
    auto trim = [](std::string_view s) {
        std::size_t a = s.find_first_not_of(" \t\r\n\v\f"), b = s.find_last_not_of(" \t\r\n\v\f");
        return a == std::string_view::npos ? std::string_view{} : s.substr(a, b - a + 1);
    };
    line = trim(line);
    if (line.empty() || !(std::isdigit(static_cast<unsigned char>(line[0])) ||
                          (line[0] == '-' && line.size() > 1 && std::isdigit(static_cast<unsigned char>(line[1]))))) {
        return std::nullopt;
    }
    std::vector<double> values;
    std::size_t         at = 0;
    while (at < line.size()) {
        std::size_t      end = line.find_first_of(", \t", at);
        std::string_view part = line.substr(at, end == std::string_view::npos ? std::string_view::npos : end - at);
        if (!part.empty()) {
            double value = 0;
            auto [ptr, error] = std::from_chars(part.data(), part.data() + part.size(), value);
            if (error != std::errc() || ptr != part.data() + part.size()) {
                return std::nullopt;
            }
            values.push_back(value);
        }
        if (end == std::string_view::npos) {
            break;
        }
        at = end + 1;
    }
    if (values.size() < 3) {
        return std::nullopt;
    }
    return Vec3{values[0], values[1], values[2]};
}

struct Placement {
    std::string name;
    Vec3        position;
    Vec3        rotation;
};

std::vector<Placement> GroundPlacements(std::string_view cfg) {
    auto head = [](std::string_view line) -> std::optional<std::string> {
        std::size_t at = line.find_first_not_of(" \t");
        if (at == std::string_view::npos) {
            return std::nullopt;
        }
        line.remove_prefix(at);
        std::size_t word = line.starts_with("GROUND") ? 6 : line.starts_with("WATER") ? 5
                                                                                      : 0;
        if (word == 0 || line.size() <= word || (line[word] != ' ' && line[word] != '\t')) {
            return std::nullopt;
        }
        line.remove_prefix(word);
        line.remove_prefix(std::min(line.size(), line.find_first_not_of(" \t")));
        if (line.size() < 2 || line[0] != '"') {
            return std::nullopt;
        }
        std::size_t close = line.find('"', 1);
        if (close == std::string_view::npos || close == 1) {
            return std::nullopt;
        }
        return std::string(line.substr(1, close - 1));
    };
    std::vector<std::string_view> lines = SplitLines(cfg);
    std::vector<Placement>        placements;
    for (std::size_t i = 0; i < lines.size();) {
        std::optional<std::string> name = head(lines[i]);
        if (!name) {
            i++;
            continue;
        }
        std::vector<Vec3> rows;
        std::size_t       j = i + 1;
        while (j < lines.size() && !head(lines[j]) && rows.size() < 2) {
            if (std::optional<Vec3> row = NumberRow(lines[j])) {
                rows.push_back(*row);
            }
            j++;
        }
        if (rows.size() >= 2) {
            placements.push_back({*name, rows[0], rows[1]});
        }
        i = j;
    }
    return placements;
}

using Matrix = std::array<double, 16>; // column-major, as stored

Matrix Compose(const Matrix &a, const Matrix &b) {
    Matrix r{};
    for (int c = 0; c < 4; c++) {
        for (int row = 0; row < 4; row++) {
            double s = 0;
            for (int k = 0; k < 4; k++) {
                s += a[static_cast<std::size_t>(k * 4 + row)] * b[static_cast<std::size_t>(c * 4 + k)];
            }
            r[static_cast<std::size_t>(c * 4 + row)] = s;
        }
    }
    return r;
}

Vec3 Transform(const Matrix &m, double x, double y, double z) {
    return {m[0] * x + m[4] * y + m[8] * z + m[12], m[1] * x + m[5] * y + m[9] * z + m[13], m[2] * x + m[6] * y + m[10] * z + m[14]};
}

Vec3 PlaceY(const Vec3 &v, const Vec3 &position, double yaw) {
    double angle = yaw * (std::numbers::pi / 180.0), c = std::cos(angle), s = std::sin(angle);
    return {v[0] * c + v[2] * s + position[0], v[1] + position[1], -v[0] * s + v[2] * c + position[2]};
}

} // namespace

std::vector<SubFile> SubFiles(std::span<const unsigned char> scene) {
    std::vector<SubFile> files;
    std::size_t          count = Get32(scene, 4);
    for (std::size_t at = kDirectory; files.size() < count && at + kDirectoryEntry <= scene.size(); at += kDirectoryEntry) {
        std::string name = NameAt(scene, at, 16);
        if (name.empty() || !std::isalnum(static_cast<unsigned char>(name[0]))) {
            break;
        }
        files.push_back({name, Get32(scene, at + 0x10), Get32(scene, at + 0x14)});
    }
    return files;
}

Bytes CarveFrame(std::span<const unsigned char> scene, std::string_view frame) {
    std::string                key = std::string(frame) + '\0';
    std::optional<std::size_t> named = Find(scene, key, 0);
    if (!named || *named < 8) {
        Bad(std::format("the scene has no frame {}", frame));
    }
    std::optional<std::size_t> mds = FindBefore(scene, std::string_view("MDS\0", 4), *named - 8);
    if (!mds) {
        Bad(std::format("the scene has no model holding {}", frame));
    }
    std::size_t table = Get32(scene, *mds + mds::kTableAt), count = Get32(scene, *mds + mds::kCountAt);
    for (std::size_t i = 0; i < count; i++) {
        std::size_t record = *mds + table + i * mds::kRecord;
        if (NameAt(scene, record + mds::kName, mds::kNameSize) != frame) {
            continue;
        }
        std::size_t mdt = *mds + Get32(scene, record + mds::kMesh);
        std::size_t size = Get32(scene, mdt + 8);
        Bytes       out(mds::kTable + mds::kRecord, 0);
        Set32(out, 0, kMdsMagic);
        Set32(out, 4, Get32(scene, *mds + 4));
        Set32(out, mds::kCountAt, 1);
        Set32(out, mds::kTableAt, mds::kTable);
        std::span<const unsigned char> record_bytes = Slice(scene, record, mds::kRecord, "a frame record");
        std::copy(record_bytes.begin(), record_bytes.end(), out.begin() + mds::kTable);
        Set32(out, mds::kTable + mds::kMesh, mds::kTable + mds::kRecord);
        Set32(out, mds::kTable + mds::kParent, 0xFFFFFFFF);
        std::span<const unsigned char> mesh = Slice(scene, mdt, size, "a frame's mesh");
        out.insert(out.end(), mesh.begin(), mesh.end());
        return out;
    }
    Bad(std::format("no model of the scene has a frame {}", frame));
}

Bytes PartHeader(std::span<const unsigned char> scene, std::string_view part) {
    for (const SubFile &file : SubFiles(scene)) {
        if (file.name == part) {
            std::span<const unsigned char> header = Slice(scene, file.offset, kPartHeader, "a part header");
            return Bytes(header.begin(), header.end());
        }
    }
    Bad(std::format("the scene has no part {}", part));
}

Bytes CarvePicture(std::span<const unsigned char> pack, std::string_view name) {
    for (std::size_t at = 0; at < pack.size() && pack[at] != 0; at += Get32(pack, at + 0x48)) {
        std::size_t bank = at + Get32(pack, at + 0x40), size = Get32(pack, at + 0x44);
        if (size < 8 || bank + 4 > pack.size() || pack[bank] != 'I' || pack[bank + 1] != 'M' ||
            (pack[bank + 2] != '2' && pack[bank + 2] != 'G') || pack[bank + 3] != 0) {
            continue;
        }
        std::size_t count = Get32(pack, bank + 4);
        for (std::size_t e = 0; e < count; e++) {
            std::size_t entry = bank + kBankHeader + e * kBankEntry;
            if (entry + kBankEntry > pack.size()) {
                break;
            }
            if (NameAt(pack, entry, 0x20) != name) {
                continue;
            }
            std::size_t picture = bank + Get32(pack, entry + kBankOffset);
            std::size_t length = kPicture + Get16(pack, picture + kPicture + 0x0C) + Get32(pack, picture + kPicture + 0x08) +
                                 Get32(pack, picture + kPicture + 0x04);
            std::span<const unsigned char> tim = Slice(pack, picture, length, "a TIM2 picture");
            return Bytes(tim.begin(), tim.end());
        }
    }
    Bad(std::format("the pack has no picture {}", name));
}

Bytes PictureBank(const std::vector<std::pair<std::string, Bytes>> &pictures) {
    std::size_t              start = (kBankHeader + pictures.size() * kBankEntry + 0xF) & ~std::size_t{0xF};
    std::vector<std::size_t> offsets;
    std::size_t              end = start;
    for (const auto &[name, tim] : pictures) {
        offsets.push_back(end);
        end += (tim.size() + 0xF) & ~std::size_t{0xF};
    }
    Bytes out(end, 0);
    out[0] = 'I';
    out[1] = 'M';
    out[2] = '2';
    Set32(out, 4, static_cast<std::uint32_t>(pictures.size()));
    for (std::size_t i = 0; i < pictures.size(); i++) {
        std::size_t entry = kBankHeader + i * kBankEntry;
        SetName(out, entry, pictures[i].first.substr(0, 0x1F), 0x20);
        Set32(out, entry + kBankOffset, static_cast<std::uint32_t>(offsets[i]));
        std::copy(pictures[i].second.begin(), pictures[i].second.end(), out.begin() + static_cast<std::ptrdiff_t>(offsets[i]));
    }
    return out;
}

Bytes BoxCollision(std::string_view name, const std::array<float, 6> &box) {
    const auto [x0, x1, y0, y1, z0, z1] = box;
    const std::array<std::array<float, 3>, 8> vertices = {
        {{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}, {x0, y1, z0}, {x1, y1, z0}, {x1, y1, z1}, {x0, y1, z1}}
    };
    const std::array<std::array<int, 3>, 12> faces = {
        {{0, 1, 2}, {0, 2, 3}, {4, 6, 5}, {4, 7, 6}, {0, 4, 5}, {0, 5, 1}, {3, 2, 6}, {3, 6, 7}, {0, 3, 7}, {0, 7, 4}, {1, 5, 6}, {1, 6, 2}}
    };
    constexpr std::size_t kPositions = 0x40;
    std::size_t           list = kPositions + vertices.size() * 0x10;
    Bytes                 mdt(list + 0x18 + faces.size() * 0x14, 0);
    Set32(mdt, 0x00, kMdtMagic);
    Set32(mdt, 0x08, static_cast<std::uint32_t>(mdt.size()));
    Set32(mdt, 0x0C, static_cast<std::uint32_t>(vertices.size()));
    Set32(mdt, 0x10, kPositions);
    Set32(mdt, 0x28, static_cast<std::uint32_t>(list));
    for (std::size_t i = 0; i < vertices.size(); i++) {
        for (std::size_t k = 0; k < 3; k++) {
            SetF32(mdt, kPositions + i * 0x10 + k * 4, vertices[i][k]);
        }
        SetF32(mdt, kPositions + i * 0x10 + 12, 1.0f);
    }
    Set32(mdt, list + 0x14, static_cast<std::uint32_t>(faces.size()));
    for (std::size_t i = 0; i < faces.size(); i++) {
        for (std::size_t k = 0; k < 3; k++) {
            Set32(mdt, list + 0x18 + i * 0x14 + k * 4, static_cast<std::uint32_t>(faces[i][k]));
        }
    }
    Bytes out(mds::kTable + mds::kRecord, 0);
    Set32(out, 0, kMdsMagic);
    Set32(out, 4, 1);
    Set32(out, mds::kCountAt, 1);
    Set32(out, mds::kTableAt, mds::kTable);
    Set32(out, mds::kTable + mds::kSize, mds::kRecord);
    SetName(out, mds::kTable + mds::kName, name, name.size());
    Set32(out, mds::kTable + mds::kMesh, mds::kTable + mds::kRecord);
    Set32(out, mds::kTable + mds::kParent, 0xFFFFFFFF);
    for (std::size_t i = 0; i < 4; i++) {
        SetF32(out, mds::kTable + mds::kRotation + i * 0x14, 1.0f);
    }
    out.insert(out.end(), mdt.begin(), mdt.end());
    return out;
}

Bytes ScriptEvent(const std::array<float, 3> &position, float radius, int label) {
    Bytes event(kFunction, 0);
    Set32(event, 0x10, kScriptEvent);
    SetF32(event, 0x18, 0.0f); // hours 0 to 24: always on
    SetF32(event, 0x1C, 24.0f);
    for (std::size_t k = 0; k < 3; k++) {
        SetF32(event, 0x40 + k * 4, position[k]);
        SetF32(event, 0x60 + k * 4, radius);
    }
    SetF32(event, 0x70, static_cast<float>(label));
    return event;
}

Bytes MoveStandPoint(std::span<const unsigned char> scene, std::string_view part, int villager, const std::array<float, 3> &position) {
    for (const SubFile &file : SubFiles(scene)) {
        if (file.name != part) {
            continue;
        }
        std::size_t definition = file.offset + Get32(scene, file.offset + kDefinition);
        std::size_t functions = definition + Get32(scene, definition + kFunctions);
        for (std::size_t i = 0; i < Get32(scene, definition + kFunctionCount); i++) {
            std::size_t function = functions + i * kFunction;
            if (Get32(scene, function + 0x10) != kStandPoint || Get32(scene, function + 0x20) != static_cast<std::uint32_t>(villager)) {
                continue;
            }
            Bytes out(scene.begin(), scene.end());
            for (std::size_t k = 0; k < 3; k++) {
                SetF32(out, function + 0x40 + k * 4, position[k]);
            }
            return out;
        }
    }
    Bad(std::format("{} has no stand point for villager {}", part, villager));
}

Bytes AppendPart(std::span<const unsigned char> scene, std::string_view name, std::span<const unsigned char> header,
                 std::span<const unsigned char> model, std::span<const unsigned char> events, std::span<const unsigned char> collision,
                 bool at_origin) {
    if (header.size() != kPartHeader) {
        Bad("a part header is 0x160 bytes");
    }
    Bytes placed(model.begin(), model.end());
    if (at_origin) {
        for (std::size_t r = 0; r < 3; r++) {
            for (std::size_t c = 0; c < 3; c++) {
                SetF32(placed, mds::kTable + mds::kRotation + (r * 4 + c) * 4, r == c ? 1.0f : 0.0f);
            }
            SetF32(placed, mds::kTable + mds::kPosition + r * 4, 0.0f);
        }
    }
    Bytes part(header.begin(), header.end());
    SetName(part, 0x08, std::string(name) + "_0.mds", 0x10);
    part.insert(part.end(), events.begin(), events.end());
    std::size_t model_at = part.size();
    part.insert(part.end(), placed.begin(), placed.end());
    std::size_t collision_at = 0;
    if (!collision.empty()) {
        PadTo16(part);
        collision_at = part.size();
        part.insert(part.end(), collision.begin(), collision.end());
    }
    for (std::size_t field : kUnusedModels) {
        Set32(part, field, static_cast<std::uint32_t>(part.size()));
    }
    Set32(part, kModelSize, static_cast<std::uint32_t>(placed.size()));
    if (!events.empty()) {
        std::size_t definition = Get32(part, kDefinition);
        std::size_t functions = kPartHeader - definition;
        Set32(part, kModelOffset, static_cast<std::uint32_t>(model_at));
        Set32(part, definition + kFunctions, static_cast<std::uint32_t>(functions));
        Set32(part, definition + kFunctionCount, static_cast<std::uint32_t>(events.size() / kFunction));
        Set32(part, definition + kDefinitionSize, static_cast<std::uint32_t>(functions + events.size()));
    }
    if (!collision.empty()) {
        Set32(part, kCollisionOffset, static_cast<std::uint32_t>(collision_at));
        Set32(part, kCollisionSize, static_cast<std::uint32_t>(collision.size()));
    }

    // The directory's slack, before the first sub-file, takes the entry.
    std::size_t count = Get32(scene, 4), entry = kDirectory + count * kDirectoryEntry, first = scene.size();
    for (const SubFile &file : SubFiles(scene)) {
        first = std::min(first, file.offset);
    }
    if (entry + kDirectoryEntry > first) {
        Bad("the scene directory has no room for another part");
    }
    Bytes out(scene.begin(), scene.end());
    PadTo16(out);
    std::size_t at = out.size();
    out.insert(out.end(), part.begin(), part.end());
    SetName(out, entry, name, 0x10);
    Set32(out, entry + 0x10, static_cast<std::uint32_t>(at));
    Set32(out, entry + 0x14, static_cast<std::uint32_t>(part.size()));
    Set32(out, 4, static_cast<std::uint32_t>(count + 1));
    return out;
}

Bytes PlaceGround(std::span<const unsigned char> cfg, std::string_view part, std::string_view anchor, std::string_view collision,
                  const std::array<int, 3> &position, int yaw) {
    std::string text = ToText(cfg);
    std::string block = std::format("\r\n\tGROUND\t\"{}\",\t\t//injected part\r\n"
                                    "\t\t\"\",\t\t\t//level1\r\n\t\t\"\",\t\t\t//level2\r\n\t\t\"\",\t\t\t//level3\r\n"
                                    "\t\t\"\",\t\t\t//\r\n\t\t\"{}\",\t\t\t//atari\r\n\t\t\"\",\t\t\t//\r\n\t\t\"\",\t\t\t//?\r\n"
                                    "\t\t{},\t{},\t{},\t//position\r\n\t\t0,\t{},\t0\t//rotation\r\n",
                                    part, collision, position[0], position[1], position[2], yaw);
    // After the anchor's GROUND line, the first position line (a number) and the rotation line after it (a number, a
    // comma), as the mod's bake finds them.
    auto starts_number = [&](std::size_t at, bool sign) {
        if (at < text.size() && sign && text[at] == '-') {
            at++;
        }
        return at < text.size() && std::isdigit(static_cast<unsigned char>(text[at]));
    };
    std::string head = std::format("\tGROUND\t\"{}\",", anchor);
    std::size_t insert = std::string::npos;
    for (std::size_t from = text.find(head); from != std::string::npos; from = text.find(head, from)) {
        std::size_t end = std::string::npos;
        for (std::size_t line = text.find("\r\n\t\t", from + head.size()); line != std::string::npos; line = text.find("\r\n\t\t", line + 1)) {
            std::size_t first = line + 4;
            if (!starts_number(first, true)) {
                continue;
            }
            std::size_t first_end = text.find_first_of("\r\n", first);
            if (first_end == std::string::npos || text.compare(first_end, 4, "\r\n\t\t") != 0) {
                continue;
            }
            std::size_t second = first_end + 4;
            std::size_t second_end = text.find_first_of("\r\n", second);
            if (!starts_number(second, false) || second_end == std::string::npos || text.compare(second_end, 2, "\r\n") != 0 ||
                text.substr(second, second_end - second).find(',') == std::string::npos) {
                continue;
            }
            end = second_end + 2;
            break;
        }
        if (end == std::string::npos) {
            break;
        }
        insert = end;
        from = end;
    }
    if (insert == std::string::npos) {
        Bad(std::format("the mapinfo has no GROUND {}", anchor));
    }
    text.insert(insert, block);
    return ToBytes(text);
}

std::vector<PlacedMesh> PlacedMeshes(std::span<const unsigned char> scene, std::span<const unsigned char> cfg) {
    std::map<std::string, SubFile> directory;
    for (const SubFile &file : SubFiles(scene)) {
        directory.try_emplace(file.name, file);
    }
    std::vector<PlacedMesh>    placed;
    std::map<std::string, int> instances;
    std::string                text = ToText(cfg);
    for (const Placement &placement : GroundPlacements(text)) {
        auto file = directory.find(placement.name);
        if (file == directory.end()) {
            continue;
        }
        std::optional<std::size_t> mds = Find(scene, std::string_view("MDS\0", 4), file->second.offset);
        if (!mds || *mds >= file->second.offset + file->second.size) {
            continue;
        }
        int                      instance = instances[placement.name]++;
        std::size_t              count = Get32(scene, *mds + mds::kCountAt), table = Get32(scene, *mds + mds::kTableAt);
        std::vector<Matrix>      locals(count);
        std::vector<int>         parents(count);
        std::vector<int>         meshes(count);
        std::vector<std::string> names(count);
        for (std::size_t i = 0; i < count; i++) {
            std::size_t record = *mds + table + i * mds::kRecord;
            for (std::size_t k = 0; k < 16; k++) {
                locals[i][k] = GetF32(scene, record + 0x30 + k * 4);
            }
            names[i] = NameAt(scene, record + mds::kName, 16);
            meshes[i] = GetI32(scene, record + mds::kMesh);
            parents[i] = GetI32(scene, record + mds::kParent);
        }
        std::vector<std::optional<Matrix>> worlds(count);
        auto                               world = [&](auto &self, std::size_t i) -> const Matrix                               &{
            if (!worlds[i]) {
                int parent = parents[i];
                worlds[i] = parent < 0 || static_cast<std::size_t>(parent) >= count ? locals[i]
                                                                                                                  : Compose(self(self, static_cast<std::size_t>(parent)), locals[i]);
            }
            return *worlds[i];
        };
        for (std::size_t i = 0; i < count; i++) {
            if (meshes[i] == 0) {
                continue;
            }
            std::optional<std::size_t> mdt;
            for (std::size_t candidate : {static_cast<std::size_t>(static_cast<std::uint32_t>(meshes[i])), *mds + static_cast<std::uint32_t>(meshes[i])}) {
                if (candidate > 0 && candidate + 3 < scene.size() && scene[candidate] == 'M' && scene[candidate + 1] == 'D' && scene[candidate + 2] == 'T') {
                    mdt = candidate;
                    break;
                }
            }
            if (!mdt) {
                continue;
            }
            MdtMesh mesh;
            try {
                mesh = MdtMesh::Parse(scene.subspan(*mdt));
            } catch (const std::exception &) {
                continue;
            }
            PlacedMesh    out{names[i], placement.name, instance, {}, {}};
            const Matrix &m = world(world, i);
            for (const auto &p : mesh.positions) {
                out.vertices.push_back(PlaceY(Transform(m, p[0], p[1], p[2]), placement.position, placement.rotation[1]));
            }
            for (const MdtMesh::Submesh &sub : mesh.submeshes) {
                const auto &records = sub.records;
                if (sub.prim == 3) {
                    for (std::size_t k = 0; k + 2 < records.size(); k += 3) {
                        out.triangles.push_back({records[k][0], records[k + 1][0], records[k + 2][0]});
                    }
                } else if (sub.prim == 4) {
                    for (std::size_t k = 0; k + 2 < records.size(); k++) {
                        out.triangles.push_back(k % 2 == 0 ? std::array<int, 3>{records[k][0], records[k + 1][0], records[k + 2][0]}
                                                           : std::array<int, 3>{records[k + 1][0], records[k][0], records[k + 2][0]});
                    }
                }
            }
            if (!out.triangles.empty()) {
                placed.push_back(std::move(out));
            }
        }
    }
    return placed;
}

} // namespace expanded::scene
