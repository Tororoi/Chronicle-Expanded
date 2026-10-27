#include "expanded/data/stb.hpp"

#include <format>
#include <stdexcept>

namespace expanded::stb {

std::size_t CodeBase(std::span<const unsigned char> script) {
    return Get32(script, kCodeBaseAt);
}

std::optional<std::size_t> Label(std::span<const unsigned char> script, std::uint32_t id) {
    std::size_t table = Get32(script, kLabelTableAt), count = Get32(script, kLabelCountAt);
    for (std::size_t i = 0; i < count; i++) {
        if (Get32(script, table + i * kLabelEntry) == id) {
            return Get32(script, table + i * kLabelEntry + 4);
        }
    }
    return std::nullopt;
}

std::size_t RequireLabel(std::span<const unsigned char> script, std::uint32_t id, std::string_view name) {
    if (std::optional<std::size_t> header = Label(script, id)) {
        return *header;
    }
    throw std::runtime_error(std::format("{} has no label {}", name, id));
}

void AppendCell(Bytes &out, std::uint32_t op, std::uint32_t a1, std::uint32_t a2) {
    Append32(out, op);
    Append32(out, a1);
    Append32(out, a2);
}

void SetCell(std::span<unsigned char> out, std::size_t at, std::uint32_t op, std::uint32_t a1, std::uint32_t a2) {
    Set32(out, at, op);
    Set32(out, at + 4, a1);
    Set32(out, at + 8, a2);
}

} // namespace expanded::stb
