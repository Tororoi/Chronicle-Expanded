#pragma once

#include <optional>

#include "shot_effect.hpp"

namespace expanded {

// The player's pellets: a CSHOT's slots.
inline constexpr int kPelletSlots = sizeof(CSHOT::used) / sizeof(CSHOT::used[0]);

// A slot of shots with no pellet in it, other than slot, or -1.
int FreePelletSlot(const CSHOT &shots, int slot);

// A pellet fired fresh into to as the one in from is: where it is and has been, its flight, life, damage, size and the
// rest.
void CopyPellet(CSHOT &shots, int from, int to);

// Fires effect in place of the pellet in slot of shots: from where the pellet is along its flight, as Xiao's shot, with
// damage and her weapon's abilities. The pellet is gone when the effect had a slot free for it: that slot, or -1.
int FireInPelletsPlace(CSHOT_EFFECT &effect, CSHOT &shots, int slot, int damage);

// What a feature keeps for the player's pellets in flight, by the slot each was fired into: one pellet's at a time, as a
// pellet fired into a slot takes it over. A slot out of range keeps nothing.
template <typename T>
class PelletSlots {
public:
    // Keeps value for the pellet in slot of shots, in place of what one fired into the slot before left.
    void Keep(const CSHOT &shots, int slot, const T &value) {
        if (InRange(slot)) {
            entries_[slot] = {&shots, value};
        }
    }

    // What is kept for the pellet in slot of shots, or null.
    T *Of(const CSHOT &shots, int slot) { return InRange(slot) && entries_[slot].shots == &shots ? &entries_[slot].value : nullptr; }

    // What is kept in slot, whichever pellet's, or null.
    T *InSlot(int slot) { return InRange(slot) && entries_[slot].shots != nullptr ? &entries_[slot].value : nullptr; }

    // What was kept for the pellet in slot of shots, now dropped, or nothing.
    std::optional<T> Take(const CSHOT &shots, int slot) {
        T *kept = Of(shots, slot);
        if (kept == nullptr) {
            return std::nullopt;
        }
        T value = *kept;
        Drop(slot);
        return value;
    }

    // Drops what is kept in slot, whichever pellet's.
    void Drop(int slot) {
        if (InRange(slot)) {
            entries_[slot] = {};
        }
    }

    void Clear() { *this = PelletSlots{}; }

    // visit(shots, slot, value) for each pellet something is kept for.
    template <typename Visit>
    void ForEach(Visit &&visit) {
        for (int slot = 0; slot < kPelletSlots; slot++) {
            if (entries_[slot].shots != nullptr) {
                visit(*entries_[slot].shots, slot, entries_[slot].value);
            }
        }
    }

    // Whether test holds for what is kept for any pellet.
    template <typename Test>
    bool Any(Test &&test) const {
        for (const Entry &entry : entries_) {
            if (entry.shots != nullptr && test(entry.value)) {
                return true;
            }
        }
        return false;
    }

private:
    struct Entry {
        const CSHOT *shots = nullptr;
        T            value{};
    };

    static bool InRange(int slot) { return slot >= 0 && slot < kPelletSlots; }

    Entry entries_[kPelletSlots];
};

} // namespace expanded
