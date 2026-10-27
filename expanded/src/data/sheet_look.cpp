#include "expanded/data/sheet_look.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "expanded/data/model_parts.hpp"

namespace expanded {

namespace {

constexpr int           kRows = 64; // the page's section limit
constexpr unsigned char kOpaque = 0x80;
constexpr int           kBudget = 256;

double JsRound(double x) {
    return std::floor(x + 0.5);
}

std::array<double, 3> RgbToHsl(double r, double g, double b) {
    r /= 255;
    g /= 255;
    b /= 255;
    double most = std::max({r, g, b}), least = std::min({r, g, b}), l = (most + least) / 2, h = 0, s = 0;
    if (most != least) {
        double d = most - least;
        s = l > 0.5 ? d / (2 - most - least) : d / (most + least);
        h = most == r ? (g - b) / d + (g < b ? 6 : 0) : most == g ? (b - r) / d + 2
                                                                  : (r - g) / d + 4;
        h *= 60;
    }
    return {h, s * 100, l * 100};
}

std::array<int, 3> HslToRgb(double h, double s, double l) {
    h = std::fmod(std::fmod(h, 360) + 360, 360);
    s = std::clamp(s, 0.0, 100.0) / 100;
    l = std::clamp(l, 0.0, 100.0) / 100;
    double c = (1 - std::abs(2 * l - 1)) * s, x = c * (1 - std::abs(std::fmod(h / 60, 2) - 1)), m = l - c / 2;
    double r, g, b;
    if (h < 60) {
        r = c, g = x, b = 0;
    } else if (h < 120) {
        r = x, g = c, b = 0;
    } else if (h < 180) {
        r = 0, g = c, b = x;
    } else if (h < 240) {
        r = 0, g = x, b = c;
    } else if (h < 300) {
        r = x, g = 0, b = c;
    } else {
        r = c, g = 0, b = x;
    }
    return {int(JsRound((r + m) * 255)), int(JsRound((g + m) * 255)), int(JsRound((b + m) * 255))};
}

int Key(const std::array<int, 3> &c) {
    return (c[0] << 16) | (c[1] << 8) | c[2];
}

Bytes Base64(std::string_view text) {
    auto value = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') {
            return c - 'A';
        }
        if (c >= 'a' && c <= 'z') {
            return c - 'a' + 26;
        }
        if (c >= '0' && c <= '9') {
            return c - '0' + 52;
        }
        return c == '+' ? 62 : c == '/' ? 63
                                        : -1;
    };
    Bytes out;
    int   bits = 0, buffer = 0;
    for (char c : text) {
        int v = value(c);
        if (v < 0) {
            continue;
        }
        buffer = (buffer << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<unsigned char>((buffer >> bits) & 0xFF));
        }
    }
    return out;
}

struct PictureAt {
    int         width, height;
    std::size_t clut, pixels;
};

PictureAt Locate(std::span<const unsigned char> tim) {
    constexpr std::size_t p = tim8::kPicture;
    Slice(tim, p, tim8::kGsWords, "a sheet's picture header");
    std::size_t clut = Get32(tim, p + tim8::kClutSize), image = Get32(tim, p + tim8::kImageSize), header = Get16(tim, p + tim8::kHeaderSize);
    int         width = Get16(tim, p + tim8::kWidth), height = Get16(tim, p + tim8::kHeight);
    if (tim[p + tim8::kType] != tim8::kType8 || image != std::size_t(width * height) || clut != 256 * 4) {
        throw std::runtime_error("look: not an 8-bit 256-colour picture");
    }
    Slice(tim, p + header, image + clut, "a sheet's pixels and CLUT");
    return {width, height, p + header + image, p + header};
}

