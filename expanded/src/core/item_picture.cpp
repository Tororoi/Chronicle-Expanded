#include "expanded/core/item_picture.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "data/data_read.hpp"
#include "gfx/gfx.hpp"
#include "itemdata.hpp"
#include "menu_draw.hpp"
#include "rect.hpp"
#include "render/draw2d.hpp"
#include "render/texture_registry.hpp"
#include "texture.hpp"
#include "tim2.hpp"

namespace expanded {

namespace {

constexpr int    kCell = 32;     // a picture's cell in the sheet, eight to a row
constexpr u_long kLinear = 0x61; // TEX1: LOD 0, linear magnification

// A picture cut out of the sheet into a texture of its own; key 0 where the item has none.
struct Picture {
    unsigned key = 0;
    u_long   tex0 = 0;
};

struct Pictures {
    std::string               path; // the sheet they were cut from
    std::vector<std::uint8_t> bank;
    std::map<int, Picture>    cut;
    bool                      read = false;
};

Pictures &Cache() {
    static Pictures pictures;
    return pictures;
}

// The menu language's weapon and attachment icon sheet, as the menus load it.
std::string SheetPath() {
    return std::string(GetMenuTextureDir()) + GetNowSelectLanguage(GetMenuLangFlag()) + "wepicon.img";
}

// The sheet decoded from its IM2 bank, or false where the bank has no eight-bit wepicon picture.
bool DecodeSheet(const std::vector<std::uint8_t> &bank, PortDecodedTexture &sheet) {
    if (bank.size() < sizeof(IMG_head)) {
        return false;
    }
    const auto *header = reinterpret_cast<const IMG_head *>(bank.data());
    const auto *entries = reinterpret_cast<const IMG_entry *>(header + 1);
    if (sizeof(IMG_head) + header->pictures * sizeof(IMG_entry) > bank.size()) {
        return false;
    }
    for (u_int i = 0; i < header->pictures; i++) {
        const IMG_entry &entry = entries[i];
        if (std::string_view(entry.name, strnlen(entry.name, sizeof(entry.name))) != "wepicon" ||
            entry.offset + 16 + sizeof(TM2_picture) > bank.size()) {
            continue;
        }
        const auto   *picture = reinterpret_cast<const TM2_picture *>(bank.data() + entry.offset + 16);
        const u_char *image = reinterpret_cast<const u_char *>(picture) + picture->header_size;
        const int     width = picture->image_width;
        const int     height = picture->image_height;
        if (picture->image_type != TIM2_IDTEX8 || image + picture->image_size + picture->clut_colors * 4 > bank.data() + bank.size()) {
            return false;
        }
        const bool                          swizzled = std::memcmp(bank.data(), "IM2", 3) == 0;
        const std::array<const u_char *, 1> levels = {image};
        return PortDecodeTexture(1, width, height, levels.data(), 1, image + picture->image_size, picture->clut_colors, swizzled,
                                 sheet);
    }
    return false;
}

// The cell at (x, y) of the sheet as straight RGBA.
PortDecodedTexture CutCell(const PortDecodedTexture &sheet, int x, int y) {
    PortDecodedTexture cell;
    cell.width = kCell;
    cell.height = kCell;
    cell.format = gfx::TextureFormat::Rgba8;
    std::vector<std::uint8_t>       &rgba = cell.levels.emplace_back(kCell * kCell * 4);
    const std::vector<std::uint8_t> &pixels = sheet.levels[0];
    for (int row = 0; row < kCell; row++) {
        for (int column = 0; column < kCell; column++) {
            const std::size_t from = static_cast<std::size_t>(y + row) * sheet.width + static_cast<std::size_t>(x + column);
            std::uint8_t     *to = &rgba[(static_cast<std::size_t>(row) * kCell + static_cast<std::size_t>(column)) * 4];
            if (sheet.format == gfx::TextureFormat::Index8) {
                const std::uint32_t colour = sheet.palette[pixels[from]];
                for (int c = 0; c < 4; c++) {
                    to[c] = static_cast<std::uint8_t>(colour >> (8 * c));
                }
            } else {
                std::memcpy(to, &pixels[from * 4], 4);
            }
        }
    }
    return cell;
}

// item_no's picture, cut from the sheet the first time it is asked for; null where it has none.
const Picture *Find(int item_no) {
    Pictures         &cache = Cache();
    const std::string path = SheetPath();
    if (cache.path != path) {
        for (const auto &[item, picture] : cache.cut) {
            PortReleaseKey(picture.key);
        }
        cache = Pictures{.path = path};
    }
    if (auto found = cache.cut.find(item_no); found != cache.cut.end()) {
        if (found->second.key == 0) {
            return nullptr;
        }
        if (PortKeyLive(found->second.key)) {
            return &found->second;
        }
    }
    Picture &picture = cache.cut[item_no] = Picture{};

    const COM_ITEM_INFO *info = GetCommonItemInfo(item_no);
    if (info == nullptr || info->icon_index < 0 || (info->kind != ITEMKIND_WEAPON && info->kind != ITEMKIND_ATTACH)) {
        return nullptr;
    }
    if (!cache.read) {
        cache.read = true;
        if (!DataReadReadFile(path, cache.bank)) {
            std::fprintf(stderr, "expanded: cannot read %s\n", path.c_str());
        }
    }
    PortDecodedTexture sheet;
    const int          x = info->icon_index % 8 * kCell;
    const int          y = info->icon_index / 8 * kCell;
    if (!DecodeSheet(cache.bank, sheet) || sheet.levels.empty() || x + kCell > static_cast<int>(sheet.width) ||
        y + kCell > static_cast<int>(sheet.height)) {
        return nullptr;
    }
    unsigned palette = 0;
    picture.key = PortCreateTexture(CutCell(sheet, x, y), PortTextureOwner::Other, &palette);
    if (picture.key == 0) {
        return nullptr;
    }
    // TBP0, TBW 1, PSMCT32, 32 by 32, TCC.
    picture.tex0 = static_cast<u_long>(picture.key) | (1ul << 14) | (5ul << 26) | (5ul << 30) | (1ul << 34);
    return &picture;
}

} // namespace

bool DrawItemPicture(int item_no, const CRect_i_ &screen, int alpha) {
    const Picture *picture = Find(item_no);
    if (picture == nullptr) {
        return false;
    }
    // The whole cell: pictures reach its edge texels, and a cell cut out alone has no neighbour to bleed in.
    const auto                 x0 = static_cast<float>(screen.x);
    const auto                 y0 = static_cast<float>(screen.y);
    const auto                 x1 = static_cast<float>(screen.x + screen.width);
    const auto                 y1 = static_cast<float>(screen.y + screen.height);
    const auto                 a = static_cast<u_char>(alpha);
    constexpr float            kEdge = kCell;
    std::vector<gfx::Vertex2D> quad = {
        draw2d::Vertex(x0, y0, 0.0f, 0.0f, 0.0f, 0x80, 0x80, 0x80, a),
        draw2d::Vertex(x1, y0, 0.0f, kEdge, 0.0f, 0x80, 0x80, 0x80, a),
        draw2d::Vertex(x1, y1, 0.0f, kEdge, kEdge, 0x80, 0x80, 0x80, a),
        draw2d::Vertex(x0, y1, 0.0f, 0.0f, kEdge, 0x80, 0x80, 0x80, a),
    };
    draw2d::DrawTextured(gfx::Primitive::Quads, quad, picture->tex0, kLinear, draw2d::SpriteState());
    draw2d::RestoreTestZbuf();
    return true;
}

} // namespace expanded
