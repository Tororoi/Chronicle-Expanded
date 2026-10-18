#include "expanded/data/stb.hpp"

#include "expanded/data/bytes.hpp"

namespace expanded::stb {

std::size_t CodeBase(std::span<const unsigned char> script) {
    return Get32(script, 8);
}

std::optional<std::size_t> Label(std::span<const unsigned char> script, std::uint32_t id) {
    std::size_t table = Get32(script, 0xC), count = Get32(script, 0x10);
    for (std::size_t i = 0; i < count; i++) {
        if (Get32(script, table + i * 8) == id) {
            return Get32(script, table + i * 8 + 4);
        }
    }
    return std::nullopt;
}

} // namespace expanded::stb