// The page's merge: each colour (weighted by its texels) a cluster; the cheapest pair merged into its weighted centroid
// until 256 are left. Cost = distance x x.sqrt(x), x = 0.12 + the pair's mean luminance, so near-black goes first.
std::unordered_map<int, int> Merge(const std::vector<int> &keys, const std::unordered_map<int, double> &weight) {
    std::unordered_map<int, int> merged;
    int                          n = static_cast<int>(keys.size());
    if (n <= kBudget) {
        return merged;
    }
    std::vector<double>           R(n), G(n), B(n), W(n), nd(n);
    std::vector<int>              nn(n);
    std::vector<bool>             alive(n, true);
    std::vector<std::vector<int>> members(n);
    for (int i = 0; i < n; i++) {
        int v = keys[i];
        R[i] = v >> 16, G[i] = (v >> 8) & 255, B[i] = v & 255, W[i] = weight.at(v);
        members[i] = {v};
    }
    auto cost = [&](int i, int j) {
        double dr = R[i] - R[j], dg = G[i] - G[j], db = B[i] - B[j];
        double lum = (0.299 * (R[i] + R[j]) + 0.587 * (G[i] + G[j]) + 0.114 * (B[i] + B[j])) / 510, x = 0.12 + lum;
        return std::sqrt(2 * dr * dr + 4 * dg * dg + 3 * db * db) * (x * std::sqrt(x));
    };
    auto nearest = [&](int i) {
        int    best = -1;
        double bd = std::numeric_limits<double>::infinity();
        for (int j = 0; j < n; j++) {
            if (j != i && alive[j]) {
                double d = cost(i, j);
                if (d < bd) {
                    bd = d, best = j;
                }
            }
        }
        nn[i] = best, nd[i] = bd;
    };
    for (int i = 0; i < n; i++) {
        nearest(i);
    }
    for (int left = n; left > kBudget; left--) {
        int    i = -1;
        double bd = std::numeric_limits<double>::infinity();
        for (int k = 0; k < n; k++) {
            if (alive[k] && nd[k] < bd) {
                bd = nd[k], i = k;
            }
        }
        int    j = nn[i];
        double w = W[i] + W[j];
        R[i] = (R[i] * W[i] + R[j] * W[j]) / w;
        G[i] = (G[i] * W[i] + G[j] * W[j]) / w;
        B[i] = (B[i] * W[i] + B[j] * W[j]) / w;
        W[i] = w;
        members[i].insert(members[i].end(), members[j].begin(), members[j].end());
        alive[j] = false;
        for (int k = 0; k < n; k++) {
            if (!alive[k]) {
                continue;
            }
            if (k == i || nn[k] == i || nn[k] == j) {
                nearest(k);
            } else if (double d = cost(k, i); d < nd[k]) {
                nd[k] = d, nn[k] = i;
            }
        }
    }
    for (int i = 0; i < n; i++) {
        if (alive[i] && members[i].size() >= 2) {
            int centre = Key({int(JsRound(R[i])), int(JsRound(G[i])), int(JsRound(B[i]))});
            for (int v : members[i]) {
                merged[v] = centre;
            }
        }
    }
    return merged;
}

} // namespace

SheetLook SheetLook::Parse(std::string_view text) {
    nlohmann::json json = nlohmann::json::parse(text);
    SheetLook      look;
    for (const nlohmann::json &s : json.at("sections")) {
        Section section;
        section.hue = s.value("hue", 0.0);
        section.sat = s.value("sat", 1.0);
        section.bri = s.value("bri", 1.0);
        section.con = s.value("con", 1.0);
        section.light = s.value("light", 0.0);
        if (s.contains("keep")) {
            for (int k : s.at("keep")) {
                section.keep.insert(k);
            }
        }
        if (s.contains("set")) {
            for (const auto &[entry, colour] : s.at("set").items()) {
                std::string hex = colour.get<std::string>();
                if (!hex.empty() && hex[0] == '#') {
                    hex.erase(0, 1);
                }
                section.set[std::stoi(entry)] = {std::stoi(hex.substr(0, 2), nullptr, 16), std::stoi(hex.substr(2, 2), nullptr, 16),
                                                 std::stoi(hex.substr(4, 2), nullptr, 16)};
            }
        }
        look.sections_.push_back(std::move(section));
    }
    if (look.sections_.empty() || look.sections_.size() > kRows) {
        throw std::runtime_error("look: 1 to 64 sections");
    }
    if (json.contains("map")) {
        const nlohmann::json &map = json.at("map");
        look.map_width_ = map.at("w");
        look.map_height_ = map.at("h");
        Bytes runs = Base64(map.at("rle").get<std::string>());
        look.map_.assign(static_cast<std::size_t>(look.map_width_ * look.map_height_), 0);
        std::size_t o = 0;
        for (std::size_t i = 0; i + 1 < runs.size(); i += 2) {
            if (runs[i + 1] >= look.sections_.size()) {
                throw std::runtime_error("look: the map names a section past the last");
            }
            for (int j = 0; j < runs[i] && o < look.map_.size(); j++) {
                look.map_[o++] = runs[i + 1];
            }
        }
    }
    return look;
}

