#include "expanded/data/cfg_text.hpp"

#include <format>
#include <stdexcept>
#include <string>

namespace expanded {

namespace {

// Where the MOTION_END after the cfg's KEY_START begins.
std::size_t MotionEnd(const std::string &text) {
    std::size_t start = text.find("KEY_START");
    std::size_t end = start == std::string::npos ? start : text.find("MOTION_END", start);
    if (end == std::string::npos) {
        throw std::runtime_error("the cfg has no KEY_START ... MOTION_END block");
    }
    return end;
}

Bytes ToBytes(const std::string &text) {
    return Bytes(text.begin(), text.end());
}

} // namespace

Bytes AppendCfgKeys(std::span<const unsigned char> cfg, std::string_view lines) {
    std::string text(cfg.begin(), cfg.end());
    text.insert(MotionEnd(text), lines);
    return ToBytes(text);
}

Bytes AppendCfgEvents(std::span<const unsigned char> cfg, std::string_view lines) {
    std::string text(cfg.begin(), cfg.end());
    std::size_t at = text.find('\n', MotionEnd(text));
    text.insert(at == std::string::npos ? text.size() : at + 1, lines);
    return ToBytes(text);
}

Bytes ReplaceCfgKeys(std::span<const unsigned char> cfg, std::string_view lines) {
    std::string text(cfg.begin(), cfg.end());
    std::size_t end = MotionEnd(text);
    std::size_t keys = text.find("\r\n", text.find("KEY_START"));
    if (keys == std::string::npos || keys > end) {
        throw std::runtime_error("the cfg's KEY_START line has no end");
    }
    text.replace(keys + 2, end - (keys + 2), lines);
    return ToBytes(text);
}

Bytes SetCfgKey(std::span<const unsigned char> cfg, int key, std::string_view line) {
    std::string text(cfg.begin(), cfg.end());
    std::size_t start = text.find("KEY_START");
    std::size_t keys = start == std::string::npos ? start : text.find('\n', start);
    int         n = 0;
    for (std::size_t at = keys; at != std::string::npos && at + 1 < text.size(); at = text.find('\n', at + 1)) {
        std::size_t      from = at + 1, to = text.find('\n', from);
        std::string_view content(text.data() + from, (to == std::string::npos ? text.size() : to) - from);
        std::size_t      first = content.find_first_not_of(" \t");
        if (first == std::string_view::npos || !content.substr(first).starts_with("KEY") || content.substr(first).starts_with("KEY_")) {
            continue;
        }
        if (n++ == key) {
            std::size_t comment = content.find("//");
            std::string tail = comment == std::string_view::npos ? "\r" : std::string(content.substr(comment));
            text.replace(from, content.size(), std::string(line) + tail);
            return ToBytes(text);
        }
    }
    throw std::runtime_error(std::format("the cfg has no KEY {}", key));
}

} // namespace expanded
