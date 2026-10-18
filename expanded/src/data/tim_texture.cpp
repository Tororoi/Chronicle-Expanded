#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include "expanded/data/model_parts.hpp"

namespace expanded {

ImgBank ImgBank::Parse(std::span<const unsigned char> bank) {
    if (bank.size() < 0x10 || bank[0] != 'I' || bank[1] != 'M' || (bank[2] != 'G' && bank[2] != '2') || bank[3] != 0) {
        throw std::runtime_error("not an IMG or IM2 bank");
    }
    ImgBank out;
    std::copy(bank.begin(), bank.begin() + 4, out.magic.begin());
    int                                              count = static_cast<int>(Get32(bank, 4));
    std::vector<std::pair<std::string, std::size_t>> table;
    for (int i = 0; i < count; i++) {
        std::size_t entry = 0x10 + static_cast<std::size_t>(i) * 0x30;
        table.emplace_back(NameAt(bank, entry, 0x20), Get32(bank, entry + 0x20));
    }
    for (const auto &[name, offset] : table) {
        std::size_t next = bank.size();
        for (const auto &other : table) {
            if (other.second > offset) {
                next = std::min(next, other.second);
            }
        }
        auto block = Slice(bank, offset, next - offset, "a texture bank entry");
        out.entries.emplace_back(name, Bytes(block.begin(), block.end()));
    }
    return out;
}

const Bytes &ImgBank::Block(std::string_view name) const {
    for (const auto &[entry, block] : entries) {
        if (entry == name) {
            return block;
        }
    }
    throw std::runtime_error("texture bank lacks " + std::string(name));
}

Bytes ImgBank::Build() const {
    Bytes out(magic.begin(), magic.end());
    Append32(out, static_cast<std::uint32_t>(entries.size()));
    out.resize(0x10 + entries.size() * 0x30, 0);
    for (std::size_t i = 0; i < entries.size(); i++) {
        std::size_t        entry = 0x10 + i * 0x30;
        const std::string &name = entries[i].first;
        std::copy_n(name.begin(), std::min<std::size_t>(name.size(), 0x1F), out.begin() + static_cast<std::ptrdiff_t>(entry));
        Set32(out, entry + 0x20, static_cast<std::uint32_t>(out.size()));
        out.insert(out.end(), entries[i].second.begin(), entries[i].second.end());
        PadTo16(out);
    }
    return out;
}

namespace tim8 {

namespace {

int BlockOffset(int x, int y, int w) {
    int block = (y & ~0xF) * w + (x & ~0xF) * 2;
    int swap = (((y + 2) >> 2) & 0x1) * 4;
    int row = (((y & ~3) >> 1) + (y & 1)) & 0x7;
    int column = row * w * 2 + ((x + swap) & 0x7) * 4;
    int byte = ((y >> 1) & 1) + ((x >> 2) & 2);
    return block + column + byte;
}

} // namespace

Picture Read(std::span<const unsigned char> tim, bool swizzled) {
    if (tim.size() < kPicture + 0x30 || tim[0] != 'T' || tim[1] != 'I' || tim[2] != 'M' || tim[3] != '2' || tim[kPicture + 0x13] != 5) {
        throw std::runtime_error("not an 8-bit TIM2 picture");
    }
    std::size_t clut = Get32(tim, kPicture + 4), image = Get32(tim, kPicture + 8), header = Get16(tim, kPicture + 0xC);
    Picture     p;
    p.width = Get16(tim, kPicture + 0x14);
    p.height = Get16(tim, kPicture + 0x16);
    auto pixels = Slice(tim, kPicture + header, image, "a TIM2 picture's pixels");
    p.pixels = swizzled ? Unswizzle(pixels, p.width, p.height) : Bytes(pixels.begin(), pixels.end());
    auto colours = Slice(tim, kPicture + header + image, clut, "a TIM2 picture's CLUT");
    p.clut.assign(colours.begin(), colours.end());
    return p;
}

Bytes Build(std::span<const unsigned char> templ, int width, int height, std::span<const unsigned char> pixels,
            std::span<const unsigned char> clut) {
    if (templ[kPicture + 0x13] != 5 || Get16(templ, kPicture + 0xC) != 0x30) {
        throw std::runtime_error("TIM2 template is not an 8-bit picture with a 0x30 header");
    }
    Bytes out(templ.begin(), templ.begin() + kPicture + 0x30);
    Set32(out, kPicture, static_cast<std::uint32_t>(0x30 + pixels.size() + clut.size()));
    Set32(out, kPicture + 4, static_cast<std::uint32_t>(clut.size()));
    Set32(out, kPicture + 8, static_cast<std::uint32_t>(pixels.size()));
    Set16(out, kPicture + 0x14, static_cast<std::uint16_t>(width));
    Set16(out, kPicture + 0x16, static_cast<std::uint16_t>(height));
    std::fill(out.begin() + kPicture + 0x18, out.begin() + kPicture + 0x30, 0);
    out.insert(out.end(), pixels.begin(), pixels.end());
    out.insert(out.end(), clut.begin(), clut.end());
    return out;
}

Bytes Resample(std::span<const unsigned char> tim, bool swizzled, int n) {
    Picture     p = Read(tim, swizzled);
    std::size_t header = Get16(tim, kPicture + 0xC);
    Bytes       pixels(static_cast<std::size_t>(n * n));
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            pixels[static_cast<std::size_t>(y * n + x)] =
                p.pixels[static_cast<std::size_t>(((y * p.height + p.height / 2) / n) * p.width + (x * p.width + p.width / 2) / n)];
        }
    }
    Bytes out(tim.begin(), tim.begin() + static_cast<std::ptrdiff_t>(kPicture + header));
    Set32(out, kPicture, static_cast<std::uint32_t>(header + 4 * static_cast<std::size_t>(n * n)));
    Set32(out, kPicture + 8, static_cast<std::uint32_t>(n * n));
    Set16(out, kPicture + 0x14, static_cast<std::uint16_t>(n));
    Set16(out, kPicture + 0x16, static_cast<std::uint16_t>(n));
    out.insert(out.end(), pixels.begin(), pixels.end());
    out.insert(out.end(), p.clut.begin(), p.clut.end());
    return out;
}

Bytes Swizzle(std::span<const unsigned char> row_major, int width, int height) {
    Bytes out(static_cast<std::size_t>(width * height));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            out[static_cast<std::size_t>(BlockOffset(x, y, width))] = row_major[static_cast<std::size_t>(y * width + x)];
        }
    }
    return out;
}

Bytes Unswizzle(std::span<const unsigned char> blocks, int width, int height) {
    Bytes out(static_cast<std::size_t>(width * height));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            std::size_t from = static_cast<std::size_t>(BlockOffset(x, y, width));
            if (from < blocks.size()) {
                out[static_cast<std::size_t>(y * width + x)] = blocks[from];
            }
        }
    }
    return out;
}

Bytes Swizzled(std::span<const unsigned char> tim) {
    std::size_t header = Get16(tim, kPicture + 0xC);
    int         width = Get16(tim, kPicture + 0x14), height = Get16(tim, kPicture + 0x16);
    Bytes       out(tim.begin(), tim.end());
    Bytes       blocks = Swizzle(Slice(tim, kPicture + header, static_cast<std::size_t>(width) * height, "a TIM2 picture's pixels"), width, height);
    std::copy(blocks.begin(), blocks.end(), out.begin() + static_cast<std::ptrdiff_t>(kPicture + header));
    return out;
}

int Csm1(int i) {
    return (i & ~0x18) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
}

} // namespace tim8

} // namespace expanded
