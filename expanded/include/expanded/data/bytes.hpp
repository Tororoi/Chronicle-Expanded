#pragma once

// Little-endian values in a byte buffer, bounds-checked (out of range throws), and the names in it. Includes no
// game header, so dcdata can build with it.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace expanded {

using Bytes = std::vector<unsigned char>;

std::uint16_t Get16(std::span<const unsigned char> data, std::size_t offset);
void          Set16(std::span<unsigned char> data, std::size_t offset, std::uint16_t value);
std::uint32_t Get32(std::span<const unsigned char> data, std::size_t offset);
std::int32_t  GetI32(std::span<const unsigned char> data, std::size_t offset);
void          Set32(std::span<unsigned char> data, std::size_t offset, std::uint32_t value);
float         GetF32(std::span<const unsigned char> data, std::size_t offset);
void          SetF32(std::span<unsigned char> data, std::size_t offset, float value);

// Appends a word, and zeros up to the next 4 or 16 bytes.
void Append32(Bytes &out, std::uint32_t value);
void PadTo4(Bytes &out);
void PadTo16(Bytes &out);

// Text as bytes, and bytes as text.
Bytes       ToBytes(std::string_view text);
std::string ToText(std::span<const unsigned char> bytes);

// size bytes at offset, throwing (naming what) when they run past the end of data.
std::span<const unsigned char> Slice(std::span<const unsigned char> data, std::size_t offset, std::size_t size, const char *what);

// The NUL-terminated name at offset, at most max bytes.
std::string NameAt(std::span<const unsigned char> data, std::size_t offset, std::size_t max);

// ASCII case folding, as the game's file lookups compare names; whether text ends with tail, folded.
char FoldCase(char c);
bool EqualsFolded(std::string_view a, std::string_view b);
bool EndsFolded(std::string_view text, std::string_view tail);

// Throws what went wrong.
[[noreturn]] void Bad(const std::string &what);

} // namespace expanded
