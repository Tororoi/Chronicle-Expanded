#include "expanded/data/mes.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>

namespace expanded::mes {

namespace {

// Zero words kept after the last message, so it still ends on a zero word.
constexpr std::size_t kGap = 16;

struct Entry {
    int id;
    int offset;
};

std::vector<Entry> Entries(std::span<const unsigned char> mes) {
    std::vector<Entry> entries;
    int                count = Get16(mes, 0);
    for (int i = 0; i < count; i++) {
        entries.push_back({Get16(mes, 4 + i * 4), Get16(mes, 4 + i * 4 + 2)});
    }
    return entries;
}

// The text's length once its trailing zeros are cut to kGap bytes.
std::size_t TrimmedText(std::span<const unsigned char> mes, std::size_t text) {
    std::size_t end = mes.size();
    while (end > text && mes[end - 1] == 0) {
        end--;
    }
    std::size_t length = end - text + kGap;
    length += length & 1;
    return std::min(mes.size() - text, length);
}

void AppendWords(Bytes &out, const Words &words) {
    for (std::uint16_t word : words) {
        out.push_back(static_cast<unsigned char>(word));
        out.push_back(static_cast<unsigned char>(word >> 8));
    }
}

Bytes Lay(std::span<const unsigned char> mes, std::uint16_t word2, std::vector<Entry> entries, std::size_t text, std::size_t length) {
    std::ranges::stable_sort(entries, {}, &Entry::id);
    Bytes out;
    out.push_back(0);
    out.push_back(0);
    out.push_back(0);
    out.push_back(0);
    Set16(out, 0, static_cast<std::uint16_t>(entries.size()));
    Set16(out, 2, word2);
    for (const Entry &entry : entries) {
        out.push_back(static_cast<unsigned char>(entry.id));
        out.push_back(static_cast<unsigned char>(entry.id >> 8));
        out.push_back(static_cast<unsigned char>(entry.offset));
        out.push_back(static_cast<unsigned char>(entry.offset >> 8));
    }
    out.insert(out.end(), mes.begin() + static_cast<std::ptrdiff_t>(text), mes.begin() + static_cast<std::ptrdiff_t>(text + length));
    return out;
}

} // namespace

Words Extract(std::span<const unsigned char> mes, int id) {
    std::size_t count = Get16(mes, 0);
    for (const Entry &entry : Entries(mes)) {
        if (entry.id != id) {
            continue;
        }
        Words words;
        for (std::size_t at = 2 * (count + entry.offset + 1); at + 1 < mes.size(); at += 2) {
            words.push_back(Get16(mes, at));
            if (words.back() == kEnd) {
                return words;
            }
        }
        break;
    }
    throw std::runtime_error(std::format("the message file has no message {}", id));
}

Bytes Append(std::span<const unsigned char> mes, const std::vector<std::pair<int, Words>> &messages) {
    std::vector<Entry> entries = Entries(mes);
    for (const auto &[id, words] : messages) {
        if (std::ranges::find(entries, id, &Entry::id) != entries.end()) {
            throw std::runtime_error(std::format("the message file has a message {} already", id));
        }
    }
    std::size_t count = entries.size(), added = messages.size(), total = count + added;
    std::size_t text = 4 + count * 4, length = TrimmedText(mes, text);
    // The index grows by added entries, so every offset grows by added words with it.
    for (Entry &entry : entries) {
        entry.offset += static_cast<int>(added);
    }
    Bytes       tail;
    std::size_t at = 4 + total * 4 + length;
    for (const auto &[id, words] : messages) {
        entries.push_back({id, static_cast<int>((at + tail.size()) / 2 - total - 1)});
        AppendWords(tail, words);
    }
    Bytes out = Lay(mes, static_cast<std::uint16_t>(Get16(mes, 2) + added), std::move(entries), text, length);
    out.insert(out.end(), tail.begin(), tail.end());
    return out;
}

} // namespace expanded::mes