std::array<int, 3> SheetLook::Colour(int s, int k, const std::vector<std::array<int, 3>> &pal) const {
    const Section &section = sections_[static_cast<std::size_t>(s)];
    if (auto set = section.set.find(k); set != section.set.end()) {
        return set->second;
    }
    if (section.keep.contains(k)) {
        return pal[static_cast<std::size_t>(k)];
    }
    double r = pal[static_cast<std::size_t>(k)][0], g = pal[static_cast<std::size_t>(k)][1], b = pal[static_cast<std::size_t>(k)][2];
    if (section.hue != 0) {
        auto [h, sa, l] = RgbToHsl(r, g, b);
        auto t = HslToRgb(h + section.hue, sa, l);
        r = t[0], g = t[1], b = t[2];
    }
    double lu = 0.299 * r / 255 + 0.587 * g / 255 + 0.114 * b / 255;
    auto   slide = [&](double c) {
        c = lu + (c / 255 - lu) * section.sat;
        c = (c - 0.5) * section.con + 0.5;
        c = c * section.bri + section.light;
        return int(JsRound(std::clamp(c, 0.0, 1.0) * 255));
    };
    return {slide(r), slide(g), slide(b)};
}

Bytes SheetLook::ApplyToSheet(std::span<const unsigned char> tim) const {
    PictureAt                       at = Locate(tim);
    Bytes                           index = tim8::Unswizzle(Slice(tim, at.pixels, std::size_t(at.width) * at.height, "a sheet's pixels"), at.width, at.height);
    std::vector<std::array<int, 3>> pal(256);
    for (int k = 0; k < 256; k++) {
        std::size_t j = at.clut + std::size_t(tim8::Csm1(k)) * 4;
        pal[std::size_t(k)] = {tim[j], tim[j + 1], tim[j + 2]};
    }
    const Bytes *map = !map_.empty() && map_width_ == at.width && map_height_ == at.height ? &map_ : nullptr;
    // Every (entry, section) pair a texel uses, and its colours weighted by texels, in first-seen order.
    std::unordered_map<int, int> pair_count;
    std::vector<int>             pair_order;
    for (std::size_t i = 0; i < index.size(); i++) {
        int key = index[i] * kRows + (map != nullptr ? (*map)[i] : 0);
        if (pair_count[key]++ == 0) {
            pair_order.push_back(key);
        }
    }
    std::unordered_map<int, double> colour_count;
    std::vector<int>                colour_order;
    for (int key : pair_order) {
        int v = Key(Colour(key % kRows, key / kRows, pal));
        if (!colour_count.contains(v)) {
            colour_order.push_back(v);
        }
        colour_count[v] += pair_count[key];
    }
    std::unordered_map<int, int> merged = Merge(colour_order, colour_count);
    Bytes                        out_index(index.size());
    std::unordered_map<int, int> slot;
    std::vector<int>             out_pal;
    for (std::size_t i = 0; i < index.size(); i++) {
        int s = map != nullptr ? (*map)[i] : 0;
        int v = Key(Colour(s, index[i], pal));
        if (auto m = merged.find(v); m != merged.end()) {
            v = m->second;
        }
        auto [found, added] = slot.try_emplace(v, static_cast<int>(out_pal.size()));
        if (added) {
            if (out_pal.size() >= kBudget) {
                throw std::runtime_error("look: more than 256 colours after merging");
            }
            out_pal.push_back(v);
        }
        out_index[i] = static_cast<unsigned char>(found->second);
    }
    Bytes out(tim.begin(), tim.end());
    Bytes blocks = tim8::Swizzle(out_index, at.width, at.height);
    std::copy(blocks.begin(), blocks.end(), out.begin() + static_cast<std::ptrdiff_t>(at.pixels));
    for (int k = 0; k < 256; k++) {
        std::size_t j = at.clut + std::size_t(tim8::Csm1(k)) * 4;
        int         v = k < static_cast<int>(out_pal.size()) ? out_pal[std::size_t(k)] : 0;
        out[j] = static_cast<unsigned char>(v >> 16);
        out[j + 1] = static_cast<unsigned char>(v >> 8);
        out[j + 2] = static_cast<unsigned char>(v);
        out[j + 3] = kOpaque;
    }
    return out;
}

