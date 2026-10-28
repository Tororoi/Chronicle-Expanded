#include "expanded/data/stb.hpp"

#include <bit>
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

void Writer::Cell(std::uint32_t op, std::uint32_t a1, std::uint32_t a2) {
    AppendCell(cells_, op, a1, a2);
}

void Writer::JumpCell(std::uint32_t op, int mark) {
    jumps_.emplace_back(cells_.size(), mark);
    Cell(op, 0, 0);
}

void Writer::PushInt(int value) {
    Cell(kPushConst, kInt, static_cast<std::uint32_t>(value));
}

void Writer::PushFloat(float value) {
    Cell(kPushConst, kFloat, std::bit_cast<std::uint32_t>(value));
}

void Writer::PushString(std::string_view text) {
    strings_.emplace_back(text, cells_.size());
    Cell(kPushConst, kString, 0);
}

void Writer::PushLocal(int local) {
    Cell(kLoad, static_cast<std::uint32_t>(local), kIntLocal);
}

void Writer::PushLocalFloat(int local) {
    Cell(kLoad, static_cast<std::uint32_t>(local), kFloatLocal);
}

void Writer::PushAddress(int local) {
    Cell(kLoadAddress, static_cast<std::uint32_t>(local), kIntLocal);
}

void Writer::PushAddressFloat(int local) {
    Cell(kLoadAddress, static_cast<std::uint32_t>(local), kFloatLocal);
}

void Writer::Ext(int entries) {
    Cell(kExt, static_cast<std::uint32_t>(entries), 0);
}

void Writer::Call(std::size_t function) {
    Cell(kCall, 0, static_cast<std::uint32_t>(function));
}

void Writer::Return() {
    Cell(kReturn, 0, 0);
}

void Writer::Yield() {
    Cell(kWait, 0, 0);
}

void Writer::Pop() {
    Cell(kPop, 0, 0);
}

void Writer::Store() {
    Cell(kStore, 0, 0);
}

void Writer::Add() {
    Cell(kAdd, 0, 0);
}

void Writer::Subtract() {
    Cell(kSubtract, 0, 0);
}

void Writer::And() {
    Cell(kAnd, 0, 0);
}

void Writer::Compare(std::uint32_t test) {
    Cell(kCompare, test, 0);
}

int Writer::Mark() {
    marks_.push_back(cells_.size());
    return static_cast<int>(marks_.size()) - 1;
}

int Writer::MarkForward() {
    marks_.push_back(SIZE_MAX);
    return static_cast<int>(marks_.size()) - 1;
}

void Writer::Place(int mark) {
    marks_.at(static_cast<std::size_t>(mark)) = cells_.size();
}

void Writer::Jump(int mark) {
    JumpCell(kJump, mark);
}

void Writer::JumpTrue(int mark) {
    JumpCell(kJumpTrue, mark);
}

void Writer::JumpFalse(int mark) {
    JumpCell(kJumpFalse, mark);
}

void Writer::UseLocals(int count) {
    locals_ = std::max(locals_, count);
}

void Writer::SetArguments(int count) {
    arguments_ = count;
    UseLocals(count);
}

Bytes Writer::Lay(std::size_t at, std::size_t base) const {
    std::size_t code = at + kFunctionHeader;
    Bytes       out(kFunctionHeader, 0);
    Set32(out, 0, static_cast<std::uint32_t>(code - base));
    Set32(out, 8, static_cast<std::uint32_t>(locals_));
    Set32(out, 12, static_cast<std::uint32_t>(arguments_));
    Bytes cells = cells_;
    for (const auto &[cell, mark] : jumps_) {
        std::size_t target = marks_.at(static_cast<std::size_t>(mark));
        if (target == SIZE_MAX) {
            throw std::runtime_error("a script jump goes to a mark never placed");
        }
        Set32(cells, cell + 4, static_cast<std::uint32_t>(code + target - base));
    }
    std::size_t text = code + cells.size();
    Bytes       blob;
    for (const auto &[string, cell] : strings_) {
        Set32(cells, cell + 8, static_cast<std::uint32_t>(text + blob.size() - base));
        blob.insert(blob.end(), string.begin(), string.end());
        blob.push_back(0);
    }
    out.insert(out.end(), cells.begin(), cells.end());
    out.insert(out.end(), blob.begin(), blob.end());
    return out;
}

Appender::Appender(std::span<const unsigned char> script) : script_(script.begin(), script.end()) {
    PadTo16(script_);
}

std::size_t Appender::Next() const {
    return script_.size() - CodeBase(script_);
}

void Appender::Add(std::uint32_t id, const Writer &function) {
    std::size_t at = script_.size();
    Bytes       laid = function.Lay(at, CodeBase(script_));
    script_.insert(script_.end(), laid.begin(), laid.end());
    PadTo16(script_);
    labels_.emplace_back(id, at);
}

Bytes Appender::Build() const {
    Bytes       out = script_;
    std::size_t table = Get32(out, kLabelTableAt), count = Get32(out, kLabelCountAt);
    std::size_t moved = out.size();
    Slice(out, table, count * kLabelEntry, "the label table");
    out.insert(out.end(), script_.begin() + static_cast<std::ptrdiff_t>(table),
               script_.begin() + static_cast<std::ptrdiff_t>(table + count * kLabelEntry));
    for (const auto &[id, at] : labels_) {
        Append32(out, id);
        Append32(out, static_cast<std::uint32_t>(at));
    }
    Set32(out, kLabelTableAt, static_cast<std::uint32_t>(moved));
    Set32(out, kLabelCountAt, static_cast<std::uint32_t>(count + labels_.size()));
    return out;
}

} // namespace expanded::stb
