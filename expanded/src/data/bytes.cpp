#include "expanded/data/bytes.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>

namespace expanded {

namespace {

void Need(std::span<const unsigned char> data, std::size_t offset, std::size_t size, const char *what) {
    if (offset > data.size() || size > data.size() - offset) {
        throw std::runtime_error(std::string(what) + " runs past the end of its data");
    }
}

} // namespace

std::uint16_t Get16(std::span<const unsigned char> data, std::size_t offset) {
    Need(data, offset, 2, "a half word");
    return static_cast<std::uint16_t>(data[offset] | data[offset + 1] << 8);
}

void Set16(std::span<unsigned char> data, std::size_t offset, std::uint16_t value) {
    Need(data, offset, 2, "a half word");
    data[offset] = static_cast<unsigned char>(value);
    data[offset + 1] = static_cast<unsigned char>(value >> 8);
}

std::uint32_t Get32(std::span<const unsigned char> data, std::size_t offset) {
    Need(data, offset, 4, "a word");
    return data[offset] | data[offset + 1] << 8 | data[offset + 2] << 16 | static_cast<std::uint32_t>(data[offset + 3]) << 24;
}

std::int32_t GetI32(std::span<const unsigned char> data, std::size_t offset) {
    return static_cast<std::int32_t>(Get32(data, offset));
}

void Set32(std::span<unsigned char> data, std::size_t offset, std::uint32_t value) {
    Need(data, offset, 4, "a word");
    for (int i = 0; i < 4; i++) {
        data[offset + i] = static_cast<unsigned char>(value >> (8 * i));
    }
}

float GetF32(std::span<const unsigned char> data, std::size_t offset) {
    return std::bit_cast<float>(Get32(data, offset));
}

void SetF32(std::span<unsigned char> data, std::size_t offset, float value) {
    Set32(data, offset, std::bit_cast<std::uint32_t>(value));
}

void Append32(Bytes &out, std::uint32_t value) {
    for (int i = 0; i < 4; i++) {
        out.push_back(static_cast<unsigned char>(value >> (8 * i)));
    }
}

void PadTo4(Bytes &out) {
    while (out.size() % 4 != 0) {
        out.push_back(0);
    }
}

void PadTo16(Bytes &out) {
    while (out.size() % 16 != 0) {
        out.push_back(0);
    }
}

Bytes ToBytes(std::string_view text) {
    return Bytes(text.begin(), text.end());
}

std::string ToText(std::span<const unsigned char> bytes) {
    return std::string(bytes.begin(), bytes.end());
}

std::span<const unsigned char> Slice(std::span<const unsigned char> data, std::size_t offset, std::size_t size, const char *what) {
    Need(data, offset, size, what);
    return data.subspan(offset, size);
}

std::string NameAt(std::span<const unsigned char> data, std::size_t offset, std::size_t max) {
    Need(data, offset, 0, "a name");
    std::size_t end = offset;
    while (end < data.size() && end - offset < max && data[end] != 0) {
        end++;
    }
    return std::string(reinterpret_cast<const char *>(data.data() + offset), end - offset);
}

char FoldCase(char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

bool EqualsFolded(std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) { return FoldCase(x) == FoldCase(y); });
}

bool EndsFolded(std::string_view text, std::string_view tail) {
    return text.size() >= tail.size() && EqualsFolded(text.substr(text.size() - tail.size()), tail);
}

[[noreturn]] void Bad(const std::string &what) {
    throw std::runtime_error(what);
}

} // namespace expanded