Bytes SheetLook::ApplyBaseToSheet(std::span<const unsigned char> tim) const {
    PictureAt                       at = Locate(tim);
    Bytes                           out(tim.begin(), tim.end());
    std::vector<std::array<int, 3>> pal(256);
    for (int j = 0; j < 256; j++) {
        std::size_t p = at.clut + std::size_t(j) * 4;
        pal[std::size_t(j)] = {out[p], out[p + 1], out[p + 2]};
    }
    SheetLook base;
    base.sections_ = {sections_[0]};
    base.sections_[0].keep.clear();
    base.sections_[0].set.clear();
    for (int j = 0; j < 256; j++) {
        auto        c = base.Colour(0, j, pal);
        std::size_t p = at.clut + std::size_t(j) * 4;
        out[p] = static_cast<unsigned char>(c[0]);
        out[p + 1] = static_cast<unsigned char>(c[1]);
        out[p + 2] = static_cast<unsigned char>(c[2]);
    }
    return out;
}

Bytes BuildGlowDisc(std::span<const unsigned char> templ, std::span<const unsigned char> torch, std::array<int, 3> core,
                    std::array<int, 3> outer) {
    constexpr double kCross = 0.125; // radius fraction where the mix is halfway
    constexpr std::size_t p = tim8::kPicture;
    std::size_t           header = Get16(torch, p + tim8::kHeaderSize), image = Get32(torch, p + tim8::kImageSize);
    int                   width = Get16(torch, p + tim8::kWidth), height = Get16(torch, p + tim8::kHeight);
    if (torch[p + tim8::kType] != tim8::kType32 || Get32(torch, p + tim8::kClutSize) != 0 || width != 64 || height != 64) {
        throw std::runtime_error("glow source is not a 64x64 32-bit TIM2");
    }
    auto px = torch.subspan(p + header, image);
    // The palette entries the GS's CLUT order leaves in place (bits 3 and 4 equal), luminance ranks onto them.
    std::vector<int> fixed;
    for (int i = 0; i < 256; i++) {
        if ((i & 0x18) == 0 || (i & 0x18) == 0x18) {
            fixed.push_back(i);
        }
    }
    std::vector<int> key(image / 4);
    for (std::size_t i = 0; i < image; i += 4) {
        key[i / 4] = (px[i] * 299 + px[i + 1] * 587 + px[i + 2] * 114) / 1000;
    }
    std::vector<int> levels = key;
    std::sort(levels.begin(), levels.end());
    levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
    if (levels.size() > fixed.size()) {
        throw std::runtime_error("glow disc needs more palette entries than the GS order leaves in place");
    }
    // The torch recoloured as a radial gradient, each pixel keeping its alpha and its share of the peak luminance.
    std::vector<double> lums(image / 4);
    for (std::size_t k = 0; k < image; k += 4) {
        lums[k / 4] = 0.30 * px[k] + 0.59 * px[k + 1] + 0.11 * px[k + 2];
    }
    double peak = *std::max_element(lums.begin(), lums.end());
    peak = peak == 0 ? 1.0 : peak;
    double cx = (width - 1) / 2.0, cy = (height - 1) / 2.0, edge = 0;
    bool   any = false;
    for (std::size_t i = 0; i < lums.size(); i++) {
        if (lums[i] > peak * 0.02) {
            double d = std::hypot(double(i % std::size_t(width)) - cx, double(i / std::size_t(width)) - cy);
            if (!any || d > edge) {
                edge = d, any = true;
            }
        }
    }
    edge = (!any || edge == 0) ? 1.0 : edge;
    double                             expo = std::log(0.5) / std::log(kCross);
    std::map<int, std::array<long, 5>> acc;
    for (std::size_t i = 0; i < lums.size(); i++) {
        double l = lums[i] / peak;
        double t = std::pow(std::min(1.0, std::hypot(double(i % std::size_t(width)) - cx, double(i / std::size_t(width)) - cy) / edge), expo);
        auto  &a = acc[key[i]];
        for (int c = 0; c < 3; c++) {
            a[std::size_t(c)] += std::min(255, int((core[std::size_t(c)] * (1.0 - t) + outer[std::size_t(c)] * t) * l));
        }
        a[3] += px[i * 4 + 3];
        a[4]++;
    }
    Bytes pixels(lums.size()), clut(256 * 4, 0);
    for (std::size_t i = 0; i < key.size(); i++) {
        auto rank = std::lower_bound(levels.begin(), levels.end(), key[i]) - levels.begin();
        pixels[i] = static_cast<unsigned char>(fixed[std::size_t(rank)]);
    }
    for (std::size_t r = 0; r < levels.size(); r++) {
        const auto &a = acc[levels[r]];
        for (int c = 0; c < 4; c++) {
            clut[std::size_t(fixed[r]) * 4 + std::size_t(c)] = static_cast<unsigned char>(a[std::size_t(c)] / a[4]);
        }
    }
    return tim8::Build(templ, width, height, pixels, clut);
}

} // namespace expanded
